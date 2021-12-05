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

// cl_cgame.c  -- client system interaction with client game

// leave this as first line for PCH reasons...
//
#include "../server/exe_headers.h"
#include "../ui/ui_shared.h"

#include "client.h"
#include "vmachine.h"
#include "qcommon/stringed_ingame.h"
#include "sys/sys_loadlib.h"

vm_t	cgvm;
/*
Ghoul2 Insert Start
*/

#if !defined(G2_H_INC)
	#include "../ghoul2/G2.h"
#endif

/*
Ghoul2 Insert End
*/

//FIXME: Temp
extern void S_UpdateAmbientSet ( const char *name, vec3_t origin );
extern int S_AddLocalSet( const char *name, vec3_t listener_origin, vec3_t origin, int entID, int time );
extern void AS_ParseSets( void );
extern sfxHandle_t AS_GetBModelSound( const char *name, int stage );
extern void	AS_AddPrecacheEntry( const char *name );
extern menuDef_t *Menus_FindByName(const char *p);

extern qboolean R_inPVS( vec3_t p1, vec3_t p2 );

void UI_SetActiveMenu( const char* menuname,const char *menuID );

qboolean CL_InitCGameVM( void *gameLibrary )
{
	typedef intptr_t SyscallProc( intptr_t, ... );
	typedef void DllEntryProc( SyscallProc * );

	DllEntryProc *dllEntry = (DllEntryProc *)Sys_LoadFunction( gameLibrary, "dllEntry" );

	// NOTE: arm64 mac has a different calling convention for fixed parameters vs. variadic parameters.
	//       As the cgame entryPoints (vmMain) in jk2 and jka use fixed arg0 to arg7 we can't use "..." around here or we end up with undefined behavior.
	//       See: https://developer.apple.com/documentation/apple-silicon/addressing-architectural-differences-in-your-macos-code
	cgvm.entryPoint = (intptr_t (*)(int,intptr_t,intptr_t,intptr_t,intptr_t,intptr_t,intptr_t,intptr_t,intptr_t))Sys_LoadFunction( gameLibrary, "vmMain" );

	if ( !cgvm.entryPoint || !dllEntry ) {
#ifdef JK2_MODE
		const char *gamename = "jospgame";
#else
		const char *gamename = "jagame";
#endif

		Com_Printf( "CL_InitCGameVM: client game entry point not found in %s" ARCH_STRING DLL_EXT ": %s\n",
					gamename, Sys_LibraryError() );
		return qfalse;
	}

	dllEntry( VM_DllSyscall );

	return qtrue;
}

/*
====================
CL_GetGameState
====================
*/
void CL_GetGameState( gameState_t *gs ) {
	*gs = cl.gameState;
}

/*
====================
CL_GetGlconfig
====================
*/
void CL_GetGlconfig( glconfig_t *glconfig ) {
	*glconfig = cls.glconfig;
}


/*
====================
CL_GetUserCmd
====================
*/
qboolean CL_GetUserCmd( int cmdNumber, usercmd_t *ucmd ) {
	// cmds[cmdNumber] is the last properly generated command

	// can't return anything that we haven't created yet
	if ( cmdNumber > cl.cmdNumber ) {
		Com_Error( ERR_DROP, "CL_GetUserCmd: %i >= %i", cmdNumber, cl.cmdNumber );
	}

	// the usercmd has been overwritten in the wrapping
	// buffer because it is too far out of date
	if ( cmdNumber <= cl.cmdNumber - CMD_BACKUP ) {
		return qfalse;
	}

	*ucmd = cl.cmds[ cmdNumber & CMD_MASK ];

	return qtrue;
}

int CL_GetCurrentCmdNumber( void ) {
	return cl.cmdNumber;
}


/*
====================
CL_GetParseEntityState
====================
*/
/*
qboolean	CL_GetParseEntityState( int parseEntityNumber, entityState_t *state ) {
	// can't return anything that hasn't been parsed yet
	if ( parseEntityNumber >= cl.parseEntitiesNum ) {
		Com_Error( ERR_DROP, "CL_GetParseEntityState: %i >= %i",
			parseEntityNumber, cl.parseEntitiesNum );
	}

	// can't return anything that has been overwritten in the circular buffer
	if ( parseEntityNumber <= cl.parseEntitiesNum - MAX_PARSE_ENTITIES ) {
		return qfalse;
	}

	*state = cl.parseEntities[ parseEntityNumber & ( MAX_PARSE_ENTITIES - 1 ) ];
	return qtrue;
}
*/

/*
====================
CL_GetCurrentSnapshotNumber
====================
*/
void	CL_GetCurrentSnapshotNumber( int *snapshotNumber, int *serverTime ) {
	*snapshotNumber = cl.frame.messageNum;
	*serverTime = cl.frame.serverTime;
}

/*
====================
CL_GetSnapshot
====================
*/
qboolean	CL_GetSnapshot( int snapshotNumber, snapshot_t *snapshot ) {
	clSnapshot_t	*clSnap;
	int				i, count;

	if ( snapshotNumber > cl.frame.messageNum ) {
		Com_Error( ERR_DROP, "CL_GetSnapshot: snapshotNumber > cl.frame.messageNum" );
	}

	// if the frame has fallen out of the circular buffer, we can't return it
	if ( cl.frame.messageNum - snapshotNumber >= PACKET_BACKUP ) {
		return qfalse;
	}

	// if the frame is not valid, we can't return it
	clSnap = &cl.frames[snapshotNumber & PACKET_MASK];
	if ( !clSnap->valid ) {
		return qfalse;
	}

	// if the entities in the frame have fallen out of their
	// circular buffer, we can't return it
	if ( cl.parseEntitiesNum - clSnap->parseEntitiesNum >= MAX_PARSE_ENTITIES ) {
		return qfalse;
	}

	// write the snapshot
	snapshot->snapFlags = clSnap->snapFlags;
	snapshot->serverCommandSequence = clSnap->serverCommandNum;
	snapshot->serverTime = clSnap->serverTime;
	memcpy( snapshot->areamask, clSnap->areamask, sizeof( snapshot->areamask ) );
	snapshot->cmdNum = clSnap->cmdNum;
	snapshot->ps = clSnap->ps;
	count = clSnap->numEntities;
	if ( count > MAX_ENTITIES_IN_SNAPSHOT ) {
		Com_DPrintf( "CL_GetSnapshot: truncated %i entities to %i\n", count, MAX_ENTITIES_IN_SNAPSHOT );
		count = MAX_ENTITIES_IN_SNAPSHOT;
	}
	snapshot->numEntities = count;
/*
Ghoul2 Insert Start
*/
 	for ( i = 0 ; i < count ; i++ )
	{

		int entNum =  ( clSnap->parseEntitiesNum + i ) & (MAX_PARSE_ENTITIES-1) ;
		snapshot->entities[i] = cl.parseEntities[ entNum ];
	}
/*
Ghoul2 Insert End
*/


	// FIXME: configstring changes and server commands!!!

	return qtrue;
}

//bg_public.h won't cooperate in here
#define EF_PERMANENT   0x00080000

qboolean CL_GetDefaultState(int index, entityState_t *state)
{
	if (index < 0 || index >= MAX_GENTITIES)
	{
		return qfalse;
	}

	// Is this safe? I think so. But it's still ugly as sin.
	if (!(sv.svEntities[index].baseline.eFlags & EF_PERMANENT))
//	if (!(cl.entityBaselines[index].eFlags & EF_PERMANENT))
	{
		return qfalse;
	}

	*state = sv.svEntities[index].baseline;
//	*state = cl.entityBaselines[index];

	return qtrue;
}

extern float cl_mPitchOverride;
extern float cl_mYawOverride;
void CL_SetUserCmdValue( int userCmdValue, float sensitivityScale, float mPitchOverride, float mYawOverride ) {
	cl.cgameUserCmdValue = userCmdValue;
	cl.cgameSensitivity = sensitivityScale;
	cl_mPitchOverride = mPitchOverride;
	cl_mYawOverride = mYawOverride;
}

extern vec3_t cl_overriddenAngles;
extern qboolean cl_overrideAngles;
void CL_SetUserCmdAngles( float pitchOverride, float yawOverride, float rollOverride ) {
	cl_overriddenAngles[PITCH] = pitchOverride;
	cl_overriddenAngles[YAW] = yawOverride;
	cl_overriddenAngles[ROLL] = rollOverride;
	cl_overrideAngles = qtrue;
}

void CL_AddCgameCommand( const char *cmdName ) {
	Cmd_AddCommand( cmdName, NULL );
}

/*
=====================
CL_ConfigstringModified
=====================
*/
void CL_ConfigstringModified( void ) {
	const char *s;
	char		*old;
	int			i, index;
	const char		*dup;
	gameState_t	oldGs;
	int			len;

	index = atoi( Cmd_Argv(1) );
	if ( index < 0 || index >= MAX_CONFIGSTRINGS ) {
		Com_Error( ERR_DROP, "configstring > MAX_CONFIGSTRINGS" );
	}
	s = Cmd_Argv(2);

	old = cl.gameState.stringData + cl.gameState.stringOffsets[ index ];
	if ( !strcmp( old, s ) ) {
		return;		// unchanged
	}

	// build the new gameState_t
	oldGs = cl.gameState;

	memset( &cl.gameState, 0, sizeof( cl.gameState ) );

	// leave the first 0 for uninitialized strings
	cl.gameState.dataCount = 1;

	for ( i = 0 ; i < MAX_CONFIGSTRINGS ; i++ ) {
		if ( i == index ) {
			dup = s;
		} else {
			dup = oldGs.stringData + oldGs.stringOffsets[ i ];
		}
		if ( !dup[0] ) {
			continue;		// leave with the default empty string
		}

		len = strlen( dup );

		if ( len + 1 + cl.gameState.dataCount > MAX_GAMESTATE_CHARS ) {
			Com_Error( ERR_DROP, "MAX_GAMESTATE_CHARS exceeded" );
		}

		// append it to the gameState string buffer
		cl.gameState.stringOffsets[ i ] = cl.gameState.dataCount;
		memcpy( cl.gameState.stringData + cl.gameState.dataCount, dup, len + 1 );
		cl.gameState.dataCount += len + 1;
	}

	if ( index == CS_SYSTEMINFO ) {
		// parse serverId and other cvars
		CL_SystemInfoChanged();
	}

}


/*
===================
CL_GetServerCommand

Set up argc/argv for the given command
===================
*/
qboolean CL_GetServerCommand( int serverCommandNumber ) {
	char	*s;
	const char	*cmd;

	// if we have irretrievably lost a reliable command, drop the connection
	if ( serverCommandNumber <= clc.serverCommandSequence - MAX_RELIABLE_COMMANDS ) {
		Com_Error( ERR_DROP, "CL_GetServerCommand: a reliable command was cycled out" );
		return qfalse;
	}

	if ( serverCommandNumber > clc.serverCommandSequence ) {
		Com_Error( ERR_DROP, "CL_GetServerCommand: requested a command not received" );
		return qfalse;
	}

	s = clc.serverCommands[ serverCommandNumber & ( MAX_RELIABLE_COMMANDS - 1 ) ];

	Com_DPrintf( "serverCommand: %i : %s\n", serverCommandNumber, s );

	Cmd_TokenizeString( s );
	cmd = Cmd_Argv(0);

	if ( !strcmp( cmd, "disconnect" ) ) {
		Com_Error (ERR_DISCONNECT,"Server disconnected\n");
	}

	if ( !strcmp( cmd, "cs" ) ) {
		CL_ConfigstringModified();
		// reparse the string, because CL_ConfigstringModified may have done another Cmd_TokenizeString()
		Cmd_TokenizeString( s );
		return qtrue;
	}

	// the clientLevelShot command is used during development
	// to generate 128*128 screenshots from the intermission
	// point of levels for the menu system to use
	// we pass it along to the cgame to make apropriate adjustments,
	// but we also clear the console and notify lines here
	if ( !strcmp( cmd, "clientLevelShot" ) ) {
		// don't do it if we aren't running the server locally,
		// otherwise malicious remote servers could overwrite
		// the existing thumbnails
		if ( !com_sv_running->integer ) {
			return qfalse;
		}
		// close the console
		Con_Close();
		// take a special screenshot next frame
		Cbuf_AddText( "wait ; wait ; wait ; wait ; screenshot levelshot\n" );
		return qtrue;
	}

	// we may want to put a "connect to other server" command here

	// cgame can now act on the command
	return qtrue;
}


/*
====================
CL_CM_LoadMap

Just adds default parameters that cgame doesn't need to know about
====================
*/
void CL_CM_LoadMap( const char *mapname, qboolean subBSP ) {
	int		checksum;

	CM_LoadMap( mapname, qtrue, &checksum, subBSP );
}

/*
====================
CL_ShutdonwCGame

====================
*/
void CL_ShutdownCGame( void ) {
	cls.cgameStarted = qfalse;

	if ( !cgvm.entryPoint) {
		return;
	}
	VM_Call( CG_SHUTDOWN );

//	VM_Free( cgvm );
//	cgvm = NULL;
}

#ifdef JK2_MODE
/*
====================
CL_ConvertJK2SysCall

Converts a JK2 syscall to a JKA syscall
====================
*/

cgameImport_t CL_ConvertJK2SysCall( cgameJK2Import_t import )
{
	// FIXME: This was a 5-minute slap-hack job in order to test if this really works. CLEAN ME UP! --eez
	switch(import)
	{
		case CG_PRINT_JK2:
			return CG_PRINT;
			break;
		case CG_ERROR_JK2:
			return CG_ERROR;
			break;
		case CG_MILLISECONDS_JK2:
			return CG_MILLISECONDS;
			break;
		case CG_CVAR_REGISTER_JK2:
			return CG_CVAR_REGISTER;
			break;
		case CG_CVAR_UPDATE_JK2:
			return CG_CVAR_UPDATE;
			break;
		case CG_CVAR_SET_JK2:
			return CG_CVAR_SET;
			break;
		case CG_ARGC_JK2:
			return CG_ARGC;
			break;
		case CG_ARGV_JK2:
			return CG_ARGV;
			break;
		case CG_ARGS_JK2:
			return CG_ARGS;
			break;
		case CG_FS_FOPENFILE_JK2:
			return CG_FS_FOPENFILE;
			break;
		case CG_FS_READ_JK2:
			return CG_FS_READ;
			break;
		case CG_FS_WRITE_JK2:
			return CG_FS_WRITE;
			break;
		case CG_FS_FCLOSEFILE_JK2:
			return CG_FS_FCLOSEFILE;
			break;
		case CG_SENDCONSOLECOMMAND_JK2:
			return CG_SENDCONSOLECOMMAND;
			break;
		case CG_ADDCOMMAND_JK2:
			return CG_ADDCOMMAND;
			break;
		case CG_SENDCLIENTCOMMAND_JK2:
			return CG_SENDCLIENTCOMMAND;
			break;
		case CG_UPDATESCREEN_JK2:
			return CG_UPDATESCREEN;
			break;
		case CG_CM_LOADMAP_JK2:
			return CG_CM_LOADMAP;
			break;
		case CG_CM_NUMINLINEMODELS_JK2:
			return CG_CM_NUMINLINEMODELS;
			break;
		case CG_CM_INLINEMODEL_JK2:
			return CG_CM_INLINEMODEL;
			break;
		case CG_CM_TEMPBOXMODEL_JK2:
			return CG_CM_TEMPBOXMODEL;
			break;
		case CG_CM_POINTCONTENTS_JK2:
			return CG_CM_POINTCONTENTS;
			break;
		case CG_CM_TRANSFORMEDPOINTCONTENTS_JK2:
			return CG_CM_TRANSFORMEDPOINTCONTENTS;
			break;
		case CG_CM_BOXTRACE_JK2:
			return CG_CM_BOXTRACE;
			break;
		case CG_CM_TRANSFORMEDBOXTRACE_JK2:
			return CG_CM_TRANSFORMEDBOXTRACE;
			break;
		case CG_CM_MARKFRAGMENTS_JK2:
			return CG_CM_MARKFRAGMENTS;
			break;
		case CG_CM_SNAPPVS_JK2:
			return CG_CM_SNAPPVS;
			break;
		case CG_S_STARTSOUND_JK2:
			return CG_S_STARTSOUND;
			break;
		case CG_S_STARTLOCALSOUND_JK2:
			return CG_S_STARTLOCALSOUND;
			break;
		case CG_S_CLEARLOOPINGSOUNDS_JK2:
			return CG_S_CLEARLOOPINGSOUNDS;
			break;
		case CG_S_ADDLOOPINGSOUND_JK2:
			return CG_S_ADDLOOPINGSOUND;
			break;
		case CG_S_UPDATEENTITYPOSITION_JK2:
			return CG_S_UPDATEENTITYPOSITION;
			break;
		case CG_S_RESPATIALIZE_JK2:
			return CG_S_RESPATIALIZE;
			break;
		case CG_S_REGISTERSOUND_JK2:
			return CG_S_REGISTERSOUND;
			break;
		case CG_S_STARTBACKGROUNDTRACK_JK2:
			return CG_S_STARTBACKGROUNDTRACK;
			break;
		case CG_R_LOADWORLDMAP_JK2:
			return CG_R_LOADWORLDMAP;
			break;
		case CG_R_REGISTERMODEL_JK2:
			return CG_R_REGISTERMODEL;
			break;
		case CG_R_REGISTERSKIN_JK2:
			return CG_R_REGISTERSKIN;
			break;
		case CG_R_REGISTERSHADER_JK2:
			return CG_R_REGISTERSHADER;
			break;
		case CG_R_REGISTERSHADERNOMIP_JK2:
			return CG_R_REGISTERSHADERNOMIP;
			break;
		case CG_R_REGISTERFONT_JK2:
			return CG_R_REGISTERFONT;
			break;
		case CG_R_FONTSTRLENPIXELS_JK2:
			return CG_R_FONTSTRLENPIXELS;
			break;
		case CG_R_FONTSTRLENCHARS_JK2:
			return CG_R_FONTSTRLENCHARS;
			break;
		case CG_R_FONTHEIGHTPIXELS_JK2:
			return CG_R_FONTHEIGHTPIXELS;
			break;
		case CG_R_FONTDRAWSTRING_JK2:
			return CG_R_FONTDRAWSTRING;
			break;
		case CG_LANGUAGE_ISASIAN_JK2:
			return CG_LANGUAGE_ISASIAN;
			break;
		case CG_LANGUAGE_USESSPACES_JK2:
			return CG_LANGUAGE_USESSPACES;
			break;
		case CG_ANYLANGUAGE_READFROMSTRING_JK2:
			return CG_ANYLANGUAGE_READFROMSTRING;
			break;
		case CG_R_CLEARSCENE_JK2:
			return CG_R_CLEARSCENE;
			break;
		case CG_R_ADDREFENTITYTOSCENE_JK2:
			return CG_R_ADDREFENTITYTOSCENE;
			break;
		case CG_R_GETLIGHTING_JK2:
			return CG_R_GETLIGHTING;
			break;
		case CG_R_ADDPOLYTOSCENE_JK2:
			return CG_R_ADDPOLYTOSCENE;
			break;
		case CG_R_ADDLIGHTTOSCENE_JK2:
			return CG_R_ADDLIGHTTOSCENE;
			break;
		case CG_R_RENDERSCENE_JK2:
			return CG_R_RENDERSCENE;
			break;
		case CG_R_SETCOLOR_JK2:
			return CG_R_SETCOLOR;
			break;
		case CG_R_DRAWSTRETCHPIC_JK2:
			return CG_R_DRAWSTRETCHPIC;
			break;
		case CG_R_DRAWSCREENSHOT_JK2:
			return CG_R_DRAWSCREENSHOT;
			break;
		case CG_R_MODELBOUNDS_JK2:
			return CG_R_MODELBOUNDS;
			break;
		case CG_R_LERPTAG_JK2:
			return CG_R_LERPTAG;
			break;
		case CG_R_DRAWROTATEPIC_JK2:
			return CG_R_DRAWROTATEPIC;
			break;
		case CG_R_DRAWROTATEPIC2_JK2:
			return CG_R_DRAWROTATEPIC2;
			break;
		case CG_R_LA_GOGGLES_JK2:
			return CG_R_LA_GOGGLES;
			break;
		case CG_R_SCISSOR_JK2:
			return CG_R_SCISSOR;
			break;
		case CG_GETGLCONFIG_JK2:
			return CG_GETGLCONFIG;
			break;
		case CG_GETGAMESTATE_JK2:
			return CG_GETGAMESTATE;
			break;
		case CG_GETCURRENTSNAPSHOTNUMBER_JK2:
			return CG_GETCURRENTSNAPSHOTNUMBER;
			break;
		case CG_GETSNAPSHOT_JK2:
			return CG_GETSNAPSHOT;
			break;
		case CG_GETSERVERCOMMAND_JK2:
			return CG_GETSERVERCOMMAND;
			break;
		case CG_GETCURRENTCMDNUMBER_JK2:
			return CG_GETCURRENTCMDNUMBER;
			break;
		case CG_GETUSERCMD_JK2:
			return CG_GETUSERCMD;
			break;
		case CG_SETUSERCMDVALUE_JK2:
			return CG_SETUSERCMDVALUE;
			break;
		case CG_SETUSERCMDANGLES_JK2:
			return CG_SETUSERCMDANGLES;
			break;
		case CG_S_UPDATEAMBIENTSET_JK2:
			return CG_S_UPDATEAMBIENTSET;
			break;
		case CG_S_ADDLOCALSET_JK2:
			return CG_S_ADDLOCALSET;
			break;
		case CG_AS_PARSESETS_JK2:
			return CG_AS_PARSESETS;
			break;
		case CG_AS_ADDENTRY_JK2:
			return CG_AS_ADDENTRY;
			break;
		case CG_AS_GETBMODELSOUND_JK2:
			return CG_AS_GETBMODELSOUND;
			break;
		case CG_S_GETSAMPLELENGTH_JK2:
			return CG_S_GETSAMPLELENGTH;
			break;
		case COM_SETORGANGLES_JK2:
			return COM_SETORGANGLES;
			break;
/*
Ghoul2 Insert Start
*/
		case CG_G2_LISTBONES_JK2:
			return CG_G2_LISTBONES;
			break;
		case CG_G2_LISTSURFACES_JK2:
			return CG_G2_LISTSURFACES;
			break;
		case CG_G2_HAVEWEGHOULMODELS_JK2:
			return CG_G2_HAVEWEGHOULMODELS;
			break;
		case CG_G2_SETMODELS_JK2:
			return CG_G2_SETMODELS;
			break;
/*
Ghoul2 Insert End
*/

		case CG_R_GET_LIGHT_STYLE_JK2:
			return CG_R_GET_LIGHT_STYLE;
			break;
		case CG_R_SET_LIGHT_STYLE_JK2:
			return CG_R_SET_LIGHT_STYLE;
			break;
		case CG_R_GET_BMODEL_VERTS_JK2:
			return CG_R_GET_BMODEL_VERTS;
			break;
		case CG_R_WORLD_EFFECT_COMMAND_JK2:
			return CG_R_WORLD_EFFECT_COMMAND;
			break;

		case CG_CIN_PLAYCINEMATIC_JK2:
			return CG_CIN_PLAYCINEMATIC;
			break;
		case CG_CIN_STOPCINEMATIC_JK2:
			return CG_CIN_STOPCINEMATIC;
			break;
		case CG_CIN_RUNCINEMATIC_JK2:
			return CG_CIN_RUNCINEMATIC;
			break;
		case CG_CIN_DRAWCINEMATIC_JK2:
			return CG_CIN_DRAWCINEMATIC;
			break;
		case CG_CIN_SETEXTENTS_JK2:
			return CG_CIN_SETEXTENTS;
			break;
		case CG_Z_MALLOC_JK2:
			return CG_Z_MALLOC;
			break;
		case CG_Z_FREE_JK2:
			return CG_Z_FREE;
			break;
		case CG_UI_MENU_RESET_JK2:
			return CG_UI_MENU_RESET;
			break;
		case CG_UI_MENU_NEW_JK2:
			return CG_UI_MENU_NEW;
			break;
		case CG_UI_PARSE_INT_JK2:
			return CG_UI_PARSE_INT;
			break;
		case CG_UI_PARSE_STRING_JK2:
			return CG_UI_PARSE_STRING;
			break;
		case CG_UI_PARSE_FLOAT_JK2:
			return CG_UI_PARSE_FLOAT;
			break;
		case CG_UI_STARTPARSESESSION_JK2:
			return CG_UI_STARTPARSESESSION;
			break;
		case CG_UI_ENDPARSESESSION_JK2:
			return CG_UI_ENDPARSESESSION;
			break;
		case CG_UI_PARSEEXT_JK2:
			return CG_UI_PARSEEXT;
			break;
		case CG_UI_MENUPAINT_ALL_JK2:
			return CG_UI_MENUPAINT_ALL;
			break;
		case CG_UI_STRING_INIT_JK2:
			return CG_UI_STRING_INIT;
			break;
		case CG_UI_GETMENUINFO_JK2:
			return CG_UI_GETMENUINFO;
			break;
		case CG_SP_REGISTER_JK2:
			return CG_SP_REGISTER;
			break;
		case CG_SP_GETSTRINGTEXT_JK2:
			return CG_SP_GETSTRINGTEXT;
			break;
		case CG_SP_GETSTRINGTEXTSTRING_JK2:
			// Both of these do the same thing --eez
			return CG_SP_GETSTRINGTEXTSTRING;
			break;
		case CG_UI_GETITEMTEXT_JK2:
			return CG_UI_GETITEMTEXT;
			break;
		case CG_ANYLANGUAGE_READFROMSTRING2_JK2:
			return CG_ANYLANGUAGE_READFROMSTRING2;
			break;
		case CG_OPENJK_MENU_PAINT_JK2:
			return CG_OPENJK_MENU_PAINT;
			break;
		case CG_OPENJK_GETMENU_BYNAME_JK2:
			return CG_OPENJK_GETMENU_BYNAME;
			break;
	}
	return (cgameImport_t)-1;
}

#endif
/*
====================
CL_CgameSystemCalls

The cgame module is making a system call
====================
*/
void CM_SnapPVS(vec3_t origin,byte *buffer);
extern void		Menu_Paint(menuDef_t *menu, qboolean forcePaint);
extern menuDef_t *Menus_FindByName(const char *p);
intptr_t CL_CgameSystemCalls( intptr_t *args ) {
#ifdef JK2_MODE
	args[0] = (intptr_t)CL_ConvertJK2SysCall((cgameJK2Import_t)args[0]);
#endif
	switch( args[0] ) {
	case CG_PRINT:
		Com_Printf( "%s", VMA(1) );
		return 0;
	case CG_ERROR:
		Com_Error( ERR_DROP, S_COLOR_RED"%s", VMA(1) );
		return 0;
	case CG_MILLISECONDS:
		return Sys_Milliseconds();
	case CG_CVAR_REGISTER:
		Cvar_Register( (vmCvar_t *) VMA(1), (const char *) VMA(2), (const char *) VMA(3), args[4] );
		return 0;
	case CG_CVAR_UPDATE:
		Cvar_Update( (vmCvar_t *) VMA(1) );
		return 0;
	case CG_CVAR_SET:
		Cvar_Set( (const char *) VMA(1), (const char *) VMA(2) );
		return 0;
	case CG_ARGC:
		return Cmd_Argc();
	case CG_ARGV:
		Cmd_ArgvBuffer( args[1], (char *) VMA(2), args[3] );
		return 0;
	case CG_ARGS:
		Cmd_ArgsBuffer( (char *) VMA(1), args[2] );
		return 0;
	case CG_FS_FOPENFILE:
		return FS_FOpenFileByMode( (const char *) VMA(1), (int *) VMA(2), (fsMode_t) args[3] );
	case CG_FS_READ:
		FS_Read( VMA(1), args[2], args[3] );
		return 0;
	case CG_FS_WRITE:
		FS_Write( VMA(1), args[2], args[3] );
		return 0;
	case CG_FS_FCLOSEFILE:
		FS_FCloseFile( args[1] );
		return 0;
	case CG_SENDCONSOLECOMMAND:
		Cbuf_AddText( (const char *) VMA(1) );
		return 0;
	case CG_ADDCOMMAND:
		CL_AddCgameCommand( (const char *) VMA(1) );
		return 0;
	case CG_SENDCLIENTCOMMAND:
		CL_AddReliableCommand( (const char *) VMA(1) );
		return 0;
	case CG_UPDATESCREEN:
		// this is used during lengthy level loading, so pump message loop
		Com_EventLoop();	// FIXME: if a server restarts here, BAD THINGS HAPPEN!
		SCR_UpdateScreen();
		return 0;
	case CG_RMG_INIT:
		return 0;
	case CG_CM_REGISTER_TERRAIN:
		return 0;

	case CG_RE_INIT_RENDERER_TERRAIN:
		return 0;

	case CG_CM_LOADMAP:
		CL_CM_LoadMap( (const char *) VMA(1), (qboolean)(args[2] != 0) );
		return 0;
	case CG_CM_NUMINLINEMODELS:
		return CM_NumInlineModels();
	case CG_CM_INLINEMODEL:
		return CM_InlineModel( args[1] );
	case CG_CM_TEMPBOXMODEL:
		return CM_TempBoxModel( (const float *) VMA(1), (const float *) VMA(2) );//, (int) VMA(3) );
	case CG_CM_POINTCONTENTS:
		return CM_PointContents( (float *)VMA(1), args[2] );
	case CG_CM_TRANSFORMEDPOINTCONTENTS:
		return CM_TransformedPointContents( (const float *) VMA(1), args[2], (const float *) VMA(3), (const float *) VMA(4) );
	case CG_CM_BOXTRACE:
		CM_BoxTrace( (trace_t *) VMA(1), (const float *) VMA(2), (const float *) VMA(3), (const float *) VMA(4), (const float *) VMA(5), args[6], args[7] );
		return 0;
	case CG_CM_TRANSFORMEDBOXTRACE:
		CM_TransformedBoxTrace( (trace_t *) VMA(1), (const float *) VMA(2), (const float *) VMA(3), (const float *) VMA(4), (const float *) VMA(5), args[6], args[7], (const float *) VMA(8), (const float *) VMA(9) );
		return 0;
	case CG_CM_MARKFRAGMENTS:
		return re.MarkFragments( args[1], (float(*)[3]) VMA(2), (const float *) VMA(3), args[4], (float *) VMA(5), args[6], (markFragment_t *) VMA(7) );
	case CG_CM_SNAPPVS:
		CM_SnapPVS((float(*))VMA(1),(byte *) VMA(2));
		return 0;
	case CG_S_STOPSOUNDS:
		S_StopSounds( );
		return 0;

	case CG_S_STARTSOUND:
		// stops an ERR_DROP internally if called illegally from game side, but note that it also gets here
		//	legally during level start where normally the internal s_soundStarted check would return. So ok to hit this.
		if (!cls.cgameStarted) {
			return 0;
		}
		S_StartSound( (float *) VMA(1), args[2], (soundChannel_t)args[3], args[4] );
		return 0;
	case CG_S_UPDATEAMBIENTSET:
		// stops an ERR_DROP internally if called illegally from game side, but note that it also gets here
		//	legally during level start where normally the internal s_soundStarted check would return. So ok to hit this.
		if (!cls.cgameStarted) {
			return 0;
		}
		S_UpdateAmbientSet( (const char *) VMA(1), (float *) VMA(2) );
		return 0;
	case CG_S_ADDLOCALSET:
		return S_AddLocalSet( (const char *) VMA(1), (float *) VMA(2), (float *) VMA(3), args[4], args[5] );
	case CG_AS_PARSESETS:
		AS_ParseSets();
		return 0;
	case CG_AS_ADDENTRY:
		AS_AddPrecacheEntry( (const char *) VMA(1) );
		return 0;
	case CG_AS_GETBMODELSOUND:
		return AS_GetBModelSound( (const char *) VMA(1), args[2] );
	case CG_S_STARTLOCALSOUND:
		// stops an ERR_DROP internally if called illegally from game side, but note that it also gets here
		//	legally during level start where normally the internal s_soundStarted check would return. So ok to hit this.
		if (!cls.cgameStarted) {
			return 0;
		}
		S_StartLocalSound( args[1], args[2] );
		return 0;
	case CG_S_CLEARLOOPINGSOUNDS:
		S_ClearLoopingSounds();
		return 0;
	case CG_S_ADDLOOPINGSOUND:
		// stops an ERR_DROP internally if called illegally from game side, but note that it also gets here
		//	legally during level start where normally the internal s_soundStarted check would return. So ok to hit this.
		if (!cls.cgameStarted) {
			return 0;
		}
		S_AddLoopingSound( args[1], (const float *) VMA(2), (const float *) VMA(3), args[4], (soundChannel_t)args[5] );
		return 0;
	case CG_S_UPDATEENTITYPOSITION:
		S_UpdateEntityPosition( args[1], (const float *) VMA(2) );
		return 0;
	case CG_S_RESPATIALIZE:
		S_Respatialize( args[1], (const float *) VMA(2), (float(*)[3]) VMA(3), (qboolean)(args[4] != 0) );
		return 0;
	case CG_S_REGISTERSOUND:
		return S_RegisterSound( (const char *) VMA(1) );
	case CG_S_STARTBACKGROUNDTRACK:
		S_StartBackgroundTrack( (const char *) VMA(1), (const char *) VMA(2), (qboolean)(args[3] != 0) );
		return 0;
	case CG_S_GETSAMPLELENGTH:
		return S_GetSampleLengthInMilliSeconds(  args[1]);
	case CG_R_LOADWORLDMAP:
		re.LoadWorld( (const char *) VMA(1) );
		return 0;
	case CG_R_REGISTERMODEL:
		return re.RegisterModel( (const char *) VMA(1) );
	case CG_R_REGISTERSKIN:
		return re.RegisterSkin( (const char *) VMA(1) );
	case CG_R_REGISTERSHADER:
		return re.RegisterShader( (const char *) VMA(1) );
	case CG_R_REGISTERSHADERNOMIP:
		return re.RegisterShaderNoMip( (const char *) VMA(1) );
	case CG_R_REGISTERFONT:
		return re.RegisterFont( (const char *) VMA(1) );
	case CG_R_FONTSTRLENPIXELS:
		return re.Font_StrLenPixels( (const char *) VMA(1), args[2], VMF(3) );
	case CG_R_FONTSTRLENCHARS:
		return re.Font_StrLenChars( (const char *) VMA(1) );
	case CG_R_FONTHEIGHTPIXELS:
		return re.Font_HeightPixels( args[1], VMF(2) );
	case CG_R_FONTDRAWSTRING:
		re.Font_DrawString(args[1],args[2], (const char *) VMA(3), (float*)args[4], args[5], args[6], VMF(7));
		return 0;
	case CG_LANGUAGE_ISASIAN:
		return re.Language_IsAsian();
	case CG_LANGUAGE_USESSPACES:
		return re.Language_UsesSpaces();
	case CG_ANYLANGUAGE_READFROMSTRING:
		return re.AnyLanguage_ReadCharFromString( (char *) VMA(1), (int *) VMA(2), (qboolean *) VMA(3) );
	case CG_ANYLANGUAGE_READFROMSTRING2:
		return re.AnyLanguage_ReadCharFromString2( (char **) VMA(1), (qboolean *) VMA(3) );
	case CG_R_SETREFRACTIONPROP:
		*(re.tr_distortionAlpha()) = VMF(1);
		*(re.tr_distortionStretch()) = VMF(2);
		*(re.tr_distortionPrePost()) = (qboolean)args[3];
		*(re.tr_distortionNegate()) = (qboolean)args[4];
		return 0;
	case CG_R_CLEARSCENE:
		re.ClearScene();
		return 0;
	case CG_R_ADDREFENTITYTOSCENE:
		re.AddRefEntityToScene( (const refEntity_t *) VMA(1) );
		return 0;

	case CG_R_INPVS:
		return re.R_inPVS((float *) VMA(1), (float *) VMA(2));

	case CG_R_GETLIGHTING:
		return re.GetLighting( (const float * ) VMA(1), (float *) VMA(2), (float *) VMA(3), (float *) VMA(4) );
	case CG_R_ADDPOLYTOSCENE:
		re.AddPolyToScene( args[1], args[2], (const polyVert_t *) VMA(3) );
		return 0;
	case CG_R_ADDLIGHTTOSCENE:
		re.AddLightToScene( (const float *) VMA(1), VMF(2), VMF(3), VMF(4), VMF(5) );
		return 0;
	case CG_R_RENDERSCENE:
		re.RenderScene( (const refdef_t *) VMA(1) );
		return 0;
	case CG_R_SETCOLOR:
		re.SetColor( (const float *) VMA(1) );
		return 0;
	case CG_R_DRAWSTRETCHPIC:
		re.DrawStretchPic( VMF(1), VMF(2), VMF(3), VMF(4), VMF(5), VMF(6), VMF(7), VMF(8), args[9] );
		return 0;
		// The below was commented out for whatever reason... /me shrugs --eez
	case CG_R_DRAWSCREENSHOT:
		re.DrawStretchRaw( VMF(1), VMF(2), VMF(3), VMF(4), SG_SCR_WIDTH, SG_SCR_HEIGHT, SCR_GetScreenshot(0), 0, qtrue);
		return 0;
	case CG_R_MODELBOUNDS:
		re.ModelBounds( args[1], (float *) VMA(2), (float *) VMA(3) );
		return 0;
	case CG_R_LERPTAG:
		re.LerpTag( (orientation_t *) VMA(1), args[2], args[3], args[4], VMF(5), (const char *) VMA(6) );
		return 0;
	case CG_R_DRAWROTATEPIC:
		re.DrawRotatePic( VMF(1), VMF(2), VMF(3), VMF(4), VMF(5), VMF(6), VMF(7), VMF(8), VMF(9), args[10] );
		return 0;
	case CG_R_DRAWROTATEPIC2:
		re.DrawRotatePic2( VMF(1), VMF(2), VMF(3), VMF(4), VMF(5), VMF(6), VMF(7), VMF(8), VMF(9), args[10] );
		return 0;
	case CG_R_SETRANGEFOG:
		re.SetRangedFog( VMF( 1 ) );
		return 0;
	case CG_R_LA_GOGGLES:
		re.LAGoggles();
		return 0;
	case CG_R_SCISSOR:
		re.Scissor( VMF(1), VMF(2), VMF(3), VMF(4));
		return 0;
	case CG_GETGLCONFIG:
		CL_GetGlconfig( (glconfig_t *) VMA(1) );
		return 0;
	case CG_GETGAMESTATE:
		CL_GetGameState( (gameState_t *) VMA(1) );
		return 0;
	case CG_GETCURRENTSNAPSHOTNUMBER:
		CL_GetCurrentSnapshotNumber( (int *) VMA(1), (int *) VMA(2) );
		return 0;
	case CG_GETSNAPSHOT:
		return CL_GetSnapshot( args[1], (snapshot_t *) VMA(2) );

	case CG_GETDEFAULTSTATE:
		return CL_GetDefaultState(args[1], (entityState_t *)VMA(2));

	case CG_GETSERVERCOMMAND:
		return CL_GetServerCommand( args[1] );
	case CG_GETCURRENTCMDNUMBER:
		return CL_GetCurrentCmdNumber();
	case CG_GETUSERCMD:
		return CL_GetUserCmd( args[1], (usercmd_s *) VMA(2) );
	case CG_SETUSERCMDVALUE:
		CL_SetUserCmdValue( args[1], VMF(2), VMF(3), VMF(4) );
		return 0;
	case CG_SETUSERCMDANGLES:
		CL_SetUserCmdAngles( VMF(1), VMF(2), VMF(3) );
		return 0;
	case COM_SETORGANGLES:
		Com_SetOrgAngles((float *)VMA(1),(float *)VMA(2));
		return 0;
/*
Ghoul2 Insert Start
*/

	case CG_G2_LISTSURFACES:
		re.G2API_ListSurfaces( (CGhoul2Info *) VMA(1) );
		return 0;

	case CG_G2_LISTBONES:
		re.G2API_ListBones( (CGhoul2Info *) VMA(1), args[2]);
		return 0;

	case CG_G2_HAVEWEGHOULMODELS:
		return re.G2API_HaveWeGhoul2Models( *((CGhoul2Info_v *)VMA(1)) );

	case CG_G2_SETMODELS:
		re.G2API_SetGhoul2ModelIndexes( *((CGhoul2Info_v *)VMA(1)),(qhandle_t *)VMA(2),(qhandle_t *)VMA(3));
		return 0;

/*
Ghoul2 Insert End
*/

	case CG_R_GET_LIGHT_STYLE:
		re.GetLightStyle(args[1], (byte*) VMA(2) );
		return 0;
	case CG_R_SET_LIGHT_STYLE:
		re.SetLightStyle(args[1], args[2] );
		return 0;

	case CG_R_GET_BMODEL_VERTS:
		re.GetBModelVerts( args[1], (float (*)[3])VMA(2), (float *)VMA(3) );
		return 0;

	case CG_R_WORLD_EFFECT_COMMAND:
		re.WorldEffectCommand( (const char *) VMA(1) );
		return 0;

	case CG_CIN_PLAYCINEMATIC:
	  return CIN_PlayCinematic( (const char *) VMA(1), args[2], args[3], args[4], args[5], args[6], (const char *) VMA(7));

	case CG_CIN_STOPCINEMATIC:
	  return CIN_StopCinematic(args[1]);

	case CG_CIN_RUNCINEMATIC:
	  return CIN_RunCinematic(args[1]);

	case CG_CIN_DRAWCINEMATIC:
	  CIN_DrawCinematic(args[1]);
	  return 0;

	case CG_CIN_SETEXTENTS:
	  CIN_SetExtents(args[1], args[2], args[3], args[4], args[5]);
	  return 0;

	case CG_Z_MALLOC:
		return (intptr_t)Z_Malloc(args[1], (memtag_t) args[2], qfalse);

	case CG_Z_FREE:
		Z_Free((void *) VMA(1));
		return 0;

	case CG_UI_SETACTIVE_MENU:
		UI_SetActiveMenu((const char *) VMA(1),NULL);
		return 0;

	case CG_UI_MENU_OPENBYNAME:
		Menus_OpenByName((const char *) VMA(1));
		return 0;

	case CG_UI_MENU_RESET:
		Menu_Reset();
		return 0;

	case CG_UI_MENU_NEW:
		Menu_New((char *) VMA(1));
		return 0;

	case CG_UI_PARSE_INT:
		PC_ParseInt((int *) VMA(1));
		return 0;

	case CG_UI_PARSE_STRING:
		PC_ParseString((const char **) VMA(1));
		return 0;

	case CG_UI_PARSE_FLOAT:
		PC_ParseFloat((float *) VMA(1));
		return 0;

	case CG_UI_STARTPARSESESSION:
		return(PC_StartParseSession((char *) VMA(1),(char **) VMA(2)));

	case CG_UI_ENDPARSESESSION:
		PC_EndParseSession((char *) VMA(1));
		return 0;

	case CG_UI_PARSEEXT:
		char **holdPtr;

		holdPtr = (char **) VMA(1);

		if(!holdPtr)
		{
			Com_Error(ERR_FATAL, "CG_UI_PARSEEXT: NULL holdPtr");
		}

		*holdPtr = PC_ParseExt();
		return 0;

	case CG_UI_MENUCLOSE_ALL:
		Menus_CloseAll();
		return 0;

	case CG_UI_MENUPAINT_ALL:
		Menu_PaintAll();
		return 0;

	case CG_OPENJK_MENU_PAINT:
		Menu_Paint( (menuDef_t *)VMA(1), (qboolean)(args[2] != 0) );
		return 0;

	case CG_OPENJK_GETMENU_BYNAME:
		return (intptr_t)Menus_FindByName( (const char *)VMA(1) );

	case CG_UI_STRING_INIT:
		String_Init();
		return 0;

	case CG_UI_GETMENUINFO:
		menuDef_t *menu;
		int		*xPos,*yPos,*w,*h,result;
#ifndef JK2_MODE
		menu = Menus_FindByName((char *) VMA(1));	// Get menu
		if (menu)
		{
			xPos = (int *) VMA(2);
			*xPos = (int) menu->window.rect.x;
			yPos = (int *) VMA(3);
			*yPos = (int) menu->window.rect.y;
			w = (int *) VMA(4);
			*w = (int) menu->window.rect.w;
			h = (int *) VMA(5);
			*h = (int) menu->window.rect.h;
			result = qtrue;
		}
		else
		{
			result = qfalse;
		}

		return result;
#else
		menu = Menus_FindByName((char *) VMA(1));	// Get menu
		if (menu)
		{
			xPos = (int *) VMA(2);
			*xPos = (int) menu->window.rect.x;
			yPos = (int *) VMA(3);
			*yPos = (int) menu->window.rect.y;
			result = qtrue;
		}
		else
		{
			result = qfalse;
		}

		return result;
#endif
		break;

	case CG_UI_GETITEMTEXT:
		itemDef_t *item;
		menu = Menus_FindByName((char *) VMA(1));	// Get menu

		if (menu)
		{
			item = (itemDef_s *) Menu_FindItemByName((menuDef_t *) menu, (char *) VMA(2));
			if (item)
			{
				Q_strncpyz( (char *) VMA(3), item->text, 256 );
				result = qtrue;
			}
			else
			{
				result = qfalse;
			}
		}
		else
		{
			result = qfalse;
		}

		return result;

	case CG_UI_GETITEMINFO:
		menu = Menus_FindByName((char *) VMA(1));	// Get menu

		if (menu)
		{
			qhandle_t *background;

			item = (itemDef_s *) Menu_FindItemByName((menuDef_t *) menu, (char *) VMA(2));
			if (item)
			{
				xPos = (int *) VMA(3);
				*xPos = (int) item->window.rect.x;
				yPos = (int *) VMA(4);
				*yPos = (int) item->window.rect.y;
				w = (int *) VMA(5);
				*w = (int) item->window.rect.w;
				h = (int *) VMA(6);
				*h = (int) item->window.rect.h;

				vec4_t *color;

				color = (vec4_t *) VMA(7);
				if (!color)
				{
					return qfalse;
				}

				(*color)[0] = (float) item->window.foreColor[0];
				(*color)[1] = (float) item->window.foreColor[1];
				(*color)[2] = (float) item->window.foreColor[2];
				(*color)[3] = (float) item->window.foreColor[3];
				background = (qhandle_t *) VMA(8);
				if (!background)
				{
					return qfalse;
				}
				*background = item->window.background;

				result = qtrue;
			}
			else
			{
				result = qfalse;
			}
		}
		else
		{
			result = qfalse;
		}

		return result;

#ifdef JK2_MODE
	case CG_SP_GETSTRINGTEXTSTRING:
	case CG_SP_GETSTRINGTEXT:
		const char* text;

		assert(VMA(1));
//		assert(VMA(2));	// can now pass in NULL to just query the size

		if (args[0] == CG_SP_GETSTRINGTEXT)
		{
			text = JK2SP_GetStringText( args[1] );
		}
		else
		{
			text = JK2SP_GetStringTextString( (const char *) VMA(1) );
		}

		if (VMA(2))	// only if dest buffer supplied...
		{
			if ( text[0] )
			{
				Q_strncpyz( (char *) VMA(2), text, args[3] );
			}
			else
			{
				Q_strncpyz( (char *) VMA(2), "??", args[3] );
			}
		}
		return strlen(text);

	case CG_SP_REGISTER:
		return JK2SP_Register((const char *)VMA(1), args[2] ? (SP_REGISTER_MENU | SP_REGISTER_REQUIRED) : SP_REGISTER_CLIENT);
#else
	case CG_SP_GETSTRINGTEXTSTRING:
		const char* text;

		assert(VMA(1));
		text = SE_GetString( (const char *) VMA(1) );

		if (VMA(2))	// only if dest buffer supplied...
		{
			if ( text[0] )
			{
				Q_strncpyz( (char *) VMA(2), text, args[3] );
			}
			else
			{
				Com_sprintf( (char *) VMA(2), args[3], "??%s", VMA(1) );
			}
		}
		return strlen(text);
#endif

	default:
		Com_Error( ERR_DROP, "Bad cgame system trap: %ld", (long int) args[0] );
	}
	return 0;
}


/*
====================
CL_InitCGame

Should only be called by CL_StartHunkUsers
====================
*/
extern qboolean Sys_LowPhysicalMemory();
void CL_InitCGame( void ) {
	const char			*info;
	const char			*mapname;
	//int		t1, t2;

	//t1 = Sys_Milliseconds();

	// put away the console
	Con_Close();

	// find the current mapname
	info = cl.gameState.stringData + cl.gameState.stringOffsets[ CS_SERVERINFO ];
	mapname = Info_ValueForKey( info, "mapname" );
	Com_sprintf( cl.mapname, sizeof( cl.mapname ), "maps/%s.bsp", mapname );

	cls.state = CA_LOADING;

	// init for this gamestate
	VM_Call( CG_INIT, clc.serverCommandSequence );

	// reset any CVAR_CHEAT cvars registered by cgame
	if ( !cl_connectedToCheatServer )
		Cvar_SetCheatState();

	// we will send a usercmd this frame, which
	// will cause the server to send us the first snapshot
	cls.state = CA_PRIMED;

	//t2 = Sys_Milliseconds();

	//Com_Printf( "CL_InitCGame: %5.2f seconds\n", (t2-t1)/1000.0 );
	// have the renderer touch all its images, so they are present
	// on the card even if the driver does deferred loading
	re.EndRegistration();

	// make sure everything is paged in
//	if (!Sys_LowPhysicalMemory())
	{
		Com_TouchMemory();
	}

	// clear anything that got printed
	Con_ClearNotify ();
}


/*
====================
CL_GameCommand

See if the current console command is claimed by the cgame
====================
*/
qboolean CL_GameCommand( void ) {
	if ( cls.state != CA_ACTIVE ) {
		return qfalse;
	}

	return (qboolean)(VM_Call( CG_CONSOLE_COMMAND ) != 0);
}



/*
=====================
CL_CGameRendering
=====================
*/
void CL_CGameRendering( stereoFrame_t stereo ) {
#if 0
	if ( cls.state == CA_ACTIVE ) {
		static int counter;

		if ( ++counter == 40 ) {
			VM_Debug( 2 );
		}
	}
#endif
	int timei=cl.serverTime;
	if (timei>60)
	{
		timei-=0;
	}
	re.G2API_SetTime(cl.serverTime,G2T_CG_TIME);
	VM_Call( CG_DRAW_ACTIVE_FRAME,timei, stereo, qfalse );
//	VM_Debug( 0 );
}


/*
=================
CL_AdjustTimeDelta

Adjust the clients view of server time.

We attempt to have cl.serverTime exactly equal the server's view
of time plus the timeNudge, but with variable latencies over
the internet it will often need to drift a bit to match conditions.

Our ideal time would be to have the adjusted time aproach, but not pass,
the very latest snapshot.

Adjustments are only made when a new snapshot arrives, which keeps the
adjustment process framerate independent and prevents massive overadjustment
during times of significant packet loss.
=================
*/

#define	RESET_TIME	300

void CL_AdjustTimeDelta( void ) {
/*
	cl.newSnapshots = qfalse;
	// if the current time is WAY off, just correct to the current value
	if ( cls.realtime + cl.serverTimeDelta < cl.frame.serverTime - RESET_TIME
		|| cls.realtime + cl.serverTimeDelta > cl.frame.serverTime + RESET_TIME  ) {
		cl.serverTimeDelta = cl.frame.serverTime - cls.realtime;
		cl.oldServerTime = cl.frame.serverTime;
		if ( cl_showTimeDelta->integer ) {
			Com_Printf( "<RESET> " );
		}
	}

	// if any of the frames between this and the previous snapshot
	// had to be extrapolated, nudge our sense of time back a little
	if ( cl.extrapolatedSnapshot ) {
		cl.extrapolatedSnapshot = qfalse;
		cl.serverTimeDelta -= 2;
	} else {
		// otherwise, move our sense of time forward to minimize total latency
		cl.serverTimeDelta++;
	}

	if ( cl_showTimeDelta->integer ) {
		Com_Printf( "%i ", cl.serverTimeDelta );
	}
*/
	int		newDelta;
	int		deltaDelta;

	cl.newSnapshots = qfalse;

	newDelta = cl.frame.serverTime - cls.realtime;
	deltaDelta = abs( newDelta - cl.serverTimeDelta );

	if ( deltaDelta > RESET_TIME ) {
		cl.serverTimeDelta = newDelta;
		cl.oldServerTime = cl.frame.serverTime;	// FIXME: is this a problem for cgame?
		cl.serverTime = cl.frame.serverTime;
		if ( cl_showTimeDelta->integer ) {
			Com_Printf( "<RESET> " );
		}
	} else if ( deltaDelta > 100 ) {
		// fast adjust, cut the difference in half
		if ( cl_showTimeDelta->integer ) {
			Com_Printf( "<FAST> " );
		}
		cl.serverTimeDelta = ( cl.serverTimeDelta + newDelta ) >> 1;
	} else {
		// slow drift adjust, only move 1 or 2 msec

		// if any of the frames between this and the previous snapshot
		// had to be extrapolated, nudge our sense of time back a little
		// the granularity of +1 / -2 is too high for timescale modified frametimes
		if ( com_timescale->value == 0 || com_timescale->value == 1 ) {
			if ( cl.extrapolatedSnapshot ) {
				cl.extrapolatedSnapshot = qfalse;
				cl.serverTimeDelta -= 2;
			} else {
				// otherwise, move our sense of time forward to minimize total latency
				cl.serverTimeDelta++;
			}
		}
	}

	if ( cl_showTimeDelta->integer ) {
		Com_Printf( "%i ", cl.serverTimeDelta );
	}
}


/*
==================
CL_FirstSnapshot
==================
*/
void CL_FirstSnapshot( void ) {

	re.RegisterMedia_LevelLoadEnd();

	cls.state = CA_ACTIVE;

	// set the timedelta so we are exactly on this first frame
	cl.serverTimeDelta = cl.frame.serverTime - cls.realtime;
	cl.oldServerTime = cl.frame.serverTime;

	// if this is the first frame of active play,
	// execute the contents of activeAction now
	// this is to allow scripting a timedemo to start right
	// after loading
	if ( cl_activeAction->string[0] ) {
		Cbuf_AddText( cl_activeAction->string );
		Cvar_Set( "activeAction", "" );
	}
}

/*
==================
CL_SetCGameTime
==================
*/
void CL_SetCGameTime( void ) {

	// getting a valid frame message ends the connection process
	if ( cls.state != CA_ACTIVE ) {
		if ( cls.state != CA_PRIMED ) {
			return;
		}
		if ( cl.newSnapshots ) {
			cl.newSnapshots = qfalse;
			CL_FirstSnapshot();
		}

		if ( cls.state != CA_ACTIVE ) {
			return;
		}
	}

	// if we have gotten to this point, cl.frame is guaranteed to be valid
	if ( !cl.frame.valid ) {
		Com_Error( ERR_DROP, "CL_SetCGameTime: !cl.snap.valid" );
	}

	// allow pause in single player
	if ( sv_paused->integer && CL_CheckPaused() && com_sv_running->integer ) {
		// paused
		return;
	}

	if ( cl.frame.serverTime < cl.oldFrameServerTime ) {
		Com_Error( ERR_DROP, "cl.frame.serverTime < cl.oldFrameServerTime" );
	}
	cl.oldFrameServerTime = cl.frame.serverTime;


	// get our current view of time

	// cl_timeNudge is a user adjustable cvar that allows more
	// or less latency to be added in the interest of better
	// smoothness or better responsiveness.
	cl.serverTime = cls.realtime + cl.serverTimeDelta - cl_timeNudge->integer;

	// guarantee that time will never flow backwards, even if
	// serverTimeDelta made an adjustment or cl_timeNudge was changed
	if ( cl.serverTime < cl.oldServerTime ) {
		cl.serverTime = cl.oldServerTime;
	}
	cl.oldServerTime = cl.serverTime;

	// note if we are almost past the latest frame (without timeNudge),
	// so we will try and adjust back a bit when the next snapshot arrives
	if ( cls.realtime + cl.serverTimeDelta >= cl.frame.serverTime - 5 ) {
		cl.extrapolatedSnapshot = qtrue;
	}

	// if we have gotten new snapshots, drift serverTimeDelta
	// don't do this every frame, or a period of packet loss would
	// make a huge adjustment
	if ( cl.newSnapshots ) {
		CL_AdjustTimeDelta();
	}
}

