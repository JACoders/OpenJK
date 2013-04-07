// sv_game.c -- interface to the game dll

// leave this as first line for PCH reasons...
//
#include "../server/exe_headers.h"

#include "../RMG/RM_Headers.h"

#include "../qcommon/cm_local.h"

#include "server.h"
#include "..\client\vmachine.h"
#include "..\client\client.h"
#include "..\renderer\tr_local.h"
#include "..\renderer\tr_WorldEffects.h"
/*
Ghoul2 Insert Start
*/
#if !defined(G2_H_INC)
	#include "..\ghoul2\G2.h"
#endif

/*
Ghoul2 Insert End
*/

//prototypes
extern void	Sys_UnloadGame( void );
extern void	*Sys_GetGameAPI( void *parms);
extern void Com_WriteCam ( const char *text );
extern void Com_FlushCamFile();

#ifdef _XBOX
extern int	*s_entityWavVol;
#else
extern int	s_entityWavVol[MAX_GENTITIES];
#endif


// these functions must be used instead of pointer arithmetic, because
// the game allocates gentities with private information after the server shared part
/*
int	SV_NumForGentity( gentity_t *ent ) {
	int		num;

	num = ( (byte *)ent - (byte *)ge->gentities ) / ge->gentitySize;

	return num;
}
*/
gentity_t	*SV_GentityNum( int num ) {
	gentity_t	*ent;

	assert (num >=0);
	ent = (gentity_t *)((byte *)ge->gentities + ge->gentitySize*(num));

	return ent;
}

svEntity_t	*SV_SvEntityForGentity( gentity_t *gEnt ) {
	if ( !gEnt || gEnt->s.number < 0 || gEnt->s.number >= MAX_GENTITIES ) {
		Com_Error( ERR_DROP, "SV_SvEntityForGentity: bad gEnt" );
	}
	return &sv.svEntities[ gEnt->s.number ];
}

gentity_t	*SV_GEntityForSvEntity( svEntity_t *svEnt ) {
	int		num;

	num = svEnt - sv.svEntities;
	return SV_GentityNum( num );
}

/*
===============
SV_GameSendServerCommand

Sends a command string to a client
===============
*/
void SV_GameSendServerCommand( int clientNum, const char *fmt, ... ) {
	char		msg[8192];
	va_list		argptr;
	
	va_start (argptr,fmt);
	vsprintf (msg, fmt, argptr);
	va_end (argptr);

	if ( clientNum == -1 ) {
		SV_SendServerCommand( NULL, "%s", msg );
	} else {
		if ( clientNum < 0 || clientNum >= 1 ) {
			return;
		}
		SV_SendServerCommand( svs.clients + clientNum, "%s", msg );	
	}
}


/*
===============
SV_GameDropClient

Disconnects the client with a message
===============
*/
void SV_GameDropClient( int clientNum, const char *reason ) {
	if ( clientNum < 0 || clientNum >= 1 ) {
		return;
	}
	SV_DropClient( svs.clients + clientNum, reason );	
}


/*
=================
SV_SetBrushModel

sets mins and maxs for inline bmodels
=================
*/
void SV_SetBrushModel( gentity_t *ent, const char *name ) {
	clipHandle_t	h;
	vec3_t			mins, maxs;

	if (!name) 
	{
		Com_Error( ERR_DROP, "SV_SetBrushModel: NULL model for ent number %d", ent->s.number );
	}

	if (name[0] == '*')
	{
		ent->s.modelindex = atoi( name + 1 );

		if (sv.mLocalSubBSPIndex != -1)
		{
			ent->s.modelindex += sv.mLocalSubBSPModelOffset;
		}

		h = CM_InlineModel( ent->s.modelindex );

		if (sv.mLocalSubBSPIndex != -1)
		{
			CM_ModelBounds( SubBSP[sv.mLocalSubBSPIndex], h, mins, maxs );
		}
		else
		{
			CM_ModelBounds( cmg, h, mins, maxs);
		}

		//CM_ModelBounds( h, mins, maxs );

		VectorCopy (mins, ent->mins);
		VectorCopy (maxs, ent->maxs);
		ent->bmodel = qtrue;

		if (0) //com_RMG && com_RMG->integer //fixme: this test really should be do we have bsp instances
		{
			ent->contents = CM_ModelContents( h, sv.mLocalSubBSPIndex );
		}
		else
		{
			ent->contents = CM_ModelContents( h, -1 );
		}
	}
	else if (name[0] == '#')
	{
		ent->s.modelindex = CM_LoadSubBSP(va("maps/%s.bsp", name + 1), qfalse);
		CM_ModelBounds( SubBSP[CM_FindSubBSP(ent->s.modelindex)], ent->s.modelindex, mins, maxs );

		VectorCopy (mins, ent->mins);
		VectorCopy (maxs, ent->maxs);
		ent->bmodel = qtrue;

		//rwwNOTE: We don't ever want to set contents -1, it includes CONTENTS_LIGHTSABER.
		//Lots of stuff will explode if there's a brush with CONTENTS_LIGHTSABER that isn't attached to a client owner.
		//ent->contents = -1;		// we don't know exactly what is in the brushes
		h = CM_InlineModel( ent->s.modelindex );
		ent->contents = CM_ModelContents( h, CM_FindSubBSP(ent->s.modelindex) );
	//	ent->contents = CONTENTS_SOLID;
	}
	else
	{
		Com_Error( ERR_DROP, "SV_SetBrushModel: %s isn't a brush model (ent %d)", name, ent->s.number );
	}
}

const char *SV_SetActiveSubBSP(int index)
{
	if (index >= 0)
	{
		sv.mLocalSubBSPIndex = CM_FindSubBSP(index);
		sv.mLocalSubBSPModelOffset = index;
		sv.mLocalSubBSPEntityParsePoint = CM_SubBSPEntityString (sv.mLocalSubBSPIndex);
		return sv.mLocalSubBSPEntityParsePoint;
	}
	else
	{
		sv.mLocalSubBSPIndex = -1;
	}

	return NULL;
}

/*
=================
SV_inPVS

Also checks portalareas so that doors block sight
=================
*/
qboolean SV_inPVS (const vec3_t p1, const vec3_t p2)
{
	int		leafnum;
	int		cluster;
	int		area1, area2;
#ifdef _XBOX 
	const byte *mask;
#else
	byte	*mask;
#endif
	int		start=0;

	if ( com_speeds->integer ) {
		start = Sys_Milliseconds ();
	}
	leafnum = CM_PointLeafnum (p1);
	cluster = CM_LeafCluster (leafnum);
	area1 = CM_LeafArea (leafnum);
	mask = CM_ClusterPVS (cluster);

	leafnum = CM_PointLeafnum (p2);
	cluster = CM_LeafCluster (leafnum);
	area2 = CM_LeafArea (leafnum);
	if ( mask && (!(mask[cluster>>3] & (1<<(cluster&7)) ) ) )
	{
		if ( com_speeds->integer ) {
			timeInPVSCheck += Sys_Milliseconds () - start;
		}
		return qfalse;
	}
	
	if (!CM_AreasConnected (area1, area2))
	{
		timeInPVSCheck += Sys_Milliseconds() - start;
		return qfalse;		// a door blocks sight
	}
	
	if ( com_speeds->integer ) {
		timeInPVSCheck += Sys_Milliseconds() - start;
	}
	return qtrue;
}


/*
=================
SV_inPVSIgnorePortals

Does NOT check portalareas
=================
*/
qboolean SV_inPVSIgnorePortals( const vec3_t p1, const vec3_t p2)
{
	int		leafnum;
	int		cluster;
	int		area1, area2;
#ifdef _XBOX
	const byte *mask;
#else
	byte	*mask;
#endif
	int		start=0;

	if ( com_speeds->integer ) {
		start = Sys_Milliseconds ();
	}
	
	leafnum = CM_PointLeafnum (p1);
	cluster = CM_LeafCluster (leafnum);
	area1 = CM_LeafArea (leafnum);
	mask = CM_ClusterPVS (cluster);

	leafnum = CM_PointLeafnum (p2);
	cluster = CM_LeafCluster (leafnum);
	area2 = CM_LeafArea (leafnum);

	if ( mask && (!(mask[cluster>>3] & (1<<(cluster&7)) ) ) )
	{
		if ( com_speeds->integer ) {
			timeInPVSCheck += Sys_Milliseconds() - start;
		}
		return qfalse;
	}

	if ( com_speeds->integer ) {
		timeInPVSCheck += Sys_Milliseconds() - start;
	}
	return qtrue;
}


/*
========================
SV_AdjustAreaPortalState
========================
*/
void SV_AdjustAreaPortalState( gentity_t *ent, qboolean open ) {
	if ( !(ent->contents&CONTENTS_OPAQUE) )	{
#ifndef FINAL_BUILD
//		Com_Printf( "INFO: entity number %d not opaque: not affecting area portal!\n", ent->s.number );
#endif
		return;
	}

	svEntity_t	*svEnt;

	svEnt = SV_SvEntityForGentity( ent );
	if ( svEnt->areanum2 == -1 ) {
		return;
	}
	CM_AdjustAreaPortalState( svEnt->areanum, svEnt->areanum2, open );
}


/*
==================
SV_GameAreaEntities
==================
*/
qboolean	SV_EntityContact( const vec3_t mins, const vec3_t maxs, const gentity_t *gEnt ) {
	const float	*origin, *angles;
	clipHandle_t	ch;
	trace_t			trace;

	// check for exact collision
	origin = gEnt->currentOrigin;
	angles = gEnt->currentAngles;

	ch = SV_ClipHandleForEntity( gEnt );
	CM_TransformedBoxTrace ( &trace, vec3_origin, vec3_origin, mins, maxs,
		ch, -1, origin, angles );

	return trace.startsolid;
}


/*
===============
SV_GetServerinfo

===============
*/
void SV_GetServerinfo( char *buffer, int bufferSize ) {
	if ( bufferSize < 1 ) {
		Com_Error( ERR_DROP, "SV_GetServerinfo: bufferSize == %i", bufferSize );
	}
	Q_strncpyz( buffer, Cvar_InfoString( CVAR_SERVERINFO ), bufferSize );
}

qboolean SV_GetEntityToken( char *buffer, int bufferSize )
{
	char	*s;

	if (sv.mLocalSubBSPIndex == -1)
	{
		s = COM_Parse( (const char **)&sv.entityParsePoint );
		Q_strncpyz( buffer, s, bufferSize );
		if ( !sv.entityParsePoint && !s[0] ) 
		{
			return qfalse;
		}
		else
		{
			return qtrue;
		}
	}
	else
	{
		s = COM_Parse( (const char **)&sv.mLocalSubBSPEntityParsePoint);
		Q_strncpyz( buffer, s, bufferSize );
		if ( !sv.mLocalSubBSPEntityParsePoint && !s[0] ) 
		{
			return qfalse;
		}
		else
		{
			return qtrue;
		}
	}
}

//==============================================

/*
===============
SV_ShutdownGameProgs

Called when either the entire server is being killed, or
it is changing to a different game directory.
===============
*/
void SV_ShutdownGameProgs (qboolean shutdownCin) {
	if (!ge) {
		return;
	}
	ge->Shutdown ();
	
#ifdef _XBOX
	if(shutdownCin) {
		SCR_StopCinematic();
	}
#else
	SCR_StopCinematic();
#endif
	CL_ShutdownCGame();	//we have cgame burried in here.
	
	Sys_UnloadGame ();	//this kills cgame as well.

	ge = NULL;
	cgvm.entryPoint = 0;
}

// this is a compile-helper function since Z_Malloc can now become a macro with __LINE__ etc
//
static void *G_ZMalloc_Helper( int iSize, memtag_t eTag, qboolean bZeroit)
{
	return Z_Malloc( iSize, eTag, bZeroit );
}

//rww - RAGDOLL_BEGIN
void G2API_SetRagDoll(CGhoul2Info_v &ghoul2,CRagDollParams *parms);
void G2API_AnimateG2Models(CGhoul2Info_v &ghoul2, int AcurrentTime,CRagDollUpdateParams *params);

qboolean	G2API_RagPCJConstraint(CGhoul2Info_v &ghoul2, const char *boneName, vec3_t min, vec3_t max);
qboolean	G2API_RagPCJGradientSpeed(CGhoul2Info_v &ghoul2, const char *boneName, const float speed);
qboolean	G2API_RagEffectorGoal(CGhoul2Info_v &ghoul2, const char *boneName, vec3_t pos);
qboolean	G2API_GetRagBonePos(CGhoul2Info_v &ghoul2, const char *boneName, vec3_t pos, vec3_t entAngles, vec3_t entPos, vec3_t entScale);
qboolean	G2API_RagEffectorKick(CGhoul2Info_v &ghoul2, const char *boneName, vec3_t velocity);
qboolean	G2API_RagForceSolve(CGhoul2Info_v &ghoul2, qboolean force);

qboolean G2API_SetBoneIKState(CGhoul2Info_v &ghoul2, int time, const char *boneName, int ikState, sharedSetBoneIKStateParams_t *params);
qboolean G2API_IKMove(CGhoul2Info_v &ghoul2, int time, sharedIKMoveParams_t *params);
//rww - RAGDOLL_END

//This is as good a place as any I guess.
#ifndef _XBOX	// Removing terrain from Xbox
void RMG_Init(int terrainID)
{
	if (!TheRandomMissionManager)
	{
		TheRandomMissionManager = new CRMManager;
	}
	TheRandomMissionManager->SetLandScape(cmg.landScape);
	if (TheRandomMissionManager->LoadMission(qtrue))
	{
		TheRandomMissionManager->SpawnMission(qtrue);
	}
//		cmg.landScapes[args[1]]->UpdatePatches();
	//sv.mRMGChecksum = cm.landScapes[terrainID]->get_rand_seed();
}

CCMLandScape *CM_RegisterTerrain(const char *config, bool server);

int InterfaceCM_RegisterTerrain (const char *info)
{
	return CM_RegisterTerrain(info, false)->GetTerrainId();
}
#endif	// _XBOX

/*
===============
SV_InitGameProgs

Init the game subsystem for a new map
===============
*/
void SV_InitGameProgs (void) {
	game_import_t	import;
	int				i;

	// unload anything we have now
	if ( ge ) {
		SV_ShutdownGameProgs (qtrue);
	}

#ifndef _XBOX
	if ( !Cvar_VariableIntegerValue("fs_restrict") && !Sys_CheckCD() ) 
	{
		Com_Error( ERR_NEED_CD, SE_GetString("CON_TEXT_NEED_CD") ); //"Game CD not in drive" );		
	}
#endif

	// load a new game dll
	import.Printf = Com_Printf;
	import.WriteCam = Com_WriteCam;
	import.FlushCamFile = Com_FlushCamFile;
	import.Error = Com_Error;

	import.Milliseconds = Sys_Milliseconds;

	import.DropClient = SV_GameDropClient;

	import.SendServerCommand = SV_GameSendServerCommand;


	import.linkentity = SV_LinkEntity;
	import.unlinkentity = SV_UnlinkEntity;
	import.EntitiesInBox = SV_AreaEntities;
	import.EntityContact = SV_EntityContact;
	import.trace = SV_Trace;
	import.pointcontents = SV_PointContents;
	import.totalMapContents = CM_TotalMapContents;
	import.SetBrushModel = SV_SetBrushModel;

	import.inPVS = SV_inPVS;
	import.inPVSIgnorePortals = SV_inPVSIgnorePortals;

	import.SetConfigstring = SV_SetConfigstring;
	import.GetConfigstring = SV_GetConfigstring;

	import.SetUserinfo = SV_SetUserinfo;
	import.GetUserinfo = SV_GetUserinfo;

	import.GetServerinfo = SV_GetServerinfo;

	import.cvar = Cvar_Get;
	import.cvar_set = Cvar_Set;
	import.Cvar_VariableIntegerValue = Cvar_VariableIntegerValue;
	import.Cvar_VariableStringBuffer = Cvar_VariableStringBuffer;

	import.argc = Cmd_Argc;
	import.argv = Cmd_Argv;
	import.SendConsoleCommand = Cbuf_AddText;

	import.FS_FOpenFile = FS_FOpenFileByMode;
	import.FS_Read = FS_Read;
	import.FS_Write = FS_Write;
	import.FS_FCloseFile = FS_FCloseFile;
	import.FS_ReadFile = FS_ReadFile;
	import.FS_FreeFile = FS_FreeFile;
	import.FS_GetFileList = FS_GetFileList;

	import.AppendToSaveGame = SG_Append;
#ifndef _XBOX
	import.ReadFromSaveGame	= SG_Read;
	import.ReadFromSaveGameOptional = SG_ReadOptional;
#endif

	import.AdjustAreaPortalState = SV_AdjustAreaPortalState;
	import.AreasConnected = CM_AreasConnected;

	import.VoiceVolume = s_entityWavVol;

	import.Malloc = G_ZMalloc_Helper;
	import.Free = Z_Free;
	import.bIsFromZone = Z_IsFromZone;
/*
Ghoul2 Insert Start
*/

	import.G2API_AddBolt = G2API_AddBolt;
	import.G2API_AttachEnt = G2API_AttachEnt;
	import.G2API_AttachG2Model = G2API_AttachG2Model;
	import.G2API_CollisionDetect = G2API_CollisionDetect;
	import.G2API_DetachEnt = G2API_DetachEnt;
	import.G2API_DetachG2Model = G2API_DetachG2Model;
	import.G2API_GetAnimFileName = G2API_GetAnimFileName;
	import.G2API_GetBoltMatrix = G2API_GetBoltMatrix;
	import.G2API_GetBoneAnim = G2API_GetBoneAnim;
	import.G2API_GetBoneAnimIndex = G2API_GetBoneAnimIndex;
	import.G2API_AddSurface = G2API_AddSurface;
	import.G2API_HaveWeGhoul2Models =G2API_HaveWeGhoul2Models;
#ifndef _XBOX
	import.G2API_InitGhoul2Model = G2API_InitGhoul2Model;
	import.G2API_SetBoneAngles = G2API_SetBoneAngles;
	import.G2API_SetBoneAnglesMatrix = G2API_SetBoneAnglesMatrix;
	import.G2API_SetBoneAnim = G2API_SetBoneAnim;
	import.G2API_SetSkin = G2API_SetSkin;
	import.G2API_CopyGhoul2Instance = G2API_CopyGhoul2Instance;
	import.G2API_SetBoneAnglesIndex = G2API_SetBoneAnglesIndex;
	import.G2API_SetBoneAnimIndex = G2API_SetBoneAnimIndex;
#endif
	import.G2API_IsPaused = G2API_IsPaused;
	import.G2API_ListBones = G2API_ListBones;
	import.G2API_ListSurfaces = G2API_ListSurfaces;
	import.G2API_PauseBoneAnim = G2API_PauseBoneAnim;
	import.G2API_PauseBoneAnimIndex = G2API_PauseBoneAnimIndex;
	import.G2API_PrecacheGhoul2Model = G2API_PrecacheGhoul2Model;
	import.G2API_RemoveBolt = G2API_RemoveBolt;
	import.G2API_RemoveBone = G2API_RemoveBone;
	import.G2API_RemoveGhoul2Model = G2API_RemoveGhoul2Model;
	import.G2API_SetLodBias = G2API_SetLodBias;
	import.G2API_SetRootSurface = G2API_SetRootSurface;
	import.G2API_SetShader = G2API_SetShader;
	import.G2API_SetSurfaceOnOff = G2API_SetSurfaceOnOff;
	import.G2API_StopBoneAngles = G2API_StopBoneAngles;
	import.G2API_StopBoneAnim = G2API_StopBoneAnim;
	import.G2API_SetGhoul2ModelFlags = G2API_SetGhoul2ModelFlags;
	import.G2API_AddBoltSurfNum = G2API_AddBoltSurfNum;
	import.G2API_RemoveSurface = G2API_RemoveSurface;
	import.G2API_GetAnimRange = G2API_GetAnimRange;
	import.G2API_GetAnimRangeIndex = G2API_GetAnimRangeIndex;
	import.G2API_GiveMeVectorFromMatrix = G2API_GiveMeVectorFromMatrix;
	import.G2API_GetGhoul2ModelFlags = G2API_GetGhoul2ModelFlags;
	import.G2API_CleanGhoul2Models = G2API_CleanGhoul2Models;
	import.TheGhoul2InfoArray = TheGhoul2InfoArray;
	import.G2API_GetParentSurface = G2API_GetParentSurface;
	import.G2API_GetSurfaceIndex = G2API_GetSurfaceIndex;
	import.G2API_GetSurfaceName = G2API_GetSurfaceName;
	import.G2API_GetGLAName = G2API_GetGLAName;
	import.G2API_SetNewOrigin = G2API_SetNewOrigin;
	import.G2API_GetBoneIndex = G2API_GetBoneIndex;
	import.G2API_StopBoneAnglesIndex = G2API_StopBoneAnglesIndex;
	import.G2API_StopBoneAnimIndex = G2API_StopBoneAnimIndex;
	import.G2API_SetBoneAnglesMatrixIndex = G2API_SetBoneAnglesMatrixIndex;
	import.G2API_SetAnimIndex = G2API_SetAnimIndex;
	import.G2API_GetAnimIndex = G2API_GetAnimIndex;

	import.G2API_SaveGhoul2Models = G2API_SaveGhoul2Models;
	import.G2API_LoadGhoul2Models = G2API_LoadGhoul2Models;
	import.G2API_LoadSaveCodeDestructGhoul2Info = G2API_LoadSaveCodeDestructGhoul2Info;
	import.G2API_GetAnimFileNameIndex = G2API_GetAnimFileNameIndex;
	import.G2API_GetAnimFileInternalNameIndex = G2API_GetAnimFileInternalNameIndex;
	import.G2API_GetSurfaceRenderStatus = G2API_GetSurfaceRenderStatus;

	//rww - RAGDOLL_BEGIN
	import.G2API_SetRagDoll = G2API_SetRagDoll;
	import.G2API_AnimateG2Models = G2API_AnimateG2Models;

	import.G2API_RagPCJConstraint = G2API_RagPCJConstraint;
	import.G2API_RagPCJGradientSpeed = G2API_RagPCJGradientSpeed;
	import.G2API_RagEffectorGoal = G2API_RagEffectorGoal;
	import.G2API_GetRagBonePos = G2API_GetRagBonePos;
	import.G2API_RagEffectorKick = G2API_RagEffectorKick;
	import.G2API_RagForceSolve = G2API_RagForceSolve;

	import.G2API_SetBoneIKState = G2API_SetBoneIKState;
    import.G2API_IKMove = G2API_IKMove;
	//rww - RAGDOLL_END

	import.G2API_AddSkinGore = G2API_AddSkinGore;
	import.G2API_ClearSkinGore = G2API_ClearSkinGore;

#ifndef _XBOX
	import.RMG_Init = RMG_Init;
	import.CM_RegisterTerrain = InterfaceCM_RegisterTerrain;
#endif
	import.SetActiveSubBSP = SV_SetActiveSubBSP;

	import.RE_RegisterSkin = RE_RegisterSkin;
	import.RE_GetAnimationCFG = RE_GetAnimationCFG;


	import.WE_GetWindVector	= R_GetWindVector;
	import.WE_GetWindGusting = R_GetWindGusting;
	import.WE_IsOutside	= R_IsOutside;
	import.WE_IsOutsideCausingPain	= R_IsOutsideCausingPain;
	import.WE_GetChanceOfSaberFizz = R_GetChanceOfSaberFizz;
	import.WE_IsShaking = R_IsShaking;
	import.WE_AddWeatherZone = R_AddWeatherZone;
	import.WE_SetTempGlobalFogColor = R_SetTempGlobalFogColor;


/*
Ghoul2 Insert End
*/

	ge = (game_export_t *)Sys_GetGameAPI (&import);

	if (!ge)
		Com_Error (ERR_DROP, "failed to load game DLL");

	//hook up the client while we're here
#ifdef _XBOX
	VM_Create("cl");
#else
	if (!VM_Create("cl"))
		Com_Error (ERR_DROP, "failed to attach to the client DLL");
#endif

	if (ge->apiversion != GAME_API_VERSION)
		Com_Error (ERR_DROP, "game is version %i, not %i", ge->apiversion,
		GAME_API_VERSION);

	sv.entityParsePoint = CM_EntityString();

	// use the current msec count for a random seed
	Z_TagFree(TAG_G_ALLOC);
	ge->Init( sv_mapname->string, sv_spawntarget->string, sv_mapChecksum->integer, CM_EntityString(), sv.time, com_frameTime, Com_Milliseconds(), eSavedGameJustLoaded, qbLoadTransition );

	// clear all gentity pointers that might still be set from
	// a previous level
	for ( i = 0 ; i < 1 ; i++ ) {
		svs.clients[i].gentity = NULL;
	}
}



/*
====================
SV_GameCommand

See if the current console command is claimed by the game
====================
*/
qboolean SV_GameCommand( void ) {
	if ( sv.state != SS_GAME ) {
		return qfalse;
	}

	return ge->ConsoleCommand();
}

