//Anything above this #include will be ignored by the compiler
#include "../qcommon/exe_headers.h"

#include "client.h"

#include "../game/botlib.h"
#include "../qcommon/stringed_ingame.h"

/*
Ghoul2 Insert Start
*/

#if !defined(G2_H_INC)
	#include "../ghoul2/G2_local.h"
#endif

/*
Ghoul2 Insert End
*/

#ifdef _XBOX
#include "../cgame/cg_local.h"
#include "../client/cl_data.h"
#include "../renderer/modelmem.h"
#endif

extern	botlib_export_t	*botlib_export;
void SP_Register(const char *Package);

vm_t *uivm;

#ifdef USE_CD_KEY

extern char cl_cdkey[34];

#endif // USE_CD_KEY

/*
====================
GetClientState
====================
*/
static void GetClientState( uiClientState_t *state ) {
	state->connectPacketCount = clc->connectPacketCount;
#ifdef _XBOX
//	if(ClientManager::splitScreenMode == qtrue)
//		cls.state = ClientManager::ActiveClient().state;
#endif
	state->connState = cls.state;
	Q_strncpyz( state->servername, cls.servername, sizeof( state->servername ) );
	Q_strncpyz( state->messageString, clc->serverMessage, sizeof( state->messageString ) );
	state->clientNum = cl->snap.ps.clientNum;
}

/*
====================
CL_GetGlConfig
====================
*/
static void CL_GetGlconfig( glconfig_t *config ) {
	*config = cls.glconfig;
}

/*
====================
GetClipboardData
====================
*/
static void GetClipboardData( char *buf, int buflen ) {
	char	*cbd;

	cbd = Sys_GetClipboardData();

	if ( !cbd ) {
		*buf = 0;
		return;
	}

	Q_strncpyz( buf, cbd, buflen );

	Z_Free( cbd );
}

/*
====================
Key_KeynumToStringBuf
====================
*/
// only ever called by binding-display code, therefore returns non-technical "friendly" names 
//	in any language that don't necessarily match those in the config file...
//
void Key_KeynumToStringBuf( int keynum, char *buf, int buflen ) 
{
	const char *psKeyName = Key_KeynumToString( keynum/*, qtrue */);

	// see if there's a more friendly (or localised) name...
	//
	const char *psKeyNameFriendly = SE_GetString( va("KEYNAMES_KEYNAME_%s",psKeyName) );

	Q_strncpyz( buf, (psKeyNameFriendly && psKeyNameFriendly[0]) ? psKeyNameFriendly : psKeyName, buflen );
}


/*
====================
Key_GetBindingBuf
====================
*/
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

/*
====================
Key_GetCatcher
====================
*/
int Key_GetCatcher( void ) {
	return cls.keyCatchers;
}

/*
====================
Ket_SetCatcher
====================
*/
void Key_SetCatcher( int catcher ) {
	cls.keyCatchers = catcher;

#ifdef _XBOX
	if(catcher == KEYCATCH_UI)
	{
//		ModelMem.EnterUI();
		int i;

		for(i=0; i<ClientManager::NumClients(); i++) {
			ClientManager::GetClient(i).swapMan1.SetUp();
			ClientManager::GetClient(i).swapMan2.SetUp();
		}
	}
	else
	{
//		ModelMem.ExitUI();
		//Horrible hack to make split screen work if force menus are skipped.
		extern int uiclientInputClosed;
		uiclientInputClosed = 0;
	}
#endif
}


#ifdef USE_CD_KEY

/*
====================
CLUI_GetCDKey
====================
*/
static void CLUI_GetCDKey( char *buf, int buflen ) {
	cvar_t	*fs;
	fs = Cvar_Get ("fs_game", "", CVAR_INIT|CVAR_SYSTEMINFO );
	if (UI_usesUniqueCDKey() && fs && fs->string[0] != 0) {
		Com_Memcpy( buf, &cl_cdkey[16], 16);
		buf[16] = 0;
	} else {
		Com_Memcpy( buf, cl_cdkey, 16);
		buf[16] = 0;
	}
}


/*
====================
CLUI_SetCDKey
====================
*/
static void CLUI_SetCDKey( char *buf ) {
	cvar_t	*fs;
	fs = Cvar_Get ("fs_game", "", CVAR_INIT|CVAR_SYSTEMINFO );
	if (UI_usesUniqueCDKey() && fs && fs->string[0] != 0) {
		Com_Memcpy( &cl_cdkey[16], buf, 16 );
		cl_cdkey[32] = 0;
		// set the flag so the fle will be written at the next opportunity
		cvar_modifiedFlags |= CVAR_ARCHIVE;
	} else {
		Com_Memcpy( cl_cdkey, buf, 16 );
		// set the flag so the fle will be written at the next opportunity
		cvar_modifiedFlags |= CVAR_ARCHIVE;
	}
}

#endif // USE_CD_KEY

/*
====================
GetConfigString
====================
*/
static int GetConfigString(int index, char *buf, int size)
{
	int		offset;

	if (index < 0 || index >= MAX_CONFIGSTRINGS)
		return qfalse;

	offset = cl->gameState.stringOffsets[index];
	if (!offset) {
		if( size ) {
			buf[0] = 0;
		}
		return qfalse;
	}

	Q_strncpyz( buf, cl->gameState.stringData+offset, size);
 
	return qtrue;
}

/*
====================
FloatAsInt
====================
*/
static int FloatAsInt( float f ) {
	int		temp;

	*(float *)&temp = f;

	return temp;
}

void *VM_ArgPtr( int intValue );
#define	VMA(x) VM_ArgPtr(args[x])
#define	VMF(x)	((float *)args)[x]

/*
====================
CL_UISystemCalls

The ui module is making a system call
====================
*/
int CL_UISystemCalls( int *args ) {
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
		return (int)strncpy( (char *)VMA(1), (const char *)VMA(2), args[3] );
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
		Cvar_Set( (const char *)VMA(1), (const char *)VMA(2) );
		return 0;

	case UI_CVAR_VARIABLEVALUE:
		return FloatAsInt( Cvar_VariableValue( (const char *)VMA(1) ) );

	case UI_CVAR_VARIABLESTRINGBUFFER:
		Cvar_VariableStringBuffer( (const char *)VMA(1), (char *)VMA(2), args[3] );
		return 0;

	case UI_CVAR_SETVALUE:
		Cvar_SetValue( (const char *)VMA(1), VMF(2) );
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
		FS_Read2( VMA(1), args[2], args[3] );
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
		return re.RegisterModel( (const char *)VMA(1) );

	case UI_R_REGISTERSKIN:
		return re.RegisterSkin( (const char *)VMA(1) );

	case UI_R_REGISTERSHADERNOMIP:
		return re.RegisterShaderNoMip( (const char *)VMA(1) );

	case UI_R_SHADERNAMEFROMINDEX:
		{
			char *gameMem = (char *)VMA(1);
			const char *retMem = re.ShaderNameFromIndex(args[2]);
			if (retMem)
			{
				strcpy(gameMem, retMem);
			}
			else
			{
				gameMem[0] = 0;
			}
		}
		return 0;

	case UI_R_CLEARSCENE:
		re.ClearScene();
		return 0;

	case UI_R_ADDREFENTITYTOSCENE:
		re.AddRefEntityToScene( (const refEntity_t *)VMA(1) );
		return 0;

	case UI_R_ADDPOLYTOSCENE:
		re.AddPolyToScene( args[1], args[2], (const polyVert_t *)VMA(3), 1 );
		return 0;

	case UI_R_ADDLIGHTTOSCENE:
		re.AddLightToScene( (const float *)VMA(1), VMF(2), VMF(3), VMF(4), VMF(5) );
		return 0;

	case UI_R_RENDERSCENE:
		re.RenderScene( (const refdef_t *)VMA(1) );
		return 0;

	case UI_R_SETCOLOR:
		re.SetColor( (const float *)VMA(1) );
		return 0;

	case UI_R_DRAWSTRETCHPIC:
		re.DrawStretchPic( VMF(1), VMF(2), VMF(3), VMF(4), VMF(5), VMF(6), VMF(7), VMF(8), args[9] );
		return 0;

  case UI_R_MODELBOUNDS:
		re.ModelBounds( args[1], (float *)VMA(2), (float *)VMA(3) );
		return 0;

	case UI_UPDATESCREEN:
		SCR_UpdateScreen();
		return 0;

	case UI_CM_LERPTAG:
		re.LerpTag( (orientation_t *)VMA(1), args[2], args[3], args[4], VMF(5), (const char *)VMA(6) );
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
		Key_SetCatcher( args[1] );
		return 0;

	case UI_GETCLIPBOARDDATA:
		GetClipboardData( (char *)VMA(1), args[2] );
		return 0;

	case UI_GETCLIENTSTATE:
		GetClientState( (uiClientState_t *)VMA(1) );
		return 0;		

	case UI_GETGLCONFIG:
		CL_GetGlconfig( (glconfig_t *)VMA(1) );
		return 0;

	case UI_GETCONFIGSTRING:
		return GetConfigString( args[1], (char *)VMA(2), args[3] );

	case UI_MEMORY_REMAINING:
		return Hunk_MemoryRemaining();

#ifdef USE_CD_KEY
	case UI_GET_CDKEY:
		CLUI_GetCDKey( (char *)VMA(1), args[2] );
		return 0;

	case UI_SET_CDKEY:
		CLUI_SetCDKey( (char *)VMA(1) );
		return 0;
#endif	// USE_CD_KEY

	case UI_R_REGISTERFONT:
		return re.RegisterFont( (const char *)VMA(1) );

	case UI_R_FONT_STRLENPIXELS:
		return re.Font_StrLenPixels( (const char *)VMA(1), args[2], VMF(3) );

	case UI_R_FONT_STRLENCHARS:
		return re.Font_StrLenChars( (const char *)VMA(1) );

	case UI_R_FONT_STRHEIGHTPIXELS:
		return re.Font_HeightPixels( args[1], VMF(2) );

	case UI_R_FONT_DRAWSTRING:
		re.Font_DrawString( args[1], args[2], (const char *)VMA(3), (const float *) VMA(4), args[5], args[6], VMF(7) );
		return 0;

	case UI_LANGUAGE_ISASIAN:
		return re.Language_IsAsian();

	case UI_LANGUAGE_USESSPACES:
		return re.Language_UsesSpaces();

	case UI_ANYLANGUAGE_READCHARFROMSTRING:
		return re.AnyLanguage_ReadCharFromString( (const char *)VMA(1), (int *) VMA(2), (qboolean *) VMA(3) );

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
		S_StartBackgroundTrack( (const char *)VMA(1), (const char *)VMA(2), qfalse);
		return 0;

	case UI_REAL_TIME:
		return Com_RealTime( (struct qtime_s *)VMA(1) );

	case UI_R_REMAP_SHADER:
		re.RemapShader( (const char *)VMA(1), (const char *)VMA(2), (const char *)VMA(3) );
		return 0;

#ifdef USE_CD_KEY
	case UI_VERIFY_CDKEY:
		return CL_CDKeyValidate((const char *)VMA(1), (const char *)VMA(2));
#endif // USE_CD_KEY

	case UI_SP_GETNUMLANGUAGES:
		assert( 0 );
		return 0;
//		return SE_GetNumLanguages();

	case UI_SP_GETLANGUAGENAME:
		assert( 0 );
		return 0;

	case UI_SP_GETSTRINGTEXTSTRING:
		const char* text;

		assert(VMA(1));
		assert(VMA(2));
		text = SE_GetString((const char *) VMA(1));
		Q_strncpyz( (char *) VMA(2), text, args[3] );
		return qtrue;

/*
Ghoul2 Insert Start
*/
/*
Ghoul2 Insert Start
*/
		
	case UI_G2_LISTSURFACES:
		G2API_ListSurfaces( (CGhoul2Info *) args[1] );
		return 0;

	case UI_G2_LISTBONES:
		G2API_ListBones( (CGhoul2Info *) args[1], args[2]);
		return 0;

	case UI_G2_HAVEWEGHOULMODELS:
		return G2API_HaveWeGhoul2Models( *((CGhoul2Info_v *)args[1]) );

	case UI_G2_SETMODELS:
		G2API_SetGhoul2ModelIndexes( *((CGhoul2Info_v *)args[1]),(qhandle_t *)VMA(2),(qhandle_t *)VMA(3));
		return 0;

	case UI_G2_GETBOLT:
		return G2API_GetBoltMatrix(*((CGhoul2Info_v *)args[1]), args[2], args[3], (mdxaBone_t *)VMA(4), (const float *)VMA(5),(const float *)VMA(6), args[7], (qhandle_t *)VMA(8), (float *)VMA(9));

	case UI_G2_GETBOLT_NOREC:
		gG2_GBMNoReconstruct = qtrue;
		return G2API_GetBoltMatrix(*((CGhoul2Info_v *)args[1]), args[2], args[3], (mdxaBone_t *)VMA(4), (const float *)VMA(5),(const float *)VMA(6), args[7], (qhandle_t *)VMA(8), (float *)VMA(9));

	case UI_G2_GETBOLT_NOREC_NOROT:
		gG2_GBMNoReconstruct = qtrue;
		gG2_GBMUseSPMethod = qtrue;
		return G2API_GetBoltMatrix(*((CGhoul2Info_v *)args[1]), args[2], args[3], (mdxaBone_t *)VMA(4), (const float *)VMA(5),(const float *)VMA(6), args[7], (qhandle_t *)VMA(8), (float *)VMA(9));

	case UI_G2_INITGHOUL2MODEL:
#ifdef _FULL_G2_LEAK_CHECKING
		g_G2AllocServer = 0;
#endif
		return	G2API_InitGhoul2Model((CGhoul2Info_v **)VMA(1), (const char *)VMA(2), args[3], (qhandle_t) args[4],
									  (qhandle_t) args[5], args[6], args[7]);


	case UI_G2_COLLISIONDETECT:
		return 0; //not supported for ui

	case UI_G2_ANGLEOVERRIDE:
		return G2API_SetBoneAngles(*((CGhoul2Info_v *)args[1]), args[2], (const char *)VMA(3), (float *)VMA(4), args[5],
							 (const Eorientations) args[6], (const Eorientations) args[7], (const Eorientations) args[8],
							 (qhandle_t *)VMA(9), args[10], args[11] );
	
	case UI_G2_CLEANMODELS:
#ifdef _FULL_G2_LEAK_CHECKING
		g_G2AllocServer = 0;
#endif
		G2API_CleanGhoul2Models((CGhoul2Info_v **)VMA(1));
	//	G2API_CleanGhoul2Models((CGhoul2Info_v **)args[1]);
		return 0;

	case UI_G2_PLAYANIM:
		return G2API_SetBoneAnim(*((CGhoul2Info_v *)args[1]), args[2], (const char *)VMA(3), args[4], args[5],
								args[6], VMF(7), args[8], VMF(9), args[10]);

	case UI_G2_GETBONEANIM:
		{
			CGhoul2Info_v &g2 = *((CGhoul2Info_v *)args[1]);
			int modelIndex = args[10];

			return G2API_GetBoneAnim(&g2[modelIndex], (const char*)VMA(2), args[3], (float *)VMA(4), (int *)VMA(5),
								(int *)VMA(6), (int *)VMA(7), (float *)VMA(8), (int *)VMA(9));
		}

	case UI_G2_GETBONEFRAME:
		{ //rwwFIXMEFIXME: Just make a G2API_GetBoneFrame func too. This is dirty.
			CGhoul2Info_v &g2 = *((CGhoul2Info_v *)args[1]);
			int modelIndex = args[6];
			int iDontCare1 = 0, iDontCare2 = 0, iDontCare3 = 0;
			float fDontCare1 = 0;

			return G2API_GetBoneAnim(&g2[modelIndex], (const char*)VMA(2), args[3], (float *)VMA(4), &iDontCare1,
								&iDontCare2, &iDontCare3, &fDontCare1, (int *)VMA(5));
		}

	case UI_G2_GETGLANAME:
		//	return (int)G2API_GetGLAName(*((CGhoul2Info_v *)VMA(1)), args[2]);
		{
			char *point = ((char *)VMA(3));
			char *local;
			local = G2API_GetGLAName(*((CGhoul2Info_v *)args[1]), args[2]);
			if (local)
			{
				strcpy(point, local);
			}
		}
		return 0;

	case UI_G2_COPYGHOUL2INSTANCE:
		return (int)G2API_CopyGhoul2Instance(*((CGhoul2Info_v *)args[1]), *((CGhoul2Info_v *)args[2]), args[3]);

	case UI_G2_COPYSPECIFICGHOUL2MODEL:
		G2API_CopySpecificG2Model(*((CGhoul2Info_v *)args[1]), args[2], *((CGhoul2Info_v *)args[3]), args[4]);
		return 0;

	case UI_G2_DUPLICATEGHOUL2INSTANCE:
#ifdef _FULL_G2_LEAK_CHECKING
		g_G2AllocServer = 0;
#endif
		G2API_DuplicateGhoul2Instance(*((CGhoul2Info_v *)args[1]), (CGhoul2Info_v **)VMA(2));
		return 0;

	case UI_G2_HASGHOUL2MODELONINDEX:
		return (int)G2API_HasGhoul2ModelOnIndex((CGhoul2Info_v **)VMA(1), args[2]);
		//return (int)G2API_HasGhoul2ModelOnIndex((CGhoul2Info_v **)args[1], args[2]);

	case UI_G2_REMOVEGHOUL2MODEL:
#ifdef _FULL_G2_LEAK_CHECKING
		g_G2AllocServer = 0;
#endif
		return (int)G2API_RemoveGhoul2Model((CGhoul2Info_v **)VMA(1), args[2]);
		//return (int)G2API_RemoveGhoul2Model((CGhoul2Info_v **)args[1], args[2]);

	case UI_G2_ADDBOLT:
		return G2API_AddBolt(*((CGhoul2Info_v *)args[1]), args[2], (const char *)VMA(3));

//	case UI_G2_REMOVEBOLT:
//		return G2API_RemoveBolt(*((CGhoul2Info_v *)VMA(1)), args[2]);

	case UI_G2_SETBOLTON:
		G2API_SetBoltInfo(*((CGhoul2Info_v *)args[1]), args[2], args[3]);
		return 0;

#ifdef _SOF2	
	case UI_G2_ADDSKINGORE:
		G2API_AddSkinGore(*((CGhoul2Info_v *)args[1]),*(SSkinGoreData *)VMA(2));
		return 0;
#endif // _SOF2
/*
Ghoul2 Insert End
*/
	case UI_G2_SETROOTSURFACE:
		return G2API_SetRootSurface(*((CGhoul2Info_v *)args[1]), args[2], (const char *)VMA(3));

	case UI_G2_SETSURFACEONOFF:
		return G2API_SetSurfaceOnOff(*((CGhoul2Info_v *)args[1]), (const char *)VMA(2), /*(const int)VMA(3)*/args[3]);

	case UI_G2_SETNEWORIGIN:
		return G2API_SetNewOrigin(*((CGhoul2Info_v *)args[1]), /*(const int)VMA(2)*/args[2]);

	case UI_G2_GETTIME:
		return G2API_GetTime(0);

	case UI_G2_SETTIME:
		G2API_SetTime(args[1], args[2]);
		return 0;

	case UI_G2_SETRAGDOLL:
		return 0; //not supported for ui
		break;
	case UI_G2_ANIMATEG2MODELS:
		return 0; //not supported for ui
		break;

	case UI_G2_SETBONEIKSTATE:
		return G2API_SetBoneIKState(*((CGhoul2Info_v *)args[1]), args[2], (const char *)VMA(3), args[4], (sharedSetBoneIKStateParams_t *)VMA(5));
	case UI_G2_IKMOVE:
		return G2API_IKMove(*((CGhoul2Info_v *)args[1]), args[2], (sharedIKMoveParams_t *)VMA(3));

	case UI_G2_GETSURFACENAME:
		{ //Since returning a pointer in such a way to a VM seems to cause MASSIVE FAILURE<tm>, we will shove data into the pointer the vm passes instead
			char *point = ((char *)VMA(4));
			char *local;
			int modelindex = args[3];

			CGhoul2Info_v &g2 = *((CGhoul2Info_v *)args[1]);

			local = G2API_GetSurfaceName(&g2[modelindex], args[2]);
			if (local)
			{
				strcpy(point, local);
			}
		}

		return 0;
	case UI_G2_SETSKIN:
		{
			CGhoul2Info_v &g2 = *((CGhoul2Info_v *)args[1]);
			int modelIndex = args[2];
			
			return G2API_SetSkin(&g2[modelIndex], args[3], args[4]);
		}

	case UI_G2_ATTACHG2MODEL:
		{
			CGhoul2Info_v *g2From = ((CGhoul2Info_v *)args[1]);
			CGhoul2Info_v *g2To = ((CGhoul2Info_v *)args[3]);
			
			return G2API_AttachG2Model(g2From[0], args[2], g2To[0], args[4], args[5]);
		}
/*
Ghoul2 Insert End
*/
	default:
		Com_Error( ERR_DROP, "Bad UI system trap: %i", args[0] );

	}

	return 0;
}

/*
====================
CL_ShutdownUI
====================
*/
void CL_ShutdownUI( void ) {
	cls.keyCatchers &= ~KEYCATCH_UI;
	cls.uiStarted = qfalse;
	if ( !uivm ) {
		return;
	}
	VM_Call( uivm, UI_SHUTDOWN );
	VM_Call( uivm, UI_MENU_RESET );
	VM_Free( uivm );
	uivm = NULL;
}

/*
====================
CL_InitUI
====================
*/

void CL_InitUI( void ) {
	int		v;
	vmInterpret_t		interpret;

	// load the dll or bytecode
	if ( cl_connectedToPureServer != 0 ) {
#if 0
		// if sv_pure is set we only allow qvms to be loaded
		interpret = VMI_COMPILED;
#else //load the module type based on what the server is doing -rww
		interpret = (vmInterpret_t)cl_connectedUI;
#endif
	}
	else {
		interpret = (vmInterpret_t)(int)Cvar_VariableValue( "vm_ui" );
	}
	uivm = VM_Create( "ui", CL_UISystemCalls, interpret );
	if ( !uivm ) {
		Com_Error( ERR_FATAL, "VM_Create on UI failed" );
	}

	// sanity check
	v = VM_Call( uivm, UI_GETAPIVERSION );
	if (v != UI_API_VERSION) {
		Com_Error( ERR_DROP, "User Interface is version %d, expected %d", v, UI_API_VERSION );
		cls.uiStarted = qfalse;
	}
	else {
		// init for this gamestate
		//rww - changed to <= CA_ACTIVE, because that is the state when we did a vid_restart
		//ingame (was just < CA_ACTIVE before, resulting in ingame menus getting wiped and
		//not reloaded on vid restart from ingame menu)
#ifdef _XBOX
//		if(ClientManager::splitScreenMode == qtrue)
//			cls.state = ClientManager::ActiveClient().state;
#endif

#ifdef _XBOX
		// This is kinda a hack to get the UI models that are ALWAYS loaded
		// forced into the UI model memory slot
//		ModelMem.EnterUI();
#endif
		VM_Call( uivm, UI_INIT, (cls.state >= CA_AUTHORIZING && cls.state <= CA_ACTIVE) );

#ifdef _XBOX
//		ModelMem.ExitUI();
#endif
	}
}

qboolean UI_usesUniqueCDKey() {
	if (uivm) {
		return (qboolean)(VM_Call( uivm, UI_HASUNIQUECDKEY) == qtrue);
	} else {
		return qfalse;
	}
}

/*
====================
UI_GameCommand

See if the current console command is claimed by the ui
====================
*/
qboolean UI_GameCommand( void ) {
	if ( !uivm ) {
		return qfalse;
	}

	return (qboolean)VM_Call( uivm, UI_CONSOLE_COMMAND, cls.realtime );
}
