/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2005 - 2015, ioquake3 contributors
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

#ifndef TR_PUBLIC_H
#define TR_PUBLIC_H

#include "../rd-common/tr_types.h"
#include "../qcommon/MiniHeap.h"
#include "../qcommon/qcommon.h"
#include "../ghoul2/ghoul2_shared.h"

#define	REF_API_VERSION 9

//
// these are the functions exported by the refresh module
//

typedef struct refexport_s {
	// called before the library is unloaded
	// if the system is just reconfiguring, pass destroyWindow = qfalse,
	// which will keep the screen from flashing to the desktop.
	void				(*Shutdown)								( qboolean destroyWindow, qboolean restarting );

	// All data that will be used in a level should be
	// registered before rendering any frames to prevent disk hits,
	// but they can still be registered at a later time
	// if necessary.
	//
	// BeginRegistration makes any existing media pointers invalid
	// and returns the current gl configuration, including screen width
	// and height, which can be used by the client to intelligently
	// size display elements
	void				(*BeginRegistration)					( glconfig_t *config );
	qhandle_t			(*RegisterModel)						( const char *name );
	qhandle_t			(*RegisterServerModel)					( const char *name );
	qhandle_t			(*RegisterSkin)							( const char *name );
	qhandle_t			(*RegisterServerSkin)					( const char *name );
	qhandle_t			(*RegisterShader)						( const char *name );
	qhandle_t			(*RegisterShaderNoMip)					( const char *name );
	const char *		(*ShaderNameFromIndex)					( int index );
	void				(*LoadWorld)							( const char *name );

	// the vis data is a large enough block of data that we go to the trouble
	// of sharing it with the clipmodel subsystem
	void				(*SetWorldVisData)						( const byte *vis );

	// EndRegistration will draw a tiny polygon with each texture, forcing
	// them to be loaded into card memory
	void				(*EndRegistration)						( void );

	// a scene is built up by calls to R_ClearScene and the various R_Add functions.
	// Nothing is drawn until R_RenderScene is called.
	void				(*ClearScene)							( void );
	void				(*ClearDecals)							( void );
	void				(*AddRefEntityToScene)					( const refEntity_t *re );
	void				(*AddMiniRefEntityToScene)				( const miniRefEntity_t *re );
	void				(*AddPolyToScene)						( qhandle_t hShader , int numVerts, const polyVert_t *verts, int num );
	void				(*AddDecalToScene)						( qhandle_t shader, const vec3_t origin, const vec3_t dir, float orientation, float r, float g, float b, float a, qboolean alphaFade, float radius, qboolean temporary );
	int					(*LightForPoint)						( vec3_t point, vec3_t ambientLight, vec3_t directedLight, vec3_t lightDir );
	void				(*AddLightToScene)						( const vec3_t org, float intensity, float r, float g, float b );
	void				(*AddAdditiveLightToScene)				( const vec3_t org, float intensity, float r, float g, float b );

	void				(*RenderScene)							( const refdef_t *fd );

	void				(*SetColor)								( const float *rgba );	// NULL = 1,1,1,1
	void				(*DrawStretchPic)						( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader );	// 0 = white
	void				(*DrawRotatePic)						( float x, float y, float w, float h, float s1, float t1, float s2, float t2, float a1, qhandle_t hShader );	// 0 = white
	void				(*DrawRotatePic2)						( float x, float y, float w, float h, float s1, float t1, float s2, float t2, float a1, qhandle_t hShader );	// 0 = white

	// Draw images for cinematic rendering, pass as 32 bit rgba
	void				(*DrawStretchRaw)						( int x, int y, int w, int h, int cols, int rows, const byte *data, int client, qboolean dirty );
	void				(*UploadCinematic)						( int cols, int rows, const byte *data, int client, qboolean dirty );

	void				(*BeginFrame)							( stereoFrame_t stereoFrame );

	// if the pointers are not NULL, timing info will be returned
	void				(*EndFrame)								( int *frontEndMsec, int *backEndMsec );


	int					(*MarkFragments)						( int numPoints, const vec3_t *points, const vec3_t projection, int maxPoints, vec3_t pointBuffer, int maxFragments, markFragment_t *fragmentBuffer );

	int					(*LerpTag)								( orientation_t *tag,  qhandle_t model, int startFrame, int endFrame, float frac, const char *tagName );
	void				(*ModelBounds)							( qhandle_t model, vec3_t mins, vec3_t maxs );
	void				(*ModelBoundsRef)						( refEntity_t *model, vec3_t mins, vec3_t maxs );

	qhandle_t			(*RegisterFont)							( const char *fontName );
	int					(*Font_StrLenPixels)					( const char *text, const int iFontIndex, const float scale );
	int					(*Font_StrLenChars)						( const char *text );
	int					(*Font_HeightPixels)					( const int iFontIndex, const float scale );
	void				(*Font_DrawString)						( int ox, int oy, const char *text, const float *rgba, const int setIndex, int iCharLimit, const float scale );
	qboolean			(*Language_IsAsian)						( void );
	qboolean			(*Language_UsesSpaces)					( void );
	unsigned int		(*AnyLanguage_ReadCharFromString)		( const char *psText, int *piAdvanceCount, qboolean *pbIsTrailingPunctuation/* = NULL*/ );

	void				(*RemapShader)							( const char *oldShader, const char *newShader, const char *offsetTime );
	qboolean			(*GetEntityToken)						( char *buffer, int size );
	qboolean			(*inPVS)								( const vec3_t p1, const vec3_t p2, byte *mask );

	void				(*GetLightStyle)						( int style, color4ub_t color );
	void				(*SetLightStyle)						( int style, int color );

	void				(*GetBModelVerts)						( int bmodelIndex, vec3_t *vec, vec3_t normal );

	// These were missing in 1.01, had direct access to renderer backend
	void				(*SetRangedFog)							( float range );
	void				(*SetRefractionProperties)				( float distortionAlpha, float distortionStretch, qboolean distortionPrePost, qboolean distortionNegate );
	float				(*GetDistanceCull)						( void );
	void				(*GetRealRes)							( int *w, int *h );
	void				(*AutomapElevationAdjustment)			( float newHeight );
	qboolean			(*InitializeWireframeAutomap)			( void );
	void				(*AddWeatherZone)						( vec3_t mins, vec3_t maxs );
	void				(*WorldEffectCommand)					( const char *command );
	void				(*RegisterMedia_LevelLoadBegin)			( const char *psMapName, ForceReload_e eForceReload );
	void				(*RegisterMedia_LevelLoadEnd)			( void );
	int					(*RegisterMedia_GetLevel)				( void );
	qboolean			(*RegisterImages_LevelLoadEnd)			( void );
	qboolean			(*RegisterModels_LevelLoadEnd)			( qboolean bDeleteEverythingNotUsedThisLevel );

	// AVI recording
	void				(*TakeVideoFrame)						( int h, int w, byte* captureBuffer, byte *encodeBuffer, qboolean motionJpeg );

	// G2 stuff
	void				(*InitSkins)							( void );
	void				(*InitShaders)							( qboolean server );
	void				(*SVModelInit)							( void );
	void				(*HunkClearCrap)						( void );

	// G2API
	int					(*G2API_AddBolt)						( CGhoul2Info_v &ghoul2, const int modelIndex, const char *boneName );
	int					(*G2API_AddBoltSurfNum)					( CGhoul2Info *ghlInfo, const int surfIndex );
	int					(*G2API_AddSurface)						( CGhoul2Info *ghlInfo, int surfaceNumber, int polyNumber, float BarycentricI, float BarycentricJ, int lod );
	void				(*G2API_AnimateG2ModelsRag)				( CGhoul2Info_v &ghoul2, int AcurrentTime, CRagDollUpdateParams *params );
	qboolean			(*G2API_AttachEnt)						( int *boltInfo, CGhoul2Info_v& ghoul2, int modelIndex, int toBoltIndex, int entNum, int toModelNum );
	qboolean			(*G2API_AttachG2Model)					( CGhoul2Info_v &ghoul2From, int modelFrom, CGhoul2Info_v &ghoul2To, int toBoltIndex, int toModel );
	void				(*G2API_AttachInstanceToEntNum)			( CGhoul2Info_v &ghoul2, int entityNum, qboolean server );
	void				(*G2API_AbsurdSmoothing)				( CGhoul2Info_v &ghoul2, qboolean status );
	void				(*G2API_BoltMatrixReconstruction)		( qboolean reconstruct );
	void				(*G2API_BoltMatrixSPMethod)				( qboolean spMethod );
	void				(*G2API_CleanEntAttachments)			( void );
	void				(*G2API_CleanGhoul2Models)				( CGhoul2Info_v **ghoul2Ptr );
	void				(*G2API_ClearAttachedInstance)			( int entityNum );
	void				(*G2API_CollisionDetect)				( CollisionRecord_t *collRecMap, CGhoul2Info_v &ghoul2, const vec3_t angles, const vec3_t position, int frameNumber, int entNum, vec3_t rayStart, vec3_t rayEnd, vec3_t scale, IHeapAllocator *G2VertSpace, int traceFlags, int useLod, float fRadius );
	void				(*G2API_CollisionDetectCache)			( CollisionRecord_t *collRecMap, CGhoul2Info_v &ghoul2, const vec3_t angles, const vec3_t position, int frameNumber, int entNum, vec3_t rayStart, vec3_t rayEnd, vec3_t scale, IHeapAllocator *G2VertSpace, int traceFlags, int useLod, float fRadius );
	int					(*G2API_CopyGhoul2Instance)				( CGhoul2Info_v &g2From, CGhoul2Info_v &g2To, int modelIndex );
	void				(*G2API_CopySpecificG2Model)			( CGhoul2Info_v &ghoul2From, int modelFrom, CGhoul2Info_v &ghoul2To, int modelTo );
	qboolean			(*G2API_DetachG2Model)					( CGhoul2Info *ghlInfo );
	qboolean			(*G2API_DoesBoneExist)					( CGhoul2Info_v& ghoul2, int modelIndex, const char *boneName );
	void				(*G2API_DuplicateGhoul2Instance)		( CGhoul2Info_v &g2From, CGhoul2Info_v **g2To );
	void				(*G2API_FreeSaveBuffer)					( char *buffer );
	qboolean			(*G2API_GetAnimFileName)				( CGhoul2Info *ghlInfo, char **filename );
	char *				(*G2API_GetAnimFileNameIndex)			( qhandle_t modelIndex );
	qboolean			(*G2API_GetAnimRange)					( CGhoul2Info *ghlInfo, const char *boneName, int *startFrame, int *endFrame );
	qboolean			(*G2API_GetBoltMatrix)					( CGhoul2Info_v &ghoul2, const int modelIndex, const int boltIndex, mdxaBone_t *matrix, const vec3_t angles, const vec3_t position, const int frameNum, qhandle_t *modelList, vec3_t scale );
	qboolean			(*G2API_GetBoneAnim)					( CGhoul2Info_v& ghoul2, int modelIndex, const char *boneName, const int currentTime, float *currentFrame, int *startFrame, int *endFrame, int *flags, float *animSpeed, qhandle_t *modelList );
	int					(*G2API_GetBoneIndex)					( CGhoul2Info *ghlInfo, const char *boneName );
	int					(*G2API_GetGhoul2ModelFlags)			( CGhoul2Info *ghlInfo );
	char *				(*G2API_GetGLAName)						( CGhoul2Info_v &ghoul2, int modelIndex );
	const char *		(*G2API_GetModelName)					( CGhoul2Info_v& ghoul2, int modelIndex );
	int					(*G2API_GetParentSurface)				( CGhoul2Info *ghlInfo, const int index );
	qboolean			(*G2API_GetRagBonePos)					( CGhoul2Info_v &ghoul2, const char *boneName, vec3_t pos, vec3_t entAngles, vec3_t entPos, vec3_t entScale );
	int					(*G2API_GetSurfaceIndex)				( CGhoul2Info *ghlInfo, const char *surfaceName );
	char *				(*G2API_GetSurfaceName)					( CGhoul2Info_v& ghlInfo, int modelIndex, int surfNumber );
	int					(*G2API_GetSurfaceOnOff)				( CGhoul2Info *ghlInfo, const char *surfaceName );
	int					(*G2API_GetSurfaceRenderStatus)			( CGhoul2Info_v& ghoul2, int modelIndex, const char *surfaceName );
	int					(*G2API_GetTime)						( int argTime );
	int					(*G2API_Ghoul2Size)						( CGhoul2Info_v &ghoul2 );
	void				(*G2API_GiveMeVectorFromMatrix)			( mdxaBone_t *boltMatrix, Eorientations flags, vec3_t vec );
	qboolean			(*G2API_HasGhoul2ModelOnIndex)			( CGhoul2Info_v **ghlRemove, const int modelIndex );
	qboolean			(*G2API_HaveWeGhoul2Models)				( CGhoul2Info_v &ghoul2 );
	qboolean			(*G2API_IKMove)							( CGhoul2Info_v &ghoul2, int time, sharedIKMoveParams_t *params );
	int					(*G2API_InitGhoul2Model)				( CGhoul2Info_v **ghoul2Ptr, const char *fileName, int modelIndex, qhandle_t customSkin, qhandle_t customShader, int modelFlags, int lodBias );
	qboolean			(*G2API_IsGhoul2InfovValid)				( CGhoul2Info_v& ghoul2 );
	qboolean			(*G2API_IsPaused)						( CGhoul2Info *ghlInfo, const char *boneName );
	void				(*G2API_ListBones)						( CGhoul2Info *ghlInfo, int frame );
	void				(*G2API_ListSurfaces)					( CGhoul2Info *ghlInfo );
	void				(*G2API_LoadGhoul2Models)				( CGhoul2Info_v &ghoul2, char *buffer );
	void				(*G2API_LoadSaveCodeDestructGhoul2Info)	( CGhoul2Info_v &ghoul2 );
	qboolean			(*G2API_OverrideServerWithClientData)	( CGhoul2Info_v& serverInstance, int modelIndex );
	qboolean			(*G2API_PauseBoneAnim)					( CGhoul2Info *ghlInfo, const char *boneName, const int currentTime );
	qhandle_t			(*G2API_PrecacheGhoul2Model)			( const char *fileName );
	qboolean			(*G2API_RagEffectorGoal)				( CGhoul2Info_v &ghoul2, const char *boneName, vec3_t pos );
	qboolean			(*G2API_RagEffectorKick)				( CGhoul2Info_v &ghoul2, const char *boneName, vec3_t velocity );
	qboolean			(*G2API_RagForceSolve)					( CGhoul2Info_v &ghoul2, qboolean force );
	qboolean			(*G2API_RagPCJConstraint)				( CGhoul2Info_v &ghoul2, const char *boneName, vec3_t min, vec3_t max );
	qboolean			(*G2API_RagPCJGradientSpeed)			( CGhoul2Info_v &ghoul2, const char *boneName, const float speed );
	qboolean			(*G2API_RemoveBolt)						( CGhoul2Info *ghlInfo, const int index );
	qboolean			(*G2API_RemoveBone)						( CGhoul2Info_v& ghoul2, int modelIndex, const char *boneName );
	qboolean			(*G2API_RemoveGhoul2Model)				( CGhoul2Info_v **ghlRemove, const int modelIndex );
	qboolean			(*G2API_RemoveGhoul2Models)				( CGhoul2Info_v **ghlRemove );
	qboolean			(*G2API_RemoveSurface)					( CGhoul2Info *ghlInfo, const int index );
	void				(*G2API_ResetRagDoll)					( CGhoul2Info_v &ghoul2 );
	qboolean			(*G2API_SaveGhoul2Models)				( CGhoul2Info_v &ghoul2, char **buffer, int *size );
	void				(*G2API_SetBoltInfo)					( CGhoul2Info_v &ghoul2, int modelIndex, int boltInfo );
	qboolean			(*G2API_SetBoneAngles)					( CGhoul2Info_v &ghoul2, const int modelIndex, const char *boneName, const vec3_t angles, const int flags, const Eorientations up, const Eorientations left, const Eorientations forward, qhandle_t *modelList, int blendTime, int currentTime  );
	qboolean			(*G2API_SetBoneAnglesIndex)				( CGhoul2Info *ghlInfo, const int index, const vec3_t angles, const int flags, const Eorientations yaw, const Eorientations pitch, const Eorientations roll, qhandle_t *modelList, int blendTime, int currentTime );
	qboolean			(*G2API_SetBoneAnglesMatrix)			( CGhoul2Info *ghlInfo, const char *boneName, const mdxaBone_t &matrix, const int flags, qhandle_t *modelList, int blendTime, int currentTime );
	qboolean			(*G2API_SetBoneAnglesMatrixIndex)		( CGhoul2Info *ghlInfo, const int index, const mdxaBone_t &matrix, const int flags, qhandle_t *modelList, int blendTime, int currentTime );
	qboolean			(*G2API_SetBoneAnim)					( CGhoul2Info_v &ghoul2, const int modelIndex, const char *boneName, const int startFrame, const int endFrame, const int flags, const float animSpeed, const int currentTime, const float setFrame /*= -1*/, const int blendTime /*= -1*/ );
	qboolean			(*G2API_SetBoneAnimIndex)				( CGhoul2Info *ghlInfo, const int index, const int startFrame, const int endFrame, const int flags, const float animSpeed, const int currentTime, const float setFrame, const int blendTime );
	qboolean			(*G2API_SetBoneIKState)					( CGhoul2Info_v &ghoul2, int time, const char *boneName, int ikState, sharedSetBoneIKStateParams_t *params );
	qboolean			(*G2API_SetGhoul2ModelFlags)			( CGhoul2Info *ghlInfo, const int flags );
	void				(*G2API_SetGhoul2ModelIndexes)			( CGhoul2Info_v &ghoul2, qhandle_t *modelList, qhandle_t *skinList );
	qboolean			(*G2API_SetLodBias)						( CGhoul2Info *ghlInfo, int lodBias );
	qboolean			(*G2API_SetNewOrigin)					( CGhoul2Info_v &ghoul2, const int boltIndex );
	void				(*G2API_SetRagDoll)						( CGhoul2Info_v &ghoul2, CRagDollParams *parms );
	qboolean			(*G2API_SetRootSurface)					( CGhoul2Info_v &ghoul2, const int modelIndex, const char *surfaceName );
	qboolean			(*G2API_SetShader)						( CGhoul2Info *ghlInfo, qhandle_t customShader );
	qboolean			(*G2API_SetSkin)						( CGhoul2Info_v& ghoul2, int modelIndex, qhandle_t customSkin, qhandle_t renderSkin );
	qboolean			(*G2API_SetSurfaceOnOff)				( CGhoul2Info_v &ghoul2, const char *surfaceName, const int flags );
	void				(*G2API_SetTime)						( int currentTime, int clock );
	qboolean			(*G2API_SkinlessModel)					( CGhoul2Info_v& ghoul2, int modelIndex );
	qboolean			(*G2API_StopBoneAngles)					( CGhoul2Info *ghlInfo, const char *boneName );
	qboolean			(*G2API_StopBoneAnglesIndex)			( CGhoul2Info *ghlInfo, const int index );
	qboolean			(*G2API_StopBoneAnim)					( CGhoul2Info *ghlInfo, const char *boneName );
	qboolean			(*G2API_StopBoneAnimIndex)				( CGhoul2Info *ghlInfo, const int index );

	#ifdef _G2_GORE
	int					(*G2API_GetNumGoreMarks)				( CGhoul2Info_v& ghoul2, int modelIndex );
	void				(*G2API_AddSkinGore)					( CGhoul2Info_v &ghoul2, SSkinGoreData &gore );
	void				(*G2API_ClearSkinGore)					( CGhoul2Info_v &ghoul2 );
	#endif // _G2_GORE

	struct {
		float				(*Font_StrLenPixels)					( const char *text, const int iFontIndex, const float scale );
	} ext;

} refexport_t;

//
// these are the functions imported by the refresh module
//
typedef struct refimport_s {
	void			(QDECL *Printf)						( int printLevel, const char *fmt, ...) __attribute__ ((format (printf, 2, 3)));
	void			(QDECL *Error)						( int errorLevel, const char *fmt, ...) NORETURN_PTR __attribute__ ((format (printf, 2, 3)));
	void			(QDECL *OPrintf)					( const char *fmt, ...) __attribute__ ((format (printf, 1, 2)));

	// milliseconds should only be used for profiling, never for anything game related. Get time from the refdef
	int				(*Milliseconds)						( void );

	// memory management (can use tr_subs)
	void *			(*Hunk_AllocateTempMemory)			( int size );
	void			(*Hunk_FreeTempMemory)				( void *buf );
	void *			(*Hunk_Alloc)						( int size, ha_pref preference );
	int				(*Hunk_MemoryRemaining)				( void );
	void *			(*Z_Malloc)							( int iSize, memtag_t eTag, qboolean bZeroit /*= qfalse*/, int iAlign /*= 4*/); // return memory NOT zero-filled by default
	void			(*Z_Free)							( void *ptr );
	int				(*Z_MemSize)						( memtag_t eTag );
	void			(*Z_MorphMallocTag)					( void *pvBuffer, memtag_t eDesiredTag );

	void			(*Cmd_ExecuteString)				( const char *text );
	int				(*Cmd_Argc)							( void );
	char *			(*Cmd_Argv)							( int arg );
	void			(*Cmd_ArgsBuffer)					( char *buffer, int bufferLength );
	void			(*Cmd_AddCommand)					( const char *cmd_name, xcommand_t function, const char *cmd_desc );
	void			(*Cmd_AddCommandList)				( const cmdList_t *cmdList );
	void			(*Cmd_RemoveCommand)				( const char *cmd_name );
	void			(*Cmd_RemoveCommandList)			( const cmdList_t *cmdList );
	cvar_t *		(*Cvar_Set)							( const char *var_name, const char *value );
	cvar_t *		(*Cvar_Get)							( const char *var_name, const char *value, uint32_t flags, const char *var_desc );
	cvar_t *		(*Cvar_SetValue)					( const char *name, float value );
	void			(*Cvar_CheckRange)					( cvar_t *cv, float minVal, float maxVal, qboolean shouldBeIntegral );
	void			(*Cvar_VariableStringBuffer)		( const char *var_name, char *buffer, int bufsize );
	char *			(*Cvar_VariableString)				( const char *var_name );
	float			(*Cvar_VariableValue)				( const char *var_name );
	int				(*Cvar_VariableIntegerValue)		( const char *var_name );
	qboolean		(*Sys_LowPhysicalMemory)			( void );
	const char *	(*SE_GetString)						( const char * psPackageAndStringReference );
	void			(*FS_FreeFile)						( void *buffer );
	void			(*FS_FreeFileList)					( char **fileList );
	int				(*FS_Read)							( void *buffer, int len, fileHandle_t f );
	long			(*FS_ReadFile)						( const char *qpath, void **buffer );
	void			(*FS_FCloseFile)					( fileHandle_t f );
	long			(*FS_FOpenFileRead)					( const char *qpath, fileHandle_t *file, qboolean uniqueFILE );
	fileHandle_t	(*FS_FOpenFileWrite)				( const char *qpath, qboolean safe );
	int				(*FS_FOpenFileByMode)				( const char *qpath, fileHandle_t *f, fsMode_t mode );
	qboolean		(*FS_FileExists)					( const char *file );
	int				(*FS_FileIsInPAK)					( const char *filename, int *pChecksum );
	char **			(*FS_ListFiles)						( const char *directory, const char *extension, int *numfiles );
	int				(*FS_Write)							( const void *buffer, int len, fileHandle_t f );
	void			(*FS_WriteFile)						( const char *qpath, const void *buffer, int size );
	void			(*CM_BoxTrace)						( trace_t *results, const vec3_t start, const vec3_t end, const vec3_t mins, const vec3_t maxs, clipHandle_t model, int brushmask, int capsule );
	void			(*CM_DrawDebugSurface)				( void (*drawPoly)(int color, int numPoints, float *points) );
	bool			(*CM_CullWorldBox)					( const cplane_t *frustum, const vec3pair_t bounds );
	byte *			(*CM_ClusterPVS)					( int cluster );
	int				(*CM_LeafArea)						( int leafnum );
	int				(*CM_LeafCluster)					( int leafnum );
	int				(*CM_PointLeafnum)					( const vec3_t p );
	int				(*CM_PointContents)					( const vec3_t p, clipHandle_t model );
	qboolean		(*Com_TheHunkMarkHasBeenMade)		( void );
	void			(*S_RestartMusic)					( void );
	qboolean		(*SND_RegisterAudio_LevelLoadEnd)	( qboolean bDeleteEverythingNotUsedThisLevel );
	e_status		(*CIN_RunCinematic)					( int handle );
	int				(*CIN_PlayCinematic)				( const char *arg0, int xpos, int ypos, int width, int height, int bits );
	void			(*CIN_UploadCinematic)				( int handle );
	void			(*CL_WriteAVIVideoFrame)			( const byte *imageBuffer, int size );

	// g2 data access
	char *			(*GetSharedMemory)					( void ); // cl.mSharedMemory

	// (c)g vm callbacks
	vm_t *			(*GetCurrentVM)						( void );
	qboolean		(*CGVMLoaded)						( void );
	int				(*CGVM_RagCallback)					( int callType );

	// window handling
	window_t		(*WIN_Init)                         ( const windowDesc_t *desc, glconfig_t *glConfig );
	void			(*WIN_SetGamma)						( glconfig_t *glConfig, byte red[256], byte green[256], byte blue[256] );
	void			(*WIN_Present)						( window_t *window );
	void            (*WIN_Shutdown)                     ( void );

	// OpenGL-specific
	void *			(*GL_GetProcAddress)				( const char *name );
	qboolean		(*GL_ExtensionSupported)			( const char *extension );

	// gpvCachedMapDiskImage
	void *			(*CM_GetCachedMapDiskImage)			( void );
	void			(*CM_SetCachedMapDiskImage)			( void *ptr );
	// gbUsingCachedMapDataRightNow
	void			(*CM_SetUsingCache)					( qboolean usingCache );

	// even the server will have this, which is a singleton
	// so before assigning to this in R_Init, check if it's NULL!
	IHeapAllocator *(*GetG2VertSpaceServer)				( void );

	// Persistent data store
	bool			(*PD_Store)							( const char *name, const void *data, size_t size );
	const void *	(*PD_Load)							( const char *name, size_t *size );
} refimport_t;

// this is the only function actually exported at the linker level
// If the module can't init to a valid rendering state, NULL will be
// returned.
#ifdef DEDICATED // dedicated server will statically compile rd-dedicated
	refexport_t *GetRefAPI( int apiVersion, refimport_t *rimp );
#else
	typedef	refexport_t* (QDECL *GetRefAPI_t) (int apiVersion, refimport_t *rimp);
#endif

#endif

