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
#include "wp_saber.h"
#include "g_vehicles.h"
#include "../cgame/cg_local.h"
#if !defined(RUFL_HSTRING_INC)
	#include "../Rufl/hstring.h"
#endif
	#include "../Ratl/string_vs.h"
	#include "../Rufl/hstring.h"
	#include "../Ratl/vector_vs.h"

extern void WP_RemoveSaber( gentity_t *ent, int saberNum );
extern qboolean NPCsPrecached;
extern vec3_t playerMins;
extern vec3_t playerMaxs;
extern stringID_table_t WPTable[];

#define		MAX_MODELS_PER_LEVEL	60

hstring		modelsAlreadyDone[MAX_MODELS_PER_LEVEL];


stringID_table_t animEventTypeTable[] =
{
	ENUM2STRING(AEV_SOUND),			//# animID AEV_SOUND framenum soundpath randomlow randomhi chancetoplay
	ENUM2STRING(AEV_FOOTSTEP),		//# animID AEV_FOOTSTEP framenum footstepType
	ENUM2STRING(AEV_EFFECT),		//# animID AEV_EFFECT framenum effectpath boltName
	ENUM2STRING(AEV_FIRE),			//# animID AEV_FIRE framenum altfire chancetofire
	ENUM2STRING(AEV_MOVE),			//# animID AEV_MOVE framenum forwardpush rightpush uppush
	ENUM2STRING(AEV_SOUNDCHAN),		//# animID AEV_SOUNDCHAN framenum CHANNEL soundpath randomlow randomhi chancetoplay
	ENUM2STRING(AEV_SABER_SWING),	//# animID AEV_SABER_SWING framenum CHANNEL randomlow randomhi chancetoplay
	ENUM2STRING(AEV_SABER_SPIN),	//# animID AEV_SABER_SPIN framenum CHANNEL chancetoplay
	//must be terminated
	{ NULL,-1 }
};

stringID_table_t footstepTypeTable[] =
{
	ENUM2STRING(FOOTSTEP_R),
	ENUM2STRING(FOOTSTEP_L),
	ENUM2STRING(FOOTSTEP_HEAVY_R),
	ENUM2STRING(FOOTSTEP_HEAVY_L),
	//must be terminated
	{ NULL,-1 }
};

stringID_table_t FPTable[] =
{
	ENUM2STRING(FP_HEAL),
	ENUM2STRING(FP_LEVITATION),
	ENUM2STRING(FP_SPEED),
	ENUM2STRING(FP_PUSH),
	ENUM2STRING(FP_PULL),
	ENUM2STRING(FP_TELEPATHY),
	ENUM2STRING(FP_GRIP),
	ENUM2STRING(FP_LIGHTNING),
	ENUM2STRING(FP_SABERTHROW),
	ENUM2STRING(FP_SABER_DEFENSE),
	ENUM2STRING(FP_SABER_OFFENSE),
	//new Jedi Academy powers
	ENUM2STRING(FP_RAGE),
	ENUM2STRING(FP_PROTECT),
	ENUM2STRING(FP_ABSORB),
	ENUM2STRING(FP_DRAIN),
	ENUM2STRING(FP_SEE),
	{ "",	-1 }
};


stringID_table_t TeamTable[] =
{
	{ "free", TEAM_FREE },			// caution, some code checks a team_t via "if (!team_t_varname)" so I guess this should stay as entry 0, great or what? -slc
	ENUM2STRING(TEAM_FREE),			// caution, some code checks a team_t via "if (!team_t_varname)" so I guess this should stay as entry 0, great or what? -slc
	{ "player", TEAM_PLAYER },
	ENUM2STRING(TEAM_PLAYER),
	{ "enemy", TEAM_ENEMY },
	ENUM2STRING(TEAM_ENEMY),
	{ "neutral", TEAM_NEUTRAL },	// most droids are team_neutral, there are some exceptions like Probe,Seeker,Interrogator
	ENUM2STRING(TEAM_NEUTRAL),	// most droids are team_neutral, there are some exceptions like Probe,Seeker,Interrogator
	{ "",	-1 }
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
	ENUM2STRING(CLASS_RANCOR),
	ENUM2STRING(CLASS_SAND_CREATURE),
	ENUM2STRING(CLASS_WAMPA),
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
	ENUM2STRING(CLASS_SABOTEUR),
	ENUM2STRING(CLASS_STORMTROOPER),
	ENUM2STRING(CLASS_SWAMP),
	ENUM2STRING(CLASS_SWAMPTROOPER),
	ENUM2STRING(CLASS_NOGHRI),
	ENUM2STRING(CLASS_TAVION),
	ENUM2STRING(CLASS_ALORA),
	ENUM2STRING(CLASS_TRANDOSHAN),
	ENUM2STRING(CLASS_UGNAUGHT),
	ENUM2STRING(CLASS_JAWA),
	ENUM2STRING(CLASS_WEEQUAY),
	ENUM2STRING(CLASS_TUSKEN),
	ENUM2STRING(CLASS_BOBAFETT),
	ENUM2STRING(CLASS_ROCKETTROOPER),
	ENUM2STRING(CLASS_SABER_DROID),
	ENUM2STRING(CLASS_PLAYER),
	ENUM2STRING(CLASS_ASSASSIN_DROID),
	ENUM2STRING(CLASS_HAZARD_TROOPER),
	ENUM2STRING(CLASS_VEHICLE),
	{ "",	-1 }
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
// NPC parameters file : ext_data/NPCs/*.npc*
//
#define MAX_NPC_DATA_SIZE 0x80000
char	NPCParms[MAX_NPC_DATA_SIZE];

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

saber_colors_t TranslateSaberColor( const char *name )
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

int CG_CheckAnimFrameForEventType( animevent_t *animEvents, int keyFrame, animEventType_t eventType, unsigned short modelIndex )
{
	for ( int i = 0; i < MAX_ANIM_EVENTS; i++ )
	{
		if ( animEvents[i].keyFrame == keyFrame )
		{//there is an animevent on this frame already
			if ( animEvents[i].eventType == eventType )
			{//and it is of the same type
				if ( animEvents[i].modelOnly == modelIndex )
				{//and it is for the same model
					return i;
				}
			}
		}
	}
	//nope
	return -1;
}

/*
======================
ParseAnimationEvtBlock
======================
*/
static void ParseAnimationEvtBlock(int glaIndex, unsigned short modelIndex, const char* aeb_filename, animevent_t *animEvents, animation_t *animations, unsigned char &lastAnimEvent, const char **text_p, bool bIsFrameSkipped)
{
	const char		*token;
	int				num, n, animNum, keyFrame, lowestVal, highestVal, curAnimEvent = 0;
	animEventType_t	eventType;
	char			stringData[MAX_QPATH];

	// get past starting bracket
	while(1)
	{
		token = COM_Parse( text_p );
		if ( !Q_stricmp( token, "{" ) )
		{
			break;
		}
	}

	//NOTE: instead of a blind increment, increase the index
	//			this way if we have an event on an anim that already
	//			has an event of that type, it stomps it

	// read information for each frame
	while ( 1 )
	{
		// Get base frame of sequence
		token = COM_Parse( text_p );
		if ( !token || !token[0])
		{
			break;
		}

		if ( !Q_stricmp( token, "}" ) )		// At end of block
		{
			break;
		}

		//Compare to same table as animations used
		//	so we don't have to use actual numbers for animation first frames,
		//	just need offsets.
		//This way when animation numbers change, this table won't have to be updated,
		//	at least not much.
		animNum = GetIDForString(animTable, token);
		if(animNum == -1)
		{//Unrecognized ANIM ENUM name,
			Com_Printf(S_COLOR_YELLOW"WARNING: Unknown ANIM %s in file %s\n", token, aeb_filename );
			//skip this entry
			SkipRestOfLine( text_p );
			continue;
		}

		if ( animations[animNum].numFrames == 0 )
		{//we don't use this anim
#ifndef FINAL_BUILD
			Com_Printf(S_COLOR_YELLOW"WARNING: %s: anim %s not used by this model\n", aeb_filename, token);
#endif
			//skip this entry
			SkipRestOfLine( text_p );
			continue;
		}

		token = COM_Parse( text_p );
		eventType = (animEventType_t)GetIDForString(animEventTypeTable, token);
		if ( eventType == AEV_NONE || eventType == (animEventType_t)-1 )
		{//Unrecognized ANIM EVENT TYPE
			Com_Printf(S_COLOR_RED"ERROR: Unknown EVENT %s in animEvent file %s\n", token, aeb_filename );
			continue;
		}

		// Get offset to frame within sequence
		token = COM_Parse( text_p );
		if ( !token )
		{
			break;
		}

		keyFrame = atoi( token );
		if ( bIsFrameSkipped &&
			(animations[animNum].numFrames>2) // important, else frame 1 gets divided down and becomes frame 0. Carcass & Assimilate also work this way
			)
		{
			keyFrame /= 2;	// if we ever use any other value in frame-skipping we'll have to figure out some way of reading it, since it's not stored anywhere
		}
		if(keyFrame >= animations[animNum].numFrames)
		{
			Com_Printf(S_COLOR_YELLOW"WARNING: Event out of range on %s in %s\n", GetStringForID(animTable,animNum), aeb_filename );
			assert(keyFrame < animations[animNum].numFrames);
			keyFrame = animations[animNum].numFrames-1;	//clamp it
		}

		//set our start frame
		keyFrame += animations[animNum].firstFrame;

		//see if this frame already has an event of this type on it, if so, overwrite it
		curAnimEvent = CG_CheckAnimFrameForEventType( animEvents, keyFrame, eventType, modelIndex );
		if ( curAnimEvent == -1 )
		{//this anim frame doesn't already have an event of this type on it
			curAnimEvent = lastAnimEvent;
		}

		//now that we know which event index we're going to plug the data into, start doing it
		animEvents[curAnimEvent].eventType = eventType;
		assert(keyFrame >= 0 && keyFrame < 65535);	//
		animEvents[curAnimEvent].keyFrame = keyFrame;
		animEvents[curAnimEvent].glaIndex = glaIndex;
		animEvents[curAnimEvent].modelOnly = modelIndex;
		int tempVal;

		//now read out the proper data based on the type
		switch ( animEvents[curAnimEvent].eventType )
		{
		case AEV_SOUNDCHAN:		//# animID AEV_SOUNDCHAN framenum CHANNEL soundpath randomlow randomhi chancetoplay
			token = COM_Parse( text_p );
			if ( !token )
			{
				break;
			}
			if ( Q_stricmp( token, "CHAN_VOICE_ATTEN" ) == 0 )
			{
				animEvents[curAnimEvent].eventData[AED_SOUNDCHANNEL] = CHAN_VOICE_ATTEN;
			}
			else if ( Q_stricmp( token, "CHAN_VOICE_GLOBAL" ) == 0 )
			{
				animEvents[curAnimEvent].eventData[AED_SOUNDCHANNEL] = CHAN_VOICE_GLOBAL;
			}
			else if ( Q_stricmp( token, "CHAN_ANNOUNCER" ) == 0 )
			{
				animEvents[curAnimEvent].eventData[AED_SOUNDCHANNEL] = CHAN_ANNOUNCER;
			}
			else if ( Q_stricmp( token, "CHAN_BODY" ) == 0 )
			{
				animEvents[curAnimEvent].eventData[AED_SOUNDCHANNEL] = CHAN_BODY;
			}
			else if ( Q_stricmp( token, "CHAN_WEAPON" ) == 0 )
			{
				animEvents[curAnimEvent].eventData[AED_SOUNDCHANNEL] = CHAN_WEAPON;
			}
			else if ( Q_stricmp( token, "CHAN_VOICE" ) == 0 )
			{
				animEvents[curAnimEvent].eventData[AED_SOUNDCHANNEL] = CHAN_VOICE;
			}
			else
			{
				animEvents[curAnimEvent].eventData[AED_SOUNDCHANNEL] = CHAN_AUTO;
			}
			//fall through to normal sound
		case AEV_SOUND:			//# animID AEV_SOUND framenum soundpath randomlow randomhi chancetoplay
			//get soundstring
			token = COM_Parse( text_p );
			if ( !token )
			{
				break;
			}
			strcpy(stringData, token);
			//get lowest value
			token = COM_Parse( text_p );
			if ( !token )
			{//WARNING!  BAD TABLE!
				break;
			}
			lowestVal = atoi( token );
			//get highest value
			token = COM_Parse( text_p );
			if ( !token )
			{//WARNING!  BAD TABLE!
				break;
			}
			highestVal = atoi( token );
			//Now precache all the sounds
			//NOTE: If we can be assured sequential handles, we can store the first sound index and count
			//		unfortunately, if these sounds were previously registered, we cannot be guaranteed sequential indices.  Thus an array
			if(lowestVal && highestVal)
			{
				assert(highestVal - lowestVal < MAX_RANDOM_ANIM_SOUNDS);
				for ( n = lowestVal, num = AED_SOUNDINDEX_START; n <= highestVal && num <= AED_SOUNDINDEX_END; n++, num++ )
				{
					animEvents[curAnimEvent].eventData[num] = G_SoundIndex( va( stringData, n ) );//cgi_S_RegisterSound
				}
				animEvents[curAnimEvent].eventData[AED_SOUND_NUMRANDOMSNDS] = num - 1;
			}
			else
			{
				animEvents[curAnimEvent].eventData[AED_SOUNDINDEX_START] = G_SoundIndex( stringData );//cgi_S_RegisterSound
#if 0 //#ifndef FINAL_BUILD (only meaningfull if using S_RegisterSound
				if ( !animEvents[curAnimEvent].eventData[AED_SOUNDINDEX_START] )
				{//couldn't register it - file not found
					Com_Printf( S_COLOR_RED "ParseAnimationSndBlock: sound %s does not exist (%s)!\n", stringData, *aeb_filename );
				}
#endif
				animEvents[curAnimEvent].eventData[AED_SOUND_NUMRANDOMSNDS] = 0;
			}
			//get probability
			token = COM_Parse( text_p );
			if ( !token )
			{//WARNING!  BAD TABLE!
				break;
			}
			animEvents[curAnimEvent].eventData[AED_SOUND_PROBABILITY] = atoi( token );

			//last part - cheat and check and see if it's a special overridable saber sound we know of...
			if ( !Q_stricmpn( "sound/weapons/saber/saberhup", stringData, 28 ) )
			{//a saber swing
				animEvents[curAnimEvent].eventType = AEV_SABER_SWING;
				animEvents[curAnimEvent].eventData[AED_SABER_SWING_SABERNUM] = 0;//since we don't know which one they meant if we're hacking this, always use first saber
				animEvents[curAnimEvent].eventData[AED_SABER_SWING_PROBABILITY] = animEvents[curAnimEvent].eventData[AED_SOUND_PROBABILITY];
				if ( lowestVal < 4 )
				{//fast swing
					animEvents[curAnimEvent].eventData[AED_SABER_SWING_TYPE] = SWING_FAST;
				}
				else if ( lowestVal < 7 )
				{//medium swing
					animEvents[curAnimEvent].eventData[AED_SABER_SWING_TYPE] = SWING_MEDIUM;
				}
				else
				{//strong swing
					animEvents[curAnimEvent].eventData[AED_SABER_SWING_TYPE] = SWING_STRONG;
				}
			}
			else if ( !Q_stricmpn( "sound/weapons/saber/saberspin", stringData, 29 ) )
			{//a saber spin
				animEvents[curAnimEvent].eventType = AEV_SABER_SPIN;
				animEvents[curAnimEvent].eventData[AED_SABER_SPIN_SABERNUM] = 0;//since we don't know which one they meant if we're hacking this, always use first saber
				animEvents[curAnimEvent].eventData[AED_SABER_SPIN_PROBABILITY] = animEvents[curAnimEvent].eventData[AED_SOUND_PROBABILITY];
				if ( stringData[29] == 'o' )
				{//saberspinoff
					animEvents[curAnimEvent].eventData[AED_SABER_SPIN_TYPE] = 0;
				}
				else if ( stringData[29] == '1' )
				{//saberspin1
					animEvents[curAnimEvent].eventData[AED_SABER_SPIN_TYPE] = 2;
				}
				else if ( stringData[29] == '2' )
				{//saberspin2
					animEvents[curAnimEvent].eventData[AED_SABER_SPIN_TYPE] = 3;
				}
				else if ( stringData[29] == '3' )
				{//saberspin3
					animEvents[curAnimEvent].eventData[AED_SABER_SPIN_TYPE] = 4;
				}
				else if ( stringData[29] == '%' )
				{//saberspin%d
					animEvents[curAnimEvent].eventData[AED_SABER_SPIN_TYPE] = 5;
				}
				else
				{//just plain saberspin
					animEvents[curAnimEvent].eventData[AED_SABER_SPIN_TYPE] = 1;
				}
			}
			break;
		case AEV_FOOTSTEP:		//# animID AEV_FOOTSTEP framenum footstepType
			//get footstep type
			token = COM_Parse( text_p );
			if ( !token )
			{
				break;
			}
			animEvents[curAnimEvent].eventData[AED_FOOTSTEP_TYPE] = GetIDForString(footstepTypeTable, token);
			//get probability
			token = COM_Parse( text_p );
			if ( !token )
			{//WARNING!  BAD TABLE!
				break;
			}
			animEvents[curAnimEvent].eventData[AED_FOOTSTEP_PROBABILITY] = atoi( token );
			break;
		case AEV_EFFECT:		//# animID AEV_EFFECT framenum effectpath boltName
			//get effect index
			token = COM_Parse( text_p );
			if ( !token )
			{
				break;
			}
			if ( token[0] && Q_stricmp( "special", token ) == 0 )
			{//special hard-coded effects
				//let cgame know it's not a real effect
				animEvents[curAnimEvent].eventData[AED_EFFECTINDEX] = -1;
				//get the name of it
				token = COM_Parse( text_p );
				animEvents[curAnimEvent].stringData = G_NewString( token );
			}
			else
			{//regular effect
				tempVal = G_EffectIndex(token);
				assert(tempVal > -32767 && tempVal < 32767);
				animEvents[curAnimEvent].eventData[AED_EFFECTINDEX] = tempVal;
				//get bolt index
				token = COM_Parse( text_p );
				if ( !token )
				{
					break;
				}
				if ( Q_stricmp( "none", token ) != 0 && Q_stricmp( "NULL", token ) != 0 )
				{//actually are specifying a bolt to use
					animEvents[curAnimEvent].stringData = G_NewString( token );
				}
			}
			//NOTE: this string will later be used to add a bolt and store the index, as below:
			//animEvent->eventData[AED_BOLTINDEX] = gi.G2API_AddBolt( &cent->gent->ghoul2[cent->gent->playerModel], animEvent->stringData );
			//get probability
			token = COM_Parse( text_p );
			if ( !token )
			{//WARNING!  BAD TABLE!
				break;
			}
			animEvents[curAnimEvent].eventData[AED_EFFECT_PROBABILITY] = atoi( token );
			break;
		case AEV_FIRE:			//# animID AEV_FIRE framenum altfire chancetofire
			//get altfire
			token = COM_Parse( text_p );
			if ( !token )
			{//WARNING!  BAD TABLE!
				break;
			}
			animEvents[curAnimEvent].eventData[AED_FIRE_ALT] = atoi( token );
			//get probability
			token = COM_Parse( text_p );
			if ( !token )
			{//WARNING!  BAD TABLE!
				break;
			}
			animEvents[curAnimEvent].eventData[AED_FIRE_PROBABILITY] = atoi( token );
			break;
		case AEV_MOVE:			//# animID AEV_MOVE framenum forwardpush rightpush uppush
			//get forward push
			token = COM_Parse( text_p );
			if ( !token )
			{//WARNING!  BAD TABLE!
				break;
			}
			tempVal = atoi(token);
			assert(tempVal > -32767 && tempVal < 32767);
			animEvents[curAnimEvent].eventData[AED_MOVE_FWD] = tempVal;
			//get right push
			token = COM_Parse( text_p );
			if ( !token )
			{//WARNING!  BAD TABLE!
				break;
			}
			tempVal = atoi(token);
			assert(tempVal > -32767 && tempVal < 32767);
			animEvents[curAnimEvent].eventData[AED_MOVE_RT] = tempVal;
			//get upwards push
			token = COM_Parse( text_p );
			if ( !token )
			{//WARNING!  BAD TABLE!
				break;
			}
			tempVal = atoi(token);
			assert(tempVal > -32767 && tempVal < 32767);
			animEvents[curAnimEvent].eventData[AED_MOVE_UP] = tempVal;
			break;
		default:				//unknown?
			SkipRestOfLine( text_p );
			continue;
			break;
		}

		if ( curAnimEvent == lastAnimEvent )
		{
			lastAnimEvent++;
		}
	}
}



/*
======================
G_ParseAnimationEvtFile

Read a configuration file containing animation events
models/players/kyle/animevents.cfg, etc

This file's presence is not required

======================
*/
static
void G_ParseAnimationEvtFile(int glaIndex, const char* eventsDirectory, int fileIndex, int iRealGLAIndex = -1, bool modelSpecific = false)
{
	int				len;
	const char*		token;
	char			text[80000];
	const char*		text_p = text;
	fileHandle_t	f;
	char			eventsPath[MAX_QPATH];
	int				modelIndex = 0;

	assert(fileIndex>=0 && fileIndex<MAX_ANIM_FILES);

	const char *psAnimFileInternalName = (iRealGLAIndex == -1 ? NULL : gi.G2API_GetAnimFileInternalNameIndex( iRealGLAIndex ));
	bool bIsFrameSkipped = (psAnimFileInternalName && strlen(psAnimFileInternalName)>5 && !Q_stricmp(&psAnimFileInternalName[strlen(psAnimFileInternalName)-5],"_skip"));

	// Open The File, Make Sure It Is Safe
	//-------------------------------------
	Com_sprintf(eventsPath, MAX_QPATH, "models/players/%s/animevents.cfg", eventsDirectory);
	len = cgi_FS_FOpenFile(eventsPath, &f, FS_READ);
	if ( len <= 0 )
	{//no file
		return;
	}
	if ( len >= (int)(sizeof( text ) - 1) )
	{
		cgi_FS_FCloseFile( f );
		CG_Printf( "File %s too long\n", eventsPath );
		return;
	}

	// Read It To The Buffer, Close The File
	//---------------------------------------
	cgi_FS_Read( text, len, f );
	text[len] = 0;
	cgi_FS_FCloseFile( f );


	// Get The Pointers To The Anim Event Arrays
	//-------------------------------------------
	animFileSet_t&	afileset		= level.knownAnimFileSets[fileIndex];
	animevent_t	*legsAnimEvents		= afileset.legsAnimEvents;
	animevent_t	*torsoAnimEvents	= afileset.torsoAnimEvents;
	animation_t	*animations			= afileset.animations;


	if (modelSpecific)
	{
		hstring		modelName(eventsDirectory);
		modelIndex = modelName.handle();
	}


	// read information for batches of sounds (UPPER or LOWER)
	COM_BeginParseSession();
	while ( 1 )
	{
		// Get base frame of sequence
		token = COM_Parse( &text_p );
		if ( !token || !token[0] )
		{
			break;
		}

		//these stomp anything set in the include file (if it's an event of the same type on the same frame)!
		if ( !Q_stricmp(token,"UPPEREVENTS") )	// A batch of upper events
		{
			ParseAnimationEvtBlock(glaIndex, modelIndex, eventsPath, torsoAnimEvents, animations, afileset.torsoAnimEventCount, &text_p, bIsFrameSkipped);
		}

		else if ( !Q_stricmp(token,"LOWEREVENTS") )	// A batch of lower events
		{
			ParseAnimationEvtBlock(glaIndex, modelIndex, eventsPath, legsAnimEvents, animations, afileset.legsAnimEventCount, &text_p, bIsFrameSkipped);
		}
	}
	COM_EndParseSession();
}

/*
======================
G_ParseAnimationFile

Read a configuration file containing animation coutns and rates
models/players/visor/animation.cfg, etc

======================
*/
qboolean G_ParseAnimationFile(int glaIndex, const char *skeletonName, int fileIndex)
{
	char			text[80000];
	int				len			= 0;
	const char		*token		= 0;
	float			fps			= 0;
	const char*		text_p		= text;
	int				animNum		= 0;
	animation_t*	animations	= level.knownAnimFileSets[fileIndex].animations;
	char			skeletonPath[MAX_QPATH];


	// Read In The File To The Text Buffer, Make Sure Everything Is Safe To Continue
	//-------------------------------------------------------------------------------
	Com_sprintf(skeletonPath, MAX_QPATH, "models/players/%s/%s.cfg", skeletonName, skeletonName);
	len = gi.RE_GetAnimationCFG(skeletonPath, text, sizeof(text));
	if ( len <= 0 )
	{
		Com_sprintf(skeletonPath, MAX_QPATH, "models/players/%s/animation.cfg", skeletonName);
		len = gi.RE_GetAnimationCFG(skeletonPath, text, sizeof(text));
		if ( len <= 0 )
		{
			return qfalse;
		}
	}
	if ( len >= (int)(sizeof( text ) - 1) )
	{
		G_Error( "G_ParseAnimationFile: File %s too long\n (%d > %d)", skeletonName, len, sizeof( text ) - 1);
		return qfalse;
	}



	// Read In Each Token
	//--------------------
	COM_BeginParseSession();
	while(1)
	{
		token = COM_Parse( &text_p );

		// If No Token, We've Reached The End Of The File
		//------------------------------------------------
		if ( !token || !token[0])
		{
			break;
		}

		// Get The Anim Number Converted From The First Token
		//----------------------------------------------------
		animNum = GetIDForString(animTable, token);
		if(animNum == -1)
		{
#ifndef FINAL_BUILD
			if (strcmp(token,"ROOT"))
			{
				Com_Printf(S_COLOR_RED"WARNING: Unknown token %s in %s\n", token, skeletonPath);
			}
#endif
			//unrecognized animation so skip to end of line,
			while (token[0])
			{
				token = COM_ParseExt( &text_p, qfalse );	//returns empty string when next token is EOL
			}
			continue;
		}

		// GLAIndex
		//----------
		animations[animNum].glaIndex = glaIndex;	// Passed Into This Func

		// First Frame
		//-------------
		token = COM_Parse( &text_p );
		if ( !token )
		{
			break;
		}
		assert(atoi(token) >= 0 && atoi(token) < 65536);
		animations[animNum].firstFrame = atoi( token );

		// Num Frames
		//------------
		token = COM_Parse( &text_p );
		if ( !token )
		{
			break;
		}
		assert(atoi(token) >= 0 && atoi(token) < 65536);
		animations[animNum].numFrames = atoi( token );

		// Loop Frames
		//-------------
		token = COM_Parse( &text_p );
		if ( !token )
		{
			break;
		}
		assert(atoi(token) >= -1 && atoi(token) < 128);
		animations[animNum].loopFrames = atoi( token );

		// FPS
		//-----
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

		// Calculate Frame Lerp
		//----------------------
		int lerp;
		if ( fps < 0 )
		{//backwards
			lerp = floor(1000.0f / fps);
			assert(lerp > -32767 && lerp < 32767);
			animations[animNum].frameLerp = lerp;
			assert(animations[animNum].frameLerp <= 1);
		}
		else
		{
			lerp = ceil(1000.0f / fps);
			assert(lerp > -32767 && lerp < 32767);
			animations[animNum].frameLerp = lerp;
			assert(animations[animNum].frameLerp >= 1);
		}

/*		lerp = ceil(1000.0f / Q_fabs(fps));
		assert(lerp > -32767 && lerp < 32767);
		animations[animNum].initialLerp = lerp;
*/
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




////////////////////////////////////////////////////////////////////////
// G_ParseAnimFileSet
//
// This function is responsible for building the animation file and
// the animation event file.
//
////////////////////////////////////////////////////////////////////////
int		G_ParseAnimFileSet(const char *skeletonName, const char *modelName=0)
{
	int			fileIndex=0;


	// Try To Find An Existing Skeleton File For This Animation Set
	//--------------------------------------------------------------
	for (fileIndex=0; fileIndex<level.numKnownAnimFileSets; fileIndex++)
	{
		if (Q_stricmp(level.knownAnimFileSets[fileIndex].filename, skeletonName)==0)
		{
			break;
		}
	}

	// Ok, We Don't Already Have One, So Time To Create A New One And Initialize It
	//------------------------------------------------------------------------------
	if (fileIndex>=level.numKnownAnimFileSets)
	{
		if (level.numKnownAnimFileSets==MAX_ANIM_FILES)
		{
			G_Error( "G_ParseAnimFileSet: MAX_ANIM_FILES" );
			return -1;
		}

		// Allocate A new File Set, And Get Some Shortcut Pointers
		//---------------------------------------------------------
		fileIndex = level.numKnownAnimFileSets;
		level.numKnownAnimFileSets++;
		strcpy(level.knownAnimFileSets[fileIndex].filename, skeletonName);

		level.knownAnimFileSets[fileIndex].torsoAnimEventCount = 0;
		level.knownAnimFileSets[fileIndex].legsAnimEventCount = 0;

		animation_t*	animations		= level.knownAnimFileSets[fileIndex].animations;
		animevent_t*	legsAnimEvents	= level.knownAnimFileSets[fileIndex].legsAnimEvents;
		animevent_t*	torsoAnimEvents	= level.knownAnimFileSets[fileIndex].torsoAnimEvents;

		int i, j;

		// Initialize The Frames Information
		//-----------------------------------
		for(i=0; i<MAX_ANIMATIONS; i++)
		{
			animations[i].firstFrame	= 0;
			animations[i].numFrames		= 0;
			animations[i].loopFrames	= -1;
			animations[i].frameLerp		= 100;
//			animations[i].initialLerp	= 100;
			animations[i].glaIndex		= 0;
		}

		// Initialize Animation Event Array
		//----------------------------------
		for(i=0; i<MAX_ANIM_EVENTS; i++)
		{
			torsoAnimEvents[i].eventType	= AEV_NONE;		//Type of event
			legsAnimEvents[i].eventType		= AEV_NONE;
			torsoAnimEvents[i].keyFrame		= (unsigned short)-1;	//65535 should never be a valid frame... :)
			legsAnimEvents[i].keyFrame		= (unsigned short)-1;	//Frame to play event on
			torsoAnimEvents[i].stringData	= NULL;			//we allow storage of one string, temporarily (in case we have to look up an index later,
			legsAnimEvents[i].stringData	= NULL;			//then make sure to set stringData to NULL so we only do the look-up once)
			torsoAnimEvents[i].modelOnly	= 0;
			legsAnimEvents[i].modelOnly		= 0;
			torsoAnimEvents[i].glaIndex		= 0;
			legsAnimEvents[i].glaIndex		= 0;


			for (j=0; j<AED_ARRAY_SIZE; j++)				//Unique IDs, can be soundIndex of sound file to play OR effect index or footstep type, etc.
			{
				torsoAnimEvents[i].eventData[j] = -1;
				legsAnimEvents[i].eventData[j]	= -1;
			}
		}

		// Get The Cinematic GLA Name
		//----------------------------
		if (Q_stricmp(skeletonName, "_humanoid")==0)
		{
			const char* mapName = strrchr( level.mapname, '/' );
			if (mapName)
			{
				mapName++;
			}
			else
			{
				mapName = level.mapname;
			}
			char  skeletonMapName[MAX_QPATH];
			Com_sprintf(skeletonMapName, MAX_QPATH, "_humanoid_%s", mapName);
			const int normalGLAIndex = gi.G2API_PrecacheGhoul2Model("models/players/_humanoid/_humanoid.gla");//double check this always comes first!

			// Make Sure To Precache The GLAs (both regular and cinematic), And Remember Their Indicies
			//------------------------------------------------------------------------------------------
			G_ParseAnimationFile(0,    skeletonName, fileIndex);
			G_ParseAnimationEvtFile(0, skeletonName, fileIndex, normalGLAIndex, false/*flag for model specific*/);

			const int cineGLAIndex = gi.G2API_PrecacheGhoul2Model( va("models/players/%s/%s.gla", skeletonMapName, skeletonMapName));
			if (cineGLAIndex)
			{
				assert(cineGLAIndex == normalGLAIndex+1);
				if (cineGLAIndex != normalGLAIndex+1)
				{
					Com_Error(ERR_DROP,"Cinematic GLA was not loaded after the normal GLA.  Cannot continue safely.");
				}
				G_ParseAnimationFile(1,    skeletonMapName, fileIndex);
				G_ParseAnimationEvtFile(1, skeletonMapName, fileIndex, cineGLAIndex, false/*flag for model specific*/);
			}
		}
		else
		{
			// non-humanoid...
			//
			// Make Sure To Precache The GLAs (both regular and cinematic), And Remember Their Indicies
			//------------------------------------------------------------------------------------------
			G_ParseAnimationFile(0,    skeletonName, fileIndex);
			G_ParseAnimationEvtFile(0, skeletonName, fileIndex);
		}
	}

	// Tack Any Additional Per Model Events
	//--------------------------------------
	if (modelName)
	{
		// Quick Search To See If We've Already Loaded This Model
		//--------------------------------------------------------
		int			curModel=0;
		hstring		curModelName(modelName);
		while (curModel<MAX_MODELS_PER_LEVEL && !modelsAlreadyDone[curModel].empty())
		{
			if (modelsAlreadyDone[curModel]==curModelName)
			{
				return fileIndex;
			}
			curModel++;
		}

		if (curModel == MAX_MODELS_PER_LEVEL)
		{
			Com_Error(ERR_DROP, "About to overflow modelsAlreadyDone, increase MAX_MODELS_PER_LEVEL\n");
		}

		// Nope, Ok, Record The Model As Found And Parse It's Event File
		//---------------------------------------------------------------
		modelsAlreadyDone[curModel]=curModelName;

		// Only Do The Event File If The Model Is Not The Same As The Skeleton
		//---------------------------------------------------------------------
		if (Q_stricmp(skeletonName, modelName)!=0)
		{
			const int iGLAIndexToCheckForSkip = (Q_stricmp(skeletonName, "_humanoid")?-1:gi.G2API_PrecacheGhoul2Model("models/players/_humanoid/_humanoid.gla")); // ;-)

			G_ParseAnimationEvtFile(0, modelName, fileIndex, iGLAIndexToCheckForSkip, true);
		}
	}

	return fileIndex;
}

extern cvar_t	*g_char_model;
void G_LoadAnimFileSet( gentity_t *ent, const char *pModelName )
{
//load its animation config
	char	animName[MAX_QPATH];
	char	*GLAName, *modelName;
	char	*slash = NULL;
	char	*strippedName;

	if ( ent->playerModel == -1 )
	{
		return;
	}
	if ( Q_stricmp( "player", pModelName ) == 0 )
	{//model is actually stored on console
		modelName = g_char_model->string;
	}
	else
	{
		modelName = (char *)pModelName;
	}
	//get the location of the animation.cfg
	GLAName = gi.G2API_GetGLAName( &ent->ghoul2[ent->playerModel] );
	//now load and parse the animation.cfg, animevents.cfg and set the animFileIndex
	if ( !GLAName)
	{
		Com_Printf( S_COLOR_RED"Failed find animation file name models/players/%s\n", modelName );
		strippedName="_humanoid";	//take a guess, maybe it's right?
	}
	else
	{
		Q_strncpyz( animName, GLAName, sizeof( animName ) );
		slash = strrchr( animName, '/' );
		if ( slash )
		{
			*slash = 0;
		}
		strippedName = COM_SkipPath( animName );
	}

	//now load and parse the animation.cfg, animevents.cfg and set the animFileIndex
	ent->client->clientInfo.animFileIndex = G_ParseAnimFileSet(strippedName, modelName);

	if (ent->client->clientInfo.animFileIndex<0)
	{
		Com_Printf( S_COLOR_RED"Failed to load animation file set models/players/%s/animation.cfg\n", modelName );
#ifndef FINAL_BUILD
		Com_Error(ERR_FATAL, "Failed to load animation file set models/players/%s/animation.cfg\n", modelName);
#endif
	}
}


void NPC_PrecacheAnimationCFG( const char *NPC_type )
{
	char	filename[MAX_QPATH];
	const char	*token;
	const char	*value;
	const char	*p;

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
			Q_strncpyz( filename, value, sizeof( filename ) );
			G_ParseAnimFileSet( filename );
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
					Q_strncpyz( animName, GLAName, sizeof( animName ) );
					slash = strrchr( animName, '/' );
					if ( slash )
					{
						*slash = 0;
					}
					strippedName = COM_SkipPath( animName );

					//must copy data out of this pointer into a different part of memory because the funcs we're about to call will call COM_ParseExt
					Q_strncpyz( filename, value, sizeof( filename ) );

					G_ParseAnimFileSet(strippedName, filename);
					COM_EndParseSession(  );
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
			gi.G2API_PrecacheGhoul2Model( weaponModel ); // correct way is item->world_model
		}
	}
}

/*
void NPC_PrecacheByClassName ( char *NPCName )

This runs all the class specific precache functions

*/

extern void NPC_ShadowTrooper_Precache( void );
extern void NPC_Gonk_Precache( void );
extern void NPC_Mouse_Precache( void );
extern void NPC_Seeker_Precache( void );
extern void NPC_Remote_Precache( void );
extern void	NPC_R2D2_Precache(void);
extern void	NPC_R5D2_Precache(void);
extern void NPC_Probe_Precache(void);
extern void NPC_Interrogator_Precache(gentity_t *self);
extern void NPC_MineMonster_Precache( void );
extern void NPC_Howler_Precache( void );
extern void NPC_Rancor_Precache( void );
extern void NPC_MutantRancor_Precache( void );
extern void NPC_Wampa_Precache( void );
extern void NPC_ATST_Precache(void);
extern void NPC_Sentry_Precache(void);
extern void NPC_Mark1_Precache(void);
extern void NPC_Mark2_Precache(void);
extern void NPC_Protocol_Precache( void );
extern void Boba_Precache( void );
extern void RT_Precache( void );
extern void SandCreature_Precache( void );
extern void NPC_TavionScepter_Precache( void );
extern void NPC_TavionSithSword_Precache( void );
extern void NPC_Rosh_Dark_Precache( void );
extern void NPC_Tusken_Precache( void );
extern void NPC_Saboteur_Precache( void );
extern void NPC_CultistDestroyer_Precache( void );
void NPC_Jawa_Precache( void )
{
	for ( int i = 1; i < 7; i++ )
	{
		G_SoundIndex( va( "sound/chars/jawa/misc/chatter%d.wav", i ) );
	}
	G_SoundIndex( "sound/chars/jawa/misc/ooh-tee-nee.wav" );
}

void NPC_PrecacheByClassName( const char* type )
{
	if (!type || !type[0])
	{
		return;
	}

	if ( !Q_stricmp( "gonk", type))
	{
		NPC_Gonk_Precache();
	}
	else if ( !Q_stricmp( "mouse", type))
	{
		NPC_Mouse_Precache();
	}
	else if ( !Q_stricmpn( "r2d2", type, 4))
	{
		NPC_R2D2_Precache();
	}
	else if ( !Q_stricmp( "atst", type))
	{
		NPC_ATST_Precache();
	}
	else if ( !Q_stricmpn( "r5d2", type, 4))
	{
		NPC_R5D2_Precache();
	}
	else if ( !Q_stricmp( "mark1", type))
	{
		NPC_Mark1_Precache();
	}
	else if ( !Q_stricmp( "mark2", type))
	{
		NPC_Mark2_Precache();
	}
	else if ( !Q_stricmp( "interrogator", type))
	{
		NPC_Interrogator_Precache(NULL);
	}
	else if ( !Q_stricmp( "probe", type))
	{
		NPC_Probe_Precache();
	}
	else if ( !Q_stricmp( "seeker", type))
	{
		NPC_Seeker_Precache();
	}
	else if ( !Q_stricmpn( "remote", type, 6))
	{
		NPC_Remote_Precache();
	}
	else if ( !Q_stricmpn( "shadowtrooper", type, 13 ) )
	{
		NPC_ShadowTrooper_Precache();
	}
	else if ( !Q_stricmp( "minemonster", type ))
	{
		NPC_MineMonster_Precache();
	}
	else if ( !Q_stricmp( "howler", type ))
	{
		NPC_Howler_Precache();
	}
	else if ( !Q_stricmp( "rancor", type ))
	{
		NPC_Rancor_Precache();
	}
	else if ( !Q_stricmp( "mutant_rancor", type ))
	{
		NPC_Rancor_Precache();
		NPC_MutantRancor_Precache();
	}
	else if ( !Q_stricmp( "wampa", type ))
	{
		NPC_Wampa_Precache();
	}
	else if ( !Q_stricmp( "sand_creature", type ))
	{
		SandCreature_Precache();
	}
	else if ( !Q_stricmp( "sentry", type ))
	{
		NPC_Sentry_Precache();
	}
	else if ( !Q_stricmp( "protocol", type ))
	{
		NPC_Protocol_Precache();
	}
	else if ( !Q_stricmp( "boba_fett", type ))
	{
		Boba_Precache();
	}
	else if ( !Q_stricmp( "rockettrooper2", type ))
	{
		RT_Precache();
	}
	else if ( !Q_stricmp( "rockettrooper2Officer", type ))
	{
		RT_Precache();
	}
	else if ( !Q_stricmp( "tavion_scepter", type ))
	{
		NPC_TavionScepter_Precache();
	}
	else if ( !Q_stricmp( "tavion_sith_sword", type ))
	{
		NPC_TavionSithSword_Precache();
	}
	else if ( !Q_stricmp( "rosh_dark", type ) )
	{
		NPC_Rosh_Dark_Precache();
	}
	else if ( !Q_stricmpn( "tusken", type, 6 ) )
	{
		NPC_Tusken_Precache();
	}
	else if ( !Q_stricmpn( "saboteur", type, 8 ) )
	{
		NPC_Saboteur_Precache();
	}
	else if ( !Q_stricmp( "cultist_destroyer", type ) )
	{
		NPC_CultistDestroyer_Precache();
	}
	else if ( !Q_stricmpn( "jawa", type, 4 ) )
	{
		NPC_Jawa_Precache();
	}
}



/*
void NPC_Precache ( char *NPCName )

Precaches NPC skins, tgas and md3s.

*/
void CG_NPC_Precache ( gentity_t *spawner )
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
	char	playerModel[MAX_QPATH] = { 0 };
	char	customSkin[MAX_QPATH];

	if ( !Q_stricmp( "random", spawner->NPC_type ) )
	{//sorry, can't precache a random just yet
		return;
	}

	strcpy(customSkin,"default");

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
				Q_strncpyz( ri.headModelName, value, sizeof(ri.headModelName));
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
				Q_strncpyz( ri.torsoModelName, value, sizeof(ri.torsoModelName));
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
			Q_strncpyz( ri.legsModelName, value, sizeof(ri.legsModelName));
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
			if ( COM_ParseString( &p, &value ) )
			{
				continue;
			}
			playerTeam = (team_t)GetIDForString( TeamTable, token );
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

		//cache weapons
		if ( !Q_stricmp( token, "weapon" ) )
		{
			if ( COM_ParseString( &p, &value ) )
			{
				continue;
			}
			int weap = GetIDForString( WPTable, value );
			if ( weap >= WP_NONE && weap < WP_NUM_WEAPONS )
			{
				if ( weap > WP_NONE )
				{
					RegisterItem( FindItemForWeapon( (weapon_t)(weap) ) );	//precache the weapon
				}
			}
			continue;
		}
		//cache sabers
		//saber name
		if ( !Q_stricmp( token, "saber" ) )
		{
			if ( COM_ParseString( &p, &value ) )
			{
				continue;
			}
			char *saberName = G_NewString( value );
			saberInfo_t saber;
			WP_SaberParseParms( saberName, &saber );
			if ( saber.model && saber.model[0] )
			{
				G_ModelIndex( saber.model );
			}
			if ( saber.skin && saber.skin[0] )
			{
				gi.RE_RegisterSkin( saber.skin );
				G_SkinIndex( saber.skin );
			}
			if ( saber.g2MarksShader[0] )
			{
				cgi_R_RegisterShader( saber.g2MarksShader );
			}
			if ( saber.g2MarksShader2[0] )
			{
				cgi_R_RegisterShader( saber.g2MarksShader2 );
			}
			if ( saber.g2WeaponMarkShader[0] )
			{
				cgi_R_RegisterShader( saber.g2WeaponMarkShader );
			}
			if ( saber.g2WeaponMarkShader2[0] )
			{
				cgi_R_RegisterShader( saber.g2WeaponMarkShader2 );
			}
			continue;
		}

		//second saber name
		if ( !Q_stricmp( token, "saber2" ) )
		{
			if ( COM_ParseString( &p, &value ) )
			{
				continue;
			}
			char *saberName = G_NewString( value );
			saberInfo_t saber;
			WP_SaberParseParms( saberName, &saber );
			if ( saber.model && saber.model[0] )
			{
				G_ModelIndex( saber.model );
			}
			if ( saber.skin && saber.skin[0] )
			{
				gi.RE_RegisterSkin( saber.skin );
				G_SkinIndex( saber.skin );
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
		if (strchr(customSkin, '|'))
		{//three part skin
			Com_sprintf( skinName, sizeof( skinName ), "models/players/%s/|%s", playerModel, customSkin );
		}
		else
		{//standard skin
			Com_sprintf( skinName, sizeof( skinName ), "models/players/%s/model_%s.skin", playerModel, customSkin );
		}
		// lets see if it's out there
		gi.RE_RegisterSkin( skinName );
	}

	//precache this NPC's possible weapons
	NPC_PrecacheWeapons( playerTeam, spawner->spawnflags, spawner->NPC_type );

	// Anything else special about them
	NPC_PrecacheByClassName( spawner->NPC_type );

	CG_RegisterNPCCustomSounds( &ci );

	//CG_RegisterNPCEffects( playerTeam );
	//FIXME: Look for a "sounds" directory and precache death, pain, alert sounds
}

void NPC_BuildRandom( gentity_t *NPC )
{
}

extern void G_MatchPlayerWeapon( gentity_t *ent );
extern void G_InitPlayerFromCvars( gentity_t *ent );
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
	char	surfOff[1024]={0};
	char	surfOn[1024]={0};
	qboolean parsingPlayer = qfalse;

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
		stats->sex			= SEX_MALE;
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
		if (g_entities[ENTITYNUM_WORLD].max_health && stats->visrange>g_entities[ENTITYNUM_WORLD].max_health)
		{
			stats->visrange	= g_entities[ENTITYNUM_WORLD].max_health;
		}
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

	Q_strncpyz( ci->name, NPCName, sizeof( ci->name ) );

	NPC->playerModel = -1;

	//Set defaults
	//FIXME: should probably put default torso and head models, but what about enemies
	//that don't have any- like Stasis?
	//Q_strncpyz( ri->headModelName,	DEFAULT_HEADMODEL,  sizeof(ri->headModelName),	qtrue);
	//Q_strncpyz( ri->torsoModelName, DEFAULT_TORSOMODEL, sizeof(ri->torsoModelName),	qtrue);
	//Q_strncpyz( ri->legsModelName,	DEFAULT_LEGSMODEL,  sizeof(ri->legsModelName),	qtrue);
	ri->headModelName[0] = 0;
	ri->torsoModelName[0]= 0;
	ri->legsModelName[0] =0;

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

	NPC->client->moveType		= MT_RUNJUMP;

	NPC->client->dismemberProbHead = 100;
	NPC->client->dismemberProbArms = 100;
	NPC->client->dismemberProbHands = 100;
	NPC->client->dismemberProbWaist = 100;
	NPC->client->dismemberProbLegs = 100;

	NPC->s.modelScale[0] = NPC->s.modelScale[1] = NPC->s.modelScale[2] = 1.0f;

	ri->customRGBA[0] = ri->customRGBA[1] = ri->customRGBA[2] = ri->customRGBA[3] = 0xFFu;

	if ( !Q_stricmp( "random", NPCName ) )
	{//Randomly assemble an NPC
		NPC_BuildRandom( NPC );
	}
	else
	{
		p = NPCParms;
		COM_BeginParseSession();
#ifdef _WIN32
#pragma region(NPC Stats)
#endif
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
			// custom color
			if ( !Q_stricmp( token, "customRGBA" ) )
			{

				// eezstreet TODO: Put these into functions, damn it! They're too big!

				if ( COM_ParseString( &p, &value ) )
				{
					continue;
				}

				if ( !Q_stricmp( value, "random") )
				{
					ri->customRGBA[0]=Q_irand(0,255);
					ri->customRGBA[1]=Q_irand(0,255);
					ri->customRGBA[2]=Q_irand(0,255);
					ri->customRGBA[3]=255;
				}
				else if ( !Q_stricmp( value, "random1") )
				{
					ri->customRGBA[3]=255;
					switch (Q_irand(0,5))
					{
					default:
					case 0:
						ri->customRGBA[0]=127;
						ri->customRGBA[1]=153;
						ri->customRGBA[2]=255;
						break;
					case 1:
						ri->customRGBA[0]=177;
						ri->customRGBA[1]=29;
						ri->customRGBA[2]=13;
						break;
					case 2:
						ri->customRGBA[0]=47;
						ri->customRGBA[1]=90;
						ri->customRGBA[2]=40;
						break;
					case 3:
						ri->customRGBA[0]=181;
						ri->customRGBA[1]=207;
						ri->customRGBA[2]=255;
						break;
					case 4:
						ri->customRGBA[0]=138;
						ri->customRGBA[1]=83;
						ri->customRGBA[2]=0;
						break;
					case 5:
						ri->customRGBA[0]=254;
						ri->customRGBA[1]=199;
						ri->customRGBA[2]=14;
						break;
					}
				}
				else if ( !Q_stricmp( value, "jedi_hf" ) )
				{
					ri->customRGBA[3]=255;
					switch (Q_irand(0,7))
					{
					default:
					case 0://red1
						ri->customRGBA[0]=165;
						ri->customRGBA[1]=48;
						ri->customRGBA[2]=21;
						break;
					case 1://yellow1
						ri->customRGBA[0]=254;
						ri->customRGBA[1]=230;
						ri->customRGBA[2]=132;
						break;
					case 2://bluegray
						ri->customRGBA[0]=181;
						ri->customRGBA[1]=207;
						ri->customRGBA[2]=255;
						break;
					case 3://pink
						ri->customRGBA[0]=233;
						ri->customRGBA[1]=183;
						ri->customRGBA[2]=208;
						break;
					case 4://lt blue
						ri->customRGBA[0]=161;
						ri->customRGBA[1]=226;
						ri->customRGBA[2]=240;
						break;
					case 5://blue
						ri->customRGBA[0]=101;
						ri->customRGBA[1]=159;
						ri->customRGBA[2]=255;
						break;
					case 6://orange
						ri->customRGBA[0]=255;
						ri->customRGBA[1]=157;
						ri->customRGBA[2]=114;
						break;
					case 7://violet
						ri->customRGBA[0]=216;
						ri->customRGBA[1]=160;
						ri->customRGBA[2]=255;
						break;
					}
				}
				else if ( !Q_stricmp( value, "jedi_hm" ) )
				{
					ri->customRGBA[3]=255;
					switch (Q_irand(0,7))
					{
					default:
					case 0://yellow
						ri->customRGBA[0]=252;
						ri->customRGBA[1]=243;
						ri->customRGBA[2]=180;
						break;
					case 1://blue
						ri->customRGBA[0]=69;
						ri->customRGBA[1]=109;
						ri->customRGBA[2]=255;
						break;
					case 2://gold
						ri->customRGBA[0]=254;
						ri->customRGBA[1]=197;
						ri->customRGBA[2]=73;
						break;
					case 3://orange
						ri->customRGBA[0]=178;
						ri->customRGBA[1]=78;
						ri->customRGBA[2]=18;
						break;
					case 4://bluegreen
						ri->customRGBA[0]=112;
						ri->customRGBA[1]=153;
						ri->customRGBA[2]=161;
						break;
					case 5://blue2
						ri->customRGBA[0]=123;
						ri->customRGBA[1]=182;
						ri->customRGBA[2]=255;
						break;
					case 6://green2
						ri->customRGBA[0]=0;
						ri->customRGBA[1]=88;
						ri->customRGBA[2]=105;
						break;
					case 7://violet
						ri->customRGBA[0]=138;
						ri->customRGBA[1]=0;
						ri->customRGBA[2]=0;
						break;
					}
				}
				else if ( !Q_stricmp( value, "jedi_kdm" ) )
				{
					ri->customRGBA[3]=255;
					switch (Q_irand(0,8))
					{
					default:
					case 0://blue
						ri->customRGBA[0]=85;
						ri->customRGBA[1]=120;
						ri->customRGBA[2]=255;
						break;
					case 1://violet
						ri->customRGBA[0]=173;
						ri->customRGBA[1]=142;
						ri->customRGBA[2]=219;
						break;
					case 2://brown1
						ri->customRGBA[0]=254;
						ri->customRGBA[1]=197;
						ri->customRGBA[2]=73;
						break;
					case 3://orange
						ri->customRGBA[0]=138;
						ri->customRGBA[1]=83;
						ri->customRGBA[2]=0;
						break;
					case 4://gold
						ri->customRGBA[0]=254;
						ri->customRGBA[1]=199;
						ri->customRGBA[2]=14;
						break;
					case 5://blue2
						ri->customRGBA[0]=68;
						ri->customRGBA[1]=194;
						ri->customRGBA[2]=217;
						break;
					case 6://red1
						ri->customRGBA[0]=170;
						ri->customRGBA[1]=3;
						ri->customRGBA[2]=30;
						break;
					case 7://yellow1
						ri->customRGBA[0]=225;
						ri->customRGBA[1]=226;
						ri->customRGBA[2]=144;
						break;
					case 8://violet2
						ri->customRGBA[0]=167;
						ri->customRGBA[1]=202;
						ri->customRGBA[2]=255;
						break;
					}
				}
				else if ( !Q_stricmp( value, "jedi_rm" ) )
				{
					ri->customRGBA[3]=255;
					switch (Q_irand(0,8))
					{
					default:
					case 0://blue
						ri->customRGBA[0]=127;
						ri->customRGBA[1]=153;
						ri->customRGBA[2]=255;
						break;
					case 1://green1
						ri->customRGBA[0]=208;
						ri->customRGBA[1]=249;
						ri->customRGBA[2]=85;
						break;
					case 2://blue2
						ri->customRGBA[0]=181;
						ri->customRGBA[1]=207;
						ri->customRGBA[2]=255;
						break;
					case 3://gold
						ri->customRGBA[0]=138;
						ri->customRGBA[1]=83;
						ri->customRGBA[2]=0;
						break;
					case 4://gold
						ri->customRGBA[0]=224;
						ri->customRGBA[1]=171;
						ri->customRGBA[2]=44;
						break;
					case 5://green2
						ri->customRGBA[0]=49;
						ri->customRGBA[1]=155;
						ri->customRGBA[2]=131;
						break;
					case 6://red1
						ri->customRGBA[0]=163;
						ri->customRGBA[1]=79;
						ri->customRGBA[2]=17;
						break;
					case 7://violet2
						ri->customRGBA[0]=148;
						ri->customRGBA[1]=104;
						ri->customRGBA[2]=228;
						break;
					case 8://green3
						ri->customRGBA[0]=138;
						ri->customRGBA[1]=136;
						ri->customRGBA[2]=0;
						break;
					}
				}
				else if ( !Q_stricmp( value, "jedi_tf" ) )
				{
					ri->customRGBA[3]=255;
					switch (Q_irand(0,5))
					{
					default:
					case 0://green1
						ri->customRGBA[0]=255;
						ri->customRGBA[1]=235;
						ri->customRGBA[2]=100;
						break;
					case 1://blue1
						ri->customRGBA[0]=62;
						ri->customRGBA[1]=155;
						ri->customRGBA[2]=255;
						break;
					case 2://red1
						ri->customRGBA[0]=255;
						ri->customRGBA[1]=110;
						ri->customRGBA[2]=120;
						break;
					case 3://purple
						ri->customRGBA[0]=180;
						ri->customRGBA[1]=150;
						ri->customRGBA[2]=255;
						break;
					case 4://flesh
						ri->customRGBA[0]=255;
						ri->customRGBA[1]=200;
						ri->customRGBA[2]=212;
						break;
					case 5://base
						ri->customRGBA[0]=255;
						ri->customRGBA[1]=255;
						ri->customRGBA[2]=255;
						break;
					}
				}
				else if ( !Q_stricmp( value, "jedi_zf" ) )
				{
					ri->customRGBA[3]=255;
					switch (Q_irand(0,7))
					{
					default:
					case 0://red1
						ri->customRGBA[0]=204;
						ri->customRGBA[1]=19;
						ri->customRGBA[2]=21;
						break;
					case 1://orange1
						ri->customRGBA[0]=255;
						ri->customRGBA[1]=107;
						ri->customRGBA[2]=40;
						break;
					case 2://pink1
						ri->customRGBA[0]=255;
						ri->customRGBA[1]=148;
						ri->customRGBA[2]=155;
						break;
					case 3://gold
						ri->customRGBA[0]=255;
						ri->customRGBA[1]=164;
						ri->customRGBA[2]=59;
						break;
					case 4://violet1
						ri->customRGBA[0]=216;
						ri->customRGBA[1]=160;
						ri->customRGBA[2]=255;
						break;
					case 5://blue1
						ri->customRGBA[0]=101;
						ri->customRGBA[1]=159;
						ri->customRGBA[2]=255;
						break;
					case 6://blue2
						ri->customRGBA[0]=161;
						ri->customRGBA[1]=226;
						ri->customRGBA[2]=240;
						break;
					case 7://blue3
						ri->customRGBA[0]=37;
						ri->customRGBA[1]=155;
						ri->customRGBA[2]=181;
						break;
					}
				}
				else
				{
					ri->customRGBA[0]=atoi(value);

					if ( COM_ParseInt( &p, &n ) )
					{
						continue;
					}
					ri->customRGBA[1]=n;

					if ( COM_ParseInt( &p, &n ) )
					{
						continue;
					}
					ri->customRGBA[2]=n;

					if ( COM_ParseInt( &p, &n ) )
					{
						continue;
					}
					ri->customRGBA[3]=n;
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
				ci->animFileIndex = G_ParseAnimFileSet(ri->legsModelName);
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
					Q_strcat( surfOff, sizeof(surfOff), "," );
					Q_strcat( surfOff, sizeof(surfOff), value );
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
					Q_strcat( surfOn, sizeof(surfOn), "," );
					Q_strcat( surfOn, sizeof(surfOn), value );
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
			if ( !parsingPlayer )
			{
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
					if ( n < 1 || n > 180 ) {
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

				// vfov
				if ( !Q_stricmp( token, "vfov" ) ) {
					if ( COM_ParseInt( &p, &n ) ) {
						SkipRestOfLine( &p );
						continue;
					}
					if ( n < 2 || n > 360 ) {
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
						if (g_entities[ENTITYNUM_WORLD].max_health && stats->visrange>g_entities[ENTITYNUM_WORLD].max_health)
						{
							stats->visrange	= g_entities[ENTITYNUM_WORLD].max_health;
						}
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
					gi.Printf( S_COLOR_YELLOW"WARNING: bad %s in NPC '%s'\n", token, NPCName );
					continue;
				}
				if ( NPC->NPC )
				{
					stats->health = n;
				}
				else if ( parsingPlayer )
				{
					NPC->client->ps.stats[STAT_MAX_HEALTH] = NPC->client->pers.maxHealth = NPC->max_health = n;
				}
				continue;
			}

			// fullName
			if ( !Q_stricmp( token, "fullName" ) )
			{
#ifndef FINAL_BUILD
				gi.Printf( S_COLOR_YELLOW"WARNING: fullname ignored in NPC '%s'\n", NPCName );
#endif
				if ( COM_ParseString( &p, &value ) )
				{
					continue;
				}
				continue;
			}

			// playerTeam
			if ( !Q_stricmp( token, "playerTeam" ) )
			{
				if ( COM_ParseString( &p, &value ) )
				{
					continue;
				}
				NPC->client->playerTeam = (team_t)GetIDForString( TeamTable, value );
				continue;
			}

			// enemyTeam
			if ( !Q_stricmp( token, "enemyTeam" ) )
			{
				if ( COM_ParseString( &p, &value ) )
				{
					continue;
				}
				NPC->client->enemyTeam = (team_t)GetIDForString( TeamTable, value );
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

				// No md3's for vehicles.
				if ( NPC->client->NPC_class == CLASS_VEHICLE )
				{
					if ( !NPC->m_pVehicle )
					{//you didn't spawn this guy right!
						Com_Printf ( S_COLOR_RED "ERROR: Tried to spawn a vehicle NPC (%s) without using NPC_Vehicle or 'NPC spawn vehicle <vehiclename>'!!!  Bad, bad, bad!  Shame on you!\n", NPCName );
						COM_EndParseSession();
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

				if ( NPC->client->NPC_class == CLASS_VEHICLE
					&& NPC->m_pVehicle
					&& NPC->m_pVehicle->m_pVehicleInfo
					&& NPC->m_pVehicle->m_pVehicleInfo->type == VH_FIGHTER )
				{//a flying vehicle's origin must be centered in bbox
					NPC->maxs[2] = NPC->client->standheight = (n/2.0f);
					NPC->mins[2] = -NPC->maxs[2];
					NPC->s.origin[2] += (DEFAULT_MINS_2-NPC->mins[2])+0.125f;
					VectorCopy(NPC->s.origin, NPC->client->ps.origin);
					VectorCopy(NPC->s.origin, NPC->currentOrigin);
					G_SetOrigin( NPC, NPC->s.origin );
					gi.linkentity(NPC);
				}
				else
				{
					NPC->mins[2] = DEFAULT_MINS_2;//Cannot change
					NPC->maxs[2] = NPC->client->standheight = n + DEFAULT_MINS_2;
				}
				NPC->s.radius = n;
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

			if ( !parsingPlayer )
			{
				if ( !Q_stricmp( token, "movetype" ) )
				{
					if ( COM_ParseString( &p, &value ) )
					{
						continue;
					}

					NPC->client->moveType = (movetype_t)MoveTypeNameToEnum(value);
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

				//sex
				if ( !Q_stricmp( token, "sex" ) )
				{
					if ( COM_ParseString( &p, &value ) )
					{
						SkipRestOfLine( &p );
						continue;
					}
					if ( value[0] == 'm' )
					{//male
						stats->sex = SEX_MALE;
					}
					else if ( value[0] == 'n' )
					{//neutral
						stats->sex = SEX_NEUTRAL;
					}
					else if ( value[0] == 'a' )
					{//asexual?
						stats->sex = SEX_NEUTRAL;
					}
					else if ( value[0] == 'f' )
					{//female
						stats->sex = SEX_FEMALE;
					}
					else if ( value[0] == 's' )
					{//shemale?
						stats->sex = SEX_SHEMALE;
					}
					else if ( value[0] == 'h' )
					{//hermaphrodite?
						stats->sex = SEX_SHEMALE;
					}
					else if ( value[0] == 't' )
					{//transsexual/transvestite?
						stats->sex = SEX_SHEMALE;
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
			}

			// snd
			if ( !Q_stricmp( token, "snd" ) )
			{
				if ( COM_ParseString( &p, &value ) )
				{
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
			if ( !Q_stricmp( token, "sndcombat" ) )
			{
				if ( COM_ParseString( &p, &value ) )
				{
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
			if ( !Q_stricmp( token, "sndextra" ) )
			{
				if ( COM_ParseString( &p, &value ) )
				{
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
			if ( !Q_stricmp( token, "sndjedi" ) )
			{
				if ( COM_ParseString( &p, &value ) )
				{
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

			//New NPC/jedi stats:
			//starting weapon
			if ( !Q_stricmp( token, "weapon" ) )
			{
				if ( COM_ParseString( &p, &value ) )
				{
					continue;
				}
				//FIXME: need to precache the weapon, too?  (in above func)
				int weap = GetIDForString( WPTable, value );
				if ( weap >= WP_NONE && weap < WP_NUM_WEAPONS )
				{
					NPC->client->ps.weapon = weap;
					NPC->client->ps.stats[STAT_WEAPONS] |= ( 1 << weap );
					if ( weap > WP_NONE )
					{
						RegisterItem( FindItemForWeapon( (weapon_t)(weap) ) );	//precache the weapon
						NPC->client->ps.ammo[weaponData[weap].ammoIndex] = ammoData[weaponData[weap].ammoIndex].max;
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
			int fp = GetIDForString( FPTable, token );
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
					NPC->client->ps.forcePowersKnown |= ( 1 << fp );
				}
				else
				{//clear
					NPC->client->ps.forcePowersKnown &= ~( 1 << fp );
				}
				NPC->client->ps.forcePowerLevel[fp] = n;
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
				NPC->client->ps.forcePowerMax = n;
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
				NPC->client->ps.forcePowerRegenRate = n;
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
				NPC->client->ps.forcePowerRegenAmount = n;
				continue;
			}

			//have a sabers.cfg and just name your saber in your NPCs.cfg/ICARUS script
			//saber name
			if ( !Q_stricmp( token, "saber" ) )
			{
				if ( COM_ParseString( &p, &value ) )
				{
					continue;
				}
				char *saberName = G_NewString( value );
				WP_SaberParseParms( saberName, &NPC->client->ps.saber[0] );
				//if it requires a specific style, make sure we know how to use it
				if ( NPC->client->ps.saber[0].stylesLearned )
				{
					NPC->client->ps.saberStylesKnown |= NPC->client->ps.saber[0].stylesLearned;
				}
				if ( NPC->client->ps.saber[0].singleBladeStyle )
				{
					NPC->client->ps.saberStylesKnown |= NPC->client->ps.saber[0].singleBladeStyle;
				}
				continue;
			}

			//second saber name
			if ( !Q_stricmp( token, "saber2" ) )
			{
				if ( COM_ParseString( &p, &value ) )
				{
					continue;
				}

				if ( !(NPC->client->ps.saber[0].saberFlags&SFL_TWO_HANDED) )
				{//can't use a second saber if first one is a two-handed saber...?
					char *saberName = G_NewString( value );
					WP_SaberParseParms( saberName, &NPC->client->ps.saber[1] );
					if ( NPC->client->ps.saber[1].stylesLearned )
					{
						NPC->client->ps.saberStylesKnown |= NPC->client->ps.saber[1].stylesLearned;
					}
					if ( NPC->client->ps.saber[1].singleBladeStyle )
					{
						NPC->client->ps.saberStylesKnown |= NPC->client->ps.saber[1].singleBladeStyle;
					}
					if ( (NPC->client->ps.saber[1].saberFlags&SFL_TWO_HANDED) )
					{//tsk tsk, can't use a twoHanded saber as second saber
						WP_RemoveSaber( NPC, 1 );
					}
					else
					{
						NPC->client->ps.dualSabers = qtrue;
					}
				}
				continue;
			}

			// saberColor
			if ( !Q_stricmpn( token, "saberColor", 10) )
			{
				if ( !NPC->client )
				{
					continue;
				}

				if (strlen(token)==10)
				{
					if ( COM_ParseString( &p, &value ) )
					{
						continue;
					}
					saber_colors_t color = TranslateSaberColor( value );
					for ( n = 0; n < MAX_BLADES; n++ )
					{
						NPC->client->ps.saber[0].blade[n].color = color;
					}
				}
				else if (strlen(token)==11)
				{
					int index = atoi(&token[10])-1;
					if (index > 7 || index < 1 )
					{
						gi.Printf( S_COLOR_YELLOW"WARNING: bad saberColor '%s' in %s\n", token, NPCName );
						continue;
					}
					if ( COM_ParseString( &p, &value ) )
					{
						continue;
					}
					NPC->client->ps.saber[0].blade[index].color = TranslateSaberColor( value );
				}
				else
				{
					gi.Printf( S_COLOR_YELLOW"WARNING: bad saberColor '%s' in %s\n", token, NPCName );
				}
				continue;
			}


			if ( !Q_stricmpn( token, "saber2Color", 11 ) )
			{
				if ( !NPC->client )
				{
					continue;
				}

				if (strlen(token)==11)
				{
					if ( COM_ParseString( &p, &value ) )
					{
						continue;
					}
					saber_colors_t color = TranslateSaberColor( value );
					for ( n = 0; n < MAX_BLADES; n++ )
					{
						NPC->client->ps.saber[1].blade[n].color = color;
					}
				}
				else if (strlen(token)==12)
				{
					n = atoi(&token[11])-1;
					if (n > 7 || n < 1 )
					{
						gi.Printf( S_COLOR_YELLOW"WARNING: bad saber2Color '%s' in %s\n", token, NPCName );
						continue;
					}
					if ( COM_ParseString( &p, &value ) )
					{
						continue;
					}
					NPC->client->ps.saber[1].blade[n].color = TranslateSaberColor( value );
				}
				else
				{
					gi.Printf( S_COLOR_YELLOW"WARNING: bad saber2Color '%s' in %s\n", token, NPCName );
				}
				continue;
			}


			//saber length
			if ( !Q_stricmpn( token, "saberLength", 11) )
			{
				if (strlen(token)==11)
				{
					n = -1;
				}
				else if (strlen(token)==12)
				{
					n = atoi(&token[11])-1;
					if (n > 7 || n < 1 )
					{
						gi.Printf( S_COLOR_YELLOW"WARNING: bad saberLength '%s' in %s\n", token, NPCName );
						continue;
					}
				}
				else
				{
					gi.Printf( S_COLOR_YELLOW"WARNING: bad saberLength '%s' in %s\n", token, NPCName );
					continue;
				}

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

				if (n == -1)//do them all
				{
					for ( n = 0; n < MAX_BLADES; n++ )
					{
						NPC->client->ps.saber[0].blade[n].lengthMax = f;
					}
				}
				else //just one
				{
					NPC->client->ps.saber[0].blade[n].lengthMax = f;
				}
				continue;
			}

			if ( !Q_stricmpn( token, "saber2Length", 12) )
			{
				if (strlen(token)==12)
				{
					n = -1;
				}
				else if (strlen(token)==13)
				{
					n = atoi(&token[12])-1;
					if (n > 7 || n < 1 )
					{
						gi.Printf( S_COLOR_YELLOW"WARNING: bad saber2Length '%s' in %s\n", token, NPCName );
						continue;
					}
				}
				else
				{
					gi.Printf( S_COLOR_YELLOW"WARNING: bad saber2Length '%s' in %s\n", token, NPCName );
					continue;
				}

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

				if (n == -1)//do them all
				{
					for ( n = 0; n < MAX_BLADES; n++ )
					{
						NPC->client->ps.saber[1].blade[n].lengthMax = f;
					}
				}
				else //just one
				{
					NPC->client->ps.saber[1].blade[n].lengthMax = f;
				}
				continue;
			}


			//saber radius
			if ( !Q_stricmpn( token, "saberRadius", 11 ) )
			{
				if (strlen(token)==11)
				{
					n = -1;
				}
				else if (strlen(token)==12)
				{
					n = atoi(&token[11])-1;
					if (n > 7 || n < 1 )
					{
						gi.Printf( S_COLOR_YELLOW"WARNING: bad saberRadius '%s' in %s\n", token, NPCName );
						continue;
					}
				}
				else
				{
					gi.Printf( S_COLOR_YELLOW"WARNING: bad saberRadius '%s' in %s\n", token, NPCName );
					continue;
				}

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

				if (n==-1)
				{//NOTE: this fills in the rest of the blades with the same length by default
					for ( n = 0; n < MAX_BLADES; n++ )
					{
						NPC->client->ps.saber[0].blade[n].radius = f;
					}
				}
				else
				{
					NPC->client->ps.saber[0].blade[n].radius = f;
				}
				continue;
			}


			if ( !Q_stricmpn( token, "saber2Radius", 12 ) )
			{
				if (strlen(token)==12)
				{
					n = -1;
				}
				else if (strlen(token)==13)
				{
					n = atoi(&token[12])-1;
					if (n > 7 || n < 1 )
					{
						gi.Printf( S_COLOR_YELLOW"WARNING: bad saber2Radius '%s' in %s\n", token, NPCName );
						continue;
					}
				}
				else
				{
					gi.Printf( S_COLOR_YELLOW"WARNING: bad saber2Radius '%s' in %s\n", token, NPCName );
					continue;
				}

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

				if (n==-1)
				{//NOTE: this fills in the rest of the blades with the same length by default
					for ( n = 0; n < MAX_BLADES; n++ )
					{
						NPC->client->ps.saber[1].blade[n].radius = f;
					}
				}
				else
				{
					NPC->client->ps.saber[1].blade[n].radius = f;
				}
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
				if ( n > SS_STAFF )
				{
					n = SS_STAFF;
				}
				else if ( n < SS_FAST )
				{
					n = SS_FAST;
				}
				if ( n )
				{//set
					NPC->client->ps.saberStylesKnown |= ( 1 << n );
				}
				else
				{//clear
					NPC->client->ps.saberStylesKnown &= ~( 1 << n );
				}
				NPC->client->ps.saberAnimLevel = n;
				if ( parsingPlayer )
				{
					cg.saberAnimLevelPending = n;
				}
				continue;
			}

			if ( !parsingPlayer )
			{
				gi.Printf( "WARNING: unknown keyword '%s' while parsing '%s'\n", token, NPCName );
			}
			SkipRestOfLine( &p );
		}
#ifdef _WIN32
#pragma endregion
#endif
		COM_EndParseSession(  );
	}

	ci->infoValid = qfalse;

/*
Ghoul2 Insert Start
*/
	if ( !md3Model )
	{
		NPC->weaponModel[0] = -1;
		if ( Q_stricmp( "player", playerModel ) == 0 )
		{//set the model from the console cvars
			G_InitPlayerFromCvars( NPC );
			//now set the weapon, etc.
			G_MatchPlayerWeapon( NPC );
			//NPC->NPC->aiFlags |= NPCAI_MATCHPLAYERWEAPON;//FIXME: may not always want this
		}
		else
		{//do a normal model load

			// If this is a vehicle, set the model name from the vehicle type array.
			if ( NPC->client->NPC_class == CLASS_VEHICLE )
			{
				int iVehIndex = BG_VehicleGetIndex( NPC->NPC_type );
				strcpy(customSkin, "default");	// Ignore any custom skin that may have come from the NPC File
				Q_strncpyz( playerModel, g_vehicleInfo[iVehIndex].model, sizeof(playerModel));
				if ( g_vehicleInfo[iVehIndex].skin && g_vehicleInfo[iVehIndex].skin[0] )
				{
					bool	forceSkin = false;

					// Iterate Over All Possible Skins
					//---------------------------------
					ratl::vector_vs<hstring, 15>	skinarray;
					ratl::string_vs<256>			skins(g_vehicleInfo[iVehIndex].skin);
					for (ratl::string_vs<256>::tokenizer i = skins.begin("|"); i!=skins.end(); i++)
					{
						if (NPC->soundSet && NPC->soundSet[0] && Q_stricmp(*i, NPC->soundSet)==0)
						{
							forceSkin = true;
						}
						skinarray.push_back(*i);
					}

					// Soundset Is The Designer Set Way To Supply A Skin
					//---------------------------------------------------
					if (forceSkin)
					{
						Q_strncpyz( customSkin, NPC->soundSet, sizeof(customSkin));
					}

					// Otherwise Choose A Random Skin
					//--------------------------------
					else
					{
						if (NPC->soundSet && NPC->soundSet[0])
						{
							gi.Printf(S_COLOR_RED"WARNING: Unable to use skin (%s)", NPC->soundSet);
						}
						Q_strncpyz( customSkin, *skinarray[Q_irand(0, skinarray.size()-1)], sizeof(customSkin));
					}
					if (NPC->soundSet && gi.bIsFromZone(NPC->soundSet, TAG_G_ALLOC)) {
						gi.Free(NPC->soundSet);
					}
					NPC->soundSet = 0; // clear the pointer
				}
			}

			G_SetG2PlayerModel( NPC, playerModel, customSkin, surfOff, surfOn );
		}
	}
/*
Ghoul2 Insert End
*/
	if(	NPCsPrecached )
	{//Spawning in after initial precache, our models are precached, we just need to set our clientInfo
		CG_RegisterClientModels( NPC->s.number );
		CG_RegisterNPCCustomSounds( ci );
		//CG_RegisterNPCEffects( NPC->client->playerTeam );
	}

	return qtrue;
}

void NPC_LoadParms( void )
{
	int			len, totallen, npcExtFNLen, fileCnt, i;
	char		*buffer, *holdChar, *marker;
	char		npcExtensionListBuf[2048];			//	The list of file names read in

	//gi.Printf( "Parsing ext_data/npcs/*.npc definitions\n" );

	//set where to store the first one
	totallen = 0;
	marker = NPCParms;
	marker[0] = '\0';

	//now load in the .npc definitions
	fileCnt = gi.FS_GetFileList("ext_data/npcs", ".npc", npcExtensionListBuf, sizeof(npcExtensionListBuf) );

	holdChar = npcExtensionListBuf;
	for ( i = 0; i < fileCnt; i++, holdChar += npcExtFNLen + 1 )
	{
		npcExtFNLen = strlen( holdChar );

		//gi.Printf( "Parsing %s\n", holdChar );

		len = gi.FS_ReadFile( va( "ext_data/npcs/%s", holdChar), (void **) &buffer );

		if ( len == -1 )
		{
			gi.Printf( "NPC_LoadParms: error reading file %s\n", holdChar );
		}
		else
		{
			if ( totallen && *(marker-1) == '}' )
			{//don't let previous file end on a } because that must be a stand-alone token
				strcat( marker, " " );
				totallen++;
				marker++;
			}
			len = COM_Compress( buffer );

			if ( totallen + len >= MAX_NPC_DATA_SIZE ) {
				G_Error( "NPC_LoadParms: ran out of space before reading %s\n(you must make the .npc files smaller)", holdChar );
			}
			strcat( marker, buffer );
			gi.FS_FreeFile( buffer );

			totallen += len;
			marker += len;
		}
	}
}
