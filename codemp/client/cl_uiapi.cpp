/*
===========================================================================
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

// cl_uiapi.c  -- client system interaction with client game
#include "qcommon/RoffSystem.h"
#include "qcommon/stringed_ingame.h"
#include "qcommon/timing.h"
#include "client.h"
#include "cl_lan.h"
#include "botlib/botlib.h"
#include "snd_ambient.h"
#include "FXExport.h"
#include "FxUtil.h"

extern IHeapAllocator *G2VertSpaceClient;
extern botlib_export_t *botlib_export;

// ui interface
static uiExport_t *uie; // ui export table
static vm_t *uivm; // ui vm, valid for legacy and new api

//
// ui vmMain calls
//

void UIVM_Init( qboolean inGameLoad ) {
	if ( uivm->isLegacy ) {
		VM_Call( uivm, UI_INIT, inGameLoad );
		return;
	}
	VMSwap v( uivm );

	uie->Init( inGameLoad );
}

void UIVM_Shutdown( void ) {
	if ( uivm->isLegacy ) {
		VM_Call( uivm, UI_SHUTDOWN );
		VM_Call( uivm, UI_MENU_RESET );
		return;
	}
	VMSwap v( uivm );

	uie->Shutdown();
	uie->MenuReset();
}

void UIVM_KeyEvent( int key, qboolean down ) {
	if ( uivm->isLegacy ) {
		VM_Call( uivm, UI_KEY_EVENT, key, down );
		return;
	}
	VMSwap v( uivm );

	uie->KeyEvent( key, down );
}

void UIVM_MouseEvent( int dx, int dy ) {
	if ( uivm->isLegacy ) {
		VM_Call( uivm, UI_MOUSE_EVENT, dx, dy );
		return;
	}
	VMSwap v( uivm );

	uie->MouseEvent( dx, dy );
}

void UIVM_Refresh( int realtime ) {
	if ( uivm->isLegacy ) {
		VM_Call( uivm, UI_REFRESH, realtime );
		return;
	}
	VMSwap v( uivm );

	uie->Refresh( realtime );
}

qboolean UIVM_IsFullscreen( void ) {
	if ( uivm->isLegacy ) {
		return (qboolean)VM_Call( uivm, UI_IS_FULLSCREEN );
	}
	VMSwap v( uivm );

	return uie->IsFullscreen();
}

void UIVM_SetActiveMenu( uiMenuCommand_t menu ) {
	if ( uivm->isLegacy ) {
		VM_Call( uivm, UI_SET_ACTIVE_MENU, menu );
		return;
	}
	VMSwap v( uivm );

	uie->SetActiveMenu( menu );
}

qboolean UIVM_ConsoleCommand( int realTime ) {
	if ( uivm->isLegacy ) {
		return (qboolean)VM_Call( uivm, UI_CONSOLE_COMMAND, realTime );
	}
	VMSwap v( uivm );

	return uie->ConsoleCommand( realTime );
}
void UIVM_DrawConnectScreen( qboolean overlay ) {
	if ( uivm->isLegacy ) {
		VM_Call( uivm, UI_DRAW_CONNECT_SCREEN, overlay );
		return;
	}
	VMSwap v( uivm );

	uie->DrawConnectScreen( overlay );
}

//
// ui syscalls
//	only used by legacy mods!
//

// wrappers and such

static int CL_Milliseconds( void ) {
	return Sys_Milliseconds();
}

static void CL_Cvar_Get( const char *var_name, const char *value, uint32_t flags ) {
	Cvar_Register( NULL, var_name, value, flags );
}

static void CL_GetClientState( uiClientState_t *state ) {
	state->connectPacketCount = clc.connectPacketCount;
	state->connState = cls.state;
	Q_strncpyz( state->servername, cls.servername, sizeof( state->servername ) );
	Q_strncpyz( state->updateInfoString, cls.updateInfoString, sizeof( state->updateInfoString ) );
	Q_strncpyz( state->messageString, clc.serverMessage, sizeof( state->messageString ) );
	state->clientNum = cl.snap.ps.clientNum;
}

static void CL_GetGlconfig( glconfig_t *config ) {
	*config = cls.glconfig;
}

static void GetClipboardData( char *buf, int buflen ) {
	char	*cbd, *c;

	c = cbd = Sys_GetClipboardData();
	if ( !cbd ) {
		*buf = 0;
		return;
	}

	for ( int i = 0, end = buflen - 1; *c && i < end; i++ )
	{
		uint32_t utf32 = ConvertUTF8ToUTF32( c, &c );
		buf[i] = ConvertUTF32ToExpectedCharset( utf32 );
	}

	Z_Free( cbd );
}

static int GetConfigString(int index, char *buf, int size)
{
	int		offset;

	if (index < 0 || index >= MAX_CONFIGSTRINGS)
		return qfalse;

	offset = cl.gameState.stringOffsets[index];
	if (!offset) {
		if( size ) {
			buf[0] = 0;
		}
		return qfalse;
	}

	Q_strncpyz( buf, cl.gameState.stringData+offset, size);

	return qtrue;
}

static void Key_GetBindingBuf( int keynum, char *buf, int buflen ) {
	char	*value;

	value = Key_GetBinding( keynum );
	if ( value ) {
		Q_strncpyz( buf, value, buflen );
	}
	else {
		*buf = 0;
	}
}

static void Key_KeynumToStringBuf( int keynum, char *buf, int buflen )
{
	const char *psKeyName = Key_KeynumToString( keynum/*, qtrue */);

	// see if there's a more friendly (or localised) name...
	//
	const char *psKeyNameFriendly = SE_GetString( va("KEYNAMES_KEYNAME_%s",psKeyName) );

	Q_strncpyz( buf, (psKeyNameFriendly && psKeyNameFriendly[0]) ? psKeyNameFriendly : psKeyName, buflen );
}

static void CL_SE_GetLanguageName( const int languageIndex, char *buffer ) {
	Q_strncpyz( buffer, SE_GetLanguageName( languageIndex ), 128 );
}

static qboolean CL_SE_GetStringTextString( const char *text, char *buffer, int bufferLength ) {
	assert( text && buffer );
	Q_strncpyz( buffer, SE_GetString( text ), bufferLength );
	return qtrue;
}

static void CL_R_ShaderNameFromIndex( char *name, int index ) {
	const char *retMem = re->ShaderNameFromIndex( index );
	if ( retMem )
		strcpy( name, retMem );
	else
		name[0] = '\0';
}

static void CL_G2API_ListModelSurfaces( void *ghlInfo ) {
	if ( !ghlInfo ) {
		return;
	}

	re->G2API_ListSurfaces( (CGhoul2Info *)ghlInfo );
}

static void CL_G2API_ListModelBones( void *ghlInfo, int frame ) {
	if ( !ghlInfo ) {
		return;
	}

	re->G2API_ListBones( (CGhoul2Info *)ghlInfo, frame );
}

static void CL_G2API_SetGhoul2ModelIndexes( void *ghoul2, qhandle_t *modelList, qhandle_t *skinList ) {
	if ( !ghoul2 ) {
		return;
	}

	re->G2API_SetGhoul2ModelIndexes( *((CGhoul2Info_v *)ghoul2), modelList, skinList );
}

static qboolean CL_G2API_HaveWeGhoul2Models( void *ghoul2) {
	if ( !ghoul2 ) {
		return qfalse;
	}

	return re->G2API_HaveWeGhoul2Models( *((CGhoul2Info_v *)ghoul2) );
}

static qboolean CL_G2API_GetBoltMatrix( void *ghoul2, const int modelIndex, const int boltIndex, mdxaBone_t *matrix, const vec3_t angles, const vec3_t position, const int frameNum, qhandle_t *modelList, vec3_t scale ) {
	if ( !ghoul2 ) {
		return qfalse;
	
	}
	return re->G2API_GetBoltMatrix( *((CGhoul2Info_v *)ghoul2), modelIndex, boltIndex, matrix, angles, position, frameNum, modelList, scale );
}

static qboolean CL_G2API_GetBoltMatrix_NoReconstruct( void *ghoul2, const int modelIndex, const int boltIndex, mdxaBone_t *matrix, const vec3_t angles, const vec3_t position, const int frameNum, qhandle_t *modelList, vec3_t scale ) {
	if ( !ghoul2 ) {
		return qfalse;
	}

	re->G2API_BoltMatrixReconstruction( qfalse );
	return re->G2API_GetBoltMatrix( *((CGhoul2Info_v *)ghoul2), modelIndex, boltIndex, matrix, angles, position, frameNum, modelList, scale );
}

static qboolean CL_G2API_GetBoltMatrix_NoRecNoRot( void *ghoul2, const int modelIndex, const int boltIndex, mdxaBone_t *matrix, const vec3_t angles, const vec3_t position, const int frameNum, qhandle_t *modelList, vec3_t scale ) {
	if ( !ghoul2 ) {
		return qfalse;
	}

	re->G2API_BoltMatrixReconstruction( qfalse );
	re->G2API_BoltMatrixSPMethod( qtrue );
	return re->G2API_GetBoltMatrix( *((CGhoul2Info_v *)ghoul2), modelIndex, boltIndex, matrix, angles, position, frameNum, modelList, scale );
}

static int CL_G2API_InitGhoul2Model( void **ghoul2Ptr, const char *fileName, int modelIndex, qhandle_t customSkin, qhandle_t customShader, int modelFlags, int lodBias ) {
	if ( !ghoul2Ptr ) {
		return 0;
	}

#ifdef _FULL_G2_LEAK_CHECKING
		g_G2AllocServer = 0;
#endif
	return re->G2API_InitGhoul2Model( (CGhoul2Info_v **)ghoul2Ptr, fileName, modelIndex, customSkin, customShader, modelFlags, lodBias );
}

static qboolean CL_G2API_SetSkin( void *ghoul2, int modelIndex, qhandle_t customSkin, qhandle_t renderSkin ) {
	if ( !ghoul2 ) {
		return qfalse;
	}

	CGhoul2Info_v &g2 = *((CGhoul2Info_v *)ghoul2);
	return re->G2API_SetSkin( g2, modelIndex, customSkin, renderSkin );
}

static void CL_G2API_CollisionDetect(
	CollisionRecord_t *collRecMap,
	void* ghoul2,
	const vec3_t angles,
	const vec3_t position,
	int frameNumber,
	int entNum,
	vec3_t rayStart,
	vec3_t rayEnd,
	vec3_t scale,
	int traceFlags,
	int useLod,
	float fRadius )
{
	if ( !ghoul2 ) {
		return;
	}

	re->G2API_CollisionDetect(
		collRecMap,
		*((CGhoul2Info_v *)ghoul2),
		angles,
		position,
		frameNumber,
		entNum,
		rayStart,
		rayEnd,
		scale,
		G2VertSpaceClient,
		traceFlags,
		useLod,
		fRadius);
}

static void CL_G2API_CollisionDetectCache(
	CollisionRecord_t *collRecMap,
	void* ghoul2,
	const vec3_t angles,
	const vec3_t position,
	int frameNumber,
	int entNum,
	vec3_t rayStart,
	vec3_t rayEnd,
	vec3_t scale,
	int traceFlags,
	int useLod,
	float fRadius)
{
	if ( !ghoul2 ) {
		return;
	}

	re->G2API_CollisionDetectCache(
		collRecMap,
		*((CGhoul2Info_v *)ghoul2),
		angles,
		position,
		frameNumber,
		entNum,
		rayStart,
		rayEnd,
		scale,
		G2VertSpaceClient,
		traceFlags,
		useLod,
		fRadius);
}

static void CL_G2API_CleanGhoul2Models( void **ghoul2Ptr ) {
	if ( !ghoul2Ptr ) {
		return;
	}

#ifdef _FULL_G2_LEAK_CHECKING
		g_G2AllocServer = 0;
#endif
	re->G2API_CleanGhoul2Models( (CGhoul2Info_v **)ghoul2Ptr );
}

static qboolean CL_G2API_SetBoneAngles(
	void *ghoul2,
	int modelIndex,
	const char *boneName,
	const vec3_t angles,
	const int flags,
	const int up,
	const int right,
	const int forward,
	qhandle_t *modelList,
	int blendTime ,
	int currentTime)
{
	if ( !ghoul2 ) {
		return qfalse;
	}

	return re->G2API_SetBoneAngles(
		*((CGhoul2Info_v *)ghoul2),
		modelIndex,
		boneName,
		angles,
		flags,
		(const Eorientations)up,
		(const Eorientations)right,
		(const Eorientations)forward,
		modelList,
		blendTime,
		currentTime);
}

static qboolean CL_G2API_SetBoneAnim(
	void *ghoul2,
	const int modelIndex,
	const char *boneName,
	const int startFrame,
	const int endFrame,
	const int flags,
	const float animSpeed,
	const int currentTime,
	const float setFrame,
	const int blendTime)
{
	if ( !ghoul2 ) {
		return qfalse;
	}

	return re->G2API_SetBoneAnim(
		*((CGhoul2Info_v *)ghoul2),
		modelIndex,
		boneName,
		startFrame,
		endFrame,
		flags,
		animSpeed,
		currentTime,
		setFrame,
		blendTime);
}

static qboolean CL_G2API_GetBoneAnim(
	void *ghoul2,
	const char *boneName,
	const int currentTime,
	float *currentFrame,
	int *startFrame,
	int *endFrame,
	int *flags,
	float *animSpeed,
	int *modelList,
	const int modelIndex)
{
	if ( !ghoul2 ) {
		return qfalse;
	}

	CGhoul2Info_v &g2 = *((CGhoul2Info_v *)ghoul2);
	return re->G2API_GetBoneAnim(
		g2,
		modelIndex,
		boneName,
		currentTime,
		currentFrame,
		startFrame,
		endFrame,
		flags,
		animSpeed,
		modelList);
}

static qboolean CL_G2API_GetBoneFrame(
	void *ghoul2,
	const char *boneName,
	const int currentTime,
	float *currentFrame,
	int *modelList,
	const int modelIndex)
{
	if ( !ghoul2 ) {
		return qfalse;
	}

	CGhoul2Info_v &g2 = *((CGhoul2Info_v *)ghoul2);
	int iDontCare1 = 0, iDontCare2 = 0, iDontCare3 = 0;
	float fDontCare1 = 0;

	return re->G2API_GetBoneAnim(
		g2,
		modelIndex,
		boneName,
		currentTime,
		currentFrame,
		&iDontCare1,
		&iDontCare2,
		&iDontCare3,
		&fDontCare1,
		modelList);
}

static void CL_G2API_GetGLAName( void *ghoul2, int modelIndex, char *fillBuf ) {
	if ( !ghoul2 )
	{
		fillBuf[0] = '\0';
		return;
	}

	char *tmp = re->G2API_GetGLAName( *((CGhoul2Info_v *)ghoul2), modelIndex );
	if ( tmp )
		strcpy( fillBuf, tmp );
	else
		fillBuf[0] = '\0';
}

static int CL_G2API_CopyGhoul2Instance( void *g2From, void *g2To, int modelIndex ) {
	if ( !g2From || !g2To ) {
		return 0;
	}

	return re->G2API_CopyGhoul2Instance( *((CGhoul2Info_v *)g2From), *((CGhoul2Info_v *)g2To), modelIndex );
}

static void CL_G2API_CopySpecificGhoul2Model( void *g2From, int modelFrom, void *g2To, int modelTo ) {
	if ( !g2From || !g2To ) {
		return;
	}

	re->G2API_CopySpecificG2Model( *((CGhoul2Info_v *)g2From), modelFrom, *((CGhoul2Info_v *)g2To), modelTo );
}

static void CL_G2API_DuplicateGhoul2Instance( void *g2From, void **g2To ) {
	if ( !g2From || !g2To ) {
		return;
	}

#ifdef _FULL_G2_LEAK_CHECKING
		g_G2AllocServer = 0;
#endif
	re->G2API_DuplicateGhoul2Instance( *((CGhoul2Info_v *)g2From), (CGhoul2Info_v **)g2To );
}

static qboolean CL_G2API_HasGhoul2ModelOnIndex( void *ghlInfo, int modelIndex ) {
	if ( !ghlInfo ) {
		return qfalse;
	}

	return re->G2API_HasGhoul2ModelOnIndex( (CGhoul2Info_v **)ghlInfo, modelIndex );
}

static qboolean CL_G2API_RemoveGhoul2Model( void *ghlInfo, int modelIndex ) {
	if ( !ghlInfo ) {
		return qfalse;
	}

#ifdef _FULL_G2_LEAK_CHECKING
		g_G2AllocServer = 0;
#endif
	return re->G2API_RemoveGhoul2Model( (CGhoul2Info_v **)ghlInfo, modelIndex );
}

static int CL_G2API_AddBolt( void *ghoul2, int modelIndex, const char *boneName ) {
	if ( !ghoul2 ) {
		return -1;
	}

	return re->G2API_AddBolt( *((CGhoul2Info_v *)ghoul2), modelIndex, boneName );
}

static void CL_G2API_SetBoltInfo( void *ghoul2, int modelIndex, int boltInfo ) {
	if ( !ghoul2 ) {
		return;
	}

	re->G2API_SetBoltInfo( *((CGhoul2Info_v *)ghoul2), modelIndex, boltInfo );
}

static qboolean CL_G2API_SetRootSurface( void *ghoul2, const int modelIndex, const char *surfaceName ) {
	if ( !ghoul2 ) {
		return qfalse;
	}

	return re->G2API_SetRootSurface( *((CGhoul2Info_v *)ghoul2), modelIndex, surfaceName );
}

static qboolean CL_G2API_SetSurfaceOnOff( void *ghoul2, const char *surfaceName, const int flags ) {
	if ( !ghoul2 ) {
		return qfalse;
	}

	return re->G2API_SetSurfaceOnOff( *((CGhoul2Info_v *)ghoul2), surfaceName, flags );
}

static qboolean CL_G2API_SetNewOrigin( void *ghoul2, const int boltIndex ) {
	if ( !ghoul2 ) {
		return qfalse;
	}

	return re->G2API_SetNewOrigin( *((CGhoul2Info_v *)ghoul2), boltIndex );
}

static int CL_G2API_GetTime( void ) {
	return re->G2API_GetTime( 0 );
}

static void CL_G2API_SetTime( int time, int clock ) {
	re->G2API_SetTime( time, clock );
}

static void CL_G2API_SetRagDoll( void *ghoul2, sharedRagDollParams_t *params ) {
	if ( !ghoul2 ) {
		return;
	}

	CRagDollParams rdParams;

	if ( !params ) {
		re->G2API_ResetRagDoll( *((CGhoul2Info_v *)ghoul2) );
		return;
	}

	VectorCopy( params->angles, rdParams.angles );
	VectorCopy( params->position, rdParams.position );
	VectorCopy( params->scale, rdParams.scale );
	VectorCopy( params->pelvisAnglesOffset, rdParams.pelvisAnglesOffset );
	VectorCopy( params->pelvisPositionOffset, rdParams.pelvisPositionOffset );

	rdParams.fImpactStrength = params->fImpactStrength;
	rdParams.fShotStrength = params->fShotStrength;
	rdParams.me = params->me;

	rdParams.startFrame = params->startFrame;
	rdParams.endFrame = params->endFrame;

	rdParams.collisionType = params->collisionType;
	rdParams.CallRagDollBegin = params->CallRagDollBegin;

	rdParams.RagPhase = (CRagDollParams::ERagPhase)params->RagPhase;
	rdParams.effectorsToTurnOff = (CRagDollParams::ERagEffector)params->effectorsToTurnOff;

	re->G2API_SetRagDoll( *((CGhoul2Info_v *)ghoul2), &rdParams );
}

static void CL_G2API_AnimateG2Models( void *ghoul2, int time, sharedRagDollUpdateParams_t *params ) {
	if ( !ghoul2 ) {
		return;
	}

	CRagDollUpdateParams rduParams;

	if ( !params )
		return;

	VectorCopy( params->angles, rduParams.angles );
	VectorCopy( params->position, rduParams.position );
	VectorCopy( params->scale, rduParams.scale );
	VectorCopy( params->velocity, rduParams.velocity );

	rduParams.me = params->me;
	rduParams.settleFrame = params->settleFrame;

	re->G2API_AnimateG2ModelsRag( *((CGhoul2Info_v *)ghoul2), time, &rduParams );
}

static qboolean CL_G2API_SetBoneIKState( void *ghoul2, int time, const char *boneName, int ikState, sharedSetBoneIKStateParams_t *params ) {
	if ( !ghoul2 ) {
		return qfalse;
	}

	return re->G2API_SetBoneIKState( *((CGhoul2Info_v *)ghoul2), time, boneName, ikState, params );
}

static qboolean CL_G2API_IKMove( void *ghoul2, int time, sharedIKMoveParams_t *params ) {
	if ( !ghoul2 ) {
		return qfalse;
	}

	return re->G2API_IKMove( *((CGhoul2Info_v *)ghoul2), time, params );
}

static void CL_G2API_GetSurfaceName( void *ghoul2, int surfNumber, int modelIndex, char *fillBuf ) {
	if ( !ghoul2 ) {
		return;
	}

	CGhoul2Info_v &g2 = *((CGhoul2Info_v *)ghoul2);
	char *tmp = re->G2API_GetSurfaceName( g2, modelIndex, surfNumber );
	strcpy( fillBuf, tmp );
}

static qboolean CL_G2API_AttachG2Model( void *ghoul2From, int modelIndexFrom, void *ghoul2To, int toBoltIndex, int toModel ) {
	if ( !ghoul2From || !ghoul2To ) {
		return qfalse;
	}

	CGhoul2Info_v *g2From = ((CGhoul2Info_v *)ghoul2From);
	CGhoul2Info_v *g2To = ((CGhoul2Info_v *)ghoul2To);

	return re->G2API_AttachG2Model(*g2From, modelIndexFrom, *g2To, toBoltIndex, toModel);
}

static void CL_Key_SetCatcher( int catcher ) {
	// Don't allow the ui module to close the console
	Key_SetCatcher( catcher | ( Key_GetCatcher( ) & KEYCATCH_CONSOLE ) );
}

static void UIVM_Cvar_Set( const char *var_name, const char *value ) {
	Cvar_VM_Set( var_name, value, VM_UI );
}

static void UIVM_Cvar_SetValue( const char *var_name, float value ) {
	Cvar_VM_SetValue( var_name, value, VM_UI );
}

static void CL_AddUICommand( const char *cmdName ) {
	Cmd_AddCommand( cmdName, NULL );
}

static void UIVM_Cmd_RemoveCommand( const char *cmd_name ) {
	Cmd_VM_RemoveCommand( cmd_name, VM_UI );
}

// legacy syscall

intptr_t CL_UISystemCalls( intptr_t *args ) {
	switch ( args[0] ) {
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

	case UI_ERROR:
		Com_Error( ERR_DROP, "%s", VMA(1) );
		return 0;

	case UI_PRINT:
		Com_Printf( "%s", VMA(1) );
		return 0;

	case UI_MILLISECONDS:
		return Sys_Milliseconds();

	case UI_CVAR_REGISTER:
		Cvar_Register( (vmCvar_t *)VMA(1), (const char *)VMA(2), (const char *)VMA(3), args[4] );
		return 0;

	case UI_CVAR_UPDATE:
		Cvar_Update( (vmCvar_t *)VMA(1) );
		return 0;

	case UI_CVAR_SET:
		Cvar_VM_Set( (const char *)VMA(1), (const char *)VMA(2), VM_UI );
		return 0;

	case UI_CVAR_VARIABLEVALUE:
		return FloatAsInt( Cvar_VariableValue( (const char *)VMA(1) ) );

	case UI_CVAR_VARIABLESTRINGBUFFER:
		Cvar_VariableStringBuffer( (const char *)VMA(1), (char *)VMA(2), args[3] );
		return 0;

	case UI_CVAR_SETVALUE:
		Cvar_VM_SetValue( (const char *)VMA(1), VMF(2), VM_UI );
		return 0;

	case UI_CVAR_RESET:
		Cvar_Reset( (const char *)VMA(1) );
		return 0;

	case UI_CVAR_CREATE:
		Cvar_Get( (const char *)VMA(1), (const char *)VMA(2), args[3] );
		return 0;

	case UI_CVAR_INFOSTRINGBUFFER:
		Cvar_InfoStringBuffer( args[1], (char *)VMA(2), args[3] );
		return 0;

	case UI_ARGC:
		return Cmd_Argc();

	case UI_ARGV:
		Cmd_ArgvBuffer( args[1], (char *)VMA(2), args[3] );
		return 0;

	case UI_CMD_EXECUTETEXT:
		Cbuf_ExecuteText( args[1], (const char *)VMA(2) );
		return 0;

	case UI_FS_FOPENFILE:
		return FS_FOpenFileByMode( (const char *)VMA(1), (int *)VMA(2), (fsMode_t)args[3] );

	case UI_FS_READ:
		FS_Read( VMA(1), args[2], args[3] );
		return 0;

	case UI_FS_WRITE:
		FS_Write( VMA(1), args[2], args[3] );
		return 0;

	case UI_FS_FCLOSEFILE:
		FS_FCloseFile( args[1] );
		return 0;

	case UI_FS_GETFILELIST:
		return FS_GetFileList( (const char *)VMA(1), (const char *)VMA(2), (char *)VMA(3), args[4] );

	case UI_R_REGISTERMODEL:
		return re->RegisterModel( (const char *)VMA(1) );

	case UI_R_REGISTERSKIN:
		return re->RegisterSkin( (const char *)VMA(1) );

	case UI_R_REGISTERSHADERNOMIP:
		return re->RegisterShaderNoMip( (const char *)VMA(1) );

	case UI_R_SHADERNAMEFROMINDEX:
		CL_R_ShaderNameFromIndex( (char *)VMA(1), args[2] );
		return 0;

	case UI_R_CLEARSCENE:
		re->ClearScene();
		return 0;

	case UI_R_ADDREFENTITYTOSCENE:
		re->AddRefEntityToScene( (const refEntity_t *)VMA(1) );
		return 0;

	case UI_R_ADDPOLYTOSCENE:
		re->AddPolyToScene( args[1], args[2], (const polyVert_t *)VMA(3), 1 );
		return 0;

	case UI_R_ADDLIGHTTOSCENE:
		re->AddLightToScene( (const float *)VMA(1), VMF(2), VMF(3), VMF(4), VMF(5) );
		return 0;

	case UI_R_RENDERSCENE:
		re->RenderScene( (const refdef_t *)VMA(1) );
		return 0;

	case UI_R_SETCOLOR:
		re->SetColor( (const float *)VMA(1) );
		return 0;

	case UI_R_DRAWSTRETCHPIC:
		re->DrawStretchPic( VMF(1), VMF(2), VMF(3), VMF(4), VMF(5), VMF(6), VMF(7), VMF(8), args[9] );
		return 0;

	case UI_R_MODELBOUNDS:
		re->ModelBounds( args[1], (float *)VMA(2), (float *)VMA(3) );
		return 0;

	case UI_UPDATESCREEN:
		SCR_UpdateScreen();
		return 0;

	case UI_CM_LERPTAG:
		re->LerpTag( (orientation_t *)VMA(1), args[2], args[3], args[4], VMF(5), (const char *)VMA(6) );
		return 0;

	case UI_S_REGISTERSOUND:
		return S_RegisterSound( (const char *)VMA(1) );

	case UI_S_STARTLOCALSOUND:
		S_StartLocalSound( args[1], args[2] );
		return 0;

	case UI_KEY_KEYNUMTOSTRINGBUF:
		Key_KeynumToStringBuf( args[1], (char *)VMA(2), args[3] );
		return 0;

	case UI_KEY_GETBINDINGBUF:
		Key_GetBindingBuf( args[1], (char *)VMA(2), args[3] );
		return 0;

	case UI_KEY_SETBINDING:
		Key_SetBinding( args[1], (const char *)VMA(2) );
		return 0;

	case UI_KEY_ISDOWN:
		return Key_IsDown( args[1] );

	case UI_KEY_GETOVERSTRIKEMODE:
		return Key_GetOverstrikeMode();

	case UI_KEY_SETOVERSTRIKEMODE:
		Key_SetOverstrikeMode( (qboolean)args[1] );
		return 0;

	case UI_KEY_CLEARSTATES:
		Key_ClearStates();
		return 0;

	case UI_KEY_GETCATCHER:
		return Key_GetCatcher();

	case UI_KEY_SETCATCHER:
		CL_Key_SetCatcher( args[1] );
		return 0;

	case UI_GETCLIPBOARDDATA:
		GetClipboardData( (char *)VMA(1), args[2] );
		return 0;

	case UI_GETCLIENTSTATE:
		CL_GetClientState( (uiClientState_t *)VMA(1) );
		return 0;

	case UI_GETGLCONFIG:
		CL_GetGlconfig( (glconfig_t *)VMA(1) );
		return 0;

	case UI_GETCONFIGSTRING:
		return GetConfigString( args[1], (char *)VMA(2), args[3] );

	case UI_LAN_LOADCACHEDSERVERS:
		LAN_LoadCachedServers();
		return 0;

	case UI_LAN_SAVECACHEDSERVERS:
		LAN_SaveServersToCache();
		return 0;

	case UI_LAN_ADDSERVER:
		return LAN_AddServer(args[1], (const char *)VMA(2), (const char *)VMA(3));

	case UI_LAN_REMOVESERVER:
		LAN_RemoveServer(args[1], (const char *)VMA(2));
		return 0;

	case UI_LAN_GETPINGQUEUECOUNT:
		return LAN_GetPingQueueCount();

	case UI_LAN_CLEARPING:
		LAN_ClearPing( args[1] );
		return 0;

	case UI_LAN_GETPING:
		LAN_GetPing( args[1], (char *)VMA(2), args[3], (int *)VMA(4) );
		return 0;

	case UI_LAN_GETPINGINFO:
		LAN_GetPingInfo( args[1], (char *)VMA(2), args[3] );
		return 0;

	case UI_LAN_GETSERVERCOUNT:
		return LAN_GetServerCount(args[1]);

	case UI_LAN_GETSERVERADDRESSSTRING:
		LAN_GetServerAddressString( args[1], args[2], (char *)VMA(3), args[4] );
		return 0;

	case UI_LAN_GETSERVERINFO:
		LAN_GetServerInfo( args[1], args[2], (char *)VMA(3), args[4] );
		return 0;

	case UI_LAN_GETSERVERPING:
		return LAN_GetServerPing( args[1], args[2] );

	case UI_LAN_MARKSERVERVISIBLE:
		LAN_MarkServerVisible( args[1], args[2], (qboolean)args[3] );
		return 0;

	case UI_LAN_SERVERISVISIBLE:
		return LAN_ServerIsVisible( args[1], args[2] );

	case UI_LAN_UPDATEVISIBLEPINGS:
		return LAN_UpdateVisiblePings( args[1] );

	case UI_LAN_RESETPINGS:
		LAN_ResetPings( args[1] );
		return 0;

	case UI_LAN_SERVERSTATUS:
		return LAN_GetServerStatus( (char *)VMA(1), (char *)VMA(2), args[3] );

	case UI_LAN_COMPARESERVERS:
		return LAN_CompareServers( args[1], args[2], args[3], args[4], args[5] );

	case UI_MEMORY_REMAINING:
		return Hunk_MemoryRemaining();

	case UI_R_REGISTERFONT:
		return re->RegisterFont( (const char *)VMA(1) );

	case UI_R_FONT_STRLENPIXELS:
		return re->Font_StrLenPixels( (const char *)VMA(1), args[2], VMF(3) );

	case UI_R_FONT_STRLENCHARS:
		return re->Font_StrLenChars( (const char *)VMA(1) );

	case UI_R_FONT_STRHEIGHTPIXELS:
		return re->Font_HeightPixels( args[1], VMF(2) );

	case UI_R_FONT_DRAWSTRING:
		re->Font_DrawString( args[1], args[2], (const char *)VMA(3), (const float *) VMA(4), args[5], args[6], VMF(7) );
		return 0;

	case UI_LANGUAGE_ISASIAN:
		return re->Language_IsAsian();

	case UI_LANGUAGE_USESSPACES:
		return re->Language_UsesSpaces();

	case UI_ANYLANGUAGE_READCHARFROMSTRING:
		return re->AnyLanguage_ReadCharFromString( (const char *)VMA(1), (int *) VMA(2), (qboolean *) VMA(3) );

	case UI_PC_ADD_GLOBAL_DEFINE:
		return botlib_export->PC_AddGlobalDefine( (char *)VMA(1) );

	case UI_PC_LOAD_SOURCE:
		return botlib_export->PC_LoadSourceHandle( (const char *)VMA(1) );

	case UI_PC_FREE_SOURCE:
		return botlib_export->PC_FreeSourceHandle( args[1] );

	case UI_PC_READ_TOKEN:
		return botlib_export->PC_ReadTokenHandle( args[1], (struct pc_token_s *)VMA(2) );

	case UI_PC_SOURCE_FILE_AND_LINE:
		return botlib_export->PC_SourceFileAndLine( args[1], (char *)VMA(2), (int *)VMA(3) );

	case UI_PC_LOAD_GLOBAL_DEFINES:
		return botlib_export->PC_LoadGlobalDefines ( (char *)VMA(1) );

	case UI_PC_REMOVE_ALL_GLOBAL_DEFINES:
		botlib_export->PC_RemoveAllGlobalDefines ( );
		return 0;

	case UI_S_STOPBACKGROUNDTRACK:
		S_StopBackgroundTrack();
		return 0;

	case UI_S_STARTBACKGROUNDTRACK:
		S_StartBackgroundTrack( (const char *)VMA(1), (const char *)VMA(2), qfalse );
		return 0;

	case UI_REAL_TIME:
		return Com_RealTime( (struct qtime_s *)VMA(1) );

	case UI_CIN_PLAYCINEMATIC:
		Com_DPrintf("UI_CIN_PlayCinematic\n");
		return CIN_PlayCinematic((const char *)VMA(1), args[2], args[3], args[4], args[5], args[6]);

	case UI_CIN_STOPCINEMATIC:
		return CIN_StopCinematic(args[1]);

	case UI_CIN_RUNCINEMATIC:
		return CIN_RunCinematic(args[1]);

	case UI_CIN_DRAWCINEMATIC:
		CIN_DrawCinematic(args[1]);
		return 0;

	case UI_CIN_SETEXTENTS:
		CIN_SetExtents(args[1], args[2], args[3], args[4], args[5]);
		return 0;

	case UI_R_REMAP_SHADER:
		re->RemapShader( (const char *)VMA(1), (const char *)VMA(2), (const char *)VMA(3) );
		return 0;

	case UI_SP_GETNUMLANGUAGES:
		return SE_GetNumLanguages();

	case UI_SP_GETLANGUAGENAME:
		CL_SE_GetLanguageName( args[1], (char *)VMA(2) );
		return 0;

	case UI_SP_GETSTRINGTEXTSTRING:
		return CL_SE_GetStringTextString( (const char *)VMA(1), (char *)VMA(2), args[3] );

	case UI_G2_LISTSURFACES:
		CL_G2API_ListModelSurfaces( VMA(1) );
		return 0;

	case UI_G2_LISTBONES:
		CL_G2API_ListModelBones( VMA(1), args[2]);
		return 0;

	case UI_G2_HAVEWEGHOULMODELS:
		return CL_G2API_HaveWeGhoul2Models( VMA(1) );

	case UI_G2_SETMODELS:
		CL_G2API_SetGhoul2ModelIndexes( VMA(1),(qhandle_t *)VMA(2),(qhandle_t *)VMA(3));
		return 0;

	case UI_G2_GETBOLT:
		return CL_G2API_GetBoltMatrix(VMA(1), args[2], args[3], (mdxaBone_t *)VMA(4), (const float *)VMA(5),(const float *)VMA(6), args[7], (qhandle_t *)VMA(8), (float *)VMA(9));

	case UI_G2_GETBOLT_NOREC:
		return CL_G2API_GetBoltMatrix_NoReconstruct(VMA(1), args[2], args[3], (mdxaBone_t *)VMA(4), (const float *)VMA(5),(const float *)VMA(6), args[7], (qhandle_t *)VMA(8), (float *)VMA(9));

	case UI_G2_GETBOLT_NOREC_NOROT:
		return CL_G2API_GetBoltMatrix_NoRecNoRot(VMA(1), args[2], args[3], (mdxaBone_t *)VMA(4), (const float *)VMA(5),(const float *)VMA(6), args[7], (qhandle_t *)VMA(8), (float *)VMA(9));

	case UI_G2_INITGHOUL2MODEL:
#ifdef _FULL_G2_LEAK_CHECKING
		g_G2AllocServer = 0;
#endif
		return CL_G2API_InitGhoul2Model((void **)VMA(1), (const char *)VMA(2), args[3], (qhandle_t) args[4], (qhandle_t) args[5], args[6], args[7]);

	case UI_G2_COLLISIONDETECT:
	case UI_G2_COLLISIONDETECTCACHE:
		return 0; //not supported for ui

	case UI_G2_ANGLEOVERRIDE:
		return CL_G2API_SetBoneAngles(VMA(1), args[2], (const char *)VMA(3), (float *)VMA(4), args[5], (const Eorientations) args[6], (const Eorientations) args[7], (const Eorientations) args[8], (qhandle_t *)VMA(9), args[10], args[11] );

	case UI_G2_CLEANMODELS:
#ifdef _FULL_G2_LEAK_CHECKING
		g_G2AllocServer = 0;
#endif
		CL_G2API_CleanGhoul2Models((void **)VMA(1));
		return 0;

	case UI_G2_PLAYANIM:
		return CL_G2API_SetBoneAnim(VMA(1), args[2], (const char *)VMA(3), args[4], args[5], args[6], VMF(7), args[8], VMF(9), args[10]);

	case UI_G2_GETBONEANIM:
		return CL_G2API_GetBoneAnim(VMA(1), (const char*)VMA(2), args[3], (float *)VMA(4), (int *)VMA(5), (int *)VMA(6), (int *)VMA(7), (float *)VMA(8), (int *)VMA(9), args[10]);

	case UI_G2_GETBONEFRAME:
		return CL_G2API_GetBoneFrame(VMA(1), (const char*)VMA(2), args[3], (float *)VMA(4), (int *)VMA(5), args[6]);

	case UI_G2_GETGLANAME:
		CL_G2API_GetGLAName( VMA(1), args[2], (char *)VMA(3) );
		return 0;

	case UI_G2_COPYGHOUL2INSTANCE:
		return (int)CL_G2API_CopyGhoul2Instance(VMA(1), VMA(2), args[3]);

	case UI_G2_COPYSPECIFICGHOUL2MODEL:
		CL_G2API_CopySpecificGhoul2Model(VMA(1), args[2], VMA(3), args[4]);
		return 0;

	case UI_G2_DUPLICATEGHOUL2INSTANCE:
#ifdef _FULL_G2_LEAK_CHECKING
		g_G2AllocServer = 0;
#endif
		CL_G2API_DuplicateGhoul2Instance(VMA(1), (void **)VMA(2));
		return 0;

	case UI_G2_HASGHOUL2MODELONINDEX:
		return (int)CL_G2API_HasGhoul2ModelOnIndex(VMA(1), args[2]);

	case UI_G2_REMOVEGHOUL2MODEL:
#ifdef _FULL_G2_LEAK_CHECKING
		g_G2AllocServer = 0;
#endif
		return (int)CL_G2API_RemoveGhoul2Model(VMA(1), args[2]);

	case UI_G2_ADDBOLT:
		return CL_G2API_AddBolt(VMA(1), args[2], (const char *)VMA(3));

	case UI_G2_SETBOLTON:
		CL_G2API_SetBoltInfo(VMA(1), args[2], args[3]);
		return 0;

	case UI_G2_SETROOTSURFACE:
		return CL_G2API_SetRootSurface(VMA(1), args[2], (const char *)VMA(3));

	case UI_G2_SETSURFACEONOFF:
		return CL_G2API_SetSurfaceOnOff(VMA(1), (const char *)VMA(2), args[3]);

	case UI_G2_SETNEWORIGIN:
		return CL_G2API_SetNewOrigin(VMA(1), args[2]);

	case UI_G2_GETTIME:
		return CL_G2API_GetTime();

	case UI_G2_SETTIME:
		CL_G2API_SetTime(args[1], args[2]);
		return 0;

	case UI_G2_SETRAGDOLL:
		return 0; //not supported for ui
		break;

	case UI_G2_ANIMATEG2MODELS:
		return 0; //not supported for ui
		break;

	case UI_G2_SETBONEIKSTATE:
		return CL_G2API_SetBoneIKState(VMA(1), args[2], (const char *)VMA(3), args[4], (sharedSetBoneIKStateParams_t *)VMA(5));

	case UI_G2_IKMOVE:
		return CL_G2API_IKMove(VMA(1), args[2], (sharedIKMoveParams_t *)VMA(3));

	case UI_G2_GETSURFACENAME:
		CL_G2API_GetSurfaceName( (void *)args[1], args[2], args[3], (char *)VMA( 4 ) );
		return 0;

	case UI_G2_SETSKIN:
		return CL_G2API_SetSkin(VMA(1), args[2], args[3], args[4]);

	case UI_G2_ATTACHG2MODEL:
		return CL_G2API_AttachG2Model(VMA(1), args[2], VMA(3), args[4], args[5]);

	default:
		Com_Error( ERR_DROP, "Bad UI system trap: %ld", (long int) args[0] );

	}

	return 0;
}

void CL_BindUI( void ) {
	static uiImport_t uii;
	uiExport_t		*ret;
	GetUIAPI_t		GetUIAPI;
	char			dllName[MAX_OSPATH] = "ui" ARCH_STRING DLL_EXT;

	memset( &uii, 0, sizeof( uii ) );

	uivm = VM_Create( VM_UI );
	if ( uivm && !uivm->isLegacy ) {
		uii.Print								= Com_Printf;
		uii.Error								= Com_Error;
		uii.Milliseconds						= CL_Milliseconds;
		uii.RealTime							= Com_RealTime;
		uii.MemoryRemaining						= Hunk_MemoryRemaining;

		uii.Cvar_Create							= CL_Cvar_Get;
		uii.Cvar_InfoStringBuffer				= Cvar_InfoStringBuffer;
		uii.Cvar_Register						= Cvar_Register;
		uii.Cvar_Reset							= Cvar_Reset;
		uii.Cvar_Set							= UIVM_Cvar_Set;
		uii.Cvar_SetValue						= UIVM_Cvar_SetValue;
		uii.Cvar_Update							= Cvar_Update;
		uii.Cvar_VariableStringBuffer			= Cvar_VariableStringBuffer;
		uii.Cvar_VariableValue					= Cvar_VariableValue;

		uii.Cmd_Argc							= Cmd_Argc;
		uii.Cmd_Argv							= Cmd_ArgvBuffer;
		uii.Cmd_ExecuteText						= Cbuf_ExecuteText;

		uii.FS_Close							= FS_FCloseFile;
		uii.FS_GetFileList						= FS_GetFileList;
		uii.FS_Open								= FS_FOpenFileByMode;
		uii.FS_Read								= FS_Read;
		uii.FS_Write							= FS_Write;

		uii.GetClientState						= CL_GetClientState;
		uii.GetClipboardData					= GetClipboardData;
		uii.GetConfigString						= GetConfigString;
		uii.GetGlconfig							= CL_GetGlconfig;
		uii.UpdateScreen						= SCR_UpdateScreen;

		uii.Key_ClearStates						= Key_ClearStates;
		uii.Key_GetBindingBuf					= Key_GetBindingBuf;
		uii.Key_IsDown							= Key_IsDown;
		uii.Key_KeynumToStringBuf				= Key_KeynumToStringBuf;
		uii.Key_SetBinding						= Key_SetBinding;
		uii.Key_GetCatcher						= Key_GetCatcher;
		uii.Key_GetOverstrikeMode				= Key_GetOverstrikeMode;
		uii.Key_SetCatcher						= CL_Key_SetCatcher;
		uii.Key_SetOverstrikeMode				= Key_SetOverstrikeMode;

		uii.PC_AddGlobalDefine					= botlib_export->PC_AddGlobalDefine;
		uii.PC_FreeSource						= botlib_export->PC_FreeSourceHandle;
		uii.PC_LoadGlobalDefines				= botlib_export->PC_LoadGlobalDefines;
		uii.PC_LoadSource						= botlib_export->PC_LoadSourceHandle;
		uii.PC_ReadToken						= botlib_export->PC_ReadTokenHandle;
		uii.PC_RemoveAllGlobalDefines			= botlib_export->PC_RemoveAllGlobalDefines;
		uii.PC_SourceFileAndLine				= botlib_export->PC_SourceFileAndLine;

		uii.CIN_DrawCinematic					= CIN_DrawCinematic;
		uii.CIN_PlayCinematic					= CIN_PlayCinematic;
		uii.CIN_RunCinematic					= CIN_RunCinematic;
		uii.CIN_SetExtents						= CIN_SetExtents;
		uii.CIN_StopCinematic					= CIN_StopCinematic;

		uii.LAN_AddServer						= LAN_AddServer;
		uii.LAN_ClearPing						= LAN_ClearPing;
		uii.LAN_CompareServers					= LAN_CompareServers;
		uii.LAN_GetPing							= LAN_GetPing;
		uii.LAN_GetPingInfo						= LAN_GetPingInfo;
		uii.LAN_GetPingQueueCount				= LAN_GetPingQueueCount;
		uii.LAN_GetServerAddressString			= LAN_GetServerAddressString;
		uii.LAN_GetServerCount					= LAN_GetServerCount;
		uii.LAN_GetServerInfo					= LAN_GetServerInfo;
		uii.LAN_GetServerPing					= LAN_GetServerPing;
		uii.LAN_LoadCachedServers				= LAN_LoadCachedServers;
		uii.LAN_MarkServerVisible				= LAN_MarkServerVisible;
		uii.LAN_RemoveServer					= LAN_RemoveServer;
		uii.LAN_ResetPings						= LAN_ResetPings;
		uii.LAN_SaveCachedServers				= LAN_SaveServersToCache;
		uii.LAN_ServerIsVisible					= LAN_ServerIsVisible;
		uii.LAN_ServerStatus					= LAN_GetServerStatus;
		uii.LAN_UpdateVisiblePings				= LAN_UpdateVisiblePings;

		uii.S_StartBackgroundTrack				= S_StartBackgroundTrack;
		uii.S_StartLocalSound					= S_StartLocalSound;
		uii.S_StopBackgroundTrack				= S_StopBackgroundTrack;
		uii.S_RegisterSound						= S_RegisterSound;

		uii.SE_GetLanguageName					= CL_SE_GetLanguageName;
		uii.SE_GetNumLanguages					= SE_GetNumLanguages;
		uii.SE_GetStringTextString				= CL_SE_GetStringTextString;

		uii.R_Language_IsAsian					= re->Language_IsAsian;
		uii.R_Language_UsesSpaces				= re->Language_UsesSpaces;
		uii.R_AnyLanguage_ReadCharFromString	= re->AnyLanguage_ReadCharFromString;

		uii.R_AddLightToScene					= re->AddLightToScene;
		uii.R_AddPolysToScene					= re->AddPolyToScene;
		uii.R_AddRefEntityToScene				= re->AddRefEntityToScene;
		uii.R_ClearScene						= re->ClearScene;
		uii.R_DrawStretchPic					= re->DrawStretchPic;
		uii.R_Font_DrawString					= re->Font_DrawString;
		uii.R_Font_HeightPixels					= re->Font_HeightPixels;
		uii.R_Font_StrLenChars					= re->Font_StrLenChars;
		uii.R_Font_StrLenPixels					= re->Font_StrLenPixels;
		uii.R_LerpTag							= re->LerpTag;
		uii.R_ModelBounds						= re->ModelBounds;
		uii.R_RegisterFont						= re->RegisterFont;
		uii.R_RegisterModel						= re->RegisterModel;
		uii.R_RegisterShaderNoMip				= re->RegisterShaderNoMip;
		uii.R_RegisterSkin						= re->RegisterSkin;
		uii.R_RemapShader						= re->RemapShader;
		uii.R_RenderScene						= re->RenderScene;
		uii.R_SetColor							= re->SetColor;
		uii.R_ShaderNameFromIndex				= CL_R_ShaderNameFromIndex;

		uii.G2_ListModelSurfaces				= CL_G2API_ListModelSurfaces;
		uii.G2_ListModelBones					= CL_G2API_ListModelBones;
		uii.G2_SetGhoul2ModelIndexes			= CL_G2API_SetGhoul2ModelIndexes;
		uii.G2_HaveWeGhoul2Models				= CL_G2API_HaveWeGhoul2Models;
		uii.G2API_GetBoltMatrix					= CL_G2API_GetBoltMatrix;
		uii.G2API_GetBoltMatrix_NoReconstruct	= CL_G2API_GetBoltMatrix_NoReconstruct;
		uii.G2API_GetBoltMatrix_NoRecNoRot		= CL_G2API_GetBoltMatrix_NoRecNoRot;
		uii.G2API_InitGhoul2Model				= CL_G2API_InitGhoul2Model;
		uii.G2API_SetSkin						= CL_G2API_SetSkin;
		uii.G2API_CollisionDetect				= CL_G2API_CollisionDetect;
		uii.G2API_CollisionDetectCache			= CL_G2API_CollisionDetectCache;
		uii.G2API_CleanGhoul2Models				= CL_G2API_CleanGhoul2Models;
		uii.G2API_SetBoneAngles					= CL_G2API_SetBoneAngles;
		uii.G2API_SetBoneAnim					= CL_G2API_SetBoneAnim;
		uii.G2API_GetBoneAnim					= CL_G2API_GetBoneAnim;
		uii.G2API_GetBoneFrame					= CL_G2API_GetBoneFrame;
		uii.G2API_GetGLAName					= CL_G2API_GetGLAName;
		uii.G2API_CopyGhoul2Instance			= CL_G2API_CopyGhoul2Instance;
		uii.G2API_CopySpecificGhoul2Model		= CL_G2API_CopySpecificGhoul2Model;
		uii.G2API_DuplicateGhoul2Instance		= CL_G2API_DuplicateGhoul2Instance;
		uii.G2API_HasGhoul2ModelOnIndex			= CL_G2API_HasGhoul2ModelOnIndex;
		uii.G2API_RemoveGhoul2Model				= CL_G2API_RemoveGhoul2Model;
		uii.G2API_AddBolt						= CL_G2API_AddBolt;
		uii.G2API_SetBoltInfo					= CL_G2API_SetBoltInfo;
		uii.G2API_SetRootSurface				= CL_G2API_SetRootSurface;
		uii.G2API_SetSurfaceOnOff				= CL_G2API_SetSurfaceOnOff;
		uii.G2API_SetNewOrigin					= CL_G2API_SetNewOrigin;
		uii.G2API_GetTime						= CL_G2API_GetTime;
		uii.G2API_SetTime						= CL_G2API_SetTime;
		uii.G2API_SetRagDoll					= CL_G2API_SetRagDoll;
		uii.G2API_AnimateG2Models				= CL_G2API_AnimateG2Models;
		uii.G2API_SetBoneIKState				= CL_G2API_SetBoneIKState;
		uii.G2API_IKMove						= CL_G2API_IKMove;
		uii.G2API_GetSurfaceName				= CL_G2API_GetSurfaceName;
		uii.G2API_AttachG2Model					= CL_G2API_AttachG2Model;

		uii.ext.R_Font_StrLenPixels				= re->ext.Font_StrLenPixels;
		uii.ext.AddCommand						= CL_AddUICommand;
		uii.ext.RemoveCommand					= UIVM_Cmd_RemoveCommand;

		GetUIAPI = (GetUIAPI_t)uivm->GetModuleAPI;
		ret = GetUIAPI( UI_API_VERSION, &uii );
		if ( !ret ) {
			//free VM?
			cls.uiStarted = qfalse;
			Com_Error( ERR_FATAL, "GetGameAPI failed on %s", dllName );
		}
		uie = ret;

		return;
	}

	// fall back to legacy syscall/vm_call api
	uivm = VM_CreateLegacy( VM_UI, CL_UISystemCalls );
	if ( !uivm ) {
		cls.uiStarted = qfalse;
		Com_Error( ERR_DROP, "VM_CreateLegacy on ui failed" );
	}
}

void CL_UnbindUI( void ) {
	UIVM_Shutdown();
	VM_Free( uivm );
	uivm = NULL;
}
