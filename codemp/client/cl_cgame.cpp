// cl_cgame.c  -- client system interaction with client game
//Anything above this #include will be ignored by the compiler
#include "qcommon/exe_headers.h"

#include "RMG/RM_Headers.h"
#include "client.h"
#include "botlib/botlib.h"
#include "RMG/RM_Headers.h"
#include "FXExport.h"
#include "FxUtil.h"
#include "qcommon/RoffSystem.h"

#ifdef _DONETPROFILE_
#include "qcommon/INetProfile.h"
#endif

#ifdef VV_LIGHTING
#include "renderer/tr_lightmanager.h"
#endif

/*
Ghoul2 Insert Start
*/

#include "qcommon/stringed_ingame.h"
#include "ghoul2/G2_gore.h"

extern CMiniHeap *G2VertSpaceClient;

#include "snd_ambient.h"
#include "qcommon/timing.h"

//extern int contentOverride;

/*
Ghoul2 Insert End
*/

extern	botlib_export_t	*botlib_export;

extern qboolean loadCamera(const char *name);
extern void startCamera(int time);
extern qboolean getCameraInfo(int time, vec3_t *origin, vec3_t *angles);

void FX_FeedTrail(effectTrailArgStruct_t *a);

int CM_LoadSubBSP(const char *name, qboolean clientload);


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

/*
====================
CL_GetCurrentSnapshotNumber
====================
*/
void	CL_GetCurrentSnapshotNumber( int *snapshotNumber, int *serverTime ) {
	*snapshotNumber = cl.snap.messageNum;
	*serverTime = cl.snap.serverTime;
}

/*
====================
CL_GetSnapshot
====================
*/
qboolean	CL_GetSnapshot( int snapshotNumber, snapshot_t *snapshot ) {
	clSnapshot_t	*clSnap;
	int				i, count;

	if ( snapshotNumber > cl.snap.messageNum ) {
		Com_Error( ERR_DROP, "CL_GetSnapshot: snapshotNumber > cl.snapshot.messageNum" );
	}

	// if the frame has fallen out of the circular buffer, we can't return it
	if ( cl.snap.messageNum - snapshotNumber >= PACKET_BACKUP ) {
		return qfalse;
	}

	// if the frame is not valid, we can't return it
	clSnap = &cl.snapshots[snapshotNumber & PACKET_MASK];
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
	snapshot->ping = clSnap->ping;
	snapshot->serverTime = clSnap->serverTime;
	Com_Memcpy( snapshot->areamask, clSnap->areamask, sizeof( snapshot->areamask ) );
	snapshot->ps = clSnap->ps;
	snapshot->vps = clSnap->vps; //get the vehicle ps
	count = clSnap->numEntities;
	if ( count > MAX_ENTITIES_IN_SNAPSHOT ) {
		Com_DPrintf( "CL_GetSnapshot: truncated %i entities to %i\n", count, MAX_ENTITIES_IN_SNAPSHOT );
		count = MAX_ENTITIES_IN_SNAPSHOT;
	}
	snapshot->numEntities = count;

 	for ( i = 0 ; i < count ; i++ ) {

		int entNum =  ( clSnap->parseEntitiesNum + i ) & (MAX_PARSE_ENTITIES-1) ;

		// copy everything but the ghoul2 pointer
		memcpy(&snapshot->entities[i], &cl.parseEntities[ entNum ], sizeof(entityState_t));
	}

	// FIXME: configstring changes and server commands!!!

	return qtrue;
}

qboolean CL_GetDefaultState(int index, entityState_t *state)
{
	if (index < 0 || index >= MAX_GENTITIES)
	{
		return qfalse;
	}

	if (!(cl.entityBaselines[index].eFlags & EF_PERMANENT))
	{
		return qfalse;
	}

	*state = cl.entityBaselines[index];

	return qtrue;
}

/*
=====================
CL_SetUserCmdValue
=====================
*/
extern float cl_mPitchOverride;
extern float cl_mYawOverride;
extern float cl_mSensitivityOverride;
void CL_SetUserCmdValue( int userCmdValue, float sensitivityScale, float mPitchOverride, float mYawOverride, float mSensitivityOverride, int fpSel, int invenSel ) {
	cl.cgameUserCmdValue = userCmdValue;
	cl.cgameSensitivity = sensitivityScale;
	cl_mPitchOverride = mPitchOverride;
	cl_mYawOverride = mYawOverride;
	cl_mSensitivityOverride = mSensitivityOverride;
	cl.cgameForceSelection = fpSel;
	cl.cgameInvenSelection = invenSel;
}

/*
=====================
CL_SetClientForceAngle
=====================
*/
void CL_SetClientForceAngle(int time, vec3_t angle)
{
	cl.cgameViewAngleForceTime = time;
	VectorCopy(angle, cl.cgameViewAngleForce);
}

/*
=====================
CL_AddCgameCommand
=====================
*/
void CL_AddCgameCommand( const char *cmdName ) {
	Cmd_AddCommand( cmdName, NULL );
}

/*
=====================
CL_CgameError
=====================
*/
void CL_CgameError( const char *string ) {
	Com_Error( ERR_DROP, "%s", string );
}


int gCLTotalClientNum = 0;
//keep track of the total number of clients
extern cvar_t	*cl_autolodscale;
//if we want to do autolodscaling

void CL_DoAutoLODScale(void)
{
	float finalLODScaleFactor = 0;

	if ( gCLTotalClientNum >= 8 )
	{
		finalLODScaleFactor = (gCLTotalClientNum/-8.0f);
	}


	Cvar_Set( "r_autolodscalevalue", va("%f", finalLODScaleFactor) );
}

/*
=====================
CL_ConfigstringModified
=====================
*/
void CL_ConfigstringModified( void ) {
	char		*old, *s;
	int			i, index;
	char		*dup;
	gameState_t	oldGs;
	int			len;

	index = atoi( Cmd_Argv(1) );
	if ( index < 0 || index >= MAX_CONFIGSTRINGS ) {
		Com_Error( ERR_DROP, "CL_ConfigstringModified: bad index %i", index );
	}
	// get everything after "cs <num>"
	s = Cmd_ArgsFrom(2);

	old = cl.gameState.stringData + cl.gameState.stringOffsets[ index ];
	if ( !strcmp( old, s ) ) {
		return;		// unchanged
	}

	// build the new gameState_t
	oldGs = cl.gameState;

	Com_Memset( &cl.gameState, 0, sizeof( cl.gameState ) );

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
		Com_Memcpy( cl.gameState.stringData + cl.gameState.dataCount, dup, len + 1 );
		cl.gameState.dataCount += len + 1;
	}

	if (cl_autolodscale && cl_autolodscale->integer)
	{
		if (index >= CS_PLAYERS &&
			index < CS_G2BONES)
		{ //this means that a client was updated in some way. Go through and count the clients.
			int clientCount = 0;
			i = CS_PLAYERS;

			while (i < CS_G2BONES)
			{
				s = cl.gameState.stringData + cl.gameState.stringOffsets[ i ];

				if (s && s[0])
				{
					clientCount++;
				}

				i++;
			}

			gCLTotalClientNum = clientCount;

#ifdef _DEBUG
			Com_DPrintf("%i clients\n", gCLTotalClientNum);
#endif

			CL_DoAutoLODScale();
		}
	}

	if ( index == CS_SYSTEMINFO ) {
		// parse serverId and other cvars
		CL_SystemInfoChanged();
	}

}
#ifndef MAX_STRINGED_SV_STRING
	#define MAX_STRINGED_SV_STRING 1024
#endif
// just copied it from CG_CheckSVStringEdRef(
void CL_CheckSVStringEdRef(char *buf, const char *str)
{ //I don't really like doing this. But it utilizes the system that was already in place.
	int i = 0;
	int b = 0;
	int strLen = 0;
	qboolean gotStrip = qfalse;

	if (!str || !str[0])
	{
		if (str)
		{
			strcpy(buf, str);
		}
		return;
	}

	strcpy(buf, str);

	strLen = strlen(str);

	if (strLen >= MAX_STRINGED_SV_STRING)
	{
		return;
	}

	while (i < strLen && str[i])
	{
		gotStrip = qfalse;

		if (str[i] == '@' && (i+1) < strLen)
		{
			if (str[i+1] == '@' && (i+2) < strLen)
			{
				if (str[i+2] == '@' && (i+3) < strLen)
				{ //@@@ should mean to insert a StringEd reference here, so insert it into buf at the current place
					char stringRef[MAX_STRINGED_SV_STRING];
					int r = 0;

					while (i < strLen && str[i] == '@')
					{
						i++;
					}

					while (i < strLen && str[i] && str[i] != ' ' && str[i] != ':' && str[i] != '.' && str[i] != '\n')
					{
						stringRef[r] = str[i];
						r++;
						i++;
					}
					stringRef[r] = 0;

					buf[b] = 0;
					Q_strcat(buf, MAX_STRINGED_SV_STRING, SE_GetString("MP_SVGAME", stringRef));
					b = strlen(buf);
				}
			}
		}

		if (!gotStrip)
		{
			buf[b] = str[i];
			b++;
		}
		i++;
	}

	buf[b] = 0;
}
/*
===================
CL_GetServerCommand

Set up argc/argv for the given command
===================
*/
qboolean CL_GetServerCommand( int serverCommandNumber ) {
	char	*s;
	char	*cmd;
	static char bigConfigString[BIG_INFO_STRING];

	// if we have irretrievably lost a reliable command, drop the connection
	if ( serverCommandNumber <= clc.serverCommandSequence - MAX_RELIABLE_COMMANDS )
	{
		int i = 0;

		// when a demo record was started after the client got a whole bunch of
		// reliable commands then the client never got those first reliable commands
		if ( clc.demoplaying )
			return qfalse;

		while (i < MAX_RELIABLE_COMMANDS)
		{ //spew out the reliable command buffer
			if (clc.reliableCommands[i][0])
			{
				Com_Printf("%i: %s\n", i, clc.reliableCommands[i]);
			}
			i++;
		}
		Com_Error( ERR_DROP, "CL_GetServerCommand: a reliable command was cycled out" );
		return qfalse;
	}

	if ( serverCommandNumber > clc.serverCommandSequence ) {
		Com_Error( ERR_DROP, "CL_GetServerCommand: requested a command not received" );
		return qfalse;
	}

	s = clc.serverCommands[ serverCommandNumber & ( MAX_RELIABLE_COMMANDS - 1 ) ];
	clc.lastExecutedServerCommand = serverCommandNumber;

	Com_DPrintf( "serverCommand: %i : %s\n", serverCommandNumber, s );

rescan:
	Cmd_TokenizeString( s );
	cmd = Cmd_Argv(0);

	if ( !strcmp( cmd, "disconnect" ) ) {
		char strEd[MAX_STRINGED_SV_STRING];
		CL_CheckSVStringEdRef(strEd, Cmd_Argv(1));
		Com_Error (ERR_SERVERDISCONNECT, "%s: %s\n", SE_GetString("MP_SVGAME_SERVER_DISCONNECTED"), strEd );
	}

	if ( !strcmp( cmd, "bcs0" ) ) {
		Com_sprintf( bigConfigString, BIG_INFO_STRING, "cs %s \"%s", Cmd_Argv(1), Cmd_Argv(2) );
		return qfalse;
	}

	if ( !strcmp( cmd, "bcs1" ) ) {
		s = Cmd_Argv(2);
		if( strlen(bigConfigString) + strlen(s) >= BIG_INFO_STRING ) {
			Com_Error( ERR_DROP, "bcs exceeded BIG_INFO_STRING" );
		}
		strcat( bigConfigString, s );
		return qfalse;
	}

	if ( !strcmp( cmd, "bcs2" ) ) {
		s = Cmd_Argv(2);
		if( strlen(bigConfigString) + strlen(s) + 1 >= BIG_INFO_STRING ) {
			Com_Error( ERR_DROP, "bcs exceeded BIG_INFO_STRING" );
		}
		strcat( bigConfigString, s );
		strcat( bigConfigString, "\"" );
		s = bigConfigString;
		goto rescan;
	}

	if ( !strcmp( cmd, "cs" ) ) {
		CL_ConfigstringModified();
		// reparse the string, because CL_ConfigstringModified may have done another Cmd_TokenizeString()
		Cmd_TokenizeString( s );
		return qtrue;
	}

	if ( !strcmp( cmd, "map_restart" ) ) {
		// clear notify lines and outgoing commands before passing
		// the restart to the cgame
		Con_ClearNotify();
		// reparse the string, because Con_ClearNotify() may have done another Cmd_TokenizeString()
		Cmd_TokenizeString( s );
		Com_Memset( cl.cmds, 0, sizeof( cl.cmds ) );
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
void CL_CM_LoadMap( const char *mapname ) {
	int		checksum;

	CM_LoadMap( mapname, qtrue, &checksum );
}

/*
====================
CL_ShutdonwCGame

====================
*/
void CL_ShutdownCGame( void ) {
	Key_SetCatcher( Key_GetCatcher( ) & ~KEYCATCH_CGAME );
	cls.cgameStarted = qfalse;
	if ( !cgvm ) {
		return;
	}
	VM_Call( cgvm, CG_SHUTDOWN );
	VM_Free( cgvm );
	cgvm = NULL;
#ifdef _DONETPROFILE_
	ClReadProf().ShowTotals();
#endif
}

static int	FloatAsInt( float f ) {
	floatint_t fi;
	fi.f = f;
	return fi.i;
}

/*
====================
CL_CgameSystemCalls

The cgame module is making a system call
====================
*/
extern int s_entityWavVol[MAX_GENTITIES];

extern int CL_GetValueForHidden(const char *s); //cl_parse.cpp

extern qboolean cl_bUseFighterPitch; //cl_input.cpp

intptr_t CL_CgameSystemCalls( intptr_t *args ) {
	switch( args[0] ) {
	//rww - alright, DO NOT EVER add a GAME/CGAME/UI generic call without adding a trap to match, and
	//all of these traps must be shared and have cases in sv_game, cl_cgame, and cl_ui. They must also
	//all be in the same order, and start at 100.
	case TRAP_MEMSET:
		Com_Memset( VMA(1), args[2], args[3] );
		return 0;
	case TRAP_MEMCPY:
		Com_Memcpy( VMA(1), VMA(2), args[3] );
		return 0;
	case TRAP_STRNCPY:
		strncpy( (char *)VMA(1), (const char *)VMA(2), args[3] );
		return args[1];
	case TRAP_SIN:
		return FloatAsInt( sin( VMF(1) ) );
	case TRAP_COS:
		return FloatAsInt( cos( VMF(1) ) );
	case TRAP_ATAN2:
		return FloatAsInt( atan2( VMF(1), VMF(2) ) );
	case TRAP_SQRT:
		return FloatAsInt( sqrt( VMF(1) ) );
	case TRAP_MATRIXMULTIPLY:
		MatrixMultiply( (vec3_t *)VMA(1), (vec3_t *)VMA(2), (vec3_t *)VMA(3) );
		return 0;
	case TRAP_ANGLEVECTORS:
		AngleVectors( (const float *)VMA(1), (float *)VMA(2), (float *)VMA(3), (float *)VMA(4) );
		return 0;
	case TRAP_PERPENDICULARVECTOR:
		PerpendicularVector( (float *)VMA(1), (const float *)VMA(2) );
		return 0;
	case TRAP_FLOOR:
		return FloatAsInt( floor( VMF(1) ) );
	case TRAP_CEIL:
		return FloatAsInt( ceil( VMF(1) ) );
	case TRAP_TESTPRINTINT:
		return 0;
	case TRAP_TESTPRINTFLOAT:
		return 0;
	case TRAP_ACOS:
		return FloatAsInt( Q_acos( VMF(1) ) );
	case TRAP_ASIN:
		return FloatAsInt( Q_asin( VMF(1) ) );


	case CG_PRINT:
		Com_Printf( "%s", (const char*)VMA(1) );
		return 0;
	case CG_ERROR:
		Com_Error( ERR_DROP, "%s", (const char*)VMA(1) );
		return 0;
	case CG_MILLISECONDS:
		return Sys_Milliseconds();
	//rww - precision timer funcs... -ALWAYS- call end after start with supplied ptr, or you'll get a nasty memory leak.
	//not that you should be using these outside of debug anyway.. because you shouldn't be. So don't.
	case CG_PRECISIONTIMER_START:
		{
			void **suppliedPtr =(void **)VMA(1); //we passed in a pointer to a point
			timing_c *newTimer = new timing_c; //create the new timer
			*suppliedPtr = newTimer; //assign the pointer within the pointer to point at the mem addr of our new timer
			newTimer->Start(); //start the timer
		}
		return 0;
	case CG_PRECISIONTIMER_END:
		{
			int r;
			timing_c *timer = (timing_c *)args[1]; //this is the pointer we assigned in start, so we can directly cast it back
			r = timer->End(); //get the result
			delete timer; //delete the timer since we're done with it
			return r; //return the result
		}
	case CG_CVAR_REGISTER:
		Cvar_Register( (vmCvar_t *)VMA(1), (const char *)VMA(2), (const char *)VMA(3), args[4] ); 
		return 0;
	case CG_CVAR_UPDATE:
		Cvar_Update( (vmCvar_t *)VMA(1) );
		return 0;
	case CG_CVAR_SET:
		Cvar_Set( (const char *)VMA(1), (const char *)VMA(2) );
		return 0;
	case CG_CVAR_VARIABLESTRINGBUFFER:
		Cvar_VariableStringBuffer( (const char *)VMA(1), (char *)VMA(2), args[3] );
		return 0;
	case CG_CVAR_GETHIDDENVALUE:
		return CL_GetValueForHidden((const char *)VMA(1));
	case CG_ARGC:
		return Cmd_Argc();
	case CG_ARGV:
		Cmd_ArgvBuffer( args[1], (char *)VMA(2), args[3] );
		return 0;
	case CG_ARGS:
		Cmd_ArgsBuffer( (char *)VMA(1), args[2] );
		return 0;

	case CG_FS_FOPENFILE:
		return FS_FOpenFileByMode( (const char *)VMA(1), (int *)VMA(2), (fsMode_t)args[3] );
	case CG_FS_READ:
		FS_Read2( VMA(1), args[2], args[3] );
		return 0;
	case CG_FS_WRITE:
		FS_Write( VMA(1), args[2], args[3] );
		return 0;
	case CG_FS_FCLOSEFILE:
		FS_FCloseFile( args[1] );
		return 0;
	case CG_FS_GETFILELIST:
		return FS_GetFileList( (const char *)VMA(1), (const char *)VMA(2), (char *)VMA(3), args[4] );

	case CG_SENDCONSOLECOMMAND:
		Cbuf_AddText( (const char *)VMA(1) );
		return 0;
	case CG_ADDCOMMAND:
		CL_AddCgameCommand( (const char *)VMA(1) );
		return 0;
	case CG_REMOVECOMMAND:
		Cmd_RemoveCommand( (const char *)VMA(1) );
		return 0;
	case CG_SENDCLIENTCOMMAND:
		CL_AddReliableCommand( (const char *)VMA(1), qfalse );
		return 0;
	case CG_UPDATESCREEN:
		// this is used during lengthy level loading, so pump message loop
//		Com_EventLoop();	// FIXME: if a server restarts here, BAD THINGS HAPPEN!
// We can't call Com_EventLoop here, a restart will crash and this _does_ happen
// if there is a map change while we are downloading at pk3.
// ZOID
		SCR_UpdateScreen();
		return 0;
	case CG_CM_LOADMAP:
		if (args[2])
		{
			CM_LoadSubBSP(va("maps/%s.bsp", ((const char *)VMA(1)) + 1), qfalse);
		}
		else
		{
			CL_CM_LoadMap( (const char *)VMA(1) );
		}
		return 0;
	case CG_CM_NUMINLINEMODELS:
		return CM_NumInlineModels();
	case CG_CM_INLINEMODEL:
		return CM_InlineModel( args[1] );
	case CG_CM_TEMPBOXMODEL:
		return CM_TempBoxModel( (const float *)VMA(1), (const float *)VMA(2), /*int capsule*/ qfalse );
	case CG_CM_TEMPCAPSULEMODEL:
		return CM_TempBoxModel( (const float *)VMA(1), (const float *)VMA(2), /*int capsule*/ qtrue );
	case CG_CM_POINTCONTENTS:
		return CM_PointContents( (const float *)VMA(1), args[2] );
	case CG_CM_TRANSFORMEDPOINTCONTENTS:
		return CM_TransformedPointContents( (const float *)VMA(1), args[2], (const float *)VMA(3), (const float *)VMA(4) );
	case CG_CM_BOXTRACE:
		CM_BoxTrace( (trace_t *)VMA(1), (const float *)VMA(2), (const float *)VMA(3), (const float *)VMA(4), (const float *)VMA(5), args[6], args[7], /*int capsule*/ qfalse );
		return 0;
	case CG_CM_CAPSULETRACE:
		CM_BoxTrace( (trace_t *)VMA(1), (const float *)VMA(2), (const float *)VMA(3), (const float *)VMA(4), (const float *)VMA(5), args[6], args[7], /*int capsule*/ qtrue );
		return 0;
	case CG_CM_TRANSFORMEDBOXTRACE:
		CM_TransformedBoxTrace( (trace_t *)VMA(1), (const float *)VMA(2), (const float *)VMA(3), (const float *)VMA(4), (const float *)VMA(5), args[6], args[7], (const float *)VMA(8), (const float *)VMA(9), /*int capsule*/ qfalse );
		return 0;
	case CG_CM_TRANSFORMEDCAPSULETRACE:
		CM_TransformedBoxTrace( (trace_t *)VMA(1), (const float *)VMA(2), (const float *)VMA(3), (const float *)VMA(4), (const float *)VMA(5), args[6], args[7], (const float *)VMA(8), (const float *)VMA(9), /*int capsule*/ qtrue );
		return 0;
	case CG_CM_MARKFRAGMENTS:
		return re.MarkFragments( args[1], (const vec3_t *)VMA(2), (const float *)VMA(3), args[4], (float *)VMA(5), args[6], (markFragment_t *)VMA(7) );
	case CG_S_GETVOICEVOLUME:
		return s_entityWavVol[args[1]];
	case CG_S_MUTESOUND:
		S_MuteSound( args[1], args[2] );
		return 0;
	case CG_S_STARTSOUND:
		S_StartSound( (float *)VMA(1), args[2], args[3], args[4] );
		return 0;
	case CG_S_STARTLOCALSOUND:
		S_StartLocalSound( args[1], args[2] );
		return 0;
	case CG_S_CLEARLOOPINGSOUNDS:
		S_ClearLoopingSounds();
		return 0;
	case CG_S_ADDLOOPINGSOUND:
		S_AddLoopingSound( args[1], (const float *)VMA(2), (const float *)VMA(3), args[4] );
		return 0;
	case CG_S_ADDREALLOOPINGSOUND:
		//S_AddRealLoopingSound( args[1], (const float *)VMA(2), (const float *)VMA(3), args[4] );
		S_AddLoopingSound( args[1], (const float *)VMA(2), (const float *)VMA(3), args[4] );
		return 0;
	case CG_S_STOPLOOPINGSOUND:
		S_StopLoopingSound( args[1] );
		return 0;
	case CG_S_UPDATEENTITYPOSITION:
		S_UpdateEntityPosition( args[1], (const float *)VMA(2) );
		return 0;
	case CG_S_RESPATIALIZE:
		S_Respatialize( args[1], (const float *)VMA(2), (vec3_t *)VMA(3), args[4] );
		return 0;
	case CG_S_SHUTUP:
		s_shutUp = (qboolean)args[1];
		return 0;
	case CG_S_REGISTERSOUND:
		return S_RegisterSound( (const char *)VMA(1) );
	case CG_S_STARTBACKGROUNDTRACK:
		S_StartBackgroundTrack( (const char *)VMA(1), (const char *)VMA(2), args[3]?qtrue:qfalse );
		return 0;

	case CG_S_UPDATEAMBIENTSET:
		S_UpdateAmbientSet((const char *)VMA(1), (float *)VMA(2));
		return 0;
	case CG_AS_PARSESETS:
		AS_ParseSets();
		return 0;
	case CG_AS_ADDPRECACHEENTRY:
		AS_AddPrecacheEntry((const char *)VMA(1));
		return 0;
	case CG_S_ADDLOCALSET:
		return S_AddLocalSet((const char *)VMA(1), (float *)VMA(2), (float *)VMA(3), args[4], args[5]);
	case CG_AS_GETBMODELSOUND:
		return AS_GetBModelSound((const char *)VMA(1), args[2]);

	case CG_R_LOADWORLDMAP:
		re.LoadWorld( (const char *)VMA(1) );
		return 0; 
	case CG_R_REGISTERMODEL:
		return re.RegisterModel( (const char *)VMA(1) );
	case CG_R_REGISTERSKIN:
		return re.RegisterSkin( (const char *)VMA(1) );
	case CG_R_REGISTERSHADER:
		return re.RegisterShader( (const char *)VMA(1) );
	case CG_R_REGISTERSHADERNOMIP:
		return re.RegisterShaderNoMip( (const char *)VMA(1) );
	case CG_R_REGISTERFONT:
		return re.RegisterFont( (const char *)VMA(1) );
	case CG_R_FONT_STRLENPIXELS:
		return re.Font_StrLenPixels( (const char *)VMA(1), args[2], VMF(3) );
	case CG_R_FONT_STRLENCHARS:
		return re.Font_StrLenChars( (const char *)VMA(1) );
	case CG_R_FONT_STRHEIGHTPIXELS:
		return re.Font_HeightPixels( args[1], VMF(2) );
	case CG_R_FONT_DRAWSTRING:
		re.Font_DrawString( args[1], args[2], (const char *)VMA(3), (const float *) VMA(4), args[5], args[6], VMF(7) );
		return 0;
	case CG_LANGUAGE_ISASIAN:
		return re.Language_IsAsian();
	case CG_LANGUAGE_USESSPACES:
		return re.Language_UsesSpaces();
	case CG_ANYLANGUAGE_READCHARFROMSTRING:
		return re.AnyLanguage_ReadCharFromString( (const char *) VMA(1), (int *) VMA(2), (qboolean *) VMA(3) );
	case CG_R_CLEARSCENE:
		re.ClearScene();
		return 0;
	case CG_R_CLEARDECALS:
		re.ClearDecals();
		return 0;
	case CG_R_ADDREFENTITYTOSCENE:
		re.AddRefEntityToScene( (const refEntity_t *)VMA(1) );
		return 0;
	case CG_R_ADDPOLYTOSCENE:
		re.AddPolyToScene( args[1], args[2], (const polyVert_t *)VMA(3), 1 );
		return 0;
	case CG_R_ADDPOLYSTOSCENE:
		re.AddPolyToScene( args[1], args[2], (const polyVert_t *)VMA(3), args[4] );
		return 0;
	case CG_R_ADDDECALTOSCENE:
		re.AddDecalToScene( (qhandle_t)args[1], (const float*)VMA(2), (const float*)VMA(3), VMF(4), VMF(5), VMF(6), VMF(7), VMF(8), (qboolean)args[9], VMF(10), (qboolean)args[11] );
		return 0;
	case CG_R_LIGHTFORPOINT:
		return re.LightForPoint( (float *)VMA(1), (float *)VMA(2), (float *)VMA(3), (float *)VMA(4) );
	case CG_R_ADDLIGHTTOSCENE:
#ifdef VV_LIGHTING
		VVLightMan.RE_AddLightToScene( (const float *)VMA(1), VMF(2), VMF(3), VMF(4), VMF(5) );
#else
		re.AddLightToScene( (const float *)VMA(1), VMF(2), VMF(3), VMF(4), VMF(5) );
#endif
		return 0;
	case CG_R_ADDADDITIVELIGHTTOSCENE:
#ifdef VV_LIGHTING
		VVLightMan.RE_AddLightToScene( (const float *)VMA(1), VMF(2), VMF(3), VMF(4), VMF(5) );
#else
		re.AddAdditiveLightToScene( (const float *)VMA(1), VMF(2), VMF(3), VMF(4), VMF(5) );
#endif
		return 0;
	case CG_R_RENDERSCENE:
		re.RenderScene( (const refdef_t *)VMA(1) );
		return 0;
	case CG_R_SETCOLOR:
		re.SetColor( (const float *)VMA(1) );
		return 0;
	case CG_R_DRAWSTRETCHPIC:
		re.DrawStretchPic( VMF(1), VMF(2), VMF(3), VMF(4), VMF(5), VMF(6), VMF(7), VMF(8), args[9] );
		return 0;
	case CG_R_MODELBOUNDS:
		re.ModelBounds( args[1], (float *)VMA(2), (float *)VMA(3) );
		return 0;
	case CG_R_LERPTAG:
		return re.LerpTag( (orientation_t *)VMA(1), args[2], args[3], args[4], VMF(5), (const char *)VMA(6) );
	case CG_R_DRAWROTATEPIC:
		re.DrawRotatePic( VMF(1), VMF(2), VMF(3), VMF(4), VMF(5), VMF(6), VMF(7), VMF(8), VMF(9), args[10] );
		return 0;
	case CG_R_DRAWROTATEPIC2:
		re.DrawRotatePic2( VMF(1), VMF(2), VMF(3), VMF(4), VMF(5), VMF(6), VMF(7), VMF(8), VMF(9), args[10] );
		return 0;
	
	case CG_R_SETRANGEFOG:
		re.SetRangedFog( VMF(1) );
		return 0;

	case CG_R_SETREFRACTIONPROP:
		re.SetRefractionProperties( VMF(1), VMF(2), (qboolean)args[3], (qboolean)args[4] );
		return 0;

	case CG_GETGLCONFIG:
		CL_GetGlconfig( (glconfig_t *)VMA(1) );
		return 0;
	case CG_GETGAMESTATE:
		CL_GetGameState( (gameState_t *)VMA(1) );
		return 0;
	case CG_GETCURRENTSNAPSHOTNUMBER:
		CL_GetCurrentSnapshotNumber( (int *)VMA(1), (int *)VMA(2) );
		return 0;
	case CG_GETSNAPSHOT:
		return CL_GetSnapshot( args[1], (snapshot_t *)VMA(2) );
	case CG_GETDEFAULTSTATE:
		return CL_GetDefaultState(args[1], (entityState_t *)VMA(2));
	case CG_GETSERVERCOMMAND:
		return CL_GetServerCommand( args[1] );
	case CG_GETCURRENTCMDNUMBER:
		return CL_GetCurrentCmdNumber();
	case CG_GETUSERCMD:
		return CL_GetUserCmd( args[1], (struct usercmd_s *)VMA(2) );
	case CG_SETUSERCMDVALUE:
		cl_bUseFighterPitch = (qboolean)args[8];
		CL_SetUserCmdValue( args[1], VMF(2), VMF(3), VMF(4), VMF(5), args[6], args[7] );
		return 0;
	case CG_SETCLIENTFORCEANGLE:
		CL_SetClientForceAngle(args[1], (float *)VMA(2));
		return 0;
	case CG_SETCLIENTTURNEXTENT:
		return 0;

	case CG_OPENUIMENU:
		VM_Call( uivm, UI_SET_ACTIVE_MENU, args[1] );
		return 0;

	case CG_MEMORY_REMAINING:
		return Hunk_MemoryRemaining();
  case CG_KEY_ISDOWN:
		return Key_IsDown( args[1] );
  case CG_KEY_GETCATCHER:
		return Key_GetCatcher();
  case CG_KEY_SETCATCHER:
		// Don't allow the cgame module to close the console
		Key_SetCatcher( args[1] | ( Key_GetCatcher( ) & KEYCATCH_CONSOLE ) );
    return 0;
  case CG_KEY_GETKEY:
		return Key_GetKey( (const char *)VMA(1) );

	case CG_PC_ADD_GLOBAL_DEFINE:
		return botlib_export->PC_AddGlobalDefine( (char *)VMA(1) );
	case CG_PC_LOAD_SOURCE:
		return botlib_export->PC_LoadSourceHandle( (const char *)VMA(1) );
	case CG_PC_FREE_SOURCE:
		return botlib_export->PC_FreeSourceHandle( args[1] );
	case CG_PC_READ_TOKEN:
		return botlib_export->PC_ReadTokenHandle( args[1], (struct pc_token_s *)VMA(2) );
	case CG_PC_SOURCE_FILE_AND_LINE:
		return botlib_export->PC_SourceFileAndLine( args[1], (char *)VMA(2), (int *)VMA(3) );
	case CG_PC_LOAD_GLOBAL_DEFINES:
		return botlib_export->PC_LoadGlobalDefines ( (char *)VMA(1) );
	case CG_PC_REMOVE_ALL_GLOBAL_DEFINES:
		botlib_export->PC_RemoveAllGlobalDefines ( );
		return 0;

	case CG_S_STOPBACKGROUNDTRACK:
		S_StopBackgroundTrack();
		return 0;

	case CG_REAL_TIME:
		return Com_RealTime( (struct qtime_s *)VMA(1) );
	case CG_SNAPVECTOR:
		Sys_SnapVector( (float *)VMA(1) );
		return 0;

	case CG_CIN_PLAYCINEMATIC:
	  return CIN_PlayCinematic((const char *)VMA(1), args[2], args[3], args[4], args[5], args[6]);

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

	case CG_R_REMAP_SHADER:
		re.RemapShader( (const char *)VMA(1), (const char *)VMA(2), (const char *)VMA(3) );
		return 0;

	case CG_R_GET_LIGHT_STYLE:
		re.GetLightStyle(args[1], (unsigned char *)VMA(2));
		return 0;

	case CG_R_SET_LIGHT_STYLE:
		re.SetLightStyle(args[1], args[2]);
		return 0;

	case CG_R_GET_BMODEL_VERTS:
		re.GetBModelVerts( args[1], (float (*)[3])VMA(2), (float *)VMA(3) );
		return 0;

	case CG_R_GETDISTANCECULL:
		{
			float *f;
			f = (float *)VMA(1);
			*f = re.GetDistanceCull();
		}
		return 0;

	case CG_R_GETREALRES:
		{
			int *w = (int *)VMA(1);
			int *h = (int *)VMA(2);
			re.GetRealRes( w, h );
		}
		return 0;

	case CG_R_AUTOMAPELEVADJ:
		re.AutomapElevationAdjustment(VMF(1));
		return 0;

	case CG_R_INITWIREFRAMEAUTO:
		return re.InitializeWireframeAutomap();

/*
	case CG_LOADCAMERA:
		return loadCamera(VMA(1));

	case CG_STARTCAMERA:
		startCamera(args[1]);
		return 0;

	case CG_GETCAMERAINFO:
		return getCameraInfo(args[1], VMA(2), VMA(3));
*/
	case CG_GET_ENTITY_TOKEN:
		return re.GetEntityToken( (char *)VMA(1), args[2] );
	case CG_R_INPVS:
		return re.inPVS( (const float *)VMA(1), (const float *)VMA(2), (byte *)VMA(3) );

#ifndef DEBUG_DISABLEFXCALLS
	case CG_FX_ADDLINE:
		FX_AddLine( (float *)VMA(1), (float *)VMA(2), VMF(3), VMF(4), VMF(5), 
									VMF(6), VMF(7), VMF(8), 
									(float *)VMA(9), (float *)VMA(10), VMF(11), 
									args[12], args[13], args[14]);
		return 0;
	case CG_FX_REGISTER_EFFECT:
		return FX_RegisterEffect((const char *)VMA(1));

	case CG_FX_PLAY_EFFECT:
		FX_PlayEffect((const char *)VMA(1), (float *)VMA(2), (float *)VMA(3), args[4], args[5] );
		return 0;

	case CG_FX_PLAY_ENTITY_EFFECT:
		assert(0);//gone!
		//FX_PlayEntityEffect((const char *)VMA(1), (float *)VMA(2), (vec3_t *)VMA(3), args[4], args[5], args[6], args[7] );
		return 0;

	case CG_FX_PLAY_EFFECT_ID:
		FX_PlayEffectID(args[1], (float *)VMA(2), (float *)VMA(3), args[4], args[5] );
		return 0;

	case CG_FX_PLAY_PORTAL_EFFECT_ID:
		FX_PlayEffectID(args[1], (float *)VMA(2), (float *)VMA(3), args[4], args[5], qtrue );
		return 0;

	case CG_FX_PLAY_ENTITY_EFFECT_ID:
		FX_PlayEntityEffectID(args[1], (float *)VMA(2), (vec3_t *)VMA(3), args[4], args[5], args[6], args[7] );
		return 0;

	case CG_FX_PLAY_BOLTED_EFFECT_ID:
		{
		//( int id, vec3_t org, void *pGhoul2, const int boltNum, const int entNum, const int modelNum, int iLooptime, qboolean isRelative );
		CGhoul2Info_v &g2 = *((CGhoul2Info_v *)args[3]);
		int boltInfo=0;
		if ( re.G2API_AttachEnt( &boltInfo, &g2[args[6]], args[4], args[5], args[6] ) )
		{
			FX_PlayBoltedEffectID(args[1], (float *)VMA(2), boltInfo, g2.mItem, args[7], (qboolean)args[8] );
			return 1;
		}
		return 0;
		}
	case CG_FX_ADD_SCHEDULED_EFFECTS:
		FX_AddScheduledEffects((qboolean)args[1]);
		return 0;

	case CG_FX_DRAW_2D_EFFECTS:
		FX_Draw2DEffects ( VMF(1), VMF(2) );
		return 0;

	case CG_FX_INIT_SYSTEM:
		return FX_InitSystem( (refdef_t*)VMA(1) );

	case CG_FX_SET_REFDEF:
		FX_SetRefDefFromCGame( (refdef_t*)VMA(1) );
		return 0;

	case CG_FX_FREE_SYSTEM:
		return FX_FreeSystem();

	case CG_FX_ADJUST_TIME:
		FX_AdjustTime(args[1]);
		return 0;

	case CG_FX_RESET:
		FX_Free ( false );
		return 0;

	case CG_FX_ADDPOLY:
		{
			addpolyArgStruct_t *p;

			p = (addpolyArgStruct_t *)VMA(1);//args[1];

			if (p)
			{
				FX_AddPoly(p->p, p->ev, p->numVerts, p->vel, p->accel, p->alpha1, p->alpha2,
					p->alphaParm, p->rgb1, p->rgb2, p->rgbParm, p->rotationDelta, p->bounce, p->motionDelay,
					p->killTime, p->shader, p->flags);
			}
		}
		return 0;

	case CG_FX_ADDBEZIER:
		{
			addbezierArgStruct_t *b;

			b = (addbezierArgStruct_t *)VMA(1);//args[1];

			if (b)
			{
				FX_AddBezier(b->start, b->end, b->control1, b->control1Vel, b->control2, b->control2Vel,
					b->size1, b->size2, b->sizeParm, b->alpha1, b->alpha2, b->alphaParm, b->sRGB,
					b->eRGB, b->rgbParm, b->killTime, b->shader, b->flags);
			}
		}
		return 0;

	case CG_FX_ADDPRIMITIVE:
		{
			effectTrailArgStruct_t *a;

			a = (effectTrailArgStruct_t *)VMA(1);//args[1];

			if (a)
			{
				FX_FeedTrail(a);
			}
		}
		return 0;

	case CG_FX_ADDSPRITE:
		{
			addspriteArgStruct_t *s;

			s = (addspriteArgStruct_t *)VMA(1);//args[1];

			if (s)
			{
				vec3_t rgb;
				rgb[0] = 1;
				rgb[1] = 1;
				rgb[2] = 1;
				//FX_AddSprite(NULL, s->origin, s->vel, s->accel, s->scale, s->dscale, s->sAlpha, s->eAlpha,
				//	s->rotation, s->bounce, s->life, s->shader, s->flags);
				FX_AddParticle(s->origin, s->vel, s->accel, s->scale, s->dscale, 0, s->sAlpha, s->eAlpha, 0,
					rgb, rgb, 0, s->rotation, 0, vec3_origin, vec3_origin, s->bounce, 0, 0, s->life,
					s->shader, s->flags);
			}
		}
		return 0;
	case CG_FX_ADDELECTRICITY:
		{
			addElectricityArgStruct_t *p;

			p = (addElectricityArgStruct_t *)VMA(1);

			if (p)
			{

				FX_AddElectricity(p->start, p->end, p->size1, p->size2, p->sizeParm, p->alpha1, p->alpha2,
					p->alphaParm, p->sRGB, p->eRGB, p->rgbParm, p->chaos, p->killTime, p->shader, p->flags);
			}
		}
		return 0;
#else
	case CG_FX_REGISTER_EFFECT:
	case CG_FX_PLAY_EFFECT:
	case CG_FX_PLAY_ENTITY_EFFECT:
	case CG_FX_PLAY_EFFECT_ID:
	case CG_FX_PLAY_PORTAL_EFFECT_ID:
	case CG_FX_PLAY_ENTITY_EFFECT_ID:
	case CG_FX_PLAY_BOLTED_EFFECT_ID:
	case CG_FX_ADD_SCHEDULED_EFFECTS:
	case CG_FX_INIT_SYSTEM:
	case CG_FX_FREE_SYSTEM:
	case CG_FX_ADJUST_TIME:
	case CG_FX_ADDPOLY:
	case CG_FX_ADDBEZIER:
	case CG_FX_ADDPRIMITIVE:
	case CG_FX_ADDSPRITE:
	case CG_FX_ADDELECTRICITY:
		return 0;
#endif

//	case CG_SP_PRINT:
//		CL_SP_Print(args[1], (byte *)VMA(2));
//		return 0;

	case CG_ROFF_CLEAN:
		return theROFFSystem.Clean(qtrue);
	
	case CG_ROFF_UPDATE_ENTITIES:
		theROFFSystem.UpdateEntities(qtrue);
		return 0;

	case CG_ROFF_CACHE:
		return theROFFSystem.Cache( (char *)VMA(1), qtrue );
		
	case CG_ROFF_PLAY:
		return theROFFSystem.Play(args[1], args[2], (qboolean)args[3], qtrue );

	case CG_ROFF_PURGE_ENT:
		return theROFFSystem.PurgeEnt( args[1], qtrue );

	//rww - dynamic vm memory allocation!
	case CG_TRUEMALLOC:
		VM_Shifted_Alloc((void **)VMA(1), args[2]);
		return 0;
	case CG_TRUEFREE:
		VM_Shifted_Free((void **)VMA(1));
		return 0;

/*
Ghoul2 Insert Start
*/
		
	case CG_G2_LISTSURFACES:
		re.G2API_ListSurfaces( (CGhoul2Info *) args[1] );
		return 0;

	case CG_G2_LISTBONES:
		re.G2API_ListBones( (CGhoul2Info *) args[1], args[2]);
		return 0;

	case CG_G2_HAVEWEGHOULMODELS:
		if ( re.G2API_HaveWeGhoul2Models )
			return re.G2API_HaveWeGhoul2Models( *((CGhoul2Info_v *)args[1]) );
		else
			return qfalse;

	case CG_G2_SETMODELS:
		re.G2API_SetGhoul2ModelIndexes( *((CGhoul2Info_v *)args[1]),(qhandle_t *)VMA(2),(qhandle_t *)VMA(3));
		return 0;

	case CG_G2_GETBOLT:
		return re.G2API_GetBoltMatrix(*((CGhoul2Info_v *)args[1]), args[2], args[3], (mdxaBone_t *)VMA(4), (const float *)VMA(5),(const float *)VMA(6), args[7], (qhandle_t *)VMA(8), (float *)VMA(9));

	case CG_G2_GETBOLT_NOREC:
		re.G2API_BoltMatrixReconstruction( qfalse );
		return re.G2API_GetBoltMatrix(*((CGhoul2Info_v *)args[1]), args[2], args[3], (mdxaBone_t *)VMA(4), (const float *)VMA(5),(const float *)VMA(6), args[7], (qhandle_t *)VMA(8), (float *)VMA(9));

	case CG_G2_GETBOLT_NOREC_NOROT:
		//gG2_GBMNoReconstruct = qtrue;
		//Yeah, this was probably BAD.
		re.G2API_BoltMatrixSPMethod( qtrue );
		return re.G2API_GetBoltMatrix(*((CGhoul2Info_v *)args[1]), args[2], args[3], (mdxaBone_t *)VMA(4), (const float *)VMA(5),(const float *)VMA(6), args[7], (qhandle_t *)VMA(8), (float *)VMA(9));

	case CG_G2_INITGHOUL2MODEL:
#ifdef _FULL_G2_LEAK_CHECKING
		g_G2AllocServer = 0;
#endif
		return	re.G2API_InitGhoul2Model((CGhoul2Info_v **)VMA(1), (const char *)VMA(2), args[3], (qhandle_t) args[4],
									  (qhandle_t) args[5], args[6], args[7]);

	case CG_G2_SETSKIN:
		{
			CGhoul2Info_v &g2 = *((CGhoul2Info_v *)args[1]);
			int modelIndex = args[2];
			
			return re.G2API_SetSkin(&g2[modelIndex], args[3], args[4]);
		}

	case CG_G2_COLLISIONDETECT:
		re.G2API_CollisionDetect ( (CollisionRecord_t*)VMA(1), *((CGhoul2Info_v *)args[2]), 
								   (const float*)VMA(3),
								   (const float*)VMA(4),
								   args[5],
								   args[6],
								   (float*)VMA(7),
								   (float*)VMA(8),
								   (float*)VMA(9),
								   G2VertSpaceClient,
								   args[10],
								   args[11],
								   VMF(12) );
		return 0;

	case CG_G2_COLLISIONDETECTCACHE:
		re.G2API_CollisionDetectCache ( (CollisionRecord_t*)VMA(1), *((CGhoul2Info_v *)args[2]), 
								   (const float*)VMA(3),
								   (const float*)VMA(4),
								   args[5],
								   args[6],
								   (float*)VMA(7),
								   (float*)VMA(8),
								   (float*)VMA(9),
								   G2VertSpaceClient,
								   args[10],
								   args[11],
								   VMF(12) );
		return 0;

	case CG_G2_ANGLEOVERRIDE:
		return re.G2API_SetBoneAngles(*((CGhoul2Info_v *)args[1]), args[2], (const char *)VMA(3), (float *)VMA(4), args[5],
							 (const Eorientations) args[6], (const Eorientations) args[7], (const Eorientations) args[8],
							 (qhandle_t *)VMA(9), args[10], args[11] );
	
	case CG_G2_CLEANMODELS:
#ifdef _FULL_G2_LEAK_CHECKING
		g_G2AllocServer = 0;
#endif
		if ( re.G2API_CleanGhoul2Models )
			re.G2API_CleanGhoul2Models((CGhoul2Info_v **)VMA(1));
	//	G2API_CleanGhoul2Models((CGhoul2Info_v **)args[1]);
		return 0;

	case CG_G2_PLAYANIM:
		return re.G2API_SetBoneAnim(*((CGhoul2Info_v *)args[1]), args[2], (const char *)VMA(3), args[4], args[5],
								args[6], VMF(7), args[8], VMF(9), args[10]);

	case CG_G2_GETBONEANIM:
		{
			CGhoul2Info_v &g2 = *((CGhoul2Info_v *)args[1]);
			int modelIndex = args[10];

			return re.G2API_GetBoneAnim(&g2[modelIndex], (const char*)VMA(2), args[3], (float *)VMA(4), (int *)VMA(5),
								(int *)VMA(6), (int *)VMA(7), (float *)VMA(8), (int *)VMA(9));
		}

	case CG_G2_GETBONEFRAME:
		{ //rwwFIXMEFIXME: Just make a G2API_GetBoneFrame func too. This is dirty.
			CGhoul2Info_v &g2 = *((CGhoul2Info_v *)args[1]);
			int modelIndex = args[6];
			int iDontCare1 = 0, iDontCare2 = 0, iDontCare3 = 0;
			float fDontCare1 = 0;

			return re.G2API_GetBoneAnim(&g2[modelIndex], (const char*)VMA(2), args[3], (float *)VMA(4), &iDontCare1,
								&iDontCare2, &iDontCare3, &fDontCare1, (int *)VMA(5));
		}

	case CG_G2_GETGLANAME:
		//	return (int)G2API_GetGLAName(*((CGhoul2Info_v *)VMA(1)), args[2]);
		{
			char *point = ((char *)VMA(3));
			char *local;
			local = re.G2API_GetGLAName(*((CGhoul2Info_v *)args[1]), args[2]);
			if (local)
			{
				strcpy(point, local);
			}
		}
		return 0;

	case CG_G2_COPYGHOUL2INSTANCE:
		return re.G2API_CopyGhoul2Instance(*((CGhoul2Info_v *)args[1]), *((CGhoul2Info_v *)args[2]), args[3]);

	case CG_G2_COPYSPECIFICGHOUL2MODEL:
		re.G2API_CopySpecificG2Model(*((CGhoul2Info_v *)args[1]), args[2], *((CGhoul2Info_v *)args[3]), args[4]);
		return 0;

	case CG_G2_DUPLICATEGHOUL2INSTANCE:
#ifdef _FULL_G2_LEAK_CHECKING
		g_G2AllocServer = 0;
#endif
		re.G2API_DuplicateGhoul2Instance(*((CGhoul2Info_v *)args[1]), (CGhoul2Info_v **)VMA(2));
		return 0;

	case CG_G2_HASGHOUL2MODELONINDEX:
		return (int)re.G2API_HasGhoul2ModelOnIndex((CGhoul2Info_v **)VMA(1), args[2]);
		//return (int)G2API_HasGhoul2ModelOnIndex((CGhoul2Info_v **)args[1], args[2]);

	case CG_G2_REMOVEGHOUL2MODEL:
#ifdef _FULL_G2_LEAK_CHECKING
		g_G2AllocServer = 0;
#endif
		return (int)re.G2API_RemoveGhoul2Model((CGhoul2Info_v **)VMA(1), args[2]);
		//return (int)G2API_RemoveGhoul2Model((CGhoul2Info_v **)args[1], args[2]);

	case CG_G2_SKINLESSMODEL:
		{
			CGhoul2Info_v &g2 = *((CGhoul2Info_v *)args[1]);
			return re.G2API_SkinlessModel(&g2[args[2]]);
		}

	case CG_G2_GETNUMGOREMARKS:
#ifdef _G2_GORE
		{
			CGhoul2Info_v &g2 = *((CGhoul2Info_v *)args[1]);
			return re.G2API_GetNumGoreMarks(&g2[args[2]]);
		}
#endif
		return 0;

	case CG_G2_ADDSKINGORE:
#ifdef _G2_GORE
		re.G2API_AddSkinGore(*((CGhoul2Info_v *)args[1]),*(SSkinGoreData *)VMA(2));
#endif
		return 0;

	case CG_G2_CLEARSKINGORE:
#ifdef _G2_GORE
		re.G2API_ClearSkinGore ( *((CGhoul2Info_v *)args[1]) );
#endif
		return 0;

	case CG_G2_SIZE:
		return re.G2API_Ghoul2Size ( *((CGhoul2Info_v *)args[1]) );
		break;

	case CG_G2_ADDBOLT:
		return	re.G2API_AddBolt(*((CGhoul2Info_v *)args[1]), args[2], (const char *)VMA(3));


	case CG_G2_ATTACHENT:
//				G2API_AttachEnt(int *boltInfo, CGhoul2Info *ghlInfoTo, int toBoltIndex, int entNum, int toModelNum)
		{
			CGhoul2Info_v &g2 = *((CGhoul2Info_v *)args[2]);
			return	re.G2API_AttachEnt( (int*)VMA(1), &g2[0], args[3], args[4], args[5] );
		}

	case CG_G2_SETBOLTON:
		re.G2API_SetBoltInfo(*((CGhoul2Info_v *)args[1]), args[2], args[3]);
		return 0;

#ifdef _SOF2	
	case CG_G2_ADDSKINGORE:
		re.G2API_AddSkinGore(*((CGhoul2Info_v *)args[1]),*(SSkinGoreData *)VMA(2));
		return 0;
#endif // _SOF2
/*
Ghoul2 Insert End
*/
	case CG_G2_SETROOTSURFACE:
		return re.G2API_SetRootSurface(*((CGhoul2Info_v *)args[1]), args[2], (const char *)VMA(3));

	case CG_G2_SETSURFACEONOFF:
		return re.G2API_SetSurfaceOnOff(*((CGhoul2Info_v *)args[1]), (const char *)VMA(2), /*(const int)VMA(3)*/args[3]);

	case CG_G2_SETNEWORIGIN:
		return re.G2API_SetNewOrigin(*((CGhoul2Info_v *)args[1]), /*(const int)VMA(2)*/args[2]);

	case CG_G2_DOESBONEEXIST:
		{
			CGhoul2Info_v &g2 = *((CGhoul2Info_v *)args[1]);
			return re.G2API_DoesBoneExist(&g2[args[2]], (const char *)VMA(3));
		}

	case CG_G2_GETSURFACERENDERSTATUS:
	{
		CGhoul2Info_v &g2 = *((CGhoul2Info_v *)args[1]);

		return re.G2API_GetSurfaceRenderStatus(&g2[args[2]], (const char *)VMA(3));
	}

	case CG_G2_GETTIME:
		return re.G2API_GetTime(0);

	case CG_G2_SETTIME:
		re.G2API_SetTime(args[1], args[2]);
		return 0;

	case CG_G2_ABSURDSMOOTHING:
		{
			CGhoul2Info_v &g2 = *((CGhoul2Info_v *)args[1]);

			re.G2API_AbsurdSmoothing(g2, (qboolean)args[2]);
		}
		return 0;


	case CG_G2_SETRAGDOLL:
		{
			//Convert the info in the shared structure over to the class-based version.
			sharedRagDollParams_t *rdParamst = (sharedRagDollParams_t *)VMA(2);
			CRagDollParams rdParams;

			if (!rdParamst)
			{
				re.G2API_ResetRagDoll(*((CGhoul2Info_v *)args[1]));
				return 0;
			}

			VectorCopy(rdParamst->angles, rdParams.angles);
			VectorCopy(rdParamst->position, rdParams.position);
			VectorCopy(rdParamst->scale, rdParams.scale);
			VectorCopy(rdParamst->pelvisAnglesOffset, rdParams.pelvisAnglesOffset);
			VectorCopy(rdParamst->pelvisPositionOffset, rdParams.pelvisPositionOffset);

			rdParams.fImpactStrength = rdParamst->fImpactStrength;
			rdParams.fShotStrength = rdParamst->fShotStrength;
			rdParams.me = rdParamst->me;

			rdParams.startFrame = rdParamst->startFrame;
			rdParams.endFrame = rdParamst->endFrame;

			rdParams.collisionType = rdParamst->collisionType;
			rdParams.CallRagDollBegin = rdParamst->CallRagDollBegin;

			rdParams.RagPhase = (CRagDollParams::ERagPhase)rdParamst->RagPhase;
			rdParams.effectorsToTurnOff = (CRagDollParams::ERagEffector)rdParamst->effectorsToTurnOff;

			re.G2API_SetRagDoll(*((CGhoul2Info_v *)args[1]), &rdParams);
		}
		return 0;
		break;
	case CG_G2_ANIMATEG2MODELS:
		{
			sharedRagDollUpdateParams_t *rduParamst = (sharedRagDollUpdateParams_t *)VMA(3);
			CRagDollUpdateParams rduParams;

			if (!rduParamst)
			{
				return 0;
			}

			VectorCopy(rduParamst->angles, rduParams.angles);
			VectorCopy(rduParamst->position, rduParams.position);
			VectorCopy(rduParamst->scale, rduParams.scale);
			VectorCopy(rduParamst->velocity, rduParams.velocity);

			rduParams.me = rduParamst->me;
			rduParams.settleFrame = rduParamst->settleFrame;

			re.G2API_AnimateG2ModelsRag(*((CGhoul2Info_v *)args[1]), args[2], &rduParams);
		}
		return 0;
		break;

	//additional ragdoll options -rww
	case CG_G2_RAGPCJCONSTRAINT:
		return re.G2API_RagPCJConstraint(*((CGhoul2Info_v *)args[1]), (const char *)VMA(2), (float *)VMA(3), (float *)VMA(4));
	case CG_G2_RAGPCJGRADIENTSPEED:
		return re.G2API_RagPCJGradientSpeed(*((CGhoul2Info_v *)args[1]), (const char *)VMA(2), VMF(3));
	case CG_G2_RAGEFFECTORGOAL:
		return re.G2API_RagEffectorGoal(*((CGhoul2Info_v *)args[1]), (const char *)VMA(2), (float *)VMA(3));
	case CG_G2_GETRAGBONEPOS:
		return re.G2API_GetRagBonePos(*((CGhoul2Info_v *)args[1]), (const char *)VMA(2), (float *)VMA(3), (float *)VMA(4), (float *)VMA(5), (float *)VMA(6));
	case CG_G2_RAGEFFECTORKICK:
		return re.G2API_RagEffectorKick(*((CGhoul2Info_v *)args[1]), (const char *)VMA(2), (float *)VMA(3));
	case CG_G2_RAGFORCESOLVE:
		return re.G2API_RagForceSolve(*((CGhoul2Info_v *)args[1]), (qboolean)args[2]);

	case CG_G2_SETBONEIKSTATE:
		return re.G2API_SetBoneIKState(*((CGhoul2Info_v *)args[1]), args[2], (const char *)VMA(3), args[4], (sharedSetBoneIKStateParams_t *)VMA(5));
	case CG_G2_IKMOVE:
		return re.G2API_IKMove(*((CGhoul2Info_v *)args[1]), args[2], (sharedIKMoveParams_t *)VMA(3));

	case CG_G2_REMOVEBONE:
		{
			CGhoul2Info_v &g2 = *((CGhoul2Info_v *)args[1]);

			return re.G2API_RemoveBone(&g2[args[3]], (const char *)VMA(2));
		}

	case CG_G2_ATTACHINSTANCETOENTNUM:
		{
			re.G2API_AttachInstanceToEntNum(*((CGhoul2Info_v *)args[1]), args[2], (qboolean)args[3]);
		}
		return 0;
	case CG_G2_CLEARATTACHEDINSTANCE:
		re.G2API_ClearAttachedInstance(args[1]);
		return 0;
	case CG_G2_CLEANENTATTACHMENTS:
		re.G2API_CleanEntAttachments();
		return 0;
	case CG_G2_OVERRIDESERVER:
		{
			CGhoul2Info_v &g2 = *((CGhoul2Info_v *)args[1]);
			return re.G2API_OverrideServerWithClientData(&g2[0]);
		}

	case CG_G2_GETSURFACENAME:
		{ //Since returning a pointer in such a way to a VM seems to cause MASSIVE FAILURE<tm>, we will shove data into the pointer the vm passes instead
			char *point = ((char *)VMA(4));
			char *local;
			int modelindex = args[3];

			CGhoul2Info_v &g2 = *((CGhoul2Info_v *)args[1]);

			local = re.G2API_GetSurfaceName(&g2[modelindex], args[2]);
			if (local)
			{
				strcpy(point, local);
			}
		}

		return 0;

	case CG_SP_GETSTRINGTEXTSTRING:
//	case CG_SP_GETSTRINGTEXT:
		const char* text;

		assert(VMA(1));
		assert(VMA(2));

//		if (args[0] == CG_SP_GETSTRINGTEXT)
//		{
//			text = SP_GetStringText( args[1] );
//		}
//		else
		{
			text = SE_GetString( (const char *) VMA(1) );
		}

		if ( text[0] )
		{
			Q_strncpyz( (char *) VMA(2), text, args[3] );
			return qtrue;
		}
		else 
		{
			Com_sprintf( (char *) VMA(2), args[3], "??%s", VMA(1) );
			return qfalse;
		}
		break;

	case CG_SET_SHARED_BUFFER:
		cl.mSharedMemory = ((char *)VMA(1));
		return 0;

	case CG_CM_REGISTER_TERRAIN:
		return CM_RegisterTerrain((const char *)VMA(1), false)->GetTerrainId();

	case CG_RMG_INIT:
		if (!com_sv_running->integer)
		{	// don't do this if we are connected locally
			if (!TheRandomMissionManager)
			{
				TheRandomMissionManager = new CRMManager;
			}
			TheRandomMissionManager->SetLandScape( cmg.landScape );
			if (TheRandomMissionManager->LoadMission(qfalse))
			{
				if (!TheRandomMissionManager->SpawnMission(qfalse))
				{
					Com_Error(ERR_DROP, "Error spawning mission for terrain");
				}
			}
			cmg.landScape->UpdatePatches();
		}
		RM_CreateRandomModels(args[1], (const char *)VMA(2));
//		TheRandomMissionManager->CreateMap();
		return 0;

	case CG_RE_INIT_RENDERER_TERRAIN:
		re.InitRendererTerrain((const char *)VMA(1));
		return 0;

	case CG_R_WEATHER_CONTENTS_OVERRIDE:
		//contentOverride = args[1];
		return 0;

	case CG_R_WORLDEFFECTCOMMAND:
		re.WorldEffectCommand((const char *)VMA(1));
		return 0;

	case CG_WE_ADDWEATHERZONE:
		re.AddWeatherZone( (vec_t *)VMA(1), (vec_t *)VMA(2) );
		return 0;

	default:
	        assert(0); // bk010102
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
void CL_InitCGame( void ) {
	const char			*info;
	const char			*mapname;
//	int					t1, t2;
	vmInterpret_t		interpret;

//	t1 = Sys_Milliseconds();

	// put away the console
	Con_Close();

	// find the current mapname
	info = cl.gameState.stringData + cl.gameState.stringOffsets[ CS_SERVERINFO ];
	mapname = Info_ValueForKey( info, "mapname" );
	Com_sprintf( cl.mapname, sizeof( cl.mapname ), "maps/%s.bsp", mapname );

	// load the dll or bytecode
	if ( cl_connectedToPureServer != 0 ) {
#if 0
		// if sv_pure is set we only allow qvms to be loaded
		interpret = VMI_COMPILED;
#else //load the module type based on what the server is doing -rww
		interpret = (vmInterpret_t)cl_connectedCGAME;
#endif
	}
	else {
		interpret = (vmInterpret_t)(int)Cvar_VariableValue( "vm_cgame" );
	}
	cgvm = VM_Create( "cgame", CL_CgameSystemCalls, interpret );
	if ( !cgvm ) {
		Com_Error( ERR_DROP, "VM_Create on cgame failed" );
	}
	cls.state = CA_LOADING;

	// init for this gamestate
	// use the lastExecutedServerCommand instead of the serverCommandSequence
	// otherwise server commands sent just before a gamestate are dropped
	VM_Call( cgvm, CG_INIT, clc.serverMessageSequence, clc.lastExecutedServerCommand, clc.clientNum );

	// reset any CVAR_CHEAT cvars registered by cgame
	if ( !clc.demoplaying && !cl_connectedToCheatServer )
		Cvar_SetCheatState();

	// we will send a usercmd this frame, which
	// will cause the server to send us the first snapshot
	cls.state = CA_PRIMED;

//	t2 = Sys_Milliseconds();

//	Com_Printf( "CL_InitCGame: %5.2f seconds\n", (t2-t1)/1000.0 );

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
#ifdef _DONETPROFILE_
	ClReadProf().Reset();
#endif
}


/*
====================
CL_GameCommand

See if the current console command is claimed by the cgame
====================
*/
qboolean CL_GameCommand( void ) {
	if ( !cgvm ) {
		return qfalse;
	}

	return (qboolean)VM_Call( cgvm, CG_CONSOLE_COMMAND );
}



/*
=====================
CL_CGameRendering
=====================
*/
void CL_CGameRendering( stereoFrame_t stereo ) {
	//rww - RAGDOLL_BEGIN
	if (!com_sv_running->integer)
	{ //set the server time to match the client time, if we don't have a server going.
		re.G2API_SetTime(cl.serverTime, 0);
	}
	re.G2API_SetTime(cl.serverTime, 1);
	//rww - RAGDOLL_END

	VM_Call( cgvm, CG_DRAW_ACTIVE_FRAME, cl.serverTime, stereo, clc.demoplaying );
	VM_Debug( 0 );
}


/*
=================
CL_AdjustTimeDelta

Adjust the clients view of server time.

We attempt to have cl.serverTime exactly equal the server's view
of time plus the timeNudge, but with variable latencies over
the internet it will often need to drift a bit to match conditions.

Our ideal time would be to have the adjusted time approach, but not pass,
the very latest snapshot.

Adjustments are only made when a new snapshot arrives with a rational
latency, which keeps the adjustment process framerate independent and
prevents massive overadjustment during times of significant packet loss
or bursted delayed packets.
=================
*/

#define	RESET_TIME	500

void CL_AdjustTimeDelta( void ) {
	int		resetTime;
	int		newDelta;
	int		deltaDelta;

	cl.newSnapshots = qfalse;

	// the delta never drifts when replaying a demo
	if ( clc.demoplaying ) {
		return;
	}

	// if the current time is WAY off, just correct to the current value
	if ( com_sv_running->integer ) {
		resetTime = 100;
	} else {
		resetTime = RESET_TIME;
	}

	newDelta = cl.snap.serverTime - cls.realtime;
	deltaDelta = abs( newDelta - cl.serverTimeDelta );

	if ( deltaDelta > RESET_TIME ) {
		cl.serverTimeDelta = newDelta;
		cl.oldServerTime = cl.snap.serverTime;	// FIXME: is this a problem for cgame?
		cl.serverTime = cl.snap.serverTime;
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
	// ignore snapshots that don't have entities
	if ( cl.snap.snapFlags & SNAPFLAG_NOT_ACTIVE ) {
		return;
	}

	re.RegisterMedia_LevelLoadEnd();

	cls.state = CA_ACTIVE;

	// set the timedelta so we are exactly on this first frame
	cl.serverTimeDelta = cl.snap.serverTime - cls.realtime;
	cl.oldServerTime = cl.snap.serverTime;

	clc.timeDemoBaseTime = cl.snap.serverTime;

	// if this is the first frame of active play,
	// execute the contents of activeAction now
	// this is to allow scripting a timedemo to start right
	// after loading
	if ( cl_activeAction->string[0] ) {
		Cbuf_AddText( cl_activeAction->string );
		Cvar_Set( "activeAction", "" );
	}
	
	Sys_BeginProfiling();
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
		if ( clc.demoplaying ) {
			// we shouldn't get the first snapshot on the same frame
			// as the gamestate, because it causes a bad time skip
			if ( !clc.firstDemoFrameSkipped ) {
				clc.firstDemoFrameSkipped = qtrue;
				return;
			}
			CL_ReadDemoMessage();
		}
		if ( cl.newSnapshots ) {
			cl.newSnapshots = qfalse;
			CL_FirstSnapshot();
		}
		if ( cls.state != CA_ACTIVE ) {
			return;
		}
	}	

	// if we have gotten to this point, cl.snap is guaranteed to be valid
	if ( !cl.snap.valid ) {
		Com_Error( ERR_DROP, "CL_SetCGameTime: !cl.snap.valid" );
	}

	// allow pause in single player
	if ( sv_paused->integer && CL_CheckPaused() && com_sv_running->integer ) {
		// paused
		return;
	}

	if ( cl.snap.serverTime < cl.oldFrameServerTime ) {
		Com_Error( ERR_DROP, "cl.snap.serverTime < cl.oldFrameServerTime" );
	}
	cl.oldFrameServerTime = cl.snap.serverTime;


	// get our current view of time

	if ( clc.demoplaying && cl_freezeDemo->integer ) {
		// cl_freezeDemo is used to lock a demo in place for single frame advances
	} else
	{
		// cl_timeNudge is a user adjustable cvar that allows more
		// or less latency to be added in the interest of better 
		// smoothness or better responsiveness.
		int tn;
		
		tn = cl_timeNudge->integer;
#ifdef _DEBUG
		if (tn<-900) {
			tn = -900;
		} else if (tn>900) {
			tn = 900;
		}
#else
		if (tn<-30) {
			tn = -30;
		} else if (tn>30) {
			tn = 30;
		}
#endif

		cl.serverTime = cls.realtime + cl.serverTimeDelta - tn;

		// guarantee that time will never flow backwards, even if
		// serverTimeDelta made an adjustment or cl_timeNudge was changed
		if ( cl.serverTime < cl.oldServerTime ) {
			cl.serverTime = cl.oldServerTime;
		}
		cl.oldServerTime = cl.serverTime;

		// note if we are almost past the latest frame (without timeNudge),
		// so we will try and adjust back a bit when the next snapshot arrives
		if ( cls.realtime + cl.serverTimeDelta >= cl.snap.serverTime - 5 ) {
			cl.extrapolatedSnapshot = qtrue;
		}
	}

	// if we have gotten new snapshots, drift serverTimeDelta
	// don't do this every frame, or a period of packet loss would
	// make a huge adjustment
	if ( cl.newSnapshots ) {
		CL_AdjustTimeDelta();
	}

	if ( !clc.demoplaying ) {
		return;
	}

	// if we are playing a demo back, we can just keep reading
	// messages from the demo file until the cgame definately
	// has valid snapshots to interpolate between

	// a timedemo will always use a deterministic set of time samples
	// no matter what speed machine it is run on,
	// while a normal demo may have different time samples
	// each time it is played back
	if ( cl_timedemo->integer ) {
		if (!clc.timeDemoStart) {
			clc.timeDemoStart = Sys_Milliseconds();
		}
		clc.timeDemoFrames++;
		cl.serverTime = clc.timeDemoBaseTime + clc.timeDemoFrames * 50;
	}

	while ( cl.serverTime >= cl.snap.serverTime ) {
		// feed another messag, which should change
		// the contents of cl.snap
		CL_ReadDemoMessage();
		if ( cls.state != CA_ACTIVE ) {
			return;		// end of demo
		}
	}
}



