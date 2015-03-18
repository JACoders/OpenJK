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

//====================================================================================
//
//rww - ICARUS callback file, all that can be handled within vm's is handled in here.
//
//====================================================================================

#include "qcommon/q_shared.h"
#include "bg_public.h"
#include "b_local.h"
#include "icarus/Q3_Interface.h"
#include "icarus/Q3_Registers.h"
#include "g_nav.h"

qboolean BG_SabersOff( playerState_t *ps );
extern stringID_table_t WPTable[];
extern stringID_table_t BSTable[];


//This is a hack I guess. It's because we can't include the file this enum is in
//unless we're using cpp. But we need it for the interpreter stuff.
//In any case, DO NOT modify this enum.

// Hack++
// This code is compiled as C++ on Xbox. We could try and rig something above
// so that we only get the C version of the includes (no full Icarus) in that
// scenario, but I think we'll just try to leave this out instead.
//#if defined(__linux__) && defined(__GCC__) || !defined(__linux__)
enum
{
	TK_EOF = -1,
	TK_UNDEFINED,
	TK_COMMENT,
	TK_EOL,
	TK_CHAR,
	TK_STRING,
	TK_INT,
	TK_INTEGER = TK_INT,
	TK_FLOAT,
	TK_IDENTIFIER,
	TK_USERDEF,
};
//#endif

#include "icarus/interpreter.h"

extern stringID_table_t animTable [MAX_ANIMATIONS+1];

stringID_table_t setTable[] =
{
	ENUM2STRING(SET_SPAWNSCRIPT),//0
	ENUM2STRING(SET_USESCRIPT),
	ENUM2STRING(SET_AWAKESCRIPT),
	ENUM2STRING(SET_ANGERSCRIPT),
	ENUM2STRING(SET_ATTACKSCRIPT),
	ENUM2STRING(SET_VICTORYSCRIPT),
	ENUM2STRING(SET_PAINSCRIPT),
	ENUM2STRING(SET_FLEESCRIPT),
	ENUM2STRING(SET_DEATHSCRIPT),
	ENUM2STRING(SET_DELAYEDSCRIPT),
	ENUM2STRING(SET_BLOCKEDSCRIPT),
	ENUM2STRING(SET_FFIRESCRIPT),
	ENUM2STRING(SET_FFDEATHSCRIPT),
	ENUM2STRING(SET_MINDTRICKSCRIPT),
	ENUM2STRING(SET_NO_MINDTRICK),
	ENUM2STRING(SET_ORIGIN),
	ENUM2STRING(SET_TELEPORT_DEST),
	ENUM2STRING(SET_ANGLES),
	ENUM2STRING(SET_XVELOCITY),
	ENUM2STRING(SET_YVELOCITY),
	ENUM2STRING(SET_ZVELOCITY),
	ENUM2STRING(SET_Z_OFFSET),
	ENUM2STRING(SET_ENEMY),
	ENUM2STRING(SET_LEADER),
	ENUM2STRING(SET_NAVGOAL),
	ENUM2STRING(SET_ANIM_UPPER),
	ENUM2STRING(SET_ANIM_LOWER),
	ENUM2STRING(SET_ANIM_BOTH),
	ENUM2STRING(SET_ANIM_HOLDTIME_LOWER),
	ENUM2STRING(SET_ANIM_HOLDTIME_UPPER),
	ENUM2STRING(SET_ANIM_HOLDTIME_BOTH),
	ENUM2STRING(SET_PLAYER_TEAM),
	ENUM2STRING(SET_ENEMY_TEAM),
	ENUM2STRING(SET_BEHAVIOR_STATE),
	ENUM2STRING(SET_BEHAVIOR_STATE),
	ENUM2STRING(SET_HEALTH),
	ENUM2STRING(SET_ARMOR),
	ENUM2STRING(SET_DEFAULT_BSTATE),
	ENUM2STRING(SET_CAPTURE),
	ENUM2STRING(SET_DPITCH),
	ENUM2STRING(SET_DYAW),
	ENUM2STRING(SET_EVENT),
	ENUM2STRING(SET_TEMP_BSTATE),
	ENUM2STRING(SET_COPY_ORIGIN),
	ENUM2STRING(SET_VIEWTARGET),
	ENUM2STRING(SET_WEAPON),
	ENUM2STRING(SET_ITEM),
	ENUM2STRING(SET_WALKSPEED),
	ENUM2STRING(SET_RUNSPEED),
	ENUM2STRING(SET_YAWSPEED),
	ENUM2STRING(SET_AGGRESSION),
	ENUM2STRING(SET_AIM),
	ENUM2STRING(SET_FRICTION),
	ENUM2STRING(SET_GRAVITY),
	ENUM2STRING(SET_IGNOREPAIN),
	ENUM2STRING(SET_IGNOREENEMIES),
	ENUM2STRING(SET_IGNOREALERTS),
	ENUM2STRING(SET_DONTSHOOT),
	ENUM2STRING(SET_DONTFIRE),
	ENUM2STRING(SET_LOCKED_ENEMY),
	ENUM2STRING(SET_NOTARGET),
	ENUM2STRING(SET_LEAN),
	ENUM2STRING(SET_CROUCHED),
	ENUM2STRING(SET_WALKING),
	ENUM2STRING(SET_RUNNING),
	ENUM2STRING(SET_CHASE_ENEMIES),
	ENUM2STRING(SET_LOOK_FOR_ENEMIES),
	ENUM2STRING(SET_FACE_MOVE_DIR),
	ENUM2STRING(SET_ALT_FIRE),
	ENUM2STRING(SET_DONT_FLEE),
	ENUM2STRING(SET_FORCED_MARCH),
	ENUM2STRING(SET_NO_RESPONSE),
	ENUM2STRING(SET_NO_COMBAT_TALK),
	ENUM2STRING(SET_NO_ALERT_TALK),
	ENUM2STRING(SET_UNDYING),
	ENUM2STRING(SET_TREASONED),
	ENUM2STRING(SET_DISABLE_SHADER_ANIM),
	ENUM2STRING(SET_SHADER_ANIM),
	ENUM2STRING(SET_INVINCIBLE),
	ENUM2STRING(SET_NOAVOID),
	ENUM2STRING(SET_SHOOTDIST),
	ENUM2STRING(SET_TARGETNAME),
	ENUM2STRING(SET_TARGET),
	ENUM2STRING(SET_TARGET2),
	ENUM2STRING(SET_LOCATION),
	ENUM2STRING(SET_PAINTARGET),
	ENUM2STRING(SET_TIMESCALE),
	ENUM2STRING(SET_VISRANGE),
	ENUM2STRING(SET_EARSHOT),
	ENUM2STRING(SET_VIGILANCE),
	ENUM2STRING(SET_HFOV),
	ENUM2STRING(SET_VFOV),
	ENUM2STRING(SET_DELAYSCRIPTTIME),
	ENUM2STRING(SET_FORWARDMOVE),
	ENUM2STRING(SET_RIGHTMOVE),
	ENUM2STRING(SET_LOCKYAW),
	ENUM2STRING(SET_SOLID),
	ENUM2STRING(SET_CAMERA_GROUP),
	ENUM2STRING(SET_CAMERA_GROUP_Z_OFS),
	ENUM2STRING(SET_CAMERA_GROUP_TAG),
	ENUM2STRING(SET_LOOK_TARGET),
	ENUM2STRING(SET_ADDRHANDBOLT_MODEL),
	ENUM2STRING(SET_REMOVERHANDBOLT_MODEL),
	ENUM2STRING(SET_ADDLHANDBOLT_MODEL),
	ENUM2STRING(SET_REMOVELHANDBOLT_MODEL),
	ENUM2STRING(SET_FACEAUX),
	ENUM2STRING(SET_FACEBLINK),
	ENUM2STRING(SET_FACEBLINKFROWN),
	ENUM2STRING(SET_FACEFROWN),
	ENUM2STRING(SET_FACENORMAL),
	ENUM2STRING(SET_FACEEYESCLOSED),
	ENUM2STRING(SET_FACEEYESOPENED),
	ENUM2STRING(SET_SCROLLTEXT),
	ENUM2STRING(SET_LCARSTEXT),
	ENUM2STRING(SET_SCROLLTEXTCOLOR),
	ENUM2STRING(SET_CAPTIONTEXTCOLOR),
	ENUM2STRING(SET_CENTERTEXTCOLOR),
	ENUM2STRING(SET_PLAYER_USABLE),
	ENUM2STRING(SET_STARTFRAME),
	ENUM2STRING(SET_ENDFRAME),
	ENUM2STRING(SET_ANIMFRAME),
	ENUM2STRING(SET_LOOP_ANIM),
	ENUM2STRING(SET_INTERFACE),
	ENUM2STRING(SET_SHIELDS),
	ENUM2STRING(SET_NO_KNOCKBACK),
	ENUM2STRING(SET_INVISIBLE),
	ENUM2STRING(SET_VAMPIRE),
	ENUM2STRING(SET_FORCE_INVINCIBLE),
	ENUM2STRING(SET_GREET_ALLIES),
	ENUM2STRING(SET_PLAYER_LOCKED),
	ENUM2STRING(SET_LOCK_PLAYER_WEAPONS),
	ENUM2STRING(SET_NO_IMPACT_DAMAGE),
	ENUM2STRING(SET_PARM1),
	ENUM2STRING(SET_PARM2),
	ENUM2STRING(SET_PARM3),
	ENUM2STRING(SET_PARM4),
	ENUM2STRING(SET_PARM5),
	ENUM2STRING(SET_PARM6),
	ENUM2STRING(SET_PARM7),
	ENUM2STRING(SET_PARM8),
	ENUM2STRING(SET_PARM9),
	ENUM2STRING(SET_PARM10),
	ENUM2STRING(SET_PARM11),
	ENUM2STRING(SET_PARM12),
	ENUM2STRING(SET_PARM13),
	ENUM2STRING(SET_PARM14),
	ENUM2STRING(SET_PARM15),
	ENUM2STRING(SET_PARM16),
	ENUM2STRING(SET_DEFEND_TARGET),
	ENUM2STRING(SET_WAIT),
	ENUM2STRING(SET_COUNT),
	ENUM2STRING(SET_SHOT_SPACING),
	ENUM2STRING(SET_VIDEO_PLAY),
	ENUM2STRING(SET_VIDEO_FADE_IN),
	ENUM2STRING(SET_VIDEO_FADE_OUT),
	ENUM2STRING(SET_REMOVE_TARGET),
	ENUM2STRING(SET_LOADGAME),
	ENUM2STRING(SET_MENU_SCREEN),
	ENUM2STRING(SET_OBJECTIVE_SHOW),
	ENUM2STRING(SET_OBJECTIVE_HIDE),
	ENUM2STRING(SET_OBJECTIVE_SUCCEEDED),
	ENUM2STRING(SET_OBJECTIVE_FAILED),
	ENUM2STRING(SET_MISSIONFAILED),
	ENUM2STRING(SET_TACTICAL_SHOW),
	ENUM2STRING(SET_TACTICAL_HIDE),
	ENUM2STRING(SET_FOLLOWDIST),
	ENUM2STRING(SET_SCALE),
	ENUM2STRING(SET_OBJECTIVE_CLEARALL),
	ENUM2STRING(SET_MISSIONSTATUSTEXT),
	ENUM2STRING(SET_WIDTH),
	ENUM2STRING(SET_CLOSINGCREDITS),
	ENUM2STRING(SET_SKILL),
	ENUM2STRING(SET_MISSIONSTATUSTIME),
	ENUM2STRING(SET_FULLNAME),
	ENUM2STRING(SET_FORCE_HEAL_LEVEL),
	ENUM2STRING(SET_FORCE_JUMP_LEVEL),
	ENUM2STRING(SET_FORCE_SPEED_LEVEL),
	ENUM2STRING(SET_FORCE_PUSH_LEVEL),
	ENUM2STRING(SET_FORCE_PULL_LEVEL),
	ENUM2STRING(SET_FORCE_MINDTRICK_LEVEL),
	ENUM2STRING(SET_FORCE_GRIP_LEVEL),
	ENUM2STRING(SET_FORCE_LIGHTNING_LEVEL),
	ENUM2STRING(SET_SABER_THROW),
	ENUM2STRING(SET_SABER_DEFENSE),
	ENUM2STRING(SET_SABER_OFFENSE),
	ENUM2STRING(SET_VIEWENTITY),
	ENUM2STRING(SET_WATCHTARGET),
	ENUM2STRING(SET_SABERACTIVE),
	ENUM2STRING(SET_ADJUST_AREA_PORTALS),
	ENUM2STRING(SET_DMG_BY_HEAVY_WEAP_ONLY),
	ENUM2STRING(SET_SHIELDED),
	ENUM2STRING(SET_NO_GROUPS),
	ENUM2STRING(SET_FIRE_WEAPON),
	ENUM2STRING(SET_INACTIVE),
	ENUM2STRING(SET_FUNC_USABLE_VISIBLE),
	ENUM2STRING(SET_MISSION_STATUS_SCREEN),
	ENUM2STRING(SET_END_SCREENDISSOLVE),
	ENUM2STRING(SET_LOOPSOUND),
	ENUM2STRING(SET_ICARUS_FREEZE),
	ENUM2STRING(SET_ICARUS_UNFREEZE),
	ENUM2STRING(SET_USE_CP_NEAREST),
	ENUM2STRING(SET_MORELIGHT),
	ENUM2STRING(SET_CINEMATIC_SKIPSCRIPT),
	ENUM2STRING(SET_NO_FORCE),
	ENUM2STRING(SET_NO_FALLTODEATH),
	ENUM2STRING(SET_DISMEMBERABLE),
	ENUM2STRING(SET_NO_ACROBATICS),
	ENUM2STRING(SET_MUSIC_STATE),
	ENUM2STRING(SET_USE_SUBTITLES),
	ENUM2STRING(SET_CLEAN_DAMAGING_ENTS),
	ENUM2STRING(SET_HUD),

//FIXME: add BOTH_ attributes here too
	{"",	SET_},
};

void Q3_TaskIDClear( int *taskID )
{
	*taskID = -1;
}

void G_DebugPrint( int printLevel, const char *format, ... )
{
	va_list		argptr;
	char		text[1024] = {0};

	//Don't print messages they don't want to see
	//if ( g_ICARUSDebug->integer < level )
	if (developer.integer != 2)
		return;

	va_start( argptr, format );
	Q_vsnprintf(text, sizeof( text ), format, argptr );
	va_end( argptr );

	//Add the color formatting
	switch ( printLevel )
	{
		case WL_ERROR:
			Com_Printf ( S_COLOR_RED"ERROR: %s", text );
			break;

		case WL_WARNING:
			Com_Printf ( S_COLOR_YELLOW"WARNING: %s", text );
			break;

		case WL_DEBUG:
			{
				int		entNum;
				char	*buffer;

				entNum = atoi( text );

				//if ( ( ICARUS_entFilter >= 0 ) && ( ICARUS_entFilter != entNum ) )
				//	return;

				buffer = (char *) text;
				buffer += 5;

				if ( ( entNum < 0 ) || ( entNum >= MAX_GENTITIES ) )
					entNum = 0;

				Com_Printf ( S_COLOR_BLUE"DEBUG: %s(%d): %s\n", g_entities[entNum].script_targetname, entNum, buffer );
				break;
			}
		default:
		case WL_VERBOSE:
			Com_Printf ( S_COLOR_GREEN"INFO: %s", text );
			break;
	}
}

/*
-------------------------
Q3_GetAnimLower
-------------------------
*/
static char *Q3_GetAnimLower( gentity_t *ent )
{
	int anim = 0;

	if ( ent->client == NULL )
	{
		G_DebugPrint( WL_WARNING, "Q3_GetAnimLower: attempted to read animation state off non-client!\n" );
		return NULL;
	}

	anim = ent->client->ps.legsAnim;

	return (char *)animTable[anim].name;
}

/*
-------------------------
Q3_GetAnimUpper
-------------------------
*/
static char *Q3_GetAnimUpper( gentity_t *ent )
{
	int anim = 0;

	if ( ent->client == NULL )
	{
		G_DebugPrint( WL_WARNING, "Q3_GetAnimUpper: attempted to read animation state off non-client!\n" );
		return NULL;
	}

	anim = ent->client->ps.torsoAnim;

	return (char *)animTable[anim].name;
}

/*
-------------------------
Q3_GetAnimBoth
-------------------------
*/
static char *Q3_GetAnimBoth( gentity_t *ent )
{
 	char	*lowerName, *upperName;

	lowerName = Q3_GetAnimLower( ent );
	upperName = Q3_GetAnimUpper( ent );

	if ( !lowerName || !lowerName[0] )
	{
		G_DebugPrint( WL_WARNING, "Q3_GetAnimBoth: NULL legs animation string found!\n" );
		return NULL;
	}

	if ( !upperName || !upperName[0] )
	{
		G_DebugPrint( WL_WARNING, "Q3_GetAnimBoth: NULL torso animation string found!\n" );
		return NULL;
	}

	if ( Q_stricmp( lowerName, upperName ) )
	{
#ifdef _DEBUG	// sigh, cut down on tester reports that aren't important
		G_DebugPrint( WL_WARNING, "Q3_GetAnimBoth: legs and torso animations did not match : returning legs\n" );
#endif
	}

	return lowerName;
}

int Q3_PlaySound( int taskID, int entID, const char *name, const char *channel )
{
	gentity_t		*ent = &g_entities[entID];
	char			finalName[MAX_QPATH];
	soundChannel_t	voice_chan = CHAN_VOICE; // set a default so the compiler doesn't bitch
	qboolean		type_voice = qfalse;
	int				soundHandle;
	qboolean		bBroadcast;

	Q_strncpyz( finalName, name, MAX_QPATH );
	Q_strupr(finalName);
	//G_AddSexToMunroString( finalName, qtrue );

	COM_StripExtension( (const char *)finalName, finalName, sizeof( finalName ) );

	soundHandle = G_SoundIndex( (char *) finalName );
	bBroadcast = qfalse;

	if ( ( Q_stricmp( channel, "CHAN_ANNOUNCER" ) == 0 ) || (ent->classname && Q_stricmp("target_scriptrunner", ent->classname ) == 0) ) {
		bBroadcast = qtrue;
	}


	// moved here from further down so I can easily check channel-type without code dup...
	//
	if ( Q_stricmp( channel, "CHAN_VOICE" ) == 0 )
	{
		voice_chan = CHAN_VOICE;
		type_voice = qtrue;
	}
	else if ( Q_stricmp( channel, "CHAN_VOICE_ATTEN" ) == 0 )
	{
		voice_chan = CHAN_AUTO;//CHAN_VOICE_ATTEN;
		type_voice = qtrue;
	}
	else if ( Q_stricmp( channel, "CHAN_VOICE_GLOBAL" ) == 0 ) // this should broadcast to everyone, put only casue animation on G_SoundOnEnt...
	{
		voice_chan = CHAN_AUTO;//CHAN_VOICE_GLOBAL;
		type_voice = qtrue;
		bBroadcast = qtrue;
	}

	// if we're in-camera, check for skipping cinematic and ifso, no subtitle print (since screen is not being
	//	updated anyway during skipping). This stops leftover subtitles being left onscreen after unskipping.
	//
	/*
	if (!in_camera ||
		(!g_skippingcin || !g_skippingcin->integer)
		)	// paranoia towards project end <g>
	{
		// Text on
		// certain NPC's we always want to use subtitles regardless of subtitle setting
		if (g_subtitles->integer == 1 || (ent->NPC && (ent->NPC->scriptFlags & SCF_USE_SUBTITLES) ) ) // Show all text
		{
			if ( in_camera)	// Cinematic
			{
				trap->SendServerCommand( -1, va("ct \"%s\" %i", finalName, soundHandle) );
			}
			else //if (precacheWav[i].speaker==SP_NONE)	//  lower screen text
			{
				sharedEntity_t		*ent2 = SV_GentityNum(0);
				// the numbers in here were either the original ones Bob entered (350), or one arrived at from checking the distance Chell stands at in stasis2 by the computer core that was submitted as a bug report...
				//
				if (bBroadcast || (DistanceSquared(ent->currentOrigin, ent2->currentOrigin) < ((voice_chan == CHAN_VOICE_ATTEN)?(350 * 350):(1200 * 1200)) ) )
				{
					trap->SendServerCommand( -1, va("ct \"%s\" %i", finalName, soundHandle) );
				}
			}
		}
		// Cinematic only
		else if (g_subtitles->integer == 2) // Show only talking head text and CINEMATIC
		{
			if ( in_camera)	// Cinematic text
			{
				trap->SendServerCommand( -1, va("ct \"%s\" %i", finalName, soundHandle));
			}
		}

	}
	*/

	if ( type_voice )
	{
		char buf[128];
		float tFVal = 0;

		trap->Cvar_VariableStringBuffer("timescale", buf, sizeof(buf));

		tFVal = atof(buf);


		if ( tFVal > 1.0f )
		{//Skip the damn sound!
			return qtrue;
		}
		else
		{
			//This the voice channel
			G_Sound( ent, voice_chan, G_SoundIndex((char *) finalName) );
		}
		//Remember we're waiting for this
		trap->ICARUS_TaskIDSet( (sharedEntity_t *)ent, TID_CHAN_VOICE, taskID );

		return qfalse;
	}

	if ( bBroadcast )
	{//Broadcast the sound
		gentity_t	*te;

		te = G_TempEntity( ent->r.currentOrigin, EV_GLOBAL_SOUND );
		te->s.eventParm = soundHandle;
		te->r.svFlags |= SVF_BROADCAST;
	}
	else
	{
		G_Sound( ent, CHAN_AUTO, soundHandle );
	}

	return qtrue;
}

/*
-------------------------
Q3_Play
-------------------------
*/
void Q3_Play( int taskID, int entID, const char *type, const char *name )
{
	gentity_t *ent = &g_entities[entID];

	if ( !Q_stricmp( type, "PLAY_ROFF" ) )
	{
		// Try to load the requested ROFF
		ent->roffid = trap->ROFF_Cache((char*)name);
		if ( ent->roffid )
		{
			ent->roffname = G_NewString( name );

			// Start the roff from the beginning
			//ent->roff_ctr = 0;

			//Save this off for later
			trap->ICARUS_TaskIDSet( (sharedEntity_t *)ent, TID_MOVE_NAV, taskID );

			// Let the ROFF playing start.
			//ent->next_roff_time = level.time;

			//rww - Maybe use pos1 and pos2? I don't think we need to care if these values are sent across the net.
			// These need to be initialised up front...
			//VectorCopy( ent->r.currentOrigin, ent->pos1 );
			//VectorCopy( ent->r.currentAngles, ent->pos2 );
			VectorCopy( ent->r.currentOrigin, ent->s.origin2 );
			VectorCopy( ent->r.currentAngles, ent->s.angles2 );

			trap->LinkEntity( (sharedEntity_t *)ent );

			trap->ROFF_Play(ent->s.number, ent->roffid, qtrue);
		}
	}
}

/*
=============
anglerCallback

Utility function
=============
*/
void anglerCallback( gentity_t *ent )
{
	//Complete the task
	trap->ICARUS_TaskIDComplete( (sharedEntity_t *)ent, TID_ANGLE_FACE );

	//Set the currentAngles, clear all movement
	VectorMA( ent->s.apos.trBase, (ent->s.apos.trDuration*0.001f), ent->s.apos.trDelta, ent->r.currentAngles );
	VectorCopy( ent->r.currentAngles, ent->s.apos.trBase );
	VectorClear( ent->s.apos.trDelta );
	ent->s.apos.trDuration = 1;
	ent->s.apos.trType = TR_STATIONARY;
	ent->s.apos.trTime = level.time;

	//Stop thinking
	ent->reached = 0;
	if ( ent->think == anglerCallback )
	{
		ent->think = 0;
	}

	//link
	trap->LinkEntity( (sharedEntity_t *)ent );
}

void MatchTeam( gentity_t *teamLeader, int moverState, int time );
void Blocked_Mover( gentity_t *ent, gentity_t *other );

/*
=============
moverCallback

Utility function
=============
*/
void moverCallback( gentity_t *ent )
{	//complete the task
	trap->ICARUS_TaskIDComplete( (sharedEntity_t *)ent, TID_MOVE_NAV );

	// play sound
	ent->s.loopSound = 0;//stop looping sound
	ent->s.loopIsSoundset = qfalse;
	G_PlayDoorSound( ent, BMS_END );//play end sound

	if ( ent->moverState == MOVER_1TO2 )
	{//reached open
		// reached pos2
		MatchTeam( ent, MOVER_POS2, level.time );
		//SetMoverState( ent, MOVER_POS2, level.time );
	}
	else if ( ent->moverState == MOVER_2TO1 )
	{//reached closed
		MatchTeam( ent, MOVER_POS1, level.time );
		//SetMoverState( ent, MOVER_POS1, level.time );
	}

	if ( ent->blocked == Blocked_Mover )
	{
		ent->blocked = 0;
	}

//	if ( !Q_stricmp( "misc_model_breakable", ent->classname ) && ent->physicsBounce )
//	{//a gravity-affected model
//		misc_model_breakable_gravity_init( ent, qfalse );
//	}
}

void Blocked_Mover( gentity_t *ent, gentity_t *other )
{
	// remove anything other than a client -- no longer the case

	// don't remove security keys or goodie keys
	if ( other->s.eType == ET_ITEM)
	{
		// should we be doing anything special if a key blocks it... move it somehow..?
	}
	// if your not a client, or your a dead client remove yourself...
	else if ( other->s.number && (!other->client || (other->client && other->health <= 0 && other->r.contents == CONTENTS_CORPSE && !other->message)) )
	{
		//if ( !other->taskManager || !other->taskManager->IsRunning() )
		{
			// if an item or weapon can we do a little explosion..?
			G_FreeEntity( other );
			return;
		}
	}

	if ( ent->damage ) {
		G_Damage( other, ent, ent, NULL, NULL, ent->damage, 0, MOD_CRUSH );
	}
}

/*
=============
moveAndRotateCallback

Utility function
=============
*/
void moveAndRotateCallback( gentity_t *ent )
{
	//stop turning
	anglerCallback( ent );
	//stop moving
	moverCallback( ent );
}

/*
=============
Q3_Lerp2Start

Lerps the origin of an entity to its starting position
=============
*/
void Q3_Lerp2Start( int entID, int taskID, float duration )
{
	gentity_t	*ent = &g_entities[entID];

	if(!ent)
	{
		G_DebugPrint( WL_WARNING, "Q3_Lerp2Start: invalid entID %d\n", entID);
		return;
	}

	if ( ent->client || Q_stricmp(ent->classname, "target_scriptrunner") == 0 )
	{
		G_DebugPrint( WL_ERROR, "Q3_Lerp2Start: ent %d is NOT a mover!\n", entID);
		return;
	}

	if ( ent->s.eType != ET_MOVER )
	{
		ent->s.eType = ET_MOVER;
	}

	//FIXME: set up correctly!!!
	ent->moverState = MOVER_2TO1;
	ent->s.eType = ET_MOVER;
	ent->reached = moverCallback;		//Callsback the the completion of the move
	if ( ent->damage )
	{
		ent->blocked = Blocked_Mover;
	}

	ent->s.pos.trDuration = duration * 10;	//In seconds
	ent->s.pos.trTime = level.time;

	trap->ICARUS_TaskIDSet( (sharedEntity_t *)ent, TID_MOVE_NAV, taskID );
	// starting sound
	G_PlayDoorLoopSound( ent );
	G_PlayDoorSound( ent, BMS_START );	//??

	trap->LinkEntity( (sharedEntity_t *)ent );
}

/*
=============
Q3_Lerp2End

Lerps the origin of an entity to its ending position
=============
*/
void Q3_Lerp2End( int entID, int taskID, float duration )
{
	gentity_t	*ent = &g_entities[entID];

	if(!ent)
	{
		G_DebugPrint( WL_WARNING, "Q3_Lerp2End: invalid entID %d\n", entID);
		return;
	}

	if ( ent->client || Q_stricmp(ent->classname, "target_scriptrunner") == 0 )
	{
		G_DebugPrint( WL_ERROR, "Q3_Lerp2End: ent %d is NOT a mover!\n", entID);
		return;
	}

	if ( ent->s.eType != ET_MOVER )
	{
		ent->s.eType = ET_MOVER;
	}

	//FIXME: set up correctly!!!
	ent->moverState = MOVER_1TO2;
	ent->s.eType = ET_MOVER;
	ent->reached = moverCallback;		//Callsback the the completion of the move
	if ( ent->damage )
	{
		ent->blocked = Blocked_Mover;
	}

	ent->s.pos.trDuration = duration * 10;	//In seconds
	ent->s.time = level.time;

	trap->ICARUS_TaskIDSet( (sharedEntity_t *)ent, TID_MOVE_NAV, taskID );
	// starting sound
	G_PlayDoorLoopSound( ent );
	G_PlayDoorSound( ent, BMS_START );	//??

	trap->LinkEntity( (sharedEntity_t *)ent );
}

void InitMoverTrData( gentity_t *ent );

/*
=============
Q3_Lerp2Pos

Lerps the origin and angles of an entity to the destination values

=============
*/
void Q3_Lerp2Pos( int taskID, int entID, vec3_t origin, vec3_t angles, float duration )
{
	gentity_t	*ent = &g_entities[entID];
	vec3_t		ang;
	int			i;
	moverState_t moverState;

	if(!ent)
	{
		G_DebugPrint( WL_WARNING, "Q3_Lerp2Pos: invalid entID %d\n", entID);
		return;
	}

	if ( ent->client || Q_stricmp(ent->classname, "target_scriptrunner") == 0 )
	{
		G_DebugPrint( WL_ERROR, "Q3_Lerp2Pos: ent %d is NOT a mover!\n", entID);
		return;
	}

	if ( ent->s.eType != ET_MOVER )
	{
		ent->s.eType = ET_MOVER;
	}

	//Don't allow a zero duration
	if ( duration == 0 )
		duration = 1;

	//
	// Movement

	moverState = ent->moverState;

	if ( moverState == MOVER_POS1 || moverState == MOVER_2TO1 )
	{
		VectorCopy( ent->r.currentOrigin, ent->pos1 );
		VectorCopy( origin, ent->pos2 );

		moverState = MOVER_1TO2;
	}
	else
	{
		VectorCopy( ent->r.currentOrigin, ent->pos2 );
		VectorCopy( origin, ent->pos1 );

		moverState = MOVER_2TO1;
	}

	InitMoverTrData( ent );

	ent->s.pos.trDuration = duration;

	// start it going
	MatchTeam( ent, moverState, level.time );
	//SetMoverState( ent, moverState, level.time );

	//Only do the angles if specified
	if ( angles != NULL )
	{
		//
		// Rotation

		for ( i = 0; i < 3; i++ )
		{
			ang[i] = AngleDelta( angles[i], ent->r.currentAngles[i] );
			ent->s.apos.trDelta[i] = ( ang[i] / ( duration * 0.001f ) );
		}

		VectorCopy( ent->r.currentAngles, ent->s.apos.trBase );

		if ( ent->alt_fire )
		{
			ent->s.apos.trType = TR_LINEAR_STOP;
		}
		else
		{
			ent->s.apos.trType = TR_NONLINEAR_STOP;
		}
		ent->s.apos.trDuration = duration;

		ent->s.apos.trTime = level.time;

		ent->reached = moveAndRotateCallback;
		trap->ICARUS_TaskIDSet( (sharedEntity_t *)ent, TID_ANGLE_FACE, taskID );
	}
	else
	{
		//Setup the last bits of information
		ent->reached = moverCallback;
	}

	if ( ent->damage )
	{
		ent->blocked = Blocked_Mover;
	}

	trap->ICARUS_TaskIDSet( (sharedEntity_t *)ent, TID_MOVE_NAV, taskID );
	// starting sound
	G_PlayDoorLoopSound( ent );
	G_PlayDoorSound( ent, BMS_START );	//??

	trap->LinkEntity( (sharedEntity_t *)ent );
}

/*
=============
Q3_LerpAngles

Lerps the angles to the destination value
=============
*/
void Q3_Lerp2Angles( int taskID, int entID, vec3_t angles, float duration )
{
	gentity_t	*ent = &g_entities[entID];
	vec3_t		ang;
	int			i;

	if(!ent)
	{
		G_DebugPrint( WL_WARNING, "Q3_Lerp2Angles: invalid entID %d\n", entID);
		return;
	}

	if ( ent->client || Q_stricmp(ent->classname, "target_scriptrunner") == 0 )
	{
		G_DebugPrint( WL_ERROR, "Q3_Lerp2Angles: ent %d is NOT a mover!\n", entID);
		return;
	}

	//If we want an instant move, don't send 0...
	ent->s.apos.trDuration = (duration>0) ? duration : 1;

	for ( i = 0; i < 3; i++ )
	{
		ang [i] = AngleSubtract( angles[i], ent->r.currentAngles[i]);
		ent->s.apos.trDelta[i] = ( ang[i] / ( ent->s.apos.trDuration * 0.001f ) );
	}

	VectorCopy( ent->r.currentAngles, ent->s.apos.trBase );

	if ( ent->alt_fire )
	{
		ent->s.apos.trType = TR_LINEAR_STOP;
	}
	else
	{
		ent->s.apos.trType = TR_NONLINEAR_STOP;
	}

	ent->s.apos.trTime = level.time;

	trap->ICARUS_TaskIDSet( (sharedEntity_t *)ent, TID_ANGLE_FACE, taskID );

	//ent->e_ReachedFunc = reachedF_NULL;
	ent->think = anglerCallback;
	ent->nextthink = level.time + duration;

	trap->LinkEntity( (sharedEntity_t *)ent );
}

/*
=============
Q3_GetTag

Gets the value of a tag by the give name
=============
*/
int	Q3_GetTag( int entID, const char *name, int lookup, vec3_t info )
{
	gentity_t	*ent = &g_entities[entID];

	if (!ent->inuse)
	{
		assert(0);
		return 0;
	}

	switch ( lookup )
	{
	case TYPE_ORIGIN:
		return TAG_GetOrigin( ent->ownername, name, info );
		break;

	case TYPE_ANGLES:
		return TAG_GetAngles( ent->ownername, name, info );
		break;
	}

	return 0;
}

//-----------------------------------------------

/*
============
Q3_Use

Uses an entity
============
*/
void Q3_Use( int entID, const char *target )
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		G_DebugPrint( WL_WARNING, "Q3_Use: invalid entID %d\n", entID);
		return;
	}

	if( !target || !target[0] )
	{
		G_DebugPrint( WL_WARNING, "Q3_Use: string is NULL!\n" );
		return;
	}

	G_UseTargets2(ent, ent, target);
}

/*
============
Q3_Kill
  Description	:
  Return type	: void
  Argument		:  int entID
  Argument		: const char *name
============
*/
void Q3_Kill( int entID, const char *name )
{
	gentity_t	*ent = &g_entities[entID];
	gentity_t	*victim = NULL;
	int			o_health;

	if( !Q_stricmp( name, "self") )
	{
		victim = ent;
	}
	else if( !Q_stricmp( name, "enemy" ) )
	{
		victim = ent->enemy;
	}
	else
	{
		victim = G_Find (NULL, FOFS(targetname), (char *) name );
	}

	if ( !victim )
	{
		G_DebugPrint( WL_WARNING, "Q3_Kill: can't find %s\n", name);
		return;
	}

	//rww - I guess this would only apply to NPCs anyway. I'm not going to bother.
	//if ( victim == ent )
	//{//don't ICARUS_FreeEnt me, I'm in the middle of a script!  (FIXME: shouldn't ICARUS handle this internally?)
	//	victim->svFlags |= SVF_KILLED_SELF;
	//}

	o_health = victim->health;
	victim->health = 0;
	if ( victim->client )
	{
		victim->flags |= FL_NO_KNOCKBACK;
	}
	//G_SetEnemy(victim, ent);
	if( victim->die != NULL )	// check can be omitted
	{
		//GEntity_DieFunc( victim, NULL, NULL, o_health, MOD_UNKNOWN );
		victim->die(victim, victim, victim, o_health, MOD_UNKNOWN);
	}
}

/*
============
Q3_RemoveEnt
  Description	:
  Return type	: void
  Argument		: sharedEntity_t *victim
============
*/
void Q3_RemoveEnt( gentity_t *victim )
{
	if( victim->client )
	{
		if ( victim->s.eType != ET_NPC )
		{
			G_DebugPrint( WL_WARNING, "Q3_RemoveEnt: You can't remove clients in MP!\n" );
			assert(0); //can't remove clients in MP
		}
		else
		{//remove the NPC
			if ( victim->client->NPC_class == CLASS_VEHICLE )
			{//eject everyone out of a vehicle that's about to remove itself
				Vehicle_t *pVeh = victim->m_pVehicle;
				if ( pVeh && pVeh->m_pVehicleInfo )
				{
					pVeh->m_pVehicleInfo->EjectAll( pVeh );
				}
			}
			victim->think = G_FreeEntity;
			victim->nextthink = level.time + 100;
		}
		/*
		//ClientDisconnect(ent);
		victim->s.eFlags |= EF_NODRAW;
		victim->s.eType = ET_INVISIBLE;
		victim->contents = 0;
		victim->health = 0;
		victim->targetname = NULL;

		if ( victim->NPC && victim->NPC->tempGoal != NULL )
		{
			G_FreeEntity( victim->NPC->tempGoal );
			victim->NPC->tempGoal = NULL;
		}
		if ( victim->client->ps.saberEntityNum != ENTITYNUM_NONE && victim->client->ps.saberEntityNum > 0 )
		{
			if ( g_entities[victim->client->ps.saberEntityNum].inuse )
			{
				G_FreeEntity( &g_entities[victim->client->ps.saberEntityNum] );
			}
			victim->client->ps.saberEntityNum = ENTITYNUM_NONE;
		}
		//Disappear in half a second
		victim->e_ThinkFunc = thinkF_G_FreeEntity;
		victim->nextthink = level.time + 500;
		return;
		*/
	}
	else
	{
		victim->think = G_FreeEntity;
		victim->nextthink = level.time + 100;
	}
}


/*
============
Q3_Remove
  Description	:
  Return type	: void
  Argument		:  int entID
  Argument		: const char *name
============
*/
void Q3_Remove( int entID, const char *name )
{
	gentity_t *ent = &g_entities[entID];
	gentity_t	*victim = NULL;

	if( !Q_stricmp( "self", name ) )
	{
		victim = ent;
		if ( !victim )
		{
			G_DebugPrint( WL_WARNING, "Q3_Remove: can't find %s\n", name );
			return;
		}
		Q3_RemoveEnt( victim );
	}
	else if( !Q_stricmp( "enemy", name ) )
	{
		victim = ent->enemy;
		if ( !victim )
		{
			G_DebugPrint( WL_WARNING, "Q3_Remove: can't find %s\n", name );
			return;
		}
		Q3_RemoveEnt( victim );
	}
	else
	{
		victim = G_Find( NULL, FOFS(targetname), (char *) name );
		if ( !victim )
		{
			G_DebugPrint( WL_WARNING, "Q3_Remove: can't find %s\n", name );
			return;
		}

		while ( victim )
		{
			Q3_RemoveEnt( victim );
			victim = G_Find( victim, FOFS(targetname), (char *) name );
		}
	}
}

/*
=================================================

  Get / Set Functions

=================================================
*/

/*
============
Q3_GetFloat
  Description	:
  Return type	: int
  Argument		:  int entID
  Argument		: int type
  Argument		: const char *name
  Argument		: float *value
============
*/
int Q3_GetFloat( int entID, int type, const char *name, float *value )
{
	gentity_t	*ent = &g_entities[entID];
	int toGet = 0;

	if ( !ent )
	{
		return 0;
	}

	toGet = GetIDForString( setTable, name );	//FIXME: May want to make a "getTable" as well
	//FIXME: I'm getting really sick of these huge switch statements!

	//NOTENOTE: return true if the value was correctly obtained
	switch ( toGet )
	{
	case SET_PARM1:
	case SET_PARM2:
	case SET_PARM3:
	case SET_PARM4:
	case SET_PARM5:
	case SET_PARM6:
	case SET_PARM7:
	case SET_PARM8:
	case SET_PARM9:
	case SET_PARM10:
	case SET_PARM11:
	case SET_PARM12:
	case SET_PARM13:
	case SET_PARM14:
	case SET_PARM15:
	case SET_PARM16:
		if (ent->parms == NULL)
		{
			G_DebugPrint( WL_ERROR, "GET_PARM: %s %s did not have any parms set!\n", ent->classname, ent->targetname );
			return 0;	// would prefer qfalse, but I'm fitting in with what's here <sigh>
		}
		*value = atof( ent->parms->parm[toGet - SET_PARM1] );
		break;

	case SET_COUNT:
		*value = ent->count;
		break;

	case SET_HEALTH:
		*value = ent->health;
		break;

	case SET_SKILL:
		return 0;
		break;

	case SET_XVELOCITY://## %f="0.0" # Velocity along X axis
		if ( ent->client == NULL )
		{
			G_DebugPrint( WL_WARNING, "Q3_GetFloat: SET_XVELOCITY, %s not a client\n", ent->targetname );
			return 0;
		}
		*value = ent->client->ps.velocity[0];
		break;

	case SET_YVELOCITY://## %f="0.0" # Velocity along Y axis
		if ( ent->client == NULL )
		{
			G_DebugPrint( WL_WARNING, "Q3_GetFloat: SET_YVELOCITY, %s not a client\n", ent->targetname );
			return 0;
		}
		*value = ent->client->ps.velocity[1];
		break;

	case SET_ZVELOCITY://## %f="0.0" # Velocity along Z axis
		if ( ent->client == NULL )
		{
			G_DebugPrint( WL_WARNING, "Q3_GetFloat: SET_ZVELOCITY, %s not a client\n", ent->targetname );
			return 0;
		}
		*value = ent->client->ps.velocity[2];
		break;

	case SET_Z_OFFSET:
		*value = ent->r.currentOrigin[2] - ent->s.origin[2];
		break;

	case SET_DPITCH://## %f="0.0" # Pitch for NPC to turn to
		return 0;
		break;

	case SET_DYAW://## %f="0.0" # Yaw for NPC to turn to
		return 0;
		break;

	case SET_WIDTH://## %f="0.0" # Width of NPC bounding box
		*value = ent->r.mins[0];
		break;
	case SET_TIMESCALE://## %f="0.0" # Speed-up slow down game (0 - 1.0)
		return 0;
		break;
	case SET_CAMERA_GROUP_Z_OFS://## %s="NULL" # all ents with this cameraGroup will be focused on
		return 0;
		break;

	case SET_VISRANGE://## %f="0.0" # How far away NPC can see
		return 0;
		break;

	case SET_EARSHOT://## %f="0.0" # How far an NPC can hear
		return 0;
		break;

	case SET_VIGILANCE://## %f="0.0" # How often to look for enemies (0 - 1.0)
		return 0;
		break;

	case SET_GRAVITY://## %f="0.0" # Change this ent's gravity - 800 default
		*value = g_gravity.value;
		break;

	case SET_FACEEYESCLOSED:
	case SET_FACEEYESOPENED:
	case SET_FACEAUX:		//## %f="0.0" # Set face to Aux expression for number of seconds
	case SET_FACEBLINK:		//## %f="0.0" # Set face to Blink expression for number of seconds
	case SET_FACEBLINKFROWN:	//## %f="0.0" # Set face to Blinkfrown expression for number of seconds
	case SET_FACEFROWN:		//## %f="0.0" # Set face to Frown expression for number of seconds
	case SET_FACENORMAL:		//## %f="0.0" # Set face to Normal expression for number of seconds
		G_DebugPrint( WL_WARNING, "Q3_GetFloat: SET_FACE___ not implemented\n" );
		return 0;
		break;
	case SET_WAIT:		//## %f="0.0" # Change an entity's wait field
		*value = ent->wait;
		break;
	case SET_FOLLOWDIST:		//## %f="0.0" # How far away to stay from leader in BS_FOLLOW_LEADER
		return 0;
		break;
	//# #sep ints
	case SET_ANIM_HOLDTIME_LOWER://## %d="0" # Hold lower anim for number of milliseconds
		if ( ent->client == NULL )
		{
			G_DebugPrint( WL_WARNING, "Q3_GetFloat: SET_ANIM_HOLDTIME_LOWER, %s not a client\n", ent->targetname );
			return 0;
		}
		*value = ent->client->ps.legsTimer;
		break;
	case SET_ANIM_HOLDTIME_UPPER://## %d="0" # Hold upper anim for number of milliseconds
		if ( ent->client == NULL )
		{
			G_DebugPrint( WL_WARNING, "Q3_GetFloat: SET_ANIM_HOLDTIME_UPPER, %s not a client\n", ent->targetname );
			return 0;
		}
		*value = ent->client->ps.torsoTimer;
		break;
	case SET_ANIM_HOLDTIME_BOTH://## %d="0" # Hold lower and upper anims for number of milliseconds
		G_DebugPrint( WL_WARNING, "Q3_GetFloat: SET_ANIM_HOLDTIME_BOTH not implemented\n" );
		return 0;
		break;
	case SET_ARMOR://## %d="0" # Change armor
		if ( ent->client == NULL )
		{
			G_DebugPrint( WL_WARNING, "Q3_GetFloat: SET_ARMOR, %s not a client\n", ent->targetname );
			return 0;
		}
		*value = ent->client->ps.stats[STAT_ARMOR];
		break;
	case SET_WALKSPEED://## %d="0" # Change walkSpeed
		return 0;
		break;
	case SET_RUNSPEED://## %d="0" # Change runSpeed
		return 0;
		break;
	case SET_YAWSPEED://## %d="0" # Change yawSpeed
		return 0;
		break;
	case SET_AGGRESSION://## %d="0" # Change aggression 1-5
		return 0;
		break;
	case SET_AIM://## %d="0" # Change aim 1-5
		return 0;
		break;
	case SET_FRICTION://## %d="0" # Change ent's friction - 6 default
		return 0;
		break;
	case SET_SHOOTDIST://## %d="0" # How far the ent can shoot - 0 uses weapon
		return 0;
		break;
	case SET_HFOV://## %d="0" # Horizontal field of view
		return 0;
		break;
	case SET_VFOV://## %d="0" # Vertical field of view
		return 0;
		break;
	case SET_DELAYSCRIPTTIME://## %d="0" # How many seconds to wait before running delayscript
		return 0;
		break;
	case SET_FORWARDMOVE://## %d="0" # NPC move forward -127(back) to 127
		return 0;
		break;
	case SET_RIGHTMOVE://## %d="0" # NPC move right -127(left) to 127
		return 0;
		break;
	case SET_STARTFRAME:	//## %d="0" # frame to start animation sequence on
		return 0;
		break;
	case SET_ENDFRAME:	//## %d="0" # frame to end animation sequence on
		return 0;
		break;
	case SET_ANIMFRAME:	//## %d="0" # of current frame
		return 0;
		break;

	case SET_SHOT_SPACING://## %d="1000" # Time between shots for an NPC - reset to defaults when changes weapon
		return 0;
		break;
	case SET_MISSIONSTATUSTIME://## %d="0" # Amount of time until Mission Status should be shown after death
		return 0;
		break;
	//# #sep booleans
	case SET_IGNOREPAIN://## %t="BOOL_TYPES" # Do not react to pain
		return 0;
		break;
	case SET_IGNOREENEMIES://## %t="BOOL_TYPES" # Do not acquire enemies
		return 0;
		break;
	case SET_IGNOREALERTS://## Do not get enemy set by allies in area(ambush)
		return 0;
		break;
	case SET_DONTSHOOT://## %t="BOOL_TYPES" # Others won't shoot you
		return 0;
		break;
	case SET_NOTARGET://## %t="BOOL_TYPES" # Others won't pick you as enemy
		*value = (ent->flags&FL_NOTARGET);
		break;
	case SET_DONTFIRE://## %t="BOOL_TYPES" # Don't fire your weapon
		return 0;
		break;

	case SET_LOCKED_ENEMY://## %t="BOOL_TYPES" # Keep current enemy until dead
		return 0;
		break;
	case SET_CROUCHED://## %t="BOOL_TYPES" # Force NPC to crouch
		return 0;
		break;
	case SET_WALKING://## %t="BOOL_TYPES" # Force NPC to move at walkSpeed
		return 0;
		break;
	case SET_RUNNING://## %t="BOOL_TYPES" # Force NPC to move at runSpeed
		return 0;
		break;
	case SET_CHASE_ENEMIES://## %t="BOOL_TYPES" # NPC will chase after enemies
		return 0;
		break;
	case SET_LOOK_FOR_ENEMIES://## %t="BOOL_TYPES" # NPC will be on the lookout for enemies
		return 0;
		break;
	case SET_FACE_MOVE_DIR://## %t="BOOL_TYPES" # NPC will face in the direction it's moving
		return 0;
		break;
	case SET_FORCED_MARCH://## %t="BOOL_TYPES" # Force NPC to move at runSpeed
		return 0;
		break;
	case SET_UNDYING://## %t="BOOL_TYPES" # Can take damage down to 1 but not die
		return 0;
		break;
	case SET_NOAVOID://## %t="BOOL_TYPES" # Will not avoid other NPCs or architecture
		return 0;
		break;

	case SET_SOLID://## %t="BOOL_TYPES" # Make yourself notsolid or solid
		*value = ent->r.contents;
		break;
	case SET_PLAYER_USABLE://## %t="BOOL_TYPES" # Can be activateby the player's "use" button
		*value = (ent->r.svFlags&SVF_PLAYER_USABLE);
		break;
	case SET_LOOP_ANIM://## %t="BOOL_TYPES" # For non-NPCs: loop your animation sequence
		return 0;
		break;
	case SET_INTERFACE://## %t="BOOL_TYPES" # Player interface on/off
		G_DebugPrint( WL_WARNING, "Q3_GetFloat: SET_INTERFACE not implemented\n" );
		return 0;
		break;
	case SET_SHIELDS://## %t="BOOL_TYPES" # NPC has no shields (Borg do not adapt)
		return 0;
		break;
	case SET_INVISIBLE://## %t="BOOL_TYPES" # Makes an NPC not solid and not visible
		*value = (ent->s.eFlags&EF_NODRAW);
		break;
	case SET_VAMPIRE://## %t="BOOL_TYPES" # Makes an NPC not solid and not visible
		return 0;
		break;
	case SET_FORCE_INVINCIBLE://## %t="BOOL_TYPES" # Makes an NPC not solid and not visible
		return 0;
		break;
	case SET_GREET_ALLIES://## %t="BOOL_TYPES" # Makes an NPC greet teammates
		return 0;
		break;
	case SET_VIDEO_FADE_IN://## %t="BOOL_TYPES" # Makes video playback fade in
		G_DebugPrint( WL_WARNING, "Q3_GetFloat: SET_VIDEO_FADE_IN not implemented\n" );
		return 0;
		break;
	case SET_VIDEO_FADE_OUT://## %t="BOOL_TYPES" # Makes video playback fade out
		G_DebugPrint( WL_WARNING, "Q3_GetFloat: SET_VIDEO_FADE_OUT not implemented\n" );
		return 0;
		break;
	case SET_PLAYER_LOCKED://## %t="BOOL_TYPES" # Makes it so player cannot move
		return 0;
		break;
	case SET_LOCK_PLAYER_WEAPONS://## %t="BOOL_TYPES" # Makes it so player cannot switch weapons
		return 0;
		break;
	case SET_NO_IMPACT_DAMAGE://## %t="BOOL_TYPES" # Makes it so player cannot switch weapons
		return 0;
		break;
	case SET_NO_KNOCKBACK://## %t="BOOL_TYPES" # Stops this ent from taking knockback from weapons
		*value = (ent->flags&FL_NO_KNOCKBACK);
		break;
	case SET_ALT_FIRE://## %t="BOOL_TYPES" # Force NPC to use altfire when shooting
		return 0;
		break;
	case SET_NO_RESPONSE://## %t="BOOL_TYPES" # NPCs will do generic responses when this is on (usescripts override generic responses as well)
		return 0;
		break;
	case SET_INVINCIBLE://## %t="BOOL_TYPES" # Completely unkillable
		*value = (ent->flags&FL_GODMODE);
		break;
	case SET_MISSIONSTATUSACTIVE:	//# Turns on Mission Status Screen
		return 0;
		break;
	case SET_NO_COMBAT_TALK://## %t="BOOL_TYPES" # NPCs will not do their combat talking noises when this is on
		return 0;
		break;
	case SET_NO_ALERT_TALK://## %t="BOOL_TYPES" # NPCs will not do their combat talking noises when this is on
		return 0;
		break;
	case SET_USE_CP_NEAREST://## %t="BOOL_TYPES" # NPCs will use their closest combat points, not try and find ones next to the player, or flank player
		return 0;
		break;
	case SET_DISMEMBERABLE://## %t="BOOL_TYPES" # NPC will not be affected by force powers
		return 0;
		break;
	case SET_NO_FORCE:
		return 0;
		break;
	case SET_NO_ACROBATICS:
		return 0;
		break;
	case SET_USE_SUBTITLES:
		return 0;
		break;
	case SET_NO_FALLTODEATH://## %t="BOOL_TYPES" # NPC will not be affected by force powers
		return 0;
		break;
	case SET_MORELIGHT://## %t="BOOL_TYPES" # NPCs will use their closest combat points, not try and find ones next to the player, or flank player
		return 0;
		break;
	case SET_TREASONED://## %t="BOOL_TYPES" # Player has turned on his own- scripts will stop: NPCs will turn on him and level changes load the brig
		return 0;
		break;
	case SET_DISABLE_SHADER_ANIM:	//## %t="BOOL_TYPES" # Shaders won't animate
		return 0;
		break;
	case SET_SHADER_ANIM:	//## %t="BOOL_TYPES" # Shader will be under frame control
		return 0;
		break;

	default:
		if ( trap->ICARUS_VariableDeclared( name ) != VTYPE_FLOAT )
			return 0;

		return trap->ICARUS_GetFloatVariable( name, value );
	}

	return 1;
}


/*
============
Q3_GetVector
  Description	:
  Return type	: int
  Argument		:  int entID
  Argument		: int type
  Argument		: const char *name
  Argument		: vec3_t value
============
*/
int Q3_GetVector( int entID, int type, const char *name, vec3_t value )
{
	gentity_t	*ent = &g_entities[entID];
	int toGet = 0;
	if ( !ent )
	{
		return 0;
	}

	toGet = GetIDForString( setTable, name );	//FIXME: May want to make a "getTable" as well
	//FIXME: I'm getting really sick of these huge switch statements!

	//NOTENOTE: return true if the value was correctly obtained
	switch ( toGet )
	{
	case SET_PARM1:
	case SET_PARM2:
	case SET_PARM3:
	case SET_PARM4:
	case SET_PARM5:
	case SET_PARM6:
	case SET_PARM7:
	case SET_PARM8:
	case SET_PARM9:
	case SET_PARM10:
	case SET_PARM11:
	case SET_PARM12:
	case SET_PARM13:
	case SET_PARM14:
	case SET_PARM15:
	case SET_PARM16:
		if ( sscanf( ent->parms->parm[toGet - SET_PARM1], "%f %f %f", &value[0], &value[1], &value[2] ) != 3 ) {
			G_DebugPrint( WL_WARNING, "Q3_GetVector: failed sscanf on SET_PARM%d (%s)\n", toGet, name );
			VectorClear( value );
		}
		break;

	case SET_ORIGIN:
		VectorCopy(ent->r.currentOrigin, value);
		break;

	case SET_ANGLES:
		VectorCopy(ent->r.currentAngles, value);
		break;

	case SET_TELEPORT_DEST://## %v="0.0 0.0 0.0" # Set origin here as soon as the area is clear
		G_DebugPrint( WL_WARNING, "Q3_GetVector: SET_TELEPORT_DEST not implemented\n" );
		return 0;
		break;

	default:

		if ( trap->ICARUS_VariableDeclared( name ) != VTYPE_VECTOR )
			return 0;

		return trap->ICARUS_GetVectorVariable( name, value );
	}

	return 1;
}

/*
============
Q3_GetString
  Description	:
  Return type	: int
  Argument		:  int entID
  Argument		: int type
  Argument		: const char *name
  Argument		: char **value
============
*/
int Q3_GetString( int entID, int type, const char *name, char **value )
{
	gentity_t	*ent = &g_entities[entID];
	int toGet = 0;
	if ( !ent )
	{
		return 0;
	}

	toGet = GetIDForString( setTable, name );	//FIXME: May want to make a "getTable" as well

	switch ( toGet )
	{
	case SET_ANIM_BOTH:
		*value = (char *) Q3_GetAnimBoth( ent );

		if ( !value || !value[0] )
			return 0;

		break;

	case SET_PARM1:
	case SET_PARM2:
	case SET_PARM3:
	case SET_PARM4:
	case SET_PARM5:
	case SET_PARM6:
	case SET_PARM7:
	case SET_PARM8:
	case SET_PARM9:
	case SET_PARM10:
	case SET_PARM11:
	case SET_PARM12:
	case SET_PARM13:
	case SET_PARM14:
	case SET_PARM15:
	case SET_PARM16:
		if ( ent->parms )
		{
			*value = (char *) ent->parms->parm[toGet - SET_PARM1];
		}
		else
		{
			G_DebugPrint( WL_WARNING, "Q3_GetString: invalid ent %s has no parms!\n", ent->targetname );
			return 0;
		}
		break;

	case SET_TARGET:
		*value = (char *) ent->target;
		break;

	case SET_LOCATION:
		return 0;
		break;

	//# #sep Scripts and other file paths
	case SET_SPAWNSCRIPT://## %s="NULL" !!"W:\game\base\scripts\!!#*.txt" # Script to run when spawned //0 - do not change these, these are equal to BSET_SPAWN, etc
		*value = ent->behaviorSet[BSET_SPAWN];
		break;
	case SET_USESCRIPT://## %s="NULL" !!"W:\game\base\scripts\!!#*.txt" # Script to run when used
		*value = ent->behaviorSet[BSET_USE];
		break;
	case SET_AWAKESCRIPT://## %s="NULL" !!"W:\game\base\scripts\!!#*.txt" # Script to run when startled
		*value = ent->behaviorSet[BSET_AWAKE];
		break;
	case SET_ANGERSCRIPT://## %s="NULL" !!"W:\game\base\scripts\!!#*.txt" # Script run when find an enemy for the first time
		*value = ent->behaviorSet[BSET_ANGER];
		break;
	case SET_ATTACKSCRIPT://## %s="NULL" !!"W:\game\base\scripts\!!#*.txt" # Script to run when you shoot
		*value = ent->behaviorSet[BSET_ATTACK];
		break;
	case SET_VICTORYSCRIPT://## %s="NULL" !!"W:\game\base\scripts\!!#*.txt" # Script to run when killed someone
		*value = ent->behaviorSet[BSET_VICTORY];
		break;
	case SET_LOSTENEMYSCRIPT://## %s="NULL" !!"W:\game\base\scripts\!!#*.txt" # Script to run when you can't find your enemy
		*value = ent->behaviorSet[BSET_LOSTENEMY];
		break;
	case SET_PAINSCRIPT://## %s="NULL" !!"W:\game\base\scripts\!!#*.txt" # Script to run when hit
		*value = ent->behaviorSet[BSET_PAIN];
		break;
	case SET_FLEESCRIPT://## %s="NULL" !!"W:\game\base\scripts\!!#*.txt" # Script to run when hit and low health
		*value = ent->behaviorSet[BSET_FLEE];
		break;
	case SET_DEATHSCRIPT://## %s="NULL" !!"W:\game\base\scripts\!!#*.txt" # Script to run when killed
		*value = ent->behaviorSet[BSET_DEATH];
		break;
	case SET_DELAYEDSCRIPT://## %s="NULL" !!"W:\game\base\scripts\!!#*.txt" # Script to run after a delay
		*value = ent->behaviorSet[BSET_DELAYED];
		break;
	case SET_BLOCKEDSCRIPT://## %s="NULL" !!"W:\game\base\scripts\!!#*.txt" # Script to run when blocked by teammate
		*value = ent->behaviorSet[BSET_BLOCKED];
		break;
	case SET_FFIRESCRIPT://## %s="NULL" !!"W:\game\base\scripts\!!#*.txt" # Script to run when player has shot own team repeatedly
		*value = ent->behaviorSet[BSET_FFIRE];
		break;
	case SET_FFDEATHSCRIPT://## %s="NULL" !!"W:\game\base\scripts\!!#*.txt" # Script to run when player kills a teammate
		*value = ent->behaviorSet[BSET_FFDEATH];
		break;

	//# #sep Standard strings
	case SET_ENEMY://## %s="NULL" # Set enemy by targetname
		return 0;
		break;
	case SET_LEADER://## %s="NULL" # Set for BS_FOLLOW_LEADER
		return 0;
		break;
	case SET_CAPTURE://## %s="NULL" # Set captureGoal by targetname
		return 0;
		break;

	case SET_TARGETNAME://## %s="NULL" # Set/change your targetname
		*value = ent->targetname;
		break;
	case SET_PAINTARGET://## %s="NULL" # Set/change what to use when hit
		return 0;
		break;
	case SET_CAMERA_GROUP://## %s="NULL" # all ents with this cameraGroup will be focused on
		return 0;
		break;
	case SET_CAMERA_GROUP_TAG://## %s="NULL" # all ents with this cameraGroup will be focused on
		return 0;
		break;
	case SET_LOOK_TARGET://## %s="NULL" # object for NPC to look at
		G_DebugPrint( WL_WARNING, "Q3_GetString: SET_LOOK_TARGET, NOT SUPPORTED IN MULTIPLAYER\n" );
		break;
	case SET_TARGET2://## %s="NULL" # Set/change your target2: on NPC's: this fires when they're knocked out by the red hypo
		return 0;
		break;

	case SET_REMOVE_TARGET://## %s="NULL" # Target that is fired when someone completes the BS_REMOVE behaviorState
		return 0;
		break;
	case SET_WEAPON:
		return 0;
		break;

	case SET_ITEM:
		return 0;
		break;
	case SET_MUSIC_STATE:
		return 0;
		break;
	//The below cannot be gotten
	case SET_NAVGOAL://## %s="NULL" # *Move to this navgoal then continue script
		G_DebugPrint( WL_WARNING, "Q3_GetString: SET_NAVGOAL not implemented\n" );
		return 0;
		break;
	case SET_VIEWTARGET://## %s="NULL" # Set angles toward ent by targetname
		G_DebugPrint( WL_WARNING, "Q3_GetString: SET_VIEWTARGET not implemented\n" );
		return 0;
		break;
	case SET_WATCHTARGET://## %s="NULL" # Set angles toward ent by targetname
		return 0;
		break;
	case SET_VIEWENTITY:
		G_DebugPrint( WL_WARNING, "Q3_GetString: SET_VIEWENTITY not implemented\n" );
		return 0;
		break;
	case SET_CAPTIONTEXTCOLOR:	//## %s=""  # Color of text RED:WHITE:BLUE: YELLOW
		G_DebugPrint( WL_WARNING, "Q3_GetString: SET_CAPTIONTEXTCOLOR not implemented\n" );
		return 0;
		break;
	case SET_CENTERTEXTCOLOR:	//## %s=""  # Color of text RED:WHITE:BLUE: YELLOW
		G_DebugPrint( WL_WARNING, "Q3_GetString: SET_CENTERTEXTCOLOR not implemented\n" );
		return 0;
		break;
	case SET_SCROLLTEXTCOLOR:	//## %s=""  # Color of text RED:WHITE:BLUE: YELLOW
		G_DebugPrint( WL_WARNING, "Q3_GetString: SET_SCROLLTEXTCOLOR not implemented\n" );
		return 0;
		break;
	case SET_COPY_ORIGIN://## %s="targetname"  # Copy the origin of the ent with targetname to your origin
		G_DebugPrint( WL_WARNING, "Q3_GetString: SET_COPY_ORIGIN not implemented\n" );
		return 0;
		break;
	case SET_DEFEND_TARGET://## %s="targetname"  # This NPC will attack the target NPC's enemies
		G_DebugPrint( WL_WARNING, "Q3_GetString: SET_COPY_ORIGIN not implemented\n" );
		return 0;
		break;
	case SET_VIDEO_PLAY://## %s="filename" !!"W:\game\base\video\!!#*.roq" # Play a Video (inGame)
		G_DebugPrint( WL_WARNING, "Q3_GetString: SET_VIDEO_PLAY not implemented\n" );
		return 0;
		break;
	case SET_LOADGAME://## %s="exitholodeck" # Load the savegame that was auto-saved when you started the holodeck
		G_DebugPrint( WL_WARNING, "Q3_GetString: SET_LOADGAME not implemented\n" );
		return 0;
		break;
	case SET_LOCKYAW://## %s="off"  # Lock legs to a certain yaw angle (or "off" or "auto" uses current)
		G_DebugPrint( WL_WARNING, "Q3_GetString: SET_LOCKYAW not implemented\n" );
		return 0;
		break;
	case SET_SCROLLTEXT:	//## %s="" # key of text string to print
		G_DebugPrint( WL_WARNING, "Q3_GetString: SET_SCROLLTEXT not implemented\n" );
		return 0;
		break;
	case SET_LCARSTEXT:	//## %s="" # key of text string to print in LCARS frame
		G_DebugPrint( WL_WARNING, "Q3_GetString: SET_LCARSTEXT not implemented\n" );
		return 0;
		break;

	case SET_FULLNAME://## %s="NULL" # Set/change your targetname
		*value = ent->fullName;
		break;
	default:

		if ( trap->ICARUS_VariableDeclared( name ) != VTYPE_STRING )
			return 0;

		return trap->ICARUS_GetStringVariable( name, (const char *) *value );
	}

	return 1;
}

/*
============
MoveOwner
  Description	:
  Return type	: void
  Argument		: sharedEntity_t *self
============
*/
qboolean SpotWouldTelefrag2( gentity_t *mover, vec3_t dest );
void MoveOwner( gentity_t *self )
{
	gentity_t *owner = &g_entities[self->r.ownerNum];

	self->nextthink = level.time + FRAMETIME;
	self->think = G_FreeEntity;

	if ( !owner || !owner->inuse )
	{
		return;
	}

	if ( SpotWouldTelefrag2( owner, self->r.currentOrigin ) )
	{
		self->think = MoveOwner;
	}
	else
	{
		G_SetOrigin( owner, self->r.currentOrigin );
		trap->ICARUS_TaskIDComplete( (sharedEntity_t *)owner, TID_MOVE_NAV );
	}
}

/*
=============
Q3_SetTeleportDest

Copies passed origin to ent running script once there is nothing there blocking the spot
=============
*/
static qboolean Q3_SetTeleportDest( int entID, vec3_t org )
{
	gentity_t	*teleEnt = &g_entities[entID];

	if ( teleEnt )
	{
		if ( SpotWouldTelefrag2( teleEnt, org ) )
		{
			gentity_t *teleporter = G_Spawn();

			G_SetOrigin( teleporter, org );
			teleporter->r.ownerNum = teleEnt->s.number;

			teleporter->think = MoveOwner;
			teleporter->nextthink = level.time + FRAMETIME;

			return qfalse;
		}
		else
		{
			G_SetOrigin( teleEnt, org );
		}
	}

	return qtrue;
}

/*
=============
Q3_SetOrigin

Sets the origin of an entity directly
=============
*/
static void Q3_SetOrigin( int entID, vec3_t origin )
{
	gentity_t	*ent = &g_entities[entID];

	if ( !ent )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetOrigin: bad ent %d\n", entID);
		return;
	}

	trap->UnlinkEntity ((sharedEntity_t *)ent);

	if(ent->client)
	{
		VectorCopy(origin, ent->client->ps.origin);
		VectorCopy(origin, ent->r.currentOrigin);
		ent->client->ps.origin[2] += 1;

		VectorClear (ent->client->ps.velocity);
		ent->client->ps.pm_time = 160;		// hold time
		ent->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;

		ent->client->ps.eFlags ^= EF_TELEPORT_BIT;

//		G_KillBox (ent);
	}
	else
	{
		G_SetOrigin( ent, origin );
	}

	trap->LinkEntity( (sharedEntity_t *)ent );
}

/*
=============
Q3_SetCopyOrigin

Copies origin of found ent into ent running script
=============`
*/
static void Q3_SetCopyOrigin( int entID, const char *name )
{
	gentity_t	*found = G_Find( NULL, FOFS(targetname), (char *) name);

	if(found)
	{
		Q3_SetOrigin( entID, found->r.currentOrigin );
		SetClientViewAngle( &g_entities[entID], found->s.angles );
	}
	else
	{
		G_DebugPrint( WL_WARNING, "Q3_SetCopyOrigin: ent %s not found!\n", name);
	}
}

/*
=============
Q3_SetVelocity

Set the velocity of an entity directly
=============
*/
static void Q3_SetVelocity( int entID, int axis, float speed )
{
	gentity_t	*found = &g_entities[entID];
	//FIXME: Not supported
	if(!found)
	{
		G_DebugPrint( WL_WARNING, "Q3_SetVelocity invalid entID %d\n", entID);
		return;
	}

	if(!found->client)
	{
		G_DebugPrint( WL_WARNING, "Q3_SetVelocity: not a client %d\n", entID);
		return;
	}

	//FIXME: add or set?
	found->client->ps.velocity[axis] += speed;

	found->client->ps.pm_time = 500;
	found->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
}

/*
=============
Q3_SetAngles

Sets the angles of an entity directly
=============
*/
static void Q3_SetAngles( int entID, vec3_t angles )
{
	gentity_t	*ent = &g_entities[entID];


	if ( !ent )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetAngles: bad ent %d\n", entID);
		return;
	}

	if (ent->client)
	{
		SetClientViewAngle( ent, angles );
	}
	else
	{
		VectorCopy( angles, ent->s.angles );
	}
	trap->LinkEntity( (sharedEntity_t *)ent );
}

/*
=============
Q3_Lerp2Origin

Lerps the origin to the destination value
=============
*/
void Q3_Lerp2Origin( int taskID, int entID, vec3_t origin, float duration )
{
	gentity_t	*ent = &g_entities[entID];
	moverState_t moverState;

	if(!ent)
	{
		G_DebugPrint( WL_WARNING, "Q3_Lerp2Origin: invalid entID %d\n", entID);
		return;
	}

	if ( ent->client || Q_stricmp(ent->classname, "target_scriptrunner") == 0 )
	{
		G_DebugPrint( WL_ERROR, "Q3_Lerp2Origin: ent %d is NOT a mover!\n", entID);
		return;
	}

	if ( ent->s.eType != ET_MOVER )
	{
		ent->s.eType = ET_MOVER;
	}

	moverState = ent->moverState;

	if ( moverState == MOVER_POS1 || moverState == MOVER_2TO1 )
	{
		VectorCopy( ent->r.currentOrigin, ent->pos1 );
		VectorCopy( origin, ent->pos2 );

		moverState = MOVER_1TO2;
	}
	else if ( moverState == MOVER_POS2 || moverState == MOVER_1TO2 )
	{
		VectorCopy( ent->r.currentOrigin, ent->pos2 );
		VectorCopy( origin, ent->pos1 );

		moverState = MOVER_2TO1;
	}

	InitMoverTrData( ent );	//FIXME: This will probably break normal things that are being moved...

	ent->s.pos.trDuration = duration;

	// start it going
	MatchTeam( ent, moverState, level.time );
	//SetMoverState( ent, moverState, level.time );

	ent->reached = moverCallback;
	if ( ent->damage )
	{
		ent->blocked = Blocked_Mover;
	}
	if ( taskID != -1 )
	{
		trap->ICARUS_TaskIDSet( (sharedEntity_t *)ent, TID_MOVE_NAV, taskID );
	}
	// starting sound
	G_PlayDoorLoopSound( ent );//start looping sound
	G_PlayDoorSound( ent, BMS_START );	//play start sound

	trap->LinkEntity( (sharedEntity_t *)ent );
}

static void Q3_SetOriginOffset( int entID, int axis, float offset )
{
	gentity_t	*ent = &g_entities[entID];
	vec3_t origin;
	float duration;

	if(!ent)
	{
		G_DebugPrint( WL_WARNING, "Q3_SetOriginOffset: invalid entID %d\n", entID);
		return;
	}

	if ( ent->client || Q_stricmp(ent->classname, "target_scriptrunner") == 0 )
	{
		G_DebugPrint( WL_ERROR, "Q3_SetOriginOffset: ent %d is NOT a mover!\n", entID);
		return;
	}

	VectorCopy( ent->s.origin, origin );
	origin[axis] += offset;
	duration = 0;
	if ( ent->speed )
	{
		duration = fabs(offset)/fabs(ent->speed)*1000.0f;
	}
	Q3_Lerp2Origin( -1, entID, origin, duration );
}

/*
=============
Q3_SetEnemy

Sets the enemy of an entity
=============
*/
static void Q3_SetEnemy( int entID, const char *name )
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetEnemy: invalid entID %d\n", entID);
		return;
	}

	if( !Q_stricmp("NONE", name) || !Q_stricmp("NULL", name))
	{
		if(ent->NPC)
		{
			G_ClearEnemy(ent);
		}
		else
		{
			ent->enemy = NULL;
		}
	}
	else
	{
		gentity_t	*enemy = G_Find( NULL, FOFS(targetname), (char *) name);

		if(enemy == NULL)
		{
			G_DebugPrint( WL_ERROR, "Q3_SetEnemy: no such enemy: '%s'\n", name );
			return;
		}
		/*else if(enemy->health <= 0)
		{
			//G_DebugPrint( WL_ERROR, "Q3_SetEnemy: ERROR - desired enemy has health %d\n", enemy->health );
			return;
		}*/
		else
		{
			if(ent->NPC)
			{
				G_SetEnemy( ent, enemy );
				ent->cantHitEnemyCounter = 0;
			}
			else
			{
				G_SetEnemy(ent, enemy);
			}
		}
	}
}


/*
=============
Q3_SetLeader

Sets the leader of an NPC
=============
*/
static void Q3_SetLeader( int entID, const char *name )
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetLeader: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->client )
	{
		G_DebugPrint( WL_ERROR, "Q3_SetLeader: ent %d is NOT a player or NPC!\n", entID);
		return;
	}

	if( !Q_stricmp("NONE", name) || !Q_stricmp("NULL", name))
	{
		ent->client->leader = NULL;
	}
	else
	{
		gentity_t	*leader = G_Find( NULL, FOFS(targetname), (char *) name);

		if(leader == NULL)
		{
			//G_DebugPrint( WL_ERROR,"Q3_SetEnemy: unable to locate enemy: '%s'\n", name );
			return;
		}
		else if(leader->health <= 0)
		{
			//G_DebugPrint( WL_ERROR,"Q3_SetEnemy: ERROR - desired enemy has health %d\n", enemy->health );
			return;
		}
		else
		{
			ent->client->leader = leader;
		}
	}
}

/*
=============
Q3_SetNavGoal

Sets the navigational goal of an entity
=============
*/
static qboolean Q3_SetNavGoal( int entID, const char *name )
{
	gentity_t	*ent  = &g_entities[ entID ];
	vec3_t		goalPos;

	if ( !ent->health )
	{
		G_DebugPrint( WL_ERROR, "Q3_SetNavGoal: tried to set a navgoal (\"%s\") on a corpse! \"%s\"\n", name, ent->script_targetname );
		return qfalse;
	}
	if ( !ent->NPC )
	{
		G_DebugPrint( WL_ERROR, "Q3_SetNavGoal: tried to set a navgoal (\"%s\") on a non-NPC: \"%s\"\n", name, ent->script_targetname );
		return qfalse;
	}
	if ( !ent->NPC->tempGoal )
	{
		G_DebugPrint( WL_ERROR, "Q3_SetNavGoal: tried to set a navgoal (\"%s\") on a dead NPC: \"%s\"\n", name, ent->script_targetname );
		return qfalse;
	}
	if ( !ent->NPC->tempGoal->inuse )
	{
		G_DebugPrint( WL_ERROR, "Q3_SetNavGoal: NPC's (\"%s\") navgoal is freed: \"%s\"\n", name, ent->script_targetname );
		return qfalse;
	}
	if( Q_stricmp( "null", name) == 0
		|| Q_stricmp( "NULL", name) == 0 )
	{
		ent->NPC->goalEntity = NULL;
		trap->ICARUS_TaskIDComplete( (sharedEntity_t *)ent, TID_MOVE_NAV );
		return qfalse;
	}
	else
	{
		//Get the position of the goal
		if ( TAG_GetOrigin2( NULL, name, goalPos ) == qfalse )
		{
			gentity_t	*targ = G_Find(NULL, FOFS(targetname), (char*)name);
			if ( !targ )
			{
				G_DebugPrint( WL_ERROR, "Q3_SetNavGoal: can't find NAVGOAL \"%s\"\n", name );
				return qfalse;
			}
			else
			{
				ent->NPC->goalEntity = targ;
				ent->NPC->goalRadius = sqrt(ent->r.maxs[0]+ent->r.maxs[0]) + sqrt(targ->r.maxs[0]+targ->r.maxs[0]);
				ent->NPC->aiFlags &= ~NPCAI_TOUCHED_GOAL;
			}
		}
		else
		{
			int	goalRadius = TAG_GetRadius( NULL, name );
			NPC_SetMoveGoal( ent, goalPos, goalRadius, qtrue, -1, NULL );
			//We know we want to clear the lastWaypoint here
			ent->NPC->goalEntity->lastWaypoint = WAYPOINT_NONE;
			ent->NPC->aiFlags &= ~NPCAI_TOUCHED_GOAL;
	#ifdef _DEBUG
			//this is *only* for debugging navigation
			ent->NPC->tempGoal->target = G_NewString( name );
	#endif// _DEBUG
		return qtrue;
		}
	}
	return qfalse;
}
/*
============
SetLowerAnim
  Description	:
  Return type	: static void
  Argument		:  int entID
  Argument		: int animID
============
*/
static void SetLowerAnim( int entID, int animID)
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		G_DebugPrint( WL_WARNING, "SetLowerAnim: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->client )
	{
		G_DebugPrint( WL_ERROR, "SetLowerAnim: ent %d is NOT a player or NPC!\n", entID);
		return;
	}

	G_SetAnim(ent,NULL,SETANIM_LEGS,animID,SETANIM_FLAG_RESTART|SETANIM_FLAG_HOLD|SETANIM_FLAG_OVERRIDE,0);
}

/*
============
SetUpperAnim
  Description	:
  Return type	: static void
  Argument		:  int entID
  Argument		: int animID
============
*/
static void SetUpperAnim ( int entID, int animID)
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		G_DebugPrint( WL_WARNING, "SetUpperAnim: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->client )
	{
		G_DebugPrint( WL_ERROR, "SetLowerAnim: ent %d is NOT a player or NPC!\n", entID);
		return;
	}

	G_SetAnim(ent,NULL,SETANIM_TORSO,animID,SETANIM_FLAG_RESTART|SETANIM_FLAG_HOLD|SETANIM_FLAG_OVERRIDE,0);
}

/*
=============
Q3_SetAnimUpper

Sets the upper animation of an entity
=============
*/
static qboolean Q3_SetAnimUpper( int entID, const char *anim_name )
{
	int			animID = 0;

	animID = GetIDForString( animTable, anim_name );

	if( animID == -1 )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetAnimUpper: unknown animation sequence '%s'\n", anim_name );
		return qfalse;
	}

	/*
	if ( !PM_HasAnimation( SV_GentityNum(entID), animID ) )
	{
		return qfalse;
	}
	*/

	SetUpperAnim( entID, animID );
	return qtrue;
}

/*
=============
Q3_SetAnimLower

Sets the lower animation of an entity
=============
*/
static qboolean Q3_SetAnimLower( int entID, const char *anim_name )
{
	int			animID = 0;

	//FIXME: Setting duck anim does not actually duck!

	animID = GetIDForString( animTable, anim_name );

	if( animID == -1 )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetAnimLower: unknown animation sequence '%s'\n", anim_name );
		return qfalse;
	}

	/*
	if ( !PM_HasAnimation( SV_GentityNum(entID), animID ) )
	{
		return qfalse;
	}
	*/

	SetLowerAnim( entID, animID );
	return qtrue;
}

/*
============
Q3_SetAnimHoldTime
  Description	:
  Return type	: static void
  Argument		:  int entID
  Argument		: int int_data
  Argument		: qboolean lower
============
*/
static void Q3_SetAnimHoldTime( int entID, int int_data, qboolean lower )
{
	G_DebugPrint( WL_WARNING, "Q3_SetAnimHoldTime is not currently supported in MP\n");
	/*
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetAnimHoldTime: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->client )
	{
		G_DebugPrint( WL_ERROR, "Q3_SetAnimHoldTime: ent %d is NOT a player or NPC!\n", entID);
		return;
	}

	if(lower)
	{
		PM_SetLegsAnimTimer( ent, &ent->client->ps.legsAnimTimer, int_data );
	}
	else
	{
		PM_SetTorsoAnimTimer( ent, &ent->client->ps.torsoAnimTimer, int_data );
	}
	*/
}

/*
============
Q3_SetHealth
  Description	:
  Return type	: static void
  Argument		:  int entID
  Argument		: int data
============
*/
static void Q3_SetHealth( int entID, int data )
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetHealth: invalid entID %d\n", entID);
		return;
	}

	if ( data < 0 )
	{
		data = 0;
	}

	ent->health = data;

	if(!ent->client)
	{
		return;
	}

	ent->client->ps.stats[STAT_HEALTH] = data;

	if ( ent->client->ps.stats[STAT_HEALTH] > ent->client->ps.stats[STAT_MAX_HEALTH] )
	{
		ent->health = ent->client->ps.stats[STAT_HEALTH] = ent->client->ps.stats[STAT_MAX_HEALTH];
	}
	if ( data == 0 )
	{
		ent->health = 1;
		if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR )
		{ //this would be silly
			return;
		}

		if ( ent->client->tempSpectate >= level.time )
		{ //this would also be silly
			return;
		}

		ent->flags &= ~FL_GODMODE;
		ent->client->ps.stats[STAT_HEALTH] = ent->health = -999;
		player_die (ent, ent, ent, 100000, MOD_FALLING);
	}
}


/*
============
Q3_SetArmor
  Description	:
  Return type	: static void
  Argument		:  int entID
  Argument		: int data
============
*/
static void Q3_SetArmor( int entID, int data )
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetArmor: invalid entID %d\n", entID);
		return;
	}

	if(!ent->client)
	{
		return;
	}

	ent->client->ps.stats[STAT_ARMOR] = data;
	if ( ent->client->ps.stats[STAT_ARMOR] > ent->client->ps.stats[STAT_MAX_HEALTH] )
	{
		ent->client->ps.stats[STAT_ARMOR] = ent->client->ps.stats[STAT_MAX_HEALTH];
	}
}


/*
============
Q3_SetBState
  Description	:
  Return type	: static qboolean
  Argument		:  int entID
  Argument		: const char *bs_name
FIXME: this should be a general NPC wrapper function
	that is called ANY time	a bState is changed...
============
*/
static qboolean Q3_SetBState( int entID, const char *bs_name )
{
	gentity_t	*ent  = &g_entities[entID];
	bState_t	bSID;

	if ( !ent )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetBState: invalid entID %d\n", entID);
		return qtrue;
	}

	if ( !ent->NPC )
	{
		G_DebugPrint( WL_ERROR, "Q3_SetBState: '%s' is not an NPC\n", ent->targetname );
		return qtrue;//ok to complete
	}

	bSID = (bState_t)(GetIDForString( BSTable, bs_name ));
	if ( bSID != (bState_t)-1 )
	{
		if ( bSID == BS_SEARCH || bSID == BS_WANDER )
		{
			//FIXME: Reimplement

			if( ent->waypoint != WAYPOINT_NONE )
			{
				NPC_BSSearchStart( ent->waypoint, bSID );
			}
			else
			{
				ent->waypoint = NAV_FindClosestWaypointForEnt( ent, WAYPOINT_NONE );

				if( ent->waypoint != WAYPOINT_NONE )
				{
					NPC_BSSearchStart( ent->waypoint, bSID );
				}
				/*else if( ent->lastWaypoint >=0 && ent->lastWaypoint < num_waypoints )
				{
					NPC_BSSearchStart( ent->lastWaypoint, bSID );
				}
				else if( ent->lastValidWaypoint >=0 && ent->lastValidWaypoint < num_waypoints )
				{
					NPC_BSSearchStart( ent->lastValidWaypoint, bSID );
				}*/
				else
				{
					G_DebugPrint( WL_ERROR, "Q3_SetBState: '%s' is not in a valid waypoint to search from!\n", ent->targetname );
					return qtrue;
				}
			}
		}


		ent->NPC->tempBehavior = BS_DEFAULT;//need to clear any temp behaviour
		if ( ent->NPC->behaviorState == BS_NOCLIP && bSID != BS_NOCLIP )
		{//need to rise up out of the floor after noclipping
			ent->r.currentOrigin[2] += 0.125;
			G_SetOrigin( ent, ent->r.currentOrigin );
		}
		ent->NPC->behaviorState = bSID;
		if ( bSID == BS_DEFAULT )
		{
			ent->NPC->defaultBehavior = bSID;
		}
	}

	ent->NPC->aiFlags &= ~NPCAI_TOUCHED_GOAL;

//	if ( bSID == BS_FLY )
//	{//FIXME: need a set bState wrapper
//		ent->client->moveType = MT_FLYSWIM;
//	}
//	else
	{
		//FIXME: these are presumptions!
		//Q3_SetGravity( entID, g_gravity->value );
		//ent->client->moveType = MT_RUNJUMP;
	}

	if ( bSID == BS_NOCLIP )
	{
		ent->client->noclip = qtrue;
	}
	else
	{
		ent->client->noclip = qfalse;
	}

/*
	if ( bSID == BS_FACE || bSID == BS_POINT_AND_SHOOT || bSID == BS_FACE_ENEMY )
	{
		ent->NPC->aimTime = level.time + 5 * 1000;//try for 5 seconds
		return qfalse;//need to wait for task complete message
	}
*/

//	if ( bSID == BS_SNIPER || bSID == BS_ADVANCE_FIGHT )
	if ( bSID == BS_ADVANCE_FIGHT )
	{
		return qfalse;//need to wait for task complete message
	}

/*
	if ( bSID == BS_SHOOT || bSID == BS_POINT_AND_SHOOT )
	{//Let them shoot right NOW
		ent->NPC->shotTime = ent->attackDebounceTime = level.time;
	}
*/
	if ( bSID == BS_JUMP )
	{
		ent->NPC->jumpState = JS_FACING;
	}

	return qtrue;//ok to complete
}


/*
============
Q3_SetTempBState
  Description	:
  Return type	: static qboolean
  Argument		:  int entID
  Argument		: const char *bs_name
============
*/
static qboolean Q3_SetTempBState( int entID, const char *bs_name )
{
	gentity_t	*ent  = &g_entities[entID];
	bState_t	bSID;

	if ( !ent )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetTempBState: invalid entID %d\n", entID);
		return qtrue;
	}

	if ( !ent->NPC )
	{
		G_DebugPrint( WL_ERROR, "Q3_SetTempBState: '%s' is not an NPC\n", ent->targetname );
		return qtrue;//ok to complete
	}

	bSID = (bState_t)(GetIDForString( BSTable, bs_name ));
	if ( bSID != (bState_t)-1 )
	{
		ent->NPC->tempBehavior = bSID;
	}

/*
	if ( bSID == BS_FACE || bSID == BS_POINT_AND_SHOOT || bSID == BS_FACE_ENEMY )
	{
		ent->NPC->aimTime = level.time + 5 * 1000;//try for 5 seconds
		return qfalse;//need to wait for task complete message
	}
*/

/*
	if ( bSID == BS_SHOOT || bSID == BS_POINT_AND_SHOOT )
	{//Let them shoot right NOW
		ent->NPC->shotTime = ent->attackDebounceTime = level.time;
	}
*/
	return qtrue;//ok to complete
}


/*
============
Q3_SetDefaultBState
  Description	:
  Return type	: static void
  Argument		:  int entID
  Argument		: const char *bs_name
============
*/
static void Q3_SetDefaultBState( int entID, const char *bs_name )
{
	gentity_t	*ent  = &g_entities[entID];
	bState_t	bSID;

	if ( !ent )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetDefaultBState: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->NPC )
	{
		G_DebugPrint( WL_ERROR, "Q3_SetDefaultBState: '%s' is not an NPC\n", ent->targetname );
		return;
	}

	bSID = (bState_t)(GetIDForString( BSTable, bs_name ));
	if ( bSID != (bState_t)-1 )
	{
		ent->NPC->defaultBehavior = bSID;
	}
}


/*
============
Q3_SetDPitch
  Description	:
  Return type	: static void
  Argument		:  int entID
  Argument		: float data
============
*/
static void Q3_SetDPitch( int entID, float data )
{
	gentity_t	*ent  = &g_entities[entID];
	int pitchMin;
	int pitchMax;

	if ( !ent )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetDPitch: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->NPC || !ent->client )
	{
		G_DebugPrint( WL_ERROR, "Q3_SetDPitch: '%s' is not an NPC\n", ent->targetname );
		return;
	}

	pitchMin = -ent->client->renderInfo.headPitchRangeUp + 1;
	pitchMax = ent->client->renderInfo.headPitchRangeDown - 1;

	//clamp angle to -180 -> 180
	data = AngleNormalize180( data );

	//Clamp it to my valid range
	if ( data < -1 )
	{
		if ( data < pitchMin )
		{
			data = pitchMin;
		}
	}
	else if ( data > 1 )
	{
		if ( data > pitchMax )
		{
			data = pitchMax;
		}
	}

	ent->NPC->lockedDesiredPitch = ent->NPC->desiredPitch = data;
}


/*
============
Q3_SetDYaw
  Description	:
  Return type	: static void
  Argument		:  int entID
  Argument		: float data
============
*/
static void Q3_SetDYaw( int entID, float data )
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetDYaw: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->NPC )
	{
		G_DebugPrint( WL_ERROR, "Q3_SetDYaw: '%s' is not an NPC\n", ent->targetname );
		return;
	}

	if(!ent->enemy)
	{//don't mess with this if they're aiming at someone
		ent->NPC->lockedDesiredYaw = ent->NPC->desiredYaw = ent->s.angles[1] = data;
	}
	else
	{
		G_DebugPrint( WL_WARNING, "Could not set DYAW: '%s' has an enemy (%s)!\n", ent->targetname, ent->enemy->targetname );
	}
}


/*
============
Q3_SetShootDist
  Description	:
  Return type	: static void
  Argument		:  int entID
  Argument		: float data
============
*/
static void Q3_SetShootDist( int entID, float data )
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetShootDist: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->NPC )
	{
		G_DebugPrint( WL_ERROR, "Q3_SetShootDist: '%s' is not an NPC\n", ent->targetname );
		return;
	}

	ent->NPC->stats.shootDistance = data;
}


/*
============
Q3_SetVisrange
  Description	:
  Return type	: static void
  Argument		:  int entID
  Argument		: float data
============
*/
static void Q3_SetVisrange( int entID, float data )
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetVisrange: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->NPC )
	{
		G_DebugPrint( WL_ERROR, "Q3_SetVisrange: '%s' is not an NPC\n", ent->targetname );
		return;
	}

	ent->NPC->stats.visrange = data;
}


/*
============
Q3_SetEarshot
  Description	:
  Return type	: static void
  Argument		:  int entID
  Argument		: float data
============
*/
static void Q3_SetEarshot( int entID, float data )
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetEarshot: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->NPC )
	{
		G_DebugPrint( WL_ERROR, "Q3_SetEarshot: '%s' is not an NPC\n", ent->targetname );
		return;
	}

	ent->NPC->stats.earshot = data;
}


/*
============
Q3_SetVigilance
  Description	:
  Return type	: static void
  Argument		:  int entID
  Argument		: float data
============
*/
static void Q3_SetVigilance( int entID, float data )
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetVigilance: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->NPC )
	{
		G_DebugPrint( WL_ERROR, "Q3_SetVigilance: '%s' is not an NPC\n", ent->targetname );
		return;
	}

	ent->NPC->stats.vigilance = data;
}


/*
============
Q3_SetVFOV
  Description	:
  Return type	: static void
  Argument		:  int entID
  Argument		: int data
============
*/
static void Q3_SetVFOV( int entID, int data )
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetVFOV: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->NPC )
	{
		G_DebugPrint( WL_ERROR, "Q3_SetVFOV: '%s' is not an NPC\n", ent->targetname );
		return;
	}

	ent->NPC->stats.vfov = data;
}


/*
============
Q3_SetHFOV
  Description	:
  Return type	: static void
  Argument		:  int entID
  Argument		: int data
============
*/
static void Q3_SetHFOV( int entID, int data )
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetHFOV: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->NPC )
	{
		G_DebugPrint( WL_ERROR, "Q3_SetHFOV: '%s' is not an NPC\n", ent->targetname );
		return;
	}

	ent->NPC->stats.hfov = data;
}


/*
============
Q3_SetWidth
  Description	:
  Return type	: static void
  Argument		:  int entID
  Argument		: float data
============
*/
static void Q3_SetWidth( int entID, int data )
{
	G_DebugPrint( WL_WARNING, "Q3_SetWidth: NOT SUPPORTED IN MP\n");
	return;
}

/*
============
Q3_SetTimeScale
  Description	:
  Return type	: static void
  Argument		:  int entID
  Argument		: const char *data
============
*/
static void Q3_SetTimeScale( int entID, const char *data )
{
	trap->Cvar_Set("timescale", data);
}


/*
============
Q3_SetInvisible
  Description	:
  Return type	: static void
  Argument		:  int entID
  Argument		: qboolean invisible
============
*/
static void Q3_SetInvisible( int entID, qboolean invisible )
{
	gentity_t	*self  = &g_entities[entID];

	if ( !self )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetInvisible: invalid entID %d\n", entID);
		return;
	}

	if ( invisible )
	{
		self->s.eFlags |= EF_NODRAW;
		if ( self->client )
		{
			self->client->ps.eFlags |= EF_NODRAW;
		}
		self->r.contents = 0;
	}
	else
	{
		self->s.eFlags &= ~EF_NODRAW;
		if ( self->client )
		{
			self->client->ps.eFlags &= ~EF_NODRAW;
		}
	}
}

/*
============
Q3_SetVampire
  Description	:
  Return type	: static void
  Argument		:  int entID
  Argument		: qboolean vampire
============
*/
static void Q3_SetVampire( int entID, qboolean vampire )
{
	G_DebugPrint( WL_WARNING, "Q3_SetVampire: NOT SUPPORTED IN MP\n");
	return;
}
/*
============
Q3_SetGreetAllies
  Description	:
  Return type	: static void
  Argument		:  int entID
  Argument		: qboolean greet
============
*/
static void Q3_SetGreetAllies( int entID, qboolean greet )
{
	gentity_t	*self  = &g_entities[entID];

	if ( !self )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetGreetAllies: invalid entID %d\n", entID);
		return;
	}

	if ( !self->NPC )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetGreetAllies: ent %s is not an NPC!\n", self->targetname );
		return;
	}

	if ( greet )
	{
		self->NPC->aiFlags |= NPCAI_GREET_ALLIES;
	}
	else
	{
		self->NPC->aiFlags &= ~NPCAI_GREET_ALLIES;
	}
}


/*
============
Q3_SetViewTarget
  Description	:
  Return type	: static void
  Argument		: int entID
  Argument		: const char *name
============
*/
static void Q3_SetViewTarget (int entID, const char *name)
{
	gentity_t	*self  = &g_entities[entID];
	gentity_t	*viewtarget = G_Find( NULL, FOFS(targetname), (char *) name);
	vec3_t		viewspot, selfspot, viewvec, viewangles;

	if ( !self )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetViewTarget: invalid entID %d\n", entID);
		return;
	}

	if ( !self->client )
	{
		G_DebugPrint( WL_ERROR, "Q3_SetViewTarget: '%s' is not a player/NPC!\n", self->targetname );
		return;
	}

	//FIXME: Exception handle here
	if (viewtarget == NULL)
	{
		G_DebugPrint( WL_WARNING, "Q3_SetViewTarget: can't find ViewTarget: '%s'\n", name );
		return;
	}

	//FIXME: should we set behavior to BS_FACE and keep facing this ent as it moves
	//around for a script-specified length of time...?
	VectorCopy ( self->s.origin, selfspot );
	selfspot[2] += self->client->ps.viewheight;

	if ( viewtarget->client )
	{
		VectorCopy ( viewtarget->client->renderInfo.eyePoint, viewspot );
	}
	else
	{
		VectorCopy ( viewtarget->s.origin, viewspot );
	}

	VectorSubtract( viewspot, selfspot, viewvec );

	vectoangles( viewvec, viewangles );

	Q3_SetDYaw( entID, viewangles[YAW] );
	Q3_SetDPitch( entID, viewangles[PITCH] );
}


/*
============
Q3_SetWatchTarget
  Description	:
  Return type	: static void
  Argument		: int entID
  Argument		: const char *name
============
*/
static void Q3_SetWatchTarget (int entID, const char *name)
{
	gentity_t	*self  = &g_entities[entID];
	gentity_t	*watchTarget = NULL;

	if ( !self )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetWatchTarget: invalid entID %d\n", entID);
		return;
	}

	if ( !self->NPC )
	{
		G_DebugPrint( WL_ERROR, "Q3_SetWatchTarget: '%s' is not an NPC!\n", self->targetname );
		return;
	}

	if ( Q_stricmp( "NULL", name ) == 0 || Q_stricmp( "NONE", name ) == 0 || ( self->targetname && (Q_stricmp( self->targetname, name ) == 0) ) )
	{//clearing watchTarget
		self->NPC->watchTarget = NULL;
	}

	watchTarget = G_Find( NULL, FOFS(targetname), (char *) name);
	if ( watchTarget == NULL )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetWatchTarget: can't find WatchTarget: '%s'\n", name );
		return;
	}

	self->NPC->watchTarget = watchTarget;
}

void Q3_SetLoopSound(int entID, const char *name)
{
	sfxHandle_t	index;
	gentity_t	*self  = &g_entities[entID];

	if ( Q_stricmp( "NULL", name ) == 0 || Q_stricmp( "NONE", name )==0)
	{
		self->s.loopSound = 0;
		self->s.loopIsSoundset = qfalse;
		return;
	}

	index = G_SoundIndex( (char*)name );

	if (index)
	{
		self->s.loopSound = index;
		self->s.loopIsSoundset = qfalse;
	}
	else
	{
		G_DebugPrint( WL_WARNING, "Q3_SetLoopSound: can't find sound file: '%s'\n", name );
	}
}

void Q3_SetICARUSFreeze( int entID, const char *name, qboolean freeze )
{
	gentity_t	*self  = G_Find( NULL, FOFS(targetname), name );
	if ( !self )
	{//hmm, targetname failed, try script_targetname?
		self = G_Find( NULL, FOFS(script_targetname), name );
	}

	if ( !self )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetICARUSFreeze: invalid ent %s\n", name);
		return;
	}

	if ( freeze )
	{
		self->r.svFlags |= SVF_ICARUS_FREEZE;
	}
	else
	{
		self->r.svFlags &= ~SVF_ICARUS_FREEZE;
	}
}

/*
============
Q3_SetViewEntity
  Description	:
  Return type	: static void
  Argument		: int entID
  Argument		: const char *name
============
*/
void Q3_SetViewEntity(int entID, const char *name)
{
	G_DebugPrint( WL_WARNING, "Q3_SetViewEntity currently unsupported in MP, ask if you need it.\n");
}

/*
============
Q3_SetWeapon
  Description	:
  Return type	: static void
  Argument		: int entID
  Argument		: const char *wp_name
============
*/
extern void ChangeWeapon( gentity_t *ent, int newWeapon );
static void Q3_SetWeapon (int entID, const char *wp_name)
{
	gentity_t	*ent  = &g_entities[entID];
	int		wp = GetIDForString( WPTable, wp_name );

	ent->client->ps.stats[STAT_WEAPONS] = (1<<wp);
	ChangeWeapon( ent, wp );
}

/*
============
Q3_SetItem
  Description	:
  Return type	: static void
  Argument		: int entID
  Argument		: const char *wp_name
============
*/
static void Q3_SetItem (int entID, const char *item_name)
{ //rww - unused in mp
	G_DebugPrint( WL_WARNING, "Q3_SetItem: NOT SUPPORTED IN MP\n");
	return;
}



/*
============
Q3_SetWalkSpeed
  Description	:
  Return type	: static void
  Argument		: int entID
  Argument		: int int_data
============
*/
static void Q3_SetWalkSpeed (int entID, int int_data)
{
	gentity_t	*self  = &g_entities[entID];

	if ( !self )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetWalkSpeed: invalid entID %d\n", entID);
		return;
	}

	if ( !self->NPC )
	{
		G_DebugPrint( WL_ERROR, "Q3_SetWalkSpeed: '%s' is not an NPC!\n", self->targetname );
		return;
	}

	if(int_data == 0)
	{
		self->NPC->stats.walkSpeed = self->client->ps.speed = 1;
	}

	self->NPC->stats.walkSpeed = self->client->ps.speed = int_data;
}


/*
============
Q3_SetRunSpeed
  Description	:
  Return type	: static void
  Argument		: int entID
  Argument		: int int_data
============
*/
static void Q3_SetRunSpeed (int entID, int int_data)
{
	gentity_t	*self  = &g_entities[entID];

	if ( !self )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetRunSpeed: invalid entID %d\n", entID);
		return;
	}

	if ( !self->NPC )
	{
		G_DebugPrint( WL_ERROR, "Q3_SetRunSpeed: '%s' is not an NPC!\n", self->targetname );
		return;
	}

	if(int_data == 0)
	{
		self->NPC->stats.runSpeed = self->client->ps.speed = 1;
	}

	self->NPC->stats.runSpeed = self->client->ps.speed = int_data;
}


/*
============
Q3_SetYawSpeed
  Description	:
  Return type	: static void
  Argument		: int entID
  Argument		: float float_data
============
*/
static void Q3_SetYawSpeed (int entID, float float_data)
{
	gentity_t	*self  = &g_entities[entID];

	if ( !self )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetYawSpeed: invalid entID %d\n", entID);
		return;
	}

	if ( !self->NPC )
	{
		G_DebugPrint( WL_ERROR, "Q3_SetYawSpeed: '%s' is not an NPC!\n", self->targetname );
		return;
	}

	self->NPC->stats.yawSpeed = float_data;
}


/*
============
Q3_SetAggression
  Description	:
  Return type	: static void
  Argument		: int entID
  Argument		: int int_data
============
*/
static void Q3_SetAggression(int entID, int int_data)
{
	gentity_t	*self  = &g_entities[entID];


	if ( !self )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetAggression: invalid entID %d\n", entID);
		return;
	}

	if ( !self->NPC )
	{
		G_DebugPrint( WL_ERROR, "Q3_SetAggression: '%s' is not an NPC!\n", self->targetname );
		return;
	}

	if(int_data < 1 || int_data > 5)
		return;

	self->NPC->stats.aggression = int_data;
}


/*
============
Q3_SetAim
  Description	:
  Return type	: static void
  Argument		: int entID
  Argument		: int int_data
============
*/
static void Q3_SetAim(int entID, int int_data)
{
	gentity_t	*self  = &g_entities[entID];

	if ( !self )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetAim: invalid entID %d\n", entID);
		return;
	}

	if ( !self->NPC )
	{
		G_DebugPrint( WL_ERROR, "Q3_SetAim: '%s' is not an NPC!\n", self->targetname );
		return;
	}

	if(int_data < 1 || int_data > 5)
		return;

	self->NPC->stats.aim = int_data;
}


/*
============
Q3_SetFriction
  Description	:
  Return type	: static void
  Argument		: int entID
  Argument		: int int_data
============
*/
static void Q3_SetFriction(int entID, int int_data)
{
	gentity_t	*self  = &g_entities[entID];

	if ( !self )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetFriction: invalid entID %d\n", entID);
		return;
	}

	if ( !self->client )
	{
		G_DebugPrint( WL_ERROR, "Q3_SetFriction: '%s' is not an NPC/player!\n", self->targetname );
		return;
	}

	G_DebugPrint( WL_WARNING, "Q3_SetFriction currently unsupported in MP\n");
	//	self->client->ps.friction = int_data;
}


/*
============
Q3_SetGravity
  Description	:
  Return type	: static void
  Argument		: int entID
  Argument		: float float_data
============
*/
static void Q3_SetGravity(int entID, float float_data)
{
	gentity_t	*self  = &g_entities[entID];

	if ( !self )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetGravity: invalid entID %d\n", entID);
		return;
	}

	if ( !self->client )
	{
		G_DebugPrint( WL_ERROR, "Q3_SetGravity: '%s' is not an NPC/player!\n", self->targetname );
		return;
	}

	//FIXME: what if we want to return them to normal global gravity?
	if ( self->NPC )
	{
		self->NPC->aiFlags |= NPCAI_CUSTOM_GRAVITY;
	}
	self->client->ps.gravity = float_data;
}


/*
============
Q3_SetWait
  Description	:
  Return type	: static void
  Argument		: int entID
  Argument		: float float_data
============
*/
static void Q3_SetWait(int entID, float float_data)
{
	gentity_t	*self  = &g_entities[entID];

	if ( !self )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetWait: invalid entID %d\n", entID);
		return;
	}

	self->wait = float_data;
}


static void Q3_SetShotSpacing(int entID, int int_data)
{
	gentity_t	*self  = &g_entities[entID];

	if ( !self )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetShotSpacing: invalid entID %d\n", entID);
		return;
	}

	if ( !self->NPC )
	{
		G_DebugPrint( WL_ERROR, "Q3_SetShotSpacing: '%s' is not an NPC!\n", self->targetname );
		return;
	}

	self->NPC->aiFlags &= ~NPCAI_BURST_WEAPON;
	self->NPC->burstSpacing = int_data;
}

/*
============
Q3_SetFollowDist
  Description	:
  Return type	: static void
  Argument		: int entID
  Argument		: float float_data
============
*/
static void Q3_SetFollowDist(int entID, float float_data)
{
	gentity_t	*self  = &g_entities[entID];

	if ( !self )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetFollowDist: invalid entID %d\n", entID);
		return;
	}

	if ( !self->client || !self->NPC )
	{
		G_DebugPrint( WL_ERROR, "Q3_SetFollowDist: '%s' is not an NPC!\n", self->targetname );
		return;
	}

	self->NPC->followDist = float_data;
}


/*
============
Q3_SetScale
  Description	:
  Return type	: static void
  Argument		: int entID
  Argument		: float float_data
============
*/
static void Q3_SetScale(int entID, float float_data)
{
	gentity_t	*self  = &g_entities[entID];

	if ( !self )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetScale: invalid entID %d\n", entID);
		return;
	}

	if (self->client)
	{
		self->client->ps.iModelScale = float_data*100.0f;
	}
	else
	{
		self->s.iModelScale = float_data*100.0f;
	}
}

/*
============
Q3_GameSideCheckStringCounterIncrement
  Description	:
  Return type	: static float
  Argument		: const char *string
============
*/
static float Q3_GameSideCheckStringCounterIncrement( const char *string )
{
	char	*numString;
	float	val = 0.0f;

	if ( string[0] == '+' )
	{//We want to increment whatever the value is by whatever follows the +
		if ( string[1] )
		{
			numString = (char *)&string[1];
			val = atof( numString );
		}
	}
	else if ( string[0] == '-' )
	{//we want to decrement
		if ( string[1] )
		{
			numString = (char *)&string[1];
			val = atof( numString ) * -1;
		}
	}

	return val;
}

/*
============
Q3_SetCount
  Description	:
  Return type	: static void
  Argument		: int entID
  Argument		: const char *data
============
*/
static void Q3_SetCount(int entID, const char *data)
{
	gentity_t	*self  = &g_entities[entID];
	float		val = 0.0f;

	//FIXME: use FOFS() stuff here to make a generic entity field setting?
	if ( !self )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetCount: invalid entID %d\n", entID);
		return;
	}

	if ( (val = Q3_GameSideCheckStringCounterIncrement( data )) )
	{
		self->count += (int)(val);
	}
	else
	{
		self->count = atoi((char *) data);
	}
}


/*
============
Q3_SetTargetName
  Description	:
  Return type	: static void
  Argument		: int entID
  Argument		: const char *targetname
============
*/
static void Q3_SetTargetName (int entID, const char *targetname)
{
	gentity_t	*self  = &g_entities[entID];

	if ( !self )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetTargetName: invalid entID %d\n", entID);
		return;
	}

	if(!Q_stricmp("NULL", ((char *)targetname)))
	{
		self->targetname = NULL;
	}
	else
	{
		self->targetname = G_NewString( targetname );
	}
}


/*
============
Q3_SetTarget
  Description	:
  Return type	: static void
  Argument		: int entID
  Argument		: const char *target
============
*/
static void Q3_SetTarget (int entID, const char *target)
{
	gentity_t	*self  = &g_entities[entID];

	if ( !self )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetTarget: invalid entID %d\n", entID);
		return;
	}

	if(!Q_stricmp("NULL", ((char *)target)))
	{
		self->target = NULL;
	}
	else
	{
		self->target = G_NewString( target );
	}
}

/*
============
Q3_SetTarget2
  Description	:
  Return type	: static void
  Argument		: int entID
  Argument		: const char *target
============
*/
static void Q3_SetTarget2 (int entID, const char *target2)
{
	G_DebugPrint( WL_WARNING, "Q3_SetTarget2 does not exist in MP\n");
	/*
	sharedEntity_t	*self  = SV_GentityNum(entID);

	if ( !self )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetTarget2: invalid entID %d\n", entID);
		return;
	}

	if(!Q_stricmp("NULL", ((char *)target2)))
	{
		self->target2 = NULL;
	}
	else
	{
		self->target2 = G_NewString( target2 );
	}
	*/
}
/*
============
Q3_SetRemoveTarget
  Description	:
  Return type	: static void
  Argument		: int entID
  Argument		: const char *target
============
*/
static void Q3_SetRemoveTarget (int entID, const char *target)
{
	gentity_t	*self  = &g_entities[entID];

	if ( !self )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetRemoveTarget: invalid entID %d\n", entID);
		return;
	}

	if ( !self->NPC )
	{
		G_DebugPrint( WL_ERROR, "Q3_SetRemoveTarget: '%s' is not an NPC!\n", self->targetname );
		return;
	}

	if( !Q_stricmp("NULL", ((char *)target)) )
	{
		self->target3 = NULL;
	}
	else
	{
		self->target3 = G_NewString( target );
	}
}


/*
============
Q3_SetPainTarget
  Description	:
  Return type	: void
  Argument		: int entID
  Argument		: const char *targetname
============
*/
static void Q3_SetPainTarget (int entID, const char *targetname)
{
	G_DebugPrint( WL_WARNING, "Q3_SetPainTarget: NOT SUPPORTED IN MP\n");
	/*
	sharedEntity_t	*self  = SV_GentityNum(entID);

	if ( !self )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetPainTarget: invalid entID %d\n", entID);
		return;
	}

	if(Q_stricmp("NULL", ((char *)targetname)) == 0)
	{
		self->paintarget = NULL;
	}
	else
	{
		self->paintarget = G_NewString((char *)targetname);
	}
	*/
}

/*
============
Q3_SetFullName
  Description	:
  Return type	: static void
  Argument		: int entID
  Argument		: const char *fullName
============
*/
static void Q3_SetFullName (int entID, const char *fullName)
{
	gentity_t	*self  = &g_entities[entID];

	if ( !self )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetFullName: invalid entID %d\n", entID);
		return;
	}

	if(!Q_stricmp("NULL", ((char *)fullName)))
	{
		self->fullName = NULL;
	}
	else
	{
		self->fullName = G_NewString( fullName );
	}
}

static void Q3_SetMusicState( const char *dms )
{
	G_DebugPrint( WL_WARNING, "Q3_SetMusicState: NOT SUPPORTED IN MP\n");
	return;
}

static void Q3_SetForcePowerLevel ( int entID, int forcePower, int forceLevel )
{
	gentity_t	*self  = &g_entities[entID];

	if ( forcePower < FP_FIRST || forceLevel >= NUM_FORCE_POWERS )
	{
		G_DebugPrint( WL_ERROR, "Q3_SetForcePowerLevel: Force Power index %d out of range (%d-%d)\n", forcePower, FP_FIRST, (NUM_FORCE_POWERS-1) );
		return;
	}

	if ( forceLevel < 0 || forceLevel >= NUM_FORCE_POWER_LEVELS )
	{
		if ( forcePower != FP_SABER_OFFENSE || forceLevel >= SS_NUM_SABER_STYLES )
		{
			G_DebugPrint( WL_ERROR, "Q3_SetForcePowerLevel: Force power setting %d out of range (0-3)\n", forceLevel );
			return;
		}
	}

	if ( !self )
	{
		G_DebugPrint( WL_ERROR, "Q3_SetForcePowerLevel: invalid entID %d\n", entID);
		return;
	}

	if ( !self->client )
	{
		G_DebugPrint( WL_ERROR, "Q3_SetForcePowerLevel: ent %s is not a player or NPC\n", self->targetname );
		return;
	}

	self->client->ps.fd.forcePowerLevel[forcePower] = forceLevel;
	if ( forceLevel )
	{
		self->client->ps.fd.forcePowersKnown |= ( 1 << forcePower );
	}
	else
	{
		self->client->ps.fd.forcePowersKnown &= ~( 1 << forcePower );
	}
}
/*
============
Q3_SetParm
  Description	:
  Return type	: void
  Argument		: int entID
  Argument		: int parmNum
  Argument		: const char *parmValue
============
*/
void Q3_SetParm (int entID, int parmNum, const char *parmValue)
{
	gentity_t	*ent = &g_entities[entID];
	float		val;

	if ( !ent )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetParm: invalid entID %d\n", entID);
		return;
	}

	if ( parmNum < 0 || parmNum >= MAX_PARMS )
	{
		G_DebugPrint( WL_WARNING, "SET_PARM: parmNum %d out of range!\n", parmNum );
		return;
	}

	if( !ent->parms )
	{
		ent->parms = (parms_t *)G_Alloc( sizeof(parms_t) );
		memset( ent->parms, 0, sizeof(parms_t) );
	}

	if ( (val = Q3_GameSideCheckStringCounterIncrement( parmValue )) )
	{
		val += atof( ent->parms->parm[parmNum] );
		Com_sprintf( ent->parms->parm[parmNum], sizeof(ent->parms->parm[parmNum]), "%f", val );
	}
	else
	{//Just copy the string
		//copy only 16 characters
		strncpy( ent->parms->parm[parmNum], parmValue, sizeof(ent->parms->parm[parmNum]) );
		//set the last character to null in case we had to truncate their passed string
		if ( ent->parms->parm[parmNum][sizeof(ent->parms->parm[parmNum]) - 1] != 0 )
		{//Tried to set a string that is too long
			ent->parms->parm[parmNum][sizeof(ent->parms->parm[parmNum]) - 1] = 0;
			G_DebugPrint( WL_WARNING, "SET_PARM: parm%d string too long, truncated to '%s'!\n", parmNum, ent->parms->parm[parmNum] );
		}
	}
}



/*
=============
Q3_SetCaptureGoal

Sets the capture spot goal of an entity
=============
*/
static void Q3_SetCaptureGoal( int entID, const char *name )
{
	gentity_t	*ent  = &g_entities[entID];
	gentity_t	*goal = G_Find( NULL, FOFS(targetname), (char *) name);

	if ( !ent )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetCaptureGoal: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->NPC )
	{
		G_DebugPrint( WL_ERROR, "Q3_SetCaptureGoal: '%s' is not an NPC!\n", ent->targetname );
		return;
	}

	//FIXME: Exception handle here
	if (goal == NULL)
	{
		G_DebugPrint( WL_ERROR, "Q3_SetCaptureGoal: can't find CaptureGoal target: '%s'\n", name );
		return;
	}

	if(ent->NPC)
	{
		ent->NPC->captureGoal = goal;
		ent->NPC->goalEntity = goal;
		ent->NPC->goalTime = level.time + 100000;
	}
}

/*
=============
Q3_SetEvent

?
=============
*/
static void Q3_SetEvent( int entID, const char *event_name )
{ //rwwFIXMEFIXME: Use in MP?
	G_DebugPrint( WL_WARNING, "Q3_SetEvent: NOT SUPPORTED IN MP (may be in future, ask if needed)\n");
	return;
}

/*
============
Q3_SetIgnorePain

?
============
*/
static void Q3_SetIgnorePain( int entID, qboolean data)
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetIgnorePain: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->NPC )
	{
		G_DebugPrint( WL_ERROR, "Q3_SetIgnorePain: '%s' is not an NPC!\n", ent->targetname );
		return;
	}

	ent->NPC->ignorePain = data;
}

/*
============
Q3_SetIgnoreEnemies

?
============
*/
static void Q3_SetIgnoreEnemies( int entID, qboolean data)
{

	G_DebugPrint( WL_WARNING, "Q3_SetIgnoreEnemies: NOT SUPPORTED IN MP");
	return;
}

/*
============
Q3_SetIgnoreAlerts

?
============
*/
static void Q3_SetIgnoreAlerts( int entID, qboolean data)
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetIgnoreAlerts: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->NPC )
	{
		G_DebugPrint( WL_ERROR, "Q3_SetIgnoreAlerts: '%s' is not an NPC!\n", ent->targetname );
		return;
	}

	if(data)
	{
		ent->NPC->scriptFlags |= SCF_IGNORE_ALERTS;
	}
	else
	{
		ent->NPC->scriptFlags &= ~SCF_IGNORE_ALERTS;
	}
}


/*
============
Q3_SetNoTarget

?
============
*/
static void Q3_SetNoTarget( int entID, qboolean data)
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetNoTarget: invalid entID %d\n", entID);
		return;
	}

	if(data)
		ent->flags |= FL_NOTARGET;
	else
		ent->flags &= ~FL_NOTARGET;
}

/*
============
Q3_SetDontShoot

?
============
*/
static void Q3_SetDontShoot( int entID, qboolean add)
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetDontShoot: invalid entID %d\n", entID);
		return;
	}

	if(add)
	{
		ent->flags |= FL_DONT_SHOOT;
	}
	else
	{
		ent->flags &= ~FL_DONT_SHOOT;
	}
}

/*
============
Q3_SetDontFire

?
============
*/
static void Q3_SetDontFire( int entID, qboolean add)
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetDontFire: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->NPC )
	{
		G_DebugPrint( WL_ERROR, "Q3_SetDontFire: '%s' is not an NPC!\n", ent->targetname );
		return;
	}

	if(add)
	{
		ent->NPC->scriptFlags |= SCF_DONT_FIRE;
	}
	else
	{
		ent->NPC->scriptFlags &= ~SCF_DONT_FIRE;
	}
}

/*
============
Q3_SetFireWeapon

?
============
*/
static void Q3_SetFireWeapon(int entID, qboolean add)
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		G_DebugPrint( WL_WARNING, "Q3_FireWeapon: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->NPC )
	{
		G_DebugPrint( WL_ERROR, "Q3_SetFireWeapon: '%s' is not an NPC!\n", ent->targetname );
		return;
	}

	if(add)
	{
		ent->NPC->scriptFlags |= SCF_FIRE_WEAPON;
	}
	else
	{
		ent->NPC->scriptFlags &= ~SCF_FIRE_WEAPON;
	}
}


/*
============
Q3_SetInactive

?
============
*/
static void Q3_SetInactive(int entID, qboolean add)
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetInactive: invalid entID %d\n", entID);
		return;
	}

	if(add)
	{
		ent->flags |= FL_INACTIVE;
	}
	else
	{
		ent->flags &= ~FL_INACTIVE;
	}
}

/*
============
Q3_SetFuncUsableVisible

?
============
*/
static void Q3_SetFuncUsableVisible(int entID, qboolean visible )
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetFuncUsableVisible: invalid entID %d\n", entID);
		return;
	}

	// Yeah, I know that this doesn't even do half of what the func_usable use code does, but if I've got two things on top of each other...and only
	//	one is visible at a time....and neither can ever be used......and finally, the shader on it has the shader_anim stuff going on....It doesn't seem
	//	like I can easily use the other version without nasty side effects.
	if( visible )
	{
		ent->r.svFlags &= ~SVF_NOCLIENT;
		ent->s.eFlags &= ~EF_NODRAW;
	}
	else
	{
		ent->r.svFlags |= SVF_NOCLIENT;
		ent->s.eFlags |= EF_NODRAW;
	}
}

/*
============
Q3_SetLockedEnemy

?
============
*/
static void Q3_SetLockedEnemy ( int entID, qboolean locked)
{
	G_DebugPrint( WL_WARNING, "Q3_SetLockedEnemy: NOT SUPPORTED IN MP\n");
	return;
}

char cinematicSkipScript[1024];

/*
============
Q3_SetCinematicSkipScript

============
*/
static void Q3_SetCinematicSkipScript( char *scriptname )
{
	G_DebugPrint( WL_WARNING, "Q3_SetCinematicSkipScript: NOT SUPPORTED IN MP\n");
	return;
}

/*
============
Q3_SetNoMindTrick

?
============
*/
static void Q3_SetNoMindTrick( int entID, qboolean add)
{
	G_DebugPrint( WL_WARNING, "Q3_SetNoMindTrick: NOT SUPPORTED IN MP\n");
	return;
}

/*
============
Q3_SetCrouched

?
============
*/
static void Q3_SetCrouched( int entID, qboolean add)
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetCrouched: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->NPC )
	{
		G_DebugPrint( WL_ERROR, "Q3_SetCrouched: '%s' is not an NPC!\n", ent->targetname );
		return;
	}

	if(add)
	{
		ent->NPC->scriptFlags |= SCF_CROUCHED;
	}
	else
	{
		ent->NPC->scriptFlags &= ~SCF_CROUCHED;
	}
}

/*
============
Q3_SetWalking

?
============
*/
static void Q3_SetWalking( int entID, qboolean add)
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetWalking: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->NPC )
	{
		G_DebugPrint( WL_ERROR, "Q3_SetWalking: '%s' is not an NPC!\n", ent->targetname );
		return;
	}

	if(add)
	{
		ent->NPC->scriptFlags |= SCF_WALKING;
	}
	else
	{
		ent->NPC->scriptFlags &= ~SCF_WALKING;
	}
	return;
}

/*
============
Q3_SetRunning

?
============
*/
static void Q3_SetRunning( int entID, qboolean add)
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetRunning: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->NPC )
	{
		G_DebugPrint( WL_ERROR, "Q3_SetRunning: '%s' is not an NPC!\n", ent->targetname );
		return;
	}

	if(add)
	{
		ent->NPC->scriptFlags |= SCF_RUNNING;
	}
	else
	{
		ent->NPC->scriptFlags &= ~SCF_RUNNING;
	}
}

/*
============
Q3_SetForcedMarch

?
============
*/
static void Q3_SetForcedMarch( int entID, qboolean add)
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetForcedMarch: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->NPC )
	{
		G_DebugPrint( WL_ERROR, "Q3_SetForcedMarch: '%s' is not an NPC!\n", ent->targetname );
		return;
	}

	if(add)
	{
		ent->NPC->scriptFlags |= SCF_FORCED_MARCH;
	}
	else
	{
		ent->NPC->scriptFlags &= ~SCF_FORCED_MARCH;
	}
}
/*
============
Q3_SetChaseEnemies

indicates whether the npc should chase after an enemy
============
*/
static void Q3_SetChaseEnemies( int entID, qboolean add)
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetChaseEnemies: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->NPC )
	{
		G_DebugPrint( WL_ERROR, "Q3_SetChaseEnemies: '%s' is not an NPC!\n", ent->targetname );
		return;
	}

	if(add)
	{
		ent->NPC->scriptFlags |= SCF_CHASE_ENEMIES;
	}
	else
	{
		ent->NPC->scriptFlags &= ~SCF_CHASE_ENEMIES;
	}
}

/*
============
Q3_SetLookForEnemies

if set npc will be on the look out for potential enemies
if not set, npc will ignore enemies
============
*/
static void Q3_SetLookForEnemies( int entID, qboolean add)
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetLookForEnemies: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->NPC )
	{
		G_DebugPrint( WL_ERROR, "Q3_SetLookForEnemies: '%s' is not an NPC!\n", ent->targetname );
		return;
	}

	if(add)
	{
		ent->NPC->scriptFlags |= SCF_LOOK_FOR_ENEMIES;
	}
	else
	{
		ent->NPC->scriptFlags &= ~SCF_LOOK_FOR_ENEMIES;
	}
}

/*
============
Q3_SetFaceMoveDir

============
*/
static void Q3_SetFaceMoveDir( int entID, qboolean add)
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetFaceMoveDir: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->NPC )
	{
		G_DebugPrint( WL_ERROR, "Q3_SetFaceMoveDir: '%s' is not an NPC!\n", ent->targetname );
		return;
	}

	if(add)
	{
		ent->NPC->scriptFlags |= SCF_FACE_MOVE_DIR;
	}
	else
	{
		ent->NPC->scriptFlags &= ~SCF_FACE_MOVE_DIR;
	}
}

/*
============
Q3_SetAltFire

?
============
*/
static void Q3_SetAltFire( int entID, qboolean add)
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetAltFire: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->NPC )
	{
		G_DebugPrint( WL_ERROR, "Q3_SetAltFire: '%s' is not an NPC!\n", ent->targetname );
		return;
	}

	if(add)
	{
		ent->NPC->scriptFlags |= SCF_ALT_FIRE;
	}
	else
	{
		ent->NPC->scriptFlags &= ~SCF_ALT_FIRE;
	}

	ChangeWeapon( ent, 	ent->client->ps.weapon );
}

/*
============
Q3_SetDontFlee

?
============
*/
static void Q3_SetDontFlee( int entID, qboolean add)
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetDontFlee: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->NPC )
	{
		G_DebugPrint( WL_ERROR, "Q3_SetDontFlee: '%s' is not an NPC!\n", ent->targetname );
		return;
	}

	if(add)
	{
		ent->NPC->scriptFlags |= SCF_DONT_FLEE;
	}
	else
	{
		ent->NPC->scriptFlags &= ~SCF_DONT_FLEE;
	}
}

/*
============
Q3_SetNoResponse

?
============
*/
static void Q3_SetNoResponse( int entID, qboolean add)
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetNoResponse: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->NPC )
	{
		G_DebugPrint( WL_ERROR, "Q3_SetNoResponse: '%s' is not an NPC!\n", ent->targetname );
		return;
	}

	if(add)
	{
		ent->NPC->scriptFlags |= SCF_NO_RESPONSE;
	}
	else
	{
		ent->NPC->scriptFlags &= ~SCF_NO_RESPONSE;
	}
}

/*
============
Q3_SetCombatTalk

?
============
*/
static void Q3_SetCombatTalk( int entID, qboolean add)
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetCombatTalk: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->NPC )
	{
		G_DebugPrint( WL_ERROR, "Q3_SetCombatTalk: '%s' is not an NPC!\n", ent->targetname );
		return;
	}

	if ( add )
	{
		ent->NPC->scriptFlags |= SCF_NO_COMBAT_TALK;
	}
	else
	{
		ent->NPC->scriptFlags &= ~SCF_NO_COMBAT_TALK;
	}
}

/*
============
Q3_SetAlertTalk

?
============
*/
static void Q3_SetAlertTalk( int entID, qboolean add)
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetAlertTalk: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->NPC )
	{
		G_DebugPrint( WL_ERROR, "Q3_SetAlertTalk: '%s' is not an NPC!\n", ent->targetname );
		return;
	}

	if ( add )
	{
		ent->NPC->scriptFlags |= SCF_NO_ALERT_TALK;
	}
	else
	{
		ent->NPC->scriptFlags &= ~SCF_NO_ALERT_TALK;
	}
}

/*
============
Q3_SetUseCpNearest

?
============
*/
static void Q3_SetUseCpNearest( int entID, qboolean add)
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetUseCpNearest: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->NPC )
	{
		G_DebugPrint( WL_ERROR, "Q3_SetUseCpNearest: '%s' is not an NPC!\n", ent->targetname );
		return;
	}

	if ( add )
	{
		ent->NPC->scriptFlags |= SCF_USE_CP_NEAREST;
	}
	else
	{
		ent->NPC->scriptFlags &= ~SCF_USE_CP_NEAREST;
	}
}

/*
============
Q3_SetNoForce

?
============
*/
static void Q3_SetNoForce( int entID, qboolean add)
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetNoForce: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->NPC )
	{
		G_DebugPrint( WL_ERROR, "Q3_SetNoForce: '%s' is not an NPC!\n", ent->targetname );
		return;
	}

	if ( add )
	{
		ent->NPC->scriptFlags |= SCF_NO_FORCE;
	}
	else
	{
		ent->NPC->scriptFlags &= ~SCF_NO_FORCE;
	}
}

/*
============
Q3_SetNoAcrobatics

?
============
*/
static void Q3_SetNoAcrobatics( int entID, qboolean add)
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetNoAcrobatics: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->NPC )
	{
		G_DebugPrint( WL_ERROR, "Q3_SetNoAcrobatics: '%s' is not an NPC!\n", ent->targetname );
		return;
	}

	if ( add )
	{
		ent->NPC->scriptFlags |= SCF_NO_ACROBATICS;
	}
	else
	{
		ent->NPC->scriptFlags &= ~SCF_NO_ACROBATICS;
	}
}

/*
============
Q3_SetUseSubtitles

?
============
*/
static void Q3_SetUseSubtitles( int entID, qboolean add)
{
	G_DebugPrint( WL_WARNING, "Q3_SetUseSubtitles: NOT SUPPORTED IN MP\n");
	return;
}

/*
============
Q3_SetNoFallToDeath

?
============
*/
static void Q3_SetNoFallToDeath( int entID, qboolean add)
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetNoFallToDeath: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->NPC )
	{
		G_DebugPrint( WL_ERROR, "Q3_SetNoFallToDeath: '%s' is not an NPC!\n", ent->targetname );
		return;
	}

	if ( add )
	{
		ent->NPC->scriptFlags |= SCF_NO_FALLTODEATH;
	}
	else
	{
		ent->NPC->scriptFlags &= ~SCF_NO_FALLTODEATH;
	}
}

/*
============
Q3_SetDismemberable

?
============
*/
static void Q3_SetDismemberable( int entID, qboolean dismemberable)
{
	G_DebugPrint( WL_WARNING, "Q3_SetDismemberable: NOT SUPPORTED IN MP\n");
	return;
}


/*
============
Q3_SetMoreLight

?
============
*/
static void Q3_SetMoreLight( int entID, qboolean add )
{
	G_DebugPrint( WL_WARNING, "Q3_SetMoreLight: NOT SUPPORTED IN MP\n");
	return;
}

/*
============
Q3_SetUndying

?
============
*/
static void Q3_SetUndying( int entID, qboolean undying)
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetUndying: invalid entID %d\n", entID);
		return;
	}

	if(undying)
	{
		ent->flags |= FL_UNDYING;
	}
	else
	{
		ent->flags &= ~FL_UNDYING;
	}
}

/*
============
Q3_SetInvincible

?
============
*/
static void Q3_SetInvincible( int entID, qboolean invincible)
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetInvincible: invalid entID %d\n", entID);
		return;
	}

	if ( !Q_stricmp( "func_breakable", ent->classname ) )
	{
		if ( invincible )
		{
			ent->spawnflags |= 1;
		}
		else
		{
			ent->spawnflags &= ~1;
		}
		return;
	}

	if ( invincible )
	{
		ent->flags |= FL_GODMODE;
	}
	else
	{
		ent->flags &= ~FL_GODMODE;
	}
}
/*
============
Q3_SetForceInvincible
  Description	:
  Return type	: static void
  Argument		:  int entID
  Argument		: qboolean forceInv
============
*/
static void Q3_SetForceInvincible( int entID, qboolean forceInv )
{
	G_DebugPrint( WL_WARNING, "Q3_SetForceInvicible: NOT SUPPORTED IN MP\n");
	return;
}

/*
============
Q3_SetNoAvoid

?
============
*/
static void Q3_SetNoAvoid( int entID, qboolean noAvoid)
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetNoAvoid: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->NPC )
	{
		G_DebugPrint( WL_ERROR, "Q3_SetNoAvoid: '%s' is not an NPC!\n", ent->targetname );
		return;
	}

	if(noAvoid)
	{
		ent->NPC->aiFlags |= NPCAI_NO_COLL_AVOID;
	}
	else
	{
		ent->NPC->aiFlags &= ~NPCAI_NO_COLL_AVOID;
	}
}

/*
============
SolidifyOwner
  Description	:
  Return type	: void
  Argument		: sharedEntity_t *self
============
*/
void SolidifyOwner( gentity_t *self )
{
	int oldContents;
	gentity_t *owner = &g_entities[self->r.ownerNum];

	self->nextthink = level.time + FRAMETIME;
	self->think = G_FreeEntity;

	if ( !owner || !owner->inuse )
	{
		return;
	}

	oldContents = owner->r.contents;
	owner->r.contents = CONTENTS_BODY;
	if ( SpotWouldTelefrag2( owner, owner->r.currentOrigin ) )
	{
		owner->r.contents = oldContents;
		self->think = SolidifyOwner;
	}
	else
	{
		trap->ICARUS_TaskIDComplete( (sharedEntity_t *)owner, TID_RESIZE );
	}
}


/*
============
Q3_SetSolid

?
============
*/
static qboolean Q3_SetSolid( int entID, qboolean solid)
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent || !ent->inuse )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetSolid: invalid entID %d\n", entID);
		return qtrue;
	}

	if ( solid )
	{//FIXME: Presumption
		int oldContents = ent->r.contents;
		ent->r.contents = CONTENTS_BODY;
		if ( SpotWouldTelefrag2( ent, ent->r.currentOrigin ) )
		{
			gentity_t *solidifier = G_Spawn();

			solidifier->r.ownerNum = ent->s.number;

			solidifier->think = SolidifyOwner;
			solidifier->nextthink = level.time + FRAMETIME;

			ent->r.contents = oldContents;
			return qfalse;
		}
		ent->clipmask |= CONTENTS_BODY;
	}
	else
	{//FIXME: Presumption
		if ( ent->s.eFlags & EF_NODRAW )
		{//We're invisible too, so set contents to none
			ent->r.contents = 0;
		}
		else
		{
			ent->r.contents = CONTENTS_CORPSE;
		}
	}
	return qtrue;
}

/*
============
Q3_SetForwardMove

?
============
*/
static void Q3_SetForwardMove( int entID, int fmoveVal)
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetForwardMove: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->client )
	{
		G_DebugPrint( WL_ERROR, "Q3_SetForwardMove: '%s' is not an NPC/player!\n", ent->targetname );
		return;
	}

	G_DebugPrint( WL_WARNING, "Q3_SetForwardMove: NOT SUPPORTED IN MP\n");
	//ent->client->forced_forwardmove = fmoveVal;
}

/*
============
Q3_SetRightMove

?
============
*/
static void Q3_SetRightMove( int entID, int rmoveVal)
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetRightMove: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->client )
	{
		G_DebugPrint( WL_ERROR, "Q3_SetRightMove: '%s' is not an NPC/player!\n", ent->targetname );
		return;
	}

	G_DebugPrint( WL_WARNING, "Q3_SetRightMove: NOT SUPPORTED IN MP\n");
	//ent->client->forced_rightmove = rmoveVal;
}

/*
============
Q3_SetLockAngle

?
============
*/
static void Q3_SetLockAngle( int entID, const char *lockAngle)
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetLockAngle: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->client )
	{
		G_DebugPrint( WL_ERROR, "Q3_SetLockAngle: '%s' is not an NPC/player!\n", ent->targetname );
		return;
	}

	G_DebugPrint( WL_WARNING, "Q3_SetLockAngle is not currently available. Ask if you really need it.\n");
	/*
	if(Q_stricmp("off", lockAngle) == 0)
	{//free it
		ent->client->renderInfo.renderFlags &= ~RF_LOCKEDANGLE;
	}
	else
	{
		ent->client->renderInfo.renderFlags |= RF_LOCKEDANGLE;


		if(Q_stricmp("auto", lockAngle) == 0)
		{//use current yaw
			ent->client->renderInfo.lockYaw = ent->client->ps.viewangles[YAW];
		}
		else
		{//specified yaw
			ent->client->renderInfo.lockYaw = atof((char *)lockAngle);
		}
	}
	*/
}


/*
============
Q3_CameraGroup

?
============
*/
static void Q3_CameraGroup( int entID, char *camG)
{
	G_DebugPrint( WL_WARNING, "Q3_CameraGroup: NOT SUPPORTED IN MP\n");
	return;
}

/*
============
Q3_CameraGroupZOfs

?
============
*/
static void Q3_CameraGroupZOfs( float camGZOfs )
{
	G_DebugPrint( WL_WARNING, "Q3_CameraGroupZOfs: NOT SUPPORTED IN MP\n");
	return;
}
/*
============
Q3_CameraGroup

?
============
*/
static void Q3_CameraGroupTag( char *camGTag )
{
	G_DebugPrint( WL_WARNING, "Q3_CameraGroupTag: NOT SUPPORTED IN MP\n");
	return;
}

/*
============
Q3_RemoveRHandModel
============
*/
static void Q3_RemoveRHandModel( int entID, char *addModel)
{
	G_DebugPrint( WL_WARNING, "Q3_RemoveRHandModel: NOT SUPPORTED IN MP\n");
}

/*
============
Q3_AddRHandModel
============
*/
static void Q3_AddRHandModel( int entID, char *addModel)
{
	G_DebugPrint( WL_WARNING, "Q3_AddRHandModel: NOT SUPPORTED IN MP\n");
}

/*
============
Q3_AddLHandModel
============
*/
static void Q3_AddLHandModel( int entID, char *addModel)
{
	G_DebugPrint( WL_WARNING, "Q3_AddLHandModel: NOT SUPPORTED IN MP\n");
}

/*
============
Q3_RemoveLHandModel
============
*/
static void Q3_RemoveLHandModel( int entID, char *addModel)
{
	G_DebugPrint( WL_WARNING, "Q3_RemoveLHandModel: NOT SUPPORTED IN MP\n");
}

/*
============
Q3_LookTarget

?
============
*/
static void Q3_LookTarget( int entID, char *targetName)
{
	gentity_t	*ent  = &g_entities[entID];
	gentity_t	*targ = NULL;

	if ( !ent )
	{
		G_DebugPrint( WL_WARNING, "Q3_LookTarget: invalid entID %d\n", entID);
		return;
	}

	if ( !ent->client )
	{
		G_DebugPrint( WL_ERROR, "Q3_LookTarget: '%s' is not an NPC/player!\n", ent->targetname );
		return;
	}

	if(Q_stricmp("none", targetName) == 0 || Q_stricmp("NULL", targetName) == 0)
	{//clearing look target
		NPC_ClearLookTarget( ent );
		return;
	}

	targ = G_Find(NULL, FOFS(targetname), targetName);
	if(!targ)
	{
		targ  = G_Find(NULL, FOFS(script_targetname), targetName);
		if (!targ)
		{
			targ  = G_Find(NULL, FOFS(NPC_targetname), targetName);
			if (!targ)
			{
				G_DebugPrint( WL_ERROR, "Q3_LookTarget: Can't find ent %s\n", targetName );
				return;
			}
		}
	}

	NPC_SetLookTarget( ent, targ->s.number, 0 );
}

/*
============
Q3_Face

?
============
*/
static void Q3_Face( int entID,int expression, float holdtime)
{
	G_DebugPrint( WL_WARNING, "Q3_Face: NOT SUPPORTED IN MP\n");
}

/*
============
Q3_SetLocation
  Description	:
  Return type	: qboolean
  Argument		:  int entID
  Argument		: const char *location
============
*/
static qboolean Q3_SetLocation( int entID, const char *location )
{
	G_DebugPrint( WL_WARNING, "Q3_SetLocation: NOT SUPPORTED IN MP\n");
	return qtrue;
}

/*
============
Q3_SetPlayerLocked
  Description	:
  Return type	: void
  Argument		:  int entID
  Argument		: qboolean locked
============
*/
qboolean	player_locked = qfalse;
static void Q3_SetPlayerLocked( int entID, qboolean locked )
{
	G_DebugPrint( WL_WARNING, "Q3_SetPlayerLocked: NOT SUPPORTED IN MP\n");
}

/*
============
Q3_SetLockPlayerWeapons
  Description	:
  Return type	: void
  Argument		:  int entID
  Argument		: qboolean locked
============
*/
static void Q3_SetLockPlayerWeapons( int entID, qboolean locked )
{
	G_DebugPrint( WL_WARNING, "Q3_SetLockPlayerWeapons: NOT SUPPORTED IN MP\n");
}


/*
============
Q3_SetNoImpactDamage
  Description	:
  Return type	: void
  Argument		:  int entID
  Argument		: qboolean locked
============
*/
static void Q3_SetNoImpactDamage( int entID, qboolean noImp )
{
	G_DebugPrint( WL_WARNING, "Q3_SetNoImpactDamage: NOT SUPPORTED IN MP\n");
}

/*
============
Q3_SetBehaviorSet

?
============
*/
static qboolean Q3_SetBehaviorSet( int entID, int toSet, const char *scriptname)
{
	gentity_t	*ent  = &g_entities[entID];
	bSet_t		bSet = BSET_INVALID;

	if ( !ent )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetBehaviorSet: invalid entID %d\n", entID);
		return qfalse;
	}

	switch(toSet)
	{
	case SET_SPAWNSCRIPT:
		bSet = BSET_SPAWN;
		break;
	case SET_USESCRIPT:
		bSet = BSET_USE;
		break;
	case SET_AWAKESCRIPT:
		bSet = BSET_AWAKE;
		break;
	case SET_ANGERSCRIPT:
		bSet = BSET_ANGER;
		break;
	case SET_ATTACKSCRIPT:
		bSet = BSET_ATTACK;
		break;
	case SET_VICTORYSCRIPT:
		bSet = BSET_VICTORY;
		break;
	case SET_LOSTENEMYSCRIPT:
		bSet = BSET_LOSTENEMY;
		break;
	case SET_PAINSCRIPT:
		bSet = BSET_PAIN;
		break;
	case SET_FLEESCRIPT:
		bSet = BSET_FLEE;
		break;
	case SET_DEATHSCRIPT:
		bSet = BSET_DEATH;
		break;
	case SET_DELAYEDSCRIPT:
		bSet = BSET_DELAYED;
		break;
	case SET_BLOCKEDSCRIPT:
		bSet = BSET_BLOCKED;
		break;
	case SET_FFIRESCRIPT:
		bSet = BSET_FFIRE;
		break;
	case SET_FFDEATHSCRIPT:
		bSet = BSET_FFDEATH;
		break;
	case SET_MINDTRICKSCRIPT:
		bSet = BSET_MINDTRICK;
		break;
	}

	if(bSet < BSET_SPAWN || bSet >= NUM_BSETS)
	{
		return qfalse;
	}

	if(!Q_stricmp("NULL", scriptname))
	{
		if ( ent->behaviorSet[bSet] != NULL )
		{
//			trap->TagFree( ent->behaviorSet[bSet] );
		}

		ent->behaviorSet[bSet] = NULL;
		//memset( &ent->behaviorSet[bSet], 0, sizeof(ent->behaviorSet[bSet]) );
	}
	else
	{
		if ( scriptname )
		{
			if ( ent->behaviorSet[bSet] != NULL )
			{
//				trap->TagFree( ent->behaviorSet[bSet] );
			}

			ent->behaviorSet[bSet] = G_NewString( (char *) scriptname );	//FIXME: This really isn't good...
		}

		//ent->behaviorSet[bSet] = scriptname;
		//strncpy( (char *) &ent->behaviorSet[bSet], scriptname, MAX_BSET_LENGTH );
	}
	return qtrue;
}

/*
============
Q3_SetDelayScriptTime

?
============
*/
static void Q3_SetDelayScriptTime(int entID, int delayTime)
{
	G_DebugPrint( WL_WARNING, "Q3_SetDelayScriptTime: NOT SUPPORTED IN MP\n");
}




/*
============
Q3_SetPlayerUsable
  Description	:
  Return type	: void
  Argument		:  int entID
  Argument		: qboolean usable
============
*/
static void Q3_SetPlayerUsable( int entID, qboolean usable )
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetPlayerUsable: invalid entID %d\n", entID);
		return;
	}

	if(usable)
	{
		ent->r.svFlags |= SVF_PLAYER_USABLE;
	}
	else
	{
		ent->r.svFlags &= ~SVF_PLAYER_USABLE;
	}
}

/*
============
Q3_SetDisableShaderAnims
  Description	:
  Return type	: static void
  Argument		:  int entID
  Argument		: int disabled
============
*/
static void Q3_SetDisableShaderAnims( int entID, int disabled )
{
	G_DebugPrint( WL_WARNING, "Q3_SetDisableShaderAnims: NOT SUPPORTED IN MP\n");
	return;
}

/*
============
Q3_SetShaderAnim
  Description	:
  Return type	: static void
  Argument		:  int entID
  Argument		: int disabled
============
*/
static void Q3_SetShaderAnim( int entID, int disabled )
{
	G_DebugPrint( WL_WARNING, "Q3_SetShaderAnim: NOT SUPPORTED IN MP\n");
	return;
}

/*
============
Q3_SetStartFrame
  Description	:
  Return type	: static void
  Argument		:  int entID
  Argument		: int startFrame
============
*/
static void Q3_SetStartFrame( int entID, int startFrame )
{
	G_DebugPrint( WL_WARNING, "Q3_SetStartFrame: NOT SUPPORTED IN MP\n");
}


/*
============
Q3_SetEndFrame
  Description	:
  Return type	: static void
  Argument		:  int entID
  Argument		: int endFrame
============
*/
static void Q3_SetEndFrame( int entID, int endFrame )
{
	G_DebugPrint( WL_WARNING, "Q3_SetEndFrame: NOT SUPPORTED IN MP\n");
}

/*
============
Q3_SetAnimFrame
  Description	:
  Return type	: static void
  Argument		:  int entID
  Argument		: int startFrame
============
*/
static void Q3_SetAnimFrame( int entID, int animFrame )
{
	G_DebugPrint( WL_WARNING, "Q3_SetAnimFrame: NOT SUPPORTED IN MP\n");
}

/*
============
Q3_SetLoopAnim
  Description	:
  Return type	: void
  Argument		:  int entID
  Argument		: qboolean loopAnim
============
*/
static void Q3_SetLoopAnim( int entID, qboolean loopAnim )
{
	G_DebugPrint( WL_WARNING, "Q3_SetLoopAnim: NOT SUPPORTED IN MP\n");
}


/*
============
Q3_SetShields
  Description	:
  Return type	: void
  Argument		:  int entID
  Argument		: qboolean shields
============
*/
static void Q3_SetShields( int entID, qboolean shields )
{
	G_DebugPrint( WL_WARNING, "Q3_SetShields: NOT SUPPORTED IN MP\n");
	return;
}

/*
============
Q3_SetSaberActive
  Description	:
  Return type	: void
  Argument		:  int entID
  Argument		: qboolean shields
============
*/
static void Q3_SetSaberActive( int entID, qboolean active )
{
	gentity_t *ent = &g_entities[entID];

	if (!ent || !ent->inuse)
	{
		return;
	}

	if (!ent->client)
	{
		G_DebugPrint( WL_WARNING, "Q3_SetSaberActive: %d is not a client\n", entID);
	}

	//fixme: Take into account player being in state where saber won't toggle? For now we simply won't care.
	if (!ent->client->ps.saberHolstered && active)
	{
		Cmd_ToggleSaber_f(ent);
	}
	else if (BG_SabersOff( &ent->client->ps ) && !active)
	{
		Cmd_ToggleSaber_f(ent);
	}
}

/*
============
Q3_SetNoKnockback
  Description	:
  Return type	: void
  Argument		:  int entID
  Argument		: qboolean noKnockback
============
*/
static void Q3_SetNoKnockback( int entID, qboolean noKnockback )
{
	gentity_t	*ent  = &g_entities[entID];

	if ( !ent )
	{
		G_DebugPrint( WL_WARNING, "Q3_SetNoKnockback: invalid entID %d\n", entID);
		return;
	}

	if ( noKnockback )
	{
		ent->flags |= FL_NO_KNOCKBACK;
	}
	else
	{
		ent->flags &= ~FL_NO_KNOCKBACK;
	}
}

/*
============
Q3_SetCleanDamagingEnts
  Description	:
  Return type	: void
============
*/
static void Q3_SetCleanDamagingEnts( void )
{
	G_DebugPrint( WL_WARNING, "Q3_SetCleanDamagingEnts: NOT SUPPORTED IN MP\n");
	return;
}

vec4_t textcolor_caption;
vec4_t textcolor_center;
vec4_t textcolor_scroll;

/*
-------------------------
Q3_SetTextColor
-------------------------
*/
static void Q3_SetTextColor ( vec4_t textcolor,const char *color)
{
	G_DebugPrint( WL_WARNING, "Q3_SetTextColor: NOT SUPPORTED IN MP\n");
	return;
}

/*
=============
Q3_SetCaptionTextColor

Change color text prints in
=============
*/
static void Q3_SetCaptionTextColor ( const char *color)
{
	Q3_SetTextColor(textcolor_caption,color);
}

/*
=============
Q3_SetCenterTextColor

Change color text prints in
=============
*/
static void Q3_SetCenterTextColor ( const char *color)
{
	Q3_SetTextColor(textcolor_center,color);
}

/*
=============
Q3_SetScrollTextColor

Change color text prints in
=============
*/
static void Q3_SetScrollTextColor ( const char *color)
{
	Q3_SetTextColor(textcolor_scroll,color);
}

/*
=============
Q3_ScrollText

Prints a message in the center of the screen
=============
*/
static void Q3_ScrollText ( const char *id)
{
	G_DebugPrint( WL_WARNING, "Q3_ScrollText: NOT SUPPORTED IN MP\n");
	//trap->SendServerCommand( -1, va("st \"%s\"", id));

	return;
}

/*
=============
Q3_LCARSText

Prints a message in the center of the screen giving it an LCARS frame around it
=============
*/
static void Q3_LCARSText ( const char *id)
{
	G_DebugPrint( WL_WARNING, "Q3_ScrollText: NOT SUPPORTED IN MP\n");
	//trap->SendServerCommand( -1, va("lt \"%s\"", id));

	return;
}

void UnLockDoors(gentity_t *const ent);
void LockDoors(gentity_t *const ent);

//returns qtrue if it got to the end, otherwise qfalse.
qboolean Q3_Set( int taskID, int entID, const char *type_name, const char *data )
{
	gentity_t	*ent = &g_entities[entID];
	float		float_data;
	int			int_data, toSet;
	vec3_t		vector_data;

	//Set this for callbacks
	toSet = GetIDForString( setTable, type_name );

	//TODO: Throw in a showscript command that will list each command and what they're doing...
	//		maybe as simple as printing that line of the script to the console preceeded by the person's name?
	//		showscript can take any number of targetnames or "all"?  Groupname?
	switch ( toSet )
	{
	case SET_ORIGIN:
		if ( sscanf( data, "%f %f %f", &vector_data[0], &vector_data[1], &vector_data[2] ) != 3 ) {
			G_DebugPrint( WL_WARNING, "Q3_Set: failed sscanf on SET_ORIGIN (%s)\n", type_name );
			VectorClear( vector_data );
		}
		G_SetOrigin( ent, vector_data );
		if ( Q_strncmp( "NPC_", ent->classname, 4 ) == 0 )
		{//hack for moving spawners
			VectorCopy( vector_data, ent->s.origin);
		}
		break;

	case SET_TELEPORT_DEST:
		if ( sscanf( data, "%f %f %f", &vector_data[0], &vector_data[1], &vector_data[2] ) != 3 ) {
			G_DebugPrint( WL_WARNING, "Q3_Set: failed sscanf on SET_TELEPORT_DEST (%s)\n", type_name );
			VectorClear( vector_data );
		}
		if ( !Q3_SetTeleportDest( entID, vector_data ) )
		{
			trap->ICARUS_TaskIDSet( (sharedEntity_t *)ent, TID_MOVE_NAV, taskID );
			return qfalse;
		}
		break;

	case SET_COPY_ORIGIN:
		Q3_SetCopyOrigin( entID, (char *) data );
		break;

	case SET_ANGLES:
		//Q3_SetAngles( entID, *(vec3_t *) data);
		if ( sscanf( data, "%f %f %f", &vector_data[0], &vector_data[1], &vector_data[2] ) != 3 ) {
			G_DebugPrint( WL_WARNING, "Q3_Set: failed sscanf on SET_ANGLES (%s)\n", type_name );
			VectorClear( vector_data );
		}
		Q3_SetAngles( entID, vector_data);
		break;

	case SET_XVELOCITY:
		float_data = atof((char *) data);
		Q3_SetVelocity( entID, 0, float_data);
		break;

	case SET_YVELOCITY:
		float_data = atof((char *) data);
		Q3_SetVelocity( entID, 1, float_data);
		break;

	case SET_ZVELOCITY:
		float_data = atof((char *) data);
		Q3_SetVelocity( entID, 2, float_data);
		break;

	case SET_Z_OFFSET:
		float_data = atof((char *) data);
		Q3_SetOriginOffset( entID, 2, float_data);
		break;

	case SET_ENEMY:
		Q3_SetEnemy( entID, (char *) data );
		break;

	case SET_LEADER:
		Q3_SetLeader( entID, (char *) data );
		break;

	case SET_NAVGOAL:
		if ( Q3_SetNavGoal( entID, (char *) data ) )
		{
			trap->ICARUS_TaskIDSet( (sharedEntity_t *)ent, TID_MOVE_NAV, taskID );
			return qfalse;	//Don't call it back
		}
		break;

	case SET_ANIM_UPPER:
		if ( Q3_SetAnimUpper( entID, (char *) data ) )
		{
			Q3_TaskIDClear( &ent->taskID[TID_ANIM_BOTH] );//We only want to wait for the top
			trap->ICARUS_TaskIDSet( (sharedEntity_t *)ent, TID_ANIM_UPPER, taskID );
			return qfalse;	//Don't call it back
		}
		break;

	case SET_ANIM_LOWER:
		if ( Q3_SetAnimLower( entID, (char *) data ) )
		{
			Q3_TaskIDClear( &ent->taskID[TID_ANIM_BOTH] );//We only want to wait for the bottom
			trap->ICARUS_TaskIDSet( (sharedEntity_t *)ent, TID_ANIM_LOWER, taskID );
			return qfalse;	//Don't call it back
		}
		break;

	case SET_ANIM_BOTH:
		{
			int	both = 0;
			if ( Q3_SetAnimUpper( entID, (char *) data ) )
			{
				trap->ICARUS_TaskIDSet( (sharedEntity_t *)ent, TID_ANIM_UPPER, taskID );
				both++;
			}
			else
			{
				G_DebugPrint( WL_ERROR, "Q3_SetAnimUpper: %s does not have anim %s!\n", ent->targetname, (char *)data );
			}
			if ( Q3_SetAnimLower( entID, (char *) data ) )
			{
				trap->ICARUS_TaskIDSet( (sharedEntity_t *)ent, TID_ANIM_LOWER, taskID );
				both++;
			}
			else
			{
				G_DebugPrint( WL_ERROR, "Q3_SetAnimLower: %s does not have anim %s!\n", ent->targetname, (char *)data );
			}
			if ( both >= 2 )
			{
				trap->ICARUS_TaskIDSet( (sharedEntity_t *)ent, TID_ANIM_BOTH, taskID );
			}
			if ( both )
			{
				return qfalse;	//Don't call it back
			}
		}
		break;

	case SET_ANIM_HOLDTIME_LOWER:
		int_data = atoi((char *) data);
		Q3_SetAnimHoldTime( entID, int_data, qtrue );
		Q3_TaskIDClear( &ent->taskID[TID_ANIM_BOTH] );//We only want to wait for the bottom
		trap->ICARUS_TaskIDSet( (sharedEntity_t *)ent, TID_ANIM_LOWER, taskID );
		return qfalse;	//Don't call it back
		break;

	case SET_ANIM_HOLDTIME_UPPER:
		int_data = atoi((char *) data);
		Q3_SetAnimHoldTime( entID, int_data, qfalse );
		Q3_TaskIDClear( &ent->taskID[TID_ANIM_BOTH] );//We only want to wait for the top
		trap->ICARUS_TaskIDSet( (sharedEntity_t *)ent, TID_ANIM_UPPER, taskID );
		return qfalse;	//Don't call it back
		break;

	case SET_ANIM_HOLDTIME_BOTH:
		int_data = atoi((char *) data);
		Q3_SetAnimHoldTime( entID, int_data, qfalse );
		Q3_SetAnimHoldTime( entID, int_data, qtrue );
		trap->ICARUS_TaskIDSet( (sharedEntity_t *)ent, TID_ANIM_BOTH, taskID );
		trap->ICARUS_TaskIDSet( (sharedEntity_t *)ent, TID_ANIM_UPPER, taskID );
		trap->ICARUS_TaskIDSet( (sharedEntity_t *)ent, TID_ANIM_LOWER, taskID );
		return qfalse;	//Don't call it back
		break;

	case SET_PLAYER_TEAM:
		G_DebugPrint( WL_WARNING, "Q3_SetPlayerTeam: Not in MP ATM, let a programmer (ideally Rich) know if you need it\n");
		break;

	case SET_ENEMY_TEAM:
		G_DebugPrint( WL_WARNING, "Q3_SetEnemyTeam: NOT SUPPORTED IN MP\n");
		break;

	case SET_HEALTH:
		int_data = atoi((char *) data);
		Q3_SetHealth( entID, int_data );
		break;

	case SET_ARMOR:
		int_data = atoi((char *) data);
		Q3_SetArmor( entID, int_data );
		break;

	case SET_BEHAVIOR_STATE:
		if( !Q3_SetBState( entID, (char *) data ) )
		{
			trap->ICARUS_TaskIDSet( (sharedEntity_t *)ent, TID_BSTATE, taskID );
			return qfalse;//don't complete
		}
		break;

	case SET_DEFAULT_BSTATE:
		Q3_SetDefaultBState( entID, (char *) data );
		break;

	case SET_TEMP_BSTATE:
		if( !Q3_SetTempBState( entID, (char *) data ) )
		{
			trap->ICARUS_TaskIDSet( (sharedEntity_t *)ent, TID_BSTATE, taskID );
			return qfalse;//don't complete
		}
		break;

	case SET_CAPTURE:
		Q3_SetCaptureGoal( entID, (char *) data );
		break;

	case SET_DPITCH://FIXME: make these set tempBehavior to BS_FACE and await completion?  Or set lockedDesiredPitch/Yaw and aimTime?
		float_data = atof((char *) data);
		Q3_SetDPitch( entID, float_data );
		trap->ICARUS_TaskIDSet( (sharedEntity_t *)ent, TID_ANGLE_FACE, taskID );
		return qfalse;
		break;

	case SET_DYAW:
		float_data = atof((char *) data);
		Q3_SetDYaw( entID, float_data );
		trap->ICARUS_TaskIDSet( (sharedEntity_t *)ent, TID_ANGLE_FACE, taskID );
		return qfalse;
		break;

	case SET_EVENT:
		Q3_SetEvent( entID, (char *) data );
		break;

	case SET_VIEWTARGET:
		Q3_SetViewTarget( entID, (char *) data );
		trap->ICARUS_TaskIDSet( (sharedEntity_t *)ent, TID_ANGLE_FACE, taskID );
		return qfalse;
		break;

	case SET_WATCHTARGET:
		Q3_SetWatchTarget( entID, (char *) data );
		break;

	case SET_VIEWENTITY:
		Q3_SetViewEntity( entID, (char *) data );
		break;

	case SET_LOOPSOUND:
		Q3_SetLoopSound( entID, (char *) data );
		break;

	case SET_ICARUS_FREEZE:
	case SET_ICARUS_UNFREEZE:
		Q3_SetICARUSFreeze( entID, (char *) data, (qboolean)(toSet==SET_ICARUS_FREEZE) );
		break;

	case SET_WEAPON:
		Q3_SetWeapon ( entID, (char *) data);
		break;

	case SET_ITEM:
		Q3_SetItem ( entID, (char *) data);
		break;

	case SET_WALKSPEED:
		int_data = atoi((char *) data);
		Q3_SetWalkSpeed ( entID, int_data);
		break;

	case SET_RUNSPEED:
		int_data = atoi((char *) data);
		Q3_SetRunSpeed ( entID, int_data);
		break;

	case SET_WIDTH:
		int_data = atoi((char *) data);
		Q3_SetWidth( entID, int_data );
		return qfalse;
		break;

	case SET_YAWSPEED:
		float_data = atof((char *) data);
		Q3_SetYawSpeed ( entID, float_data);
		break;

	case SET_AGGRESSION:
		int_data = atoi((char *) data);
		Q3_SetAggression ( entID, int_data);
		break;

	case SET_AIM:
		int_data = atoi((char *) data);
		Q3_SetAim ( entID, int_data);
		break;

	case SET_FRICTION:
		int_data = atoi((char *) data);
		Q3_SetFriction ( entID, int_data);
		break;

	case SET_GRAVITY:
		float_data = atof((char *) data);
		Q3_SetGravity ( entID, float_data);
		break;

	case SET_WAIT:
		float_data = atof((char *) data);
		Q3_SetWait( entID, float_data);
		break;

	case SET_FOLLOWDIST:
		float_data = atof((char *) data);
		Q3_SetFollowDist( entID, float_data);
		break;

	case SET_SCALE:
		float_data = atof((char *) data);
		Q3_SetScale( entID, float_data);
		break;

	case SET_COUNT:
		Q3_SetCount( entID, (char *) data);
		break;

	case SET_SHOT_SPACING:
		int_data = atoi((char *) data);
		Q3_SetShotSpacing( entID, int_data );
		break;

	case SET_IGNOREPAIN:
		if(!Q_stricmp("true", ((char *)data)))
			Q3_SetIgnorePain( entID, qtrue);
		else if(!Q_stricmp("false", ((char *)data)))
			Q3_SetIgnorePain( entID, qfalse);
		break;

	case SET_IGNOREENEMIES:
		if(!Q_stricmp("true", ((char *)data)))
			Q3_SetIgnoreEnemies( entID, qtrue);
		else if(!Q_stricmp("false", ((char *)data)))
			Q3_SetIgnoreEnemies( entID, qfalse);
		break;

	case SET_IGNOREALERTS:
		if(!Q_stricmp("true", ((char *)data)))
			Q3_SetIgnoreAlerts( entID, qtrue);
		else if(!Q_stricmp("false", ((char *)data)))
			Q3_SetIgnoreAlerts( entID, qfalse);
		break;

	case SET_DONTSHOOT:
		if(!Q_stricmp("true", ((char *)data)))
			Q3_SetDontShoot( entID, qtrue);
		else if(!Q_stricmp("false", ((char *)data)))
			Q3_SetDontShoot( entID, qfalse);
		break;

	case SET_DONTFIRE:
		if(!Q_stricmp("true", ((char *)data)))
			Q3_SetDontFire( entID, qtrue);
		else if(!Q_stricmp("false", ((char *)data)))
			Q3_SetDontFire( entID, qfalse);
		break;

	case SET_LOCKED_ENEMY:
		if(!Q_stricmp("true", ((char *)data)))
			Q3_SetLockedEnemy( entID, qtrue);
		else if(!Q_stricmp("false", ((char *)data)))
			Q3_SetLockedEnemy( entID, qfalse);
		break;

	case SET_NOTARGET:
		if(!Q_stricmp("true", ((char *)data)))
			Q3_SetNoTarget( entID, qtrue);
		else if(!Q_stricmp("false", ((char *)data)))
			Q3_SetNoTarget( entID, qfalse);
		break;

	case SET_LEAN:
		G_DebugPrint( WL_WARNING, "SET_LEAN NOT SUPPORTED IN MP\n" );
		break;

	case SET_SHOOTDIST:
		float_data = atof((char *) data);
		Q3_SetShootDist( entID, float_data );
		break;

	case SET_TIMESCALE:
		Q3_SetTimeScale( entID, (char *) data );
		break;

	case SET_VISRANGE:
		float_data = atof((char *) data);
		Q3_SetVisrange( entID, float_data );
		break;

	case SET_EARSHOT:
		float_data = atof((char *) data);
		Q3_SetEarshot( entID, float_data );
		break;

	case SET_VIGILANCE:
		float_data = atof((char *) data);
		Q3_SetVigilance( entID, float_data );
		break;

	case SET_VFOV:
		int_data = atoi((char *) data);
		Q3_SetVFOV( entID, int_data );
		break;

	case SET_HFOV:
		int_data = atoi((char *) data);
		Q3_SetHFOV( entID, int_data );
		break;

	case SET_TARGETNAME:
		Q3_SetTargetName( entID, (char *) data );
		break;

	case SET_TARGET:
		Q3_SetTarget( entID, (char *) data );
		break;

	case SET_TARGET2:
		Q3_SetTarget2( entID, (char *) data );
		break;

	case SET_LOCATION:
		if ( !Q3_SetLocation( entID, (char *) data ) )
		{
			trap->ICARUS_TaskIDSet( (sharedEntity_t *)ent, TID_LOCATION, taskID );
			return qfalse;
		}
		break;

	case SET_PAINTARGET:
		Q3_SetPainTarget( entID, (char *) data );
		break;

	case SET_DEFEND_TARGET:
		G_DebugPrint( WL_WARNING, "Q3_SetDefendTarget unimplemented\n", entID );
		//Q3_SetEnemy( entID, (char *) data);
		break;

	case SET_PARM1:
	case SET_PARM2:
	case SET_PARM3:
	case SET_PARM4:
	case SET_PARM5:
	case SET_PARM6:
	case SET_PARM7:
	case SET_PARM8:
	case SET_PARM9:
	case SET_PARM10:
	case SET_PARM11:
	case SET_PARM12:
	case SET_PARM13:
	case SET_PARM14:
	case SET_PARM15:
	case SET_PARM16:
		Q3_SetParm( entID, (toSet-SET_PARM1), (char *) data );
		break;

	case SET_SPAWNSCRIPT:
	case SET_USESCRIPT:
	case SET_AWAKESCRIPT:
	case SET_ANGERSCRIPT:
	case SET_ATTACKSCRIPT:
	case SET_VICTORYSCRIPT:
	case SET_PAINSCRIPT:
	case SET_FLEESCRIPT:
	case SET_DEATHSCRIPT:
	case SET_DELAYEDSCRIPT:
	case SET_BLOCKEDSCRIPT:
	case SET_FFIRESCRIPT:
	case SET_FFDEATHSCRIPT:
	case SET_MINDTRICKSCRIPT:
		if( !Q3_SetBehaviorSet(entID, toSet, (char *) data) )
			G_DebugPrint( WL_ERROR, "Q3_SetBehaviorSet: Invalid bSet %s\n", type_name );
		break;

	case SET_NO_MINDTRICK:
		if(!Q_stricmp("true", ((char *)data)))
			Q3_SetNoMindTrick( entID, qtrue);
		else
			Q3_SetNoMindTrick( entID, qfalse);
		break;

	case SET_CINEMATIC_SKIPSCRIPT :
		Q3_SetCinematicSkipScript((char *) data);
		break;


	case SET_DELAYSCRIPTTIME:
		int_data = atoi((char *) data);
		Q3_SetDelayScriptTime( entID, int_data );
		break;

	case SET_CROUCHED:
		if(!Q_stricmp("true", ((char *)data)))
			Q3_SetCrouched( entID, qtrue);
		else
			Q3_SetCrouched( entID, qfalse);
		break;

	case SET_WALKING:
		if(!Q_stricmp("true", ((char *)data)))
			Q3_SetWalking( entID, qtrue);
		else
			Q3_SetWalking( entID, qfalse);
		break;

	case SET_RUNNING:
		if(!Q_stricmp("true", ((char *)data)))
			Q3_SetRunning( entID, qtrue);
		else
			Q3_SetRunning( entID, qfalse);
		break;

	case SET_CHASE_ENEMIES:
		if(!Q_stricmp("true", ((char *)data)))
			Q3_SetChaseEnemies( entID, qtrue);
		else
			Q3_SetChaseEnemies( entID, qfalse);
		break;

	case SET_LOOK_FOR_ENEMIES:
		if(!Q_stricmp("true", ((char *)data)))
			Q3_SetLookForEnemies( entID, qtrue);
		else
			Q3_SetLookForEnemies( entID, qfalse);
		break;

	case SET_FACE_MOVE_DIR:
		if(!Q_stricmp("true", ((char *)data)))
			Q3_SetFaceMoveDir( entID, qtrue);
		else
			Q3_SetFaceMoveDir( entID, qfalse);
		break;

	case SET_ALT_FIRE:
		if(!Q_stricmp("true", ((char *)data)))
			Q3_SetAltFire( entID, qtrue);
		else
			Q3_SetAltFire( entID, qfalse);
		break;

	case SET_DONT_FLEE:
		if(!Q_stricmp("true", ((char *)data)))
			Q3_SetDontFlee( entID, qtrue);
		else
			Q3_SetDontFlee( entID, qfalse);
		break;

	case SET_FORCED_MARCH:
		if(!Q_stricmp("true", ((char *)data)))
			Q3_SetForcedMarch( entID, qtrue);
		else
			Q3_SetForcedMarch( entID, qfalse);
		break;

	case SET_NO_RESPONSE:
		if(!Q_stricmp("true", ((char *)data)))
			Q3_SetNoResponse( entID, qtrue);
		else
			Q3_SetNoResponse( entID, qfalse);
		break;

	case SET_NO_COMBAT_TALK:
		if(!Q_stricmp("true", ((char *)data)))
			Q3_SetCombatTalk( entID, qtrue);
		else
			Q3_SetCombatTalk( entID, qfalse);
		break;

	case SET_NO_ALERT_TALK:
		if(!Q_stricmp("true", ((char *)data)))
			Q3_SetAlertTalk( entID, qtrue);
		else
			Q3_SetAlertTalk( entID, qfalse);
		break;

	case SET_USE_CP_NEAREST:
		if(!Q_stricmp("true", ((char *)data)))
			Q3_SetUseCpNearest( entID, qtrue);
		else
			Q3_SetUseCpNearest( entID, qfalse);
		break;

	case SET_NO_FORCE:
		if(!Q_stricmp("true", ((char *)data)))
			Q3_SetNoForce( entID, qtrue);
		else
			Q3_SetNoForce( entID, qfalse);
		break;

	case SET_NO_ACROBATICS:
		if(!Q_stricmp("true", ((char *)data)))
			Q3_SetNoAcrobatics( entID, qtrue);
		else
			Q3_SetNoAcrobatics( entID, qfalse);
		break;

	case SET_USE_SUBTITLES:
		if(!Q_stricmp("true", ((char *)data)))
			Q3_SetUseSubtitles( entID, qtrue);
		else
			Q3_SetUseSubtitles( entID, qfalse);
		break;

	case SET_NO_FALLTODEATH:
		if(!Q_stricmp("true", ((char *)data)))
			Q3_SetNoFallToDeath( entID, qtrue);
		else
			Q3_SetNoFallToDeath( entID, qfalse);
		break;

	case SET_DISMEMBERABLE:
		if(!Q_stricmp("true", ((char *)data)))
			Q3_SetDismemberable( entID, qtrue);
		else
			Q3_SetDismemberable( entID, qfalse);
		break;

	case SET_MORELIGHT:
		if(!Q_stricmp("true", ((char *)data)))
			Q3_SetMoreLight( entID, qtrue);
		else
			Q3_SetMoreLight( entID, qfalse);
		break;


	case SET_TREASONED:
		G_DebugPrint( WL_VERBOSE, "SET_TREASONED is disabled, do not use\n" );
		/*
		G_TeamRetaliation( NULL, SV_GentityNum(0), qfalse );
		ffireLevel = FFIRE_LEVEL_RETALIATION;
		*/
		break;

	case SET_UNDYING:
		if(!Q_stricmp("true", ((char *)data)))
			Q3_SetUndying( entID, qtrue);
		else
			Q3_SetUndying( entID, qfalse);
		break;

	case SET_INVINCIBLE:
		if(!Q_stricmp("true", ((char *)data)))
			Q3_SetInvincible( entID, qtrue);
		else
			Q3_SetInvincible( entID, qfalse);
		break;

	case SET_NOAVOID:
		if(!Q_stricmp("true", ((char *)data)))
			Q3_SetNoAvoid( entID, qtrue);
		else
			Q3_SetNoAvoid( entID, qfalse);
		break;

	case SET_SOLID:
		if(!Q_stricmp("true", ((char *)data)))
		{
			if ( !Q3_SetSolid( entID, qtrue) )
			{
				trap->ICARUS_TaskIDSet( (sharedEntity_t *)ent, TID_RESIZE, taskID );
				return qfalse;
			}
		}
		else
		{
			Q3_SetSolid( entID, qfalse);
		}
		break;

	case SET_INVISIBLE:
		if( !Q_stricmp("true", ((char *)data)) )
			Q3_SetInvisible( entID, qtrue );
		else
			Q3_SetInvisible( entID, qfalse );
		break;

	case SET_VAMPIRE:
		if( !Q_stricmp("true", ((char *)data)) )
			Q3_SetVampire( entID, qtrue );
		else
			Q3_SetVampire( entID, qfalse );
		break;

	case SET_FORCE_INVINCIBLE:
		if( !Q_stricmp("true", ((char *)data)) )
			Q3_SetForceInvincible( entID, qtrue );
		else
			Q3_SetForceInvincible( entID, qfalse );
		break;

	case SET_GREET_ALLIES:
		if( !Q_stricmp("true", ((char *)data)) )
			Q3_SetGreetAllies( entID, qtrue );
		else
			Q3_SetGreetAllies( entID, qfalse );
		break;

	case SET_PLAYER_LOCKED:
		if( !Q_stricmp("true", ((char *)data)) )
			Q3_SetPlayerLocked( entID, qtrue );
		else
			Q3_SetPlayerLocked( entID, qfalse );
		break;

	case SET_LOCK_PLAYER_WEAPONS:
		if( !Q_stricmp("true", ((char *)data)) )
			Q3_SetLockPlayerWeapons( entID, qtrue );
		else
			Q3_SetLockPlayerWeapons( entID, qfalse );
		break;

	case SET_NO_IMPACT_DAMAGE:
		if( !Q_stricmp("true", ((char *)data)) )
			Q3_SetNoImpactDamage( entID, qtrue );
		else
			Q3_SetNoImpactDamage( entID, qfalse );
		break;

	case SET_FORWARDMOVE:
		int_data = atoi((char *) data);
		Q3_SetForwardMove( entID, int_data);
		break;

	case SET_RIGHTMOVE:
		int_data = atoi((char *) data);
		Q3_SetRightMove( entID, int_data);
		break;

	case SET_LOCKYAW:
		Q3_SetLockAngle( entID, data);
		break;

	case SET_CAMERA_GROUP:
		Q3_CameraGroup(entID, (char *)data);
		break;
	case SET_CAMERA_GROUP_Z_OFS:
		float_data = atof((char *) data);
		Q3_CameraGroupZOfs( float_data );
		break;
	case SET_CAMERA_GROUP_TAG:
		Q3_CameraGroupTag( (char *)data );
		break;

	//FIXME: put these into camera commands
	case SET_LOOK_TARGET:
		Q3_LookTarget(entID, (char *)data);
		break;

	case SET_ADDRHANDBOLT_MODEL:
		Q3_AddRHandModel(entID, (char *)data);
		break;

	case SET_REMOVERHANDBOLT_MODEL:
		Q3_RemoveRHandModel(entID, (char *)data);
		break;

	case SET_ADDLHANDBOLT_MODEL:
		Q3_AddLHandModel(entID, (char *)data);
		break;

	case SET_REMOVELHANDBOLT_MODEL:
		Q3_RemoveLHandModel(entID, (char *)data);
		break;

	case SET_FACEEYESCLOSED:
	case SET_FACEEYESOPENED:
	case SET_FACEAUX:
	case SET_FACEBLINK:
	case SET_FACEBLINKFROWN:
	case SET_FACEFROWN:
	case SET_FACENORMAL:
		float_data = atof((char *) data);
		Q3_Face(entID, toSet, float_data);
		break;

	case SET_SCROLLTEXT:
		Q3_ScrollText( (char *)data );
		break;

	case SET_LCARSTEXT:
		Q3_LCARSText( (char *)data );
		break;

	case SET_CAPTIONTEXTCOLOR:
		Q3_SetCaptionTextColor ( (char *)data );
		break;
	case SET_CENTERTEXTCOLOR:
		Q3_SetCenterTextColor ( (char *)data );
		break;
	case SET_SCROLLTEXTCOLOR:
		Q3_SetScrollTextColor ( (char *)data );
		break;

	case SET_PLAYER_USABLE:
		if(!Q_stricmp("true", ((char *)data)))
		{
			Q3_SetPlayerUsable(entID, qtrue);
		}
		else
		{
			Q3_SetPlayerUsable(entID, qfalse);
		}
		break;

	case SET_STARTFRAME:
		int_data = atoi((char *) data);
		Q3_SetStartFrame(entID, int_data);
		break;

	case SET_ENDFRAME:
		int_data = atoi((char *) data);
		Q3_SetEndFrame(entID, int_data);

		trap->ICARUS_TaskIDSet( (sharedEntity_t *)ent, TID_ANIM_BOTH, taskID );
		return qfalse;
		break;

	case SET_ANIMFRAME:
		int_data = atoi((char *) data);
		Q3_SetAnimFrame(entID, int_data);
		return qfalse;
		break;

	case SET_LOOP_ANIM:
		if(!Q_stricmp("true", ((char *)data)))
		{
			Q3_SetLoopAnim(entID, qtrue);
		}
		else
		{
			Q3_SetLoopAnim(entID, qfalse);
		}
		break;

	case SET_INTERFACE:
		G_DebugPrint( WL_WARNING, "Q3_SetInterface: NOT SUPPORTED IN MP\n");

		break;

	case SET_SHIELDS:
		if(!Q_stricmp("true", ((char *)data)))
		{
			Q3_SetShields(entID, qtrue);
		}
		else
		{
			Q3_SetShields(entID, qfalse);
		}
		break;

	case SET_SABERACTIVE:
		if(!Q_stricmp("true", ((char *)data)))
		{
			Q3_SetSaberActive( entID, qtrue );
		}
		else
		{
			Q3_SetSaberActive( entID, qfalse );
		}
		break;

	case SET_ADJUST_AREA_PORTALS:
		G_DebugPrint( WL_WARNING, "Q3_SetAdjustAreaPortals: NOT SUPPORTED IN MP\n");
		break;

	case SET_DMG_BY_HEAVY_WEAP_ONLY:
		G_DebugPrint( WL_WARNING, "Q3_SetDmgByHeavyWeapOnly: NOT SUPPORTED IN MP\n");
		break;

	case SET_SHIELDED:
		G_DebugPrint( WL_WARNING, "Q3_SetShielded: NOT SUPPORTED IN MP\n");
		break;

	case SET_NO_GROUPS:
		G_DebugPrint( WL_WARNING, "Q3_SetNoGroups: NOT SUPPORTED IN MP\n");
		break;

	case SET_FIRE_WEAPON:
		if(!Q_stricmp("true", ((char *)data)))
		{
			Q3_SetFireWeapon( entID, qtrue);
		}
		else if(!Q_stricmp("false", ((char *)data)))
		{
			Q3_SetFireWeapon( entID, qfalse);
		}
		break;

	case SET_INACTIVE:
		if(!Q_stricmp("true", ((char *)data)))
		{
			Q3_SetInactive( entID, qtrue);
		}
		else if(!Q_stricmp("false", ((char *)data)))
		{
			Q3_SetInactive( entID, qfalse);
		}
		else if(!Q_stricmp("unlocked", ((char *)data)))
		{
			UnLockDoors(&g_entities[entID]);
		}
		else if(!Q_stricmp("locked", ((char *)data)))
		{
			LockDoors(&g_entities[entID]);
		}
		break;
	case SET_END_SCREENDISSOLVE:
		G_DebugPrint( WL_WARNING, "SET_END_SCREENDISSOLVE: NOT SUPPORTED IN MP\n");
		break;

	case SET_MISSION_STATUS_SCREEN:
		//Cvar_Set("cg_missionstatusscreen", "1");
		G_DebugPrint( WL_WARNING, "SET_MISSION_STATUS_SCREEN: NOT SUPPORTED IN MP\n");
		break;

	case SET_FUNC_USABLE_VISIBLE:
		if(!Q_stricmp("true", ((char *)data)))
		{
			Q3_SetFuncUsableVisible( entID, qtrue);
		}
		else if(!Q_stricmp("false", ((char *)data)))
		{
			Q3_SetFuncUsableVisible( entID, qfalse);
		}
		break;

	case SET_NO_KNOCKBACK:
		if(!Q_stricmp("true", ((char *)data)))
		{
			Q3_SetNoKnockback(entID, qtrue);
		}
		else
		{
			Q3_SetNoKnockback(entID, qfalse);
		}
		break;

	case SET_VIDEO_PLAY:
		// don't do this check now, James doesn't want a scripted cinematic to also skip any Video cinematics as well,
		//	the "timescale" and "skippingCinematic" cvars will be set back to normal in the Video code, so doing a
		//	skip will now only skip one section of a multiple-part story (eg VOY1 bridge sequence)
		//
//		if ( g_timescale->value <= 1.0f )
		{
			G_DebugPrint( WL_WARNING, "SET_VIDEO_PLAY: NOT SUPPORTED IN MP\n");
			//SV_SendConsoleCommand( va("inGameCinematic %s\n", (char *)data) );
		}
		break;

	case SET_VIDEO_FADE_IN:
		G_DebugPrint( WL_WARNING, "SET_VIDEO_FADE_IN: NOT SUPPORTED IN MP\n");
		break;

	case SET_VIDEO_FADE_OUT:
		G_DebugPrint( WL_WARNING, "SET_VIDEO_FADE_OUT: NOT SUPPORTED IN MP\n");
		break;
	case SET_REMOVE_TARGET:
		Q3_SetRemoveTarget( entID, (const char *) data );
		break;

	case SET_LOADGAME:
		//trap->SendConsoleCommand( va("load %s\n", (const char *) data ) );
		G_DebugPrint( WL_WARNING, "SET_LOADGAME: NOT SUPPORTED IN MP\n");
		break;

	case SET_MENU_SCREEN:
		//UI_SetActiveMenu( (const char *) data );
		break;

	case SET_OBJECTIVE_SHOW:
		G_DebugPrint( WL_WARNING, "SET_OBJECTIVE_SHOW: NOT SUPPORTED IN MP\n");
		break;
	case SET_OBJECTIVE_HIDE:
		G_DebugPrint( WL_WARNING, "SET_OBJECTIVE_HIDE: NOT SUPPORTED IN MP\n");
		break;
	case SET_OBJECTIVE_SUCCEEDED:
		G_DebugPrint( WL_WARNING, "SET_OBJECTIVE_SUCCEEDED: NOT SUPPORTED IN MP\n");
		break;
	case SET_OBJECTIVE_FAILED:
		G_DebugPrint( WL_WARNING, "SET_OBJECTIVE_FAILED: NOT SUPPORTED IN MP\n");
		break;

	case SET_OBJECTIVE_CLEARALL:
		G_DebugPrint( WL_WARNING, "SET_OBJECTIVE_CLEARALL: NOT SUPPORTED IN MP\n");
		break;

	case SET_MISSIONFAILED:
		G_DebugPrint( WL_WARNING, "SET_MISSIONFAILED: NOT SUPPORTED IN MP\n");
		break;

	case SET_MISSIONSTATUSTEXT:
		G_DebugPrint( WL_WARNING, "SET_MISSIONSTATUSTEXT: NOT SUPPORTED IN MP\n");
		break;

	case SET_MISSIONSTATUSTIME:
		G_DebugPrint( WL_WARNING, "SET_MISSIONSTATUSTIME: NOT SUPPORTED IN MP\n");
		break;

	case SET_CLOSINGCREDITS:
		G_DebugPrint( WL_WARNING, "SET_CLOSINGCREDITS: NOT SUPPORTED IN MP\n");
		break;

	case SET_SKILL:
//		//can never be set
		break;

	case SET_FULLNAME:
		Q3_SetFullName( entID, (char *) data );
		break;

	case SET_DISABLE_SHADER_ANIM:
		if(!Q_stricmp("true", ((char *)data)))
		{
			Q3_SetDisableShaderAnims( entID, qtrue);
		}
		else
		{
			Q3_SetDisableShaderAnims( entID, qfalse);
		}
		break;

	case SET_SHADER_ANIM:
		if(!Q_stricmp("true", ((char *)data)))
		{
			Q3_SetShaderAnim( entID, qtrue);
		}
		else
		{
			Q3_SetShaderAnim( entID, qfalse);
		}
		break;

	case SET_MUSIC_STATE:
		Q3_SetMusicState( (char *) data );
		break;

	case SET_CLEAN_DAMAGING_ENTS:
		Q3_SetCleanDamagingEnts();
		break;

	case SET_HUD:
		G_DebugPrint( WL_WARNING, "SET_HUD: NOT SUPPORTED IN MP\n");
		break;

	case SET_FORCE_HEAL_LEVEL:
	case SET_FORCE_JUMP_LEVEL:
	case SET_FORCE_SPEED_LEVEL:
	case SET_FORCE_PUSH_LEVEL:
	case SET_FORCE_PULL_LEVEL:
	case SET_FORCE_MINDTRICK_LEVEL:
	case SET_FORCE_GRIP_LEVEL:
	case SET_FORCE_LIGHTNING_LEVEL:
	case SET_SABER_THROW:
	case SET_SABER_DEFENSE:
	case SET_SABER_OFFENSE:
		int_data = atoi((char *) data);
		Q3_SetForcePowerLevel( entID, (toSet-SET_FORCE_HEAL_LEVEL), int_data );
		break;

	default:
		//G_DebugPrint( WL_ERROR, "Q3_Set: '%s' is not a valid set field\n", type_name );
		trap->ICARUS_SetVar( taskID, entID, type_name, data );
		break;
	}

	return qtrue;
}
