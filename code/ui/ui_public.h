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

#ifndef __UI_PUBLIC_H__
#define __UI_PUBLIC_H__


#include "../client/keycodes.h"


#define UI_API_VERSION	3


typedef struct {
	//============== general Quake services ==================

	// print message on the local console
	void		(*Printf)( const char *fmt, ... );

	// abort the game
	NORETURN_PTR void	(*Error)( int level, const char *fmt, ... );

	// console variable interaction
	void		(*Cvar_Set)( const char *name, const char *value );
	float		(*Cvar_VariableValue)( const char *var_name );
	void		(*Cvar_VariableStringBuffer)( const char *var_name, char *buffer, int bufsize );
	void		(*Cvar_SetValue)( const char *var_name, float value );
	void		(*Cvar_Reset)( const char *name );
	void		(*Cvar_Create)( const char *var_name, const char *var_value, int flags );
	void		(*Cvar_InfoStringBuffer)( int bit, char *buffer, int bufsize );

	// console command interaction
	int			(*Argc)( void );
	void		(*Argv)( int n, char *buffer, int bufferLength );
	void		(*Cmd_ExecuteText)( int exec_when, const char *text );
	void		(*Cmd_TokenizeString)( const char *text );

	// filesystem access
	int			(*FS_FOpenFile)( const char *qpath, fileHandle_t *file, fsMode_t mode );
	int 		(*FS_Read)( void *buffer, int len, fileHandle_t f );
	int 		(*FS_Write)( const void *buffer, int len, fileHandle_t f );
	void		(*FS_FCloseFile)( fileHandle_t f );
	int			(*FS_GetFileList)(  const char *path, const char *extension, char *listbuf, int bufsize );
	long		(*FS_ReadFile)( const char *name, void **buf );
	void		(*FS_FreeFile)( void *buf );

	// =========== renderer function calls ================

	qhandle_t	(*R_RegisterModel)( const char *name );			// returns rgb axis if not found
	qhandle_t	(*R_RegisterSkin)( const char *name );			// returns all white if not found
	qhandle_t	(*R_RegisterShader)( const char *name );			// returns white if not found
	qhandle_t	(*R_RegisterShaderNoMip)( const char *name );			// returns white if not found
	qhandle_t	(*R_RegisterFont)( const char *name );			// returns 0 for bad font

	int			(*R_Font_StrLenPixels)(const char *text, const int setIndex, const float scale );
	int			(*R_Font_HeightPixels)(const int setIndex, const float scale );
	void		(*R_Font_DrawString)(int ox, int oy, const char *text, const float *rgba, const int setIndex, int iMaxPixelWidth, const float scale );
	int			(*R_Font_StrLenChars)(const char *text);
	qboolean	(*Language_IsAsian) (void);
	qboolean	(*Language_UsesSpaces) (void);
	unsigned int (*AnyLanguage_ReadCharFromString)( char *psText, int *piAdvanceCount, qboolean *pbIsTrailingPunctuation /* = NULL */);

	// a scene is built up by calls to R_ClearScene and the various R_Add functions.
	// Nothing is drawn until R_RenderScene is called.
	void		(*R_ClearScene)( void );
	void		(*R_AddRefEntityToScene)( const refEntity_t *re );
	void		(*R_AddPolyToScene)( qhandle_t hShader , int numVerts, const polyVert_t *verts );
	void		(*R_AddLightToScene)( const vec3_t org, float intensity, float r, float g, float b );
	void		(*R_RenderScene)( const refdef_t *fd );

	void		(*R_ModelBounds)( qhandle_t handle, vec3_t mins, vec3_t maxs );

	void		(*R_SetColor)( const float *rgba );	// NULL = 1,1,1,1
	void		(*R_DrawStretchPic) ( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader );	// 0 = white
	void		(*R_ScissorPic) ( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader );	// 0 = white

	// force a screen update, only used during gamestate load
	void		(*UpdateScreen)( void );

#ifdef JK2_MODE
	// stuff for savegame screenshots...
	void		(*PrecacheScreenshot)( void );
#endif

	//========= model collision ===============

	// R_LerpTag is only valid for md3 models
	void		(*R_LerpTag)( orientation_t *tag, clipHandle_t mod, int startFrame, int endFrame,
						 float frac, const char *tagName );

	// =========== sound function calls ===============

	void		(*S_StartLocalSound)( sfxHandle_t sfxHandle, int channelNum );
	sfxHandle_t	(*S_RegisterSound)( const char* name);
	void		(*S_StartLocalLoopingSound)( sfxHandle_t sfxHandle);
	void		(*S_StopSounds)( void );


	// =========== getting save game picture ===============
	void	(*DrawStretchRaw) (int x, int y, int w, int h, int cols, int rows, const byte *data, int client, qboolean dirty);
#ifdef JK2_MODE
	qboolean(*SG_GetSaveImage)( const char *psPathlessBaseName, void *pvAddress );
#endif
	int		(*SG_GetSaveGameComment)(const char *psPathlessBaseName, char *sComment, char *sMapName);
	qboolean (*SG_GameAllowedToSaveHere)(qboolean inCamera);
	void (*SG_StoreSaveGameComment)(const char *sComment);
	//byte *(*SCR_GetScreenshot)(qboolean *);

	// =========== data shared with the client system =============

	// keyboard and key binding interaction
	void		(*Key_KeynumToStringBuf)( int keynum, char *buf, int buflen );
	void		(*Key_GetBindingBuf)( int keynum, char *buf, int buflen );
	void		(*Key_SetBinding)( int keynum, const char *binding );
	qboolean	(*Key_IsDown)( int keynum );
	qboolean	(*Key_GetOverstrikeMode)( void );
	void		(*Key_SetOverstrikeMode)( qboolean state );
	void		(*Key_ClearStates)( void );
	int			(*Key_GetCatcher)( void );
	void		(*Key_SetCatcher)( int catcher );

#ifdef JK2_MODE
	qboolean	(*SP_Register)( const char *Package, unsigned char Registration );
	const char *(*SP_GetStringText)(unsigned short ID);
	const char *(*SP_GetStringTextString)(const char *Reference);
#endif
	void		(*GetClipboardData)( char *buf, int bufsize );

	void		(*GetGlconfig)( glconfig_t *config );

	connstate_t	(*GetClientState)( void );

	void		(*GetConfigString)( int index, char* buff, int buffsize );

	int			(*Milliseconds)( void );
	void		(*Draw_DataPad)(int HUDType);
} uiimport_t;

typedef enum {
	DP_HUD=0,
	DP_OBJECTIVES,
	DP_WEAPONS,
	DP_INVENTORY,
	DP_FORCEPOWERS
}dpTypes_t;

typedef enum {
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
	UI_ARGC,						//	10
	UI_ARGV,
	UI_CMD_EXECUTETEXT,
	UI_FS_FOPENFILE,
	UI_FS_READ,
	UI_FS_WRITE,
	UI_FS_FCLOSEFILE,
	UI_FS_GETFILELIST,
	UI_R_REGISTERMODEL,
	UI_R_REGISTERSKIN,
	UI_R_REGISTERSHADERNOMIP,		//	20
	UI_R_CLEARSCENE,
	UI_R_ADDREFENTITYTOSCENE,
	UI_R_ADDPOLYTOSCENE,
	UI_R_ADDLIGHTTOSCENE,
	UI_R_RENDERSCENE,
	UI_R_SETCOLOR,
	UI_R_DRAWSTRETCHPIC,
	UI_UPDATESCREEN,
	UI_CM_LERPTAG,
	UI_CM_LOADMODEL,				//	30
	UI_S_REGISTERSOUND,
	UI_S_STARTLOCALSOUND,
	UI_KEY_KEYNUMTOSTRINGBUF,
	UI_KEY_GETBINDINGBUF,
	UI_KEY_SETBINDING,
	UI_KEY_ISDOWN,
	UI_KEY_GETOVERSTRIKEMODE,
	UI_KEY_SETOVERSTRIKEMODE,
	UI_KEY_CLEARSTATES,
	UI_KEY_GETCATCHER,				//	40
	UI_KEY_SETCATCHER,
	UI_GETCLIPBOARDDATA,
	UI_GETGLCONFIG,
	UI_GETCLIENTSTATE,
	UI_GETCONFIGSTRING,
	UI_LAN_GETPINGQUEUECOUNT,
	UI_LAN_CLEARPING,
	UI_LAN_GETPING,
	UI_LAN_GETPINGINFO,
	UI_CVAR_REGISTER,				//	50
	UI_CVAR_UPDATE,
	UI_MEMORY_REMAINING,
	UI_GET_CDKEY,
	UI_SET_CDKEY,
	UI_R_REGISTERFONT,
	UI_R_MODELBOUNDS,
	UI_PC_ADD_GLOBAL_DEFINE,
	UI_PC_LOAD_SOURCE,
	UI_PC_FREE_SOURCE,
	UI_PC_READ_TOKEN,				//	60
	UI_PC_SOURCE_FILE_AND_LINE,
	UI_S_STOPBACKGROUNDTRACK,
	UI_S_STARTBACKGROUNDTRACK,
	UI_REAL_TIME,
	UI_LAN_GETSERVERCOUNT,
	UI_LAN_GETSERVERADDRESSSTRING,
	UI_LAN_GETSERVERINFO,
	UI_LAN_MARKSERVERVISIBLE,
	UI_LAN_UPDATEVISIBLEPINGS,
	UI_LAN_RESETPINGS,				//	70
	UI_LAN_LOADCACHEDSERVERS,
	UI_LAN_SAVECACHEDSERVERS,
	UI_LAN_ADDSERVER,
	UI_LAN_REMOVESERVER,
	UI_CIN_PLAYCINEMATIC,
	UI_CIN_STOPCINEMATIC,
	UI_CIN_RUNCINEMATIC,
	UI_CIN_DRAWCINEMATIC,
	UI_CIN_SETEXTENTS,
	UI_R_REMAP_SHADER,				//	80
	UI_VERIFY_CDKEY,
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
	UI_FLOOR,
	UI_CEIL
} uiImport_t;

#endif
