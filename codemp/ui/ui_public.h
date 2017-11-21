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

#pragma once

#define UI_API_VERSION 3
#define UI_LEGACY_API_VERSION 7

typedef struct uiClientState_s {
	connstate_t		connState;
	int				connectPacketCount;
	int				clientNum;
	char			servername[MAX_STRING_CHARS];
	char			updateInfoString[MAX_STRING_CHARS];
	char			messageString[MAX_STRING_CHARS];
} uiClientState_t;

typedef enum uiMenuCommand_e {
	UIMENU_NONE,
	UIMENU_MAIN,
	UIMENU_INGAME,
	UIMENU_PLAYERCONFIG,
	UIMENU_TEAM,
	UIMENU_POSTGAME,
	UIMENU_PLAYERFORCE,
	UIMENU_SIEGEMESSAGE,
	UIMENU_SIEGEOBJECTIVES,
	UIMENU_VOICECHAT,
	UIMENU_CLOSEALL,
	UIMENU_CLASSSEL
} uiMenuCommand_t;

#define SORT_HOST			0
#define SORT_MAP			1
#define SORT_CLIENTS		2
#define SORT_GAME			3
#define SORT_PING			4

typedef enum uiImportLegacy_e {
	UI_ERROR,
	UI_PRINT,
	UI_MILLISECONDS,
	UI_CVAR_SET,
	UI_CVAR_VARIABLEVALUE,
	UI_CVAR_VARIABLESTRINGBUFFER,
	UI_CVAR_SETVALUE,
	UI_CVAR_RESET,
	UI_CVAR_CREATE,
	UI_CVAR_INFOSTRINGBUFFER,
	UI_ARGC,
	UI_ARGV,
	UI_CMD_EXECUTETEXT,
	UI_FS_FOPENFILE,
	UI_FS_READ,
	UI_FS_WRITE,
	UI_FS_FCLOSEFILE,
	UI_FS_GETFILELIST,
	UI_R_REGISTERMODEL,
	UI_R_REGISTERSKIN,
	UI_R_REGISTERSHADERNOMIP,
	UI_R_SHADERNAMEFROMINDEX,
	UI_R_CLEARSCENE,
	UI_R_ADDREFENTITYTOSCENE,
	UI_R_ADDPOLYTOSCENE,
	UI_R_ADDLIGHTTOSCENE,
	UI_R_RENDERSCENE,
	UI_R_SETCOLOR,
	UI_R_DRAWSTRETCHPIC,
	UI_UPDATESCREEN,
	UI_CM_LERPTAG,
	UI_CM_LOADMODEL,
	UI_S_REGISTERSOUND,
	UI_S_STARTLOCALSOUND,
	UI_KEY_KEYNUMTOSTRINGBUF,
	UI_KEY_GETBINDINGBUF,
	UI_KEY_SETBINDING,
	UI_KEY_ISDOWN,
	UI_KEY_GETOVERSTRIKEMODE,
	UI_KEY_SETOVERSTRIKEMODE,
	UI_KEY_CLEARSTATES,
	UI_KEY_GETCATCHER,
	UI_KEY_SETCATCHER,
	UI_GETCLIPBOARDDATA,
	UI_GETGLCONFIG,
	UI_GETCLIENTSTATE,
	UI_GETCONFIGSTRING,
	UI_LAN_GETPINGQUEUECOUNT,
	UI_LAN_CLEARPING,
	UI_LAN_GETPING,
	UI_LAN_GETPINGINFO,
	UI_CVAR_REGISTER,
	UI_CVAR_UPDATE,
	UI_MEMORY_REMAINING,
	UI_GET_CDKEY,
	UI_SET_CDKEY,
	UI_VERIFY_CDKEY,
	UI_R_REGISTERFONT,
	UI_R_FONT_STRLENPIXELS,
	UI_R_FONT_STRLENCHARS,
	UI_R_FONT_STRHEIGHTPIXELS,
	UI_R_FONT_DRAWSTRING,
	UI_LANGUAGE_ISASIAN,
	UI_LANGUAGE_USESSPACES,
	UI_ANYLANGUAGE_READCHARFROMSTRING,
	UI_R_MODELBOUNDS,
	UI_PC_ADD_GLOBAL_DEFINE,
	UI_PC_LOAD_SOURCE,
	UI_PC_FREE_SOURCE,
	UI_PC_READ_TOKEN,
	UI_PC_SOURCE_FILE_AND_LINE,
	UI_PC_LOAD_GLOBAL_DEFINES,
	UI_PC_REMOVE_ALL_GLOBAL_DEFINES,
	UI_S_STOPBACKGROUNDTRACK,
	UI_S_STARTBACKGROUNDTRACK,
	UI_REAL_TIME,
	UI_LAN_GETSERVERCOUNT,
	UI_LAN_GETSERVERADDRESSSTRING,
	UI_LAN_GETSERVERINFO,
	UI_LAN_MARKSERVERVISIBLE,
	UI_LAN_UPDATEVISIBLEPINGS,
	UI_LAN_RESETPINGS,
	UI_LAN_LOADCACHEDSERVERS,
	UI_LAN_SAVECACHEDSERVERS,
	UI_LAN_ADDSERVER,
	UI_LAN_REMOVESERVER,
	UI_CIN_PLAYCINEMATIC,
	UI_CIN_STOPCINEMATIC,
	UI_CIN_RUNCINEMATIC,
	UI_CIN_DRAWCINEMATIC,
	UI_CIN_SETEXTENTS,
	UI_R_REMAP_SHADER,
	UI_LAN_SERVERSTATUS,
	UI_LAN_GETSERVERPING,
	UI_LAN_SERVERISVISIBLE,
	UI_LAN_COMPARESERVERS,

	UI_MEMSET = 100,
	UI_MEMCPY,
	UI_STRNCPY,
	UI_SIN,
	UI_COS,
	UI_ATAN2,
	UI_SQRT,
	UI_MATRIXMULTIPLY,
	UI_ANGLEVECTORS,
	UI_PERPENDICULARVECTOR,
	UI_FLOOR,
	UI_CEIL,
	UI_TESTPRINTINT,
	UI_TESTPRINTFLOAT,
	UI_ACOS,
	UI_ASIN,

	UI_SP_GETNUMLANGUAGES,
	UI_SP_GETLANGUAGENAME,
	UI_SP_GETSTRINGTEXTSTRING = 200,
	UI_G2_LISTSURFACES,
	UI_G2_LISTBONES,
	UI_G2_SETMODELS,
	UI_G2_HAVEWEGHOULMODELS,
	UI_G2_GETBOLT,
	UI_G2_GETBOLT_NOREC,
	UI_G2_GETBOLT_NOREC_NOROT,
	UI_G2_INITGHOUL2MODEL,
	UI_G2_COLLISIONDETECT,
	UI_G2_COLLISIONDETECTCACHE,
	UI_G2_CLEANMODELS,
	UI_G2_ANGLEOVERRIDE,
	UI_G2_PLAYANIM,
	UI_G2_GETBONEANIM,
	UI_G2_GETBONEFRAME, //trimmed down version of GBA, so I don't have to pass all those unused args across the VM-exe border
	UI_G2_GETGLANAME,
	UI_G2_COPYGHOUL2INSTANCE,
	UI_G2_COPYSPECIFICGHOUL2MODEL,
	UI_G2_DUPLICATEGHOUL2INSTANCE,
	UI_G2_HASGHOUL2MODELONINDEX,
	UI_G2_REMOVEGHOUL2MODEL,
	UI_G2_ADDBOLT,
	UI_G2_SETBOLTON,
	UI_G2_SETROOTSURFACE,
	UI_G2_SETSURFACEONOFF,
	UI_G2_SETNEWORIGIN,
	UI_G2_GETTIME,
	UI_G2_SETTIME,
	UI_G2_SETRAGDOLL,
	UI_G2_ANIMATEG2MODELS,
	UI_G2_SETBONEIKSTATE,
	UI_G2_IKMOVE,
	UI_G2_GETSURFACENAME,
	UI_G2_SETSKIN,
	UI_G2_ATTACHG2MODEL,
} uiImportLegacy_t;

typedef enum uiExportLegacy_e {
	UI_GETAPIVERSION = 0,
	UI_INIT,
	UI_SHUTDOWN,
	UI_KEY_EVENT,
	UI_MOUSE_EVENT,
	UI_REFRESH,
	UI_IS_FULLSCREEN,
	UI_SET_ACTIVE_MENU,
	UI_CONSOLE_COMMAND,
	UI_DRAW_CONNECT_SCREEN,
	UI_HASUNIQUECDKEY,
	UI_MENU_RESET
} uiExportLegacy_t;

typedef struct uiImport_s {
	void			(*Print)								( const char *msg, ... );
	NORETURN_PTR void (*Error)( int level, const char *fmt, ... );
	int				(*Milliseconds)							( void );
	int				(*RealTime)								( qtime_t *qtime );
	int				(*MemoryRemaining)						( void );

	void			(*Cvar_Create)							( const char *var_name, const char *var_value, uint32_t flags );
	void			(*Cvar_InfoStringBuffer)				( int bit, char *buffer, int bufsize );
	void			(*Cvar_Register)						( vmCvar_t *cvar, const char *var_name, const char *value, uint32_t flags );
	void			(*Cvar_Reset)							( const char *name );
	void			(*Cvar_Set)								( const char *var_name, const char *value );
	void			(*Cvar_SetValue)						( const char *var_name, float value );
	void			(*Cvar_Update)							( vmCvar_t *cvar );
	void			(*Cvar_VariableStringBuffer)			( const char *var_name, char *buffer, int bufsize );
	float			(*Cvar_VariableValue)					( const char *var_name );

	int				(*Cmd_Argc)								( void );
	void			(*Cmd_Argv)								( int n, char *buffer, int bufferLength );
	void			(*Cmd_ExecuteText)						( int exec_when, const char *text );

	void			(*FS_Close)								( fileHandle_t f );
	int				(*FS_GetFileList)						( const char *path, const char *extension, char *listbuf, int bufsize );
	int				(*FS_Open)								( const char *qpath, fileHandle_t *f, fsMode_t mode );
	int				(*FS_Read)								( void *buffer, int len, fileHandle_t f );
	int				(*FS_Write)								( const void *buffer, int len, fileHandle_t f );

	void			(*GetClientState)						( uiClientState_t *state );
	void			(*GetClipboardData)						( char *buf, int bufsize );
	int				(*GetConfigString)						( int index, char *buff, int buffsize );
	void			(*GetGlconfig)							( glconfig_t *glconfig );
	void			(*UpdateScreen)							( void );

	void			(*Key_ClearStates)						( void );
	void			(*Key_GetBindingBuf)					( int keynum, char *buf, int buflen );
	qboolean		(*Key_IsDown)							( int keynum );
	void			(*Key_KeynumToStringBuf)				( int keynum, char *buf, int buflen );
	void			(*Key_SetBinding)						( int keynum, const char *binding );
	int				(*Key_GetCatcher)						( void );
	qboolean		(*Key_GetOverstrikeMode)				( void );
	void			(*Key_SetCatcher)						( int catcher );
	void			(*Key_SetOverstrikeMode)				( qboolean state );

	int				(*PC_AddGlobalDefine)					( char *define );
	int				(*PC_FreeSource)						( int handle );
	int				(*PC_LoadGlobalDefines)					( const char* filename );
	int				(*PC_LoadSource)						( const char *filename );
	int				(*PC_ReadToken)							( int handle, pc_token_t *pc_token );
	void			(*PC_RemoveAllGlobalDefines)			( void );
	int				(*PC_SourceFileAndLine)					( int handle, char *filename, int *line );

	void			(*CIN_DrawCinematic)					( int handle );
	int				(*CIN_PlayCinematic)					( const char *arg0, int xpos, int ypos, int width, int height, int bits );
	e_status		(*CIN_RunCinematic)						( int handle );
	void			(*CIN_SetExtents)						( int handle, int x, int y, int w, int h );
	e_status		(*CIN_StopCinematic)					( int handle );

	int				(*LAN_AddServer)						( int source, const char *name, const char *addr );
	void			(*LAN_ClearPing)						( int n );
	int				(*LAN_CompareServers)					( int source, int sortKey, int sortDir, int s1, int s2 );
	void			(*LAN_GetPing)							( int n, char *buf, int buflen, int *pingtime );
	void			(*LAN_GetPingInfo)						( int n, char *buf, int buflen );
	int				(*LAN_GetPingQueueCount)				( void );
	void			(*LAN_GetServerAddressString)			( int source, int n, char *buf, int buflen );
	int				(*LAN_GetServerCount)					( int source );
	void			(*LAN_GetServerInfo)					( int source, int n, char *buf, int buflen );
	int				(*LAN_GetServerPing)					( int source, int n );
	void			(*LAN_LoadCachedServers)				( void );
	void			(*LAN_MarkServerVisible)				( int source, int n, qboolean visible );
	void			(*LAN_RemoveServer)						( int source, const char *addr );
	void			(*LAN_ResetPings)						( int n );
	void			(*LAN_SaveCachedServers)				( void );
	int				(*LAN_ServerIsVisible)					( int source, int n );
	int				(*LAN_ServerStatus)						( const char *serverAddress, char *serverStatus, int maxLen );
	qboolean		(*LAN_UpdateVisiblePings)				( int source );

	void			(*S_StartBackgroundTrack)				( const char *intro, const char *loop, qboolean bReturnWithoutStarting );
	void			(*S_StartLocalSound)					( sfxHandle_t sfx, int channelNum );
	void			(*S_StopBackgroundTrack)				( void );
	sfxHandle_t		(*S_RegisterSound)						( const char *sample );

	void			(*SE_GetLanguageName)					( const int languageIndex, char *buffer );
	int				(*SE_GetNumLanguages)					( void );
	qboolean		(*SE_GetStringTextString)				( const char *text, char *buffer, int bufferLength );

	qboolean		(*R_Language_IsAsian)					( void );
	qboolean		(*R_Language_UsesSpaces)				( void );
	unsigned int	(*R_AnyLanguage_ReadCharFromString)		( const char *psText, int *piAdvanceCount, qboolean *pbIsTrailingPunctuation );

	void			(*R_AddLightToScene)					( const vec3_t org, float intensity, float r, float g, float b );
	void			(*R_AddPolysToScene)					( qhandle_t hShader, int numVerts, const polyVert_t *verts, int num );
	void			(*R_AddRefEntityToScene)				( const refEntity_t *re );
	void			(*R_ClearScene)							( void );
	void			(*R_DrawStretchPic)						( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader );
	int				(*R_Font_StrLenPixels)					( const char *text, const int iFontIndex, const float scale );
	int				(*R_Font_StrLenChars)					( const char *text );
	int				(*R_Font_HeightPixels)					( const int iFontIndex, const float scale );
	void			(*R_Font_DrawString)					( int ox, int oy, const char *text, const float *rgba, const int setIndex, int iCharLimit, const float scale );
	int				(*R_LerpTag)							( orientation_t *tag, clipHandle_t mod, int startFrame, int endFrame, float frac, const char *tagName );
	void			(*R_ModelBounds)						( clipHandle_t model, vec3_t mins, vec3_t maxs );
	qhandle_t		(*R_RegisterModel)						( const char *name );
	qhandle_t		(*R_RegisterSkin)						( const char *name );
	qhandle_t		(*R_RegisterShaderNoMip)				( const char *name );
	qhandle_t		(*R_RegisterFont)						( const char *fontName );
	void			(*R_RemapShader)						( const char *oldShader, const char *newShader, const char *timeOffset );
	void			(*R_RenderScene)						( const refdef_t *fd );
	void			(*R_SetColor)							( const float *rgba );
	void			(*R_ShaderNameFromIndex)				( char *name, int index );

	void			(*G2_ListModelSurfaces)					( void *ghlInfo );
	void			(*G2_ListModelBones)					( void *ghlInfo, int frame );
	void			(*G2_SetGhoul2ModelIndexes)				( void *ghoul2, qhandle_t *modelList, qhandle_t *skinList );
	qboolean		(*G2_HaveWeGhoul2Models)				( void *ghoul2 );
	qboolean		(*G2API_GetBoltMatrix)					( void *ghoul2, const int modelIndex, const int boltIndex, mdxaBone_t *matrix, const vec3_t angles, const vec3_t position, const int frameNum, qhandle_t *modelList, vec3_t scale );
	qboolean		(*G2API_GetBoltMatrix_NoReconstruct)	( void *ghoul2, const int modelIndex, const int boltIndex, mdxaBone_t *matrix, const vec3_t angles, const vec3_t position, const int frameNum, qhandle_t *modelList, vec3_t scale );
	qboolean		(*G2API_GetBoltMatrix_NoRecNoRot)		( void *ghoul2, const int modelIndex, const int boltIndex, mdxaBone_t *matrix, const vec3_t angles, const vec3_t position, const int frameNum, qhandle_t *modelList, vec3_t scale );
	int				(*G2API_InitGhoul2Model)				( void **ghoul2Ptr, const char *fileName, int modelIndex, qhandle_t customSkin, qhandle_t customShader, int modelFlags, int lodBias );
	void			(*G2API_CollisionDetect)				( CollisionRecord_t *collRecMap, void* ghoul2, const vec3_t angles, const vec3_t position, int frameNumber, int entNum, vec3_t rayStart, vec3_t rayEnd, vec3_t scale, int traceFlags, int useLod, float fRadius );
	void			(*G2API_CollisionDetectCache)			( CollisionRecord_t *collRecMap, void* ghoul2, const vec3_t angles, const vec3_t position, int frameNumber, int entNum, vec3_t rayStart, vec3_t rayEnd, vec3_t scale, int traceFlags, int useLod, float fRadius );
	void			(*G2API_CleanGhoul2Models)				( void **ghoul2Ptr );
	qboolean		(*G2API_SetBoneAngles)					( void *ghoul2, int modelIndex, const char *boneName, const vec3_t angles, const int flags, const int up, const int right, const int forward, qhandle_t *modelList, int blendTime , int currentTime );
	qboolean		(*G2API_SetBoneAnim)					( void *ghoul2, const int modelIndex, const char *boneName, const int startFrame, const int endFrame, const int flags, const float animSpeed, const int currentTime, const float setFrame, const int blendTime );
	qboolean		(*G2API_GetBoneAnim)					( void *ghoul2, const char *boneName, const int currentTime, float *currentFrame, int *startFrame, int *endFrame, int *flags, float *animSpeed, int *modelList, const int modelIndex );
	qboolean		(*G2API_GetBoneFrame)					( void *ghoul2, const char *boneName, const int currentTime, float *currentFrame, int *modelList, const int modelIndex );
	void			(*G2API_GetGLAName)						( void *ghoul2, int modelIndex, char *fillBuf );
	int				(*G2API_CopyGhoul2Instance)				( void *g2From, void *g2To, int modelIndex );
	void			(*G2API_CopySpecificGhoul2Model)		( void *g2From, int modelFrom, void *g2To, int modelTo );
	void			(*G2API_DuplicateGhoul2Instance)		( void *g2From, void **g2To );
	qboolean		(*G2API_HasGhoul2ModelOnIndex)			( void *ghlInfo, int modelIndex );
	qboolean		(*G2API_RemoveGhoul2Model)				( void *ghlInfo, int modelIndex );
	int				(*G2API_AddBolt)						( void *ghoul2, int modelIndex, const char *boneName );
	void			(*G2API_SetBoltInfo)					( void *ghoul2, int modelIndex, int boltInfo );
	qboolean		(*G2API_SetRootSurface)					( void *ghoul2, const int modelIndex, const char *surfaceName );
	qboolean		(*G2API_SetSurfaceOnOff)				( void *ghoul2, const char *surfaceName, const int flags );
	qboolean		(*G2API_SetNewOrigin)					( void *ghoul2, const int boltIndex );
	int				(*G2API_GetTime)						( void );
	void			(*G2API_SetTime)						( int time, int clock );
	void			(*G2API_SetRagDoll)						( void *ghoul2, sharedRagDollParams_t *params );
	void			(*G2API_AnimateG2Models)				( void *ghoul2, int time, sharedRagDollUpdateParams_t *params );
	qboolean		(*G2API_SetBoneIKState)					( void *ghoul2, int time, const char *boneName, int ikState, sharedSetBoneIKStateParams_t *params );
	qboolean		(*G2API_IKMove)							( void *ghoul2, int time, sharedIKMoveParams_t *params );
	void			(*G2API_GetSurfaceName)					( void *ghoul2, int surfNumber, int modelIndex, char *fillBuf );
	qboolean		(*G2API_SetSkin)						( void *ghoul2, int modelIndex, qhandle_t customSkin, qhandle_t renderSkin );
	qboolean		(*G2API_AttachG2Model)					( void *ghoul2From, int modelIndexFrom, void *ghoul2To, int toBoltIndex, int toModel );

	struct {
		float			(*R_Font_StrLenPixels)					( const char *text, const int iFontIndex, const float scale );
		void			(*AddCommand)							( const char *cmd_name );
		void			(*RemoveCommand)						( const char *cmd_name );
	} ext;
} uiImport_t;

typedef struct uiExport_s {
	void		(*Init)					( qboolean inGameLoad );
	void		(*Shutdown)				( void );
	void		(*KeyEvent)				( int key, qboolean down );
	void		(*MouseEvent)			( int dx, int dy );
	void		(*Refresh)				( int realtime );
	qboolean	(*IsFullscreen)			( void );
	void		(*SetActiveMenu)		( uiMenuCommand_t menu );
	qboolean	(*ConsoleCommand)		( int realTime );
	void		(*DrawConnectScreen)	( qboolean overlay );
	void		(*MenuReset)			( void );
} uiExport_t;

//linking of ui library
typedef uiExport_t* (QDECL *GetUIAPI_t)( int apiVersion, uiImport_t *import );
