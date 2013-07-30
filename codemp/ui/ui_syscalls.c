// Copyright (C) 1999-2000 Id Software, Inc.
//
// this file is only included when building a dll
// syscalls.asm is included instead when building a qvm
#include "ui_local.h"

static intptr_t (QDECL *Q_syscall)( intptr_t arg, ... ) = (intptr_t (QDECL *)( intptr_t, ...))-1;

#ifdef OJK_NEW_VM_API
	static void TranslateSyscalls( void );
#endif

Q_EXPORT_C Q_EXPORT void dllEntry( intptr_t (QDECL *syscallptr)( intptr_t arg,... ) ) {
	Q_syscall = syscallptr;

	#ifdef OJK_NEW_VM_API
		TranslateSyscalls();
	#endif
}

int PASSFLOAT( float x ) {
	floatint_t fi;
	fi.f = x;
	return fi.i;
}
void trap_Print( const char *string ) {
	Q_syscall( UI_PRINT, string );
}
void trap_Error( const char *string ) {
	Q_syscall( UI_ERROR, string );
	exit(1);
}
int trap_Milliseconds( void ) {
	return Q_syscall( UI_MILLISECONDS ); 
}
void trap_Cvar_Register( vmCvar_t *cvar, const char *var_name, const char *value, int flags ) {
	Q_syscall( UI_CVAR_REGISTER, cvar, var_name, value, flags );
}
void trap_Cvar_Update( vmCvar_t *cvar ) {
	Q_syscall( UI_CVAR_UPDATE, cvar );
}
void trap_Cvar_Set( const char *var_name, const char *value ) {
	Q_syscall( UI_CVAR_SET, var_name, value );
}
float trap_Cvar_VariableValue( const char *var_name ) {
	floatint_t fi;
	fi.i = Q_syscall( UI_CVAR_VARIABLEVALUE, var_name );
	return fi.f;
}
void trap_Cvar_VariableStringBuffer( const char *var_name, char *buffer, int bufsize ) {
	Q_syscall( UI_CVAR_VARIABLESTRINGBUFFER, var_name, buffer, bufsize );
}
void trap_Cvar_SetValue( const char *var_name, float value ) {
	Q_syscall( UI_CVAR_SETVALUE, var_name, PASSFLOAT( value ) );
}
void trap_Cvar_Reset( const char *name ) {
	Q_syscall( UI_CVAR_RESET, name ); 
}
void trap_Cvar_Create( const char *var_name, const char *var_value, int flags ) {
	Q_syscall( UI_CVAR_CREATE, var_name, var_value, flags );
}
void trap_Cvar_InfoStringBuffer( int bit, char *buffer, int bufsize ) {
	Q_syscall( UI_CVAR_INFOSTRINGBUFFER, bit, buffer, bufsize );
}
int trap_Argc( void ) {
	return Q_syscall( UI_ARGC );
}
void trap_Argv( int n, char *buffer, int bufferLength ) {
	Q_syscall( UI_ARGV, n, buffer, bufferLength );
}
void trap_Cmd_ExecuteText( int exec_when, const char *text ) {
	Q_syscall( UI_CMD_EXECUTETEXT, exec_when, text );
}
int trap_FS_FOpenFile( const char *qpath, fileHandle_t *f, fsMode_t mode ) {
	return Q_syscall( UI_FS_FOPENFILE, qpath, f, mode );
}
void trap_FS_Read( void *buffer, int len, fileHandle_t f ) {
	Q_syscall( UI_FS_READ, buffer, len, f );
}
void trap_FS_Write( const void *buffer, int len, fileHandle_t f ) {
	Q_syscall( UI_FS_WRITE, buffer, len, f );
}
void trap_FS_FCloseFile( fileHandle_t f ) {
	Q_syscall( UI_FS_FCLOSEFILE, f );
}
int trap_FS_GetFileList(  const char *path, const char *extension, char *listbuf, int bufsize ) {
	return Q_syscall( UI_FS_GETFILELIST, path, extension, listbuf, bufsize );
}
qhandle_t trap_R_RegisterModel( const char *name ) {
	return Q_syscall( UI_R_REGISTERMODEL, name );
}
qhandle_t trap_R_RegisterSkin( const char *name ) {
	return Q_syscall( UI_R_REGISTERSKIN, name );
}
qhandle_t trap_R_RegisterFont( const char *fontName ) {
	return Q_syscall( UI_R_REGISTERFONT, fontName);
}
int	trap_R_Font_StrLenPixels(const char *text, const int iFontIndex, const float scale) {
	return Q_syscall( UI_R_FONT_STRLENPIXELS, text, iFontIndex, PASSFLOAT(scale));
}
int trap_R_Font_StrLenChars(const char *text) {
	return Q_syscall( UI_R_FONT_STRLENCHARS, text);
}
int trap_R_Font_HeightPixels(const int iFontIndex, const float scale) {
	return Q_syscall( UI_R_FONT_STRHEIGHTPIXELS, iFontIndex, PASSFLOAT(scale));
}
void trap_R_Font_DrawString(int ox, int oy, const char *text, const float *rgba, const int setIndex, int iCharLimit, const float scale) {
	Q_syscall( UI_R_FONT_DRAWSTRING, ox, oy, text, rgba, setIndex, iCharLimit, PASSFLOAT(scale));
}
qboolean trap_Language_IsAsian(void) {
	return Q_syscall( UI_LANGUAGE_ISASIAN );
}
qboolean trap_Language_UsesSpaces(void) {
	return Q_syscall( UI_LANGUAGE_USESSPACES );
}
unsigned int trap_AnyLanguage_ReadCharFromString( const char *psText, int *piAdvanceCount, qboolean *pbIsTrailingPunctuation ) {
	return Q_syscall( UI_ANYLANGUAGE_READCHARFROMSTRING, psText, piAdvanceCount, pbIsTrailingPunctuation);
}
qhandle_t trap_R_RegisterShaderNoMip( const char *name ) {
	char buf[1024];
	
	if (name[0] == '*') {
		trap_Cvar_VariableStringBuffer(name+1, buf, sizeof(buf));
		if (buf[0])
			return Q_syscall( UI_R_REGISTERSHADERNOMIP, &buf );
	}
	return Q_syscall( UI_R_REGISTERSHADERNOMIP, name );
}
void trap_R_ShaderNameFromIndex(char *name, int index) {
	Q_syscall( UI_R_SHADERNAMEFROMINDEX, name, index );
}
void trap_R_ClearScene( void ) {
	Q_syscall( UI_R_CLEARSCENE );
}
void trap_R_AddRefEntityToScene( const refEntity_t *re ) {
	Q_syscall( UI_R_ADDREFENTITYTOSCENE, re );
}
void trap_R_AddPolyToScene( qhandle_t hShader , int numVerts, const polyVert_t *verts ) {
	Q_syscall( UI_R_ADDPOLYTOSCENE, hShader, numVerts, verts );
}
void trap_R_AddLightToScene( const vec3_t org, float intensity, float r, float g, float b ) {
	Q_syscall( UI_R_ADDLIGHTTOSCENE, org, PASSFLOAT(intensity), PASSFLOAT(r), PASSFLOAT(g), PASSFLOAT(b) );
}
void trap_R_RenderScene( const refdef_t *fd ) {
	Q_syscall( UI_R_RENDERSCENE, fd );
}
void trap_R_SetColor( const float *rgba ) {
	Q_syscall( UI_R_SETCOLOR, rgba );
}
void trap_R_DrawStretchPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader ) {
	Q_syscall( UI_R_DRAWSTRETCHPIC, PASSFLOAT(x), PASSFLOAT(y), PASSFLOAT(w), PASSFLOAT(h), PASSFLOAT(s1), PASSFLOAT(t1), PASSFLOAT(s2), PASSFLOAT(t2), hShader );
}
void	trap_R_ModelBounds( clipHandle_t model, vec3_t mins, vec3_t maxs ) {
	Q_syscall( UI_R_MODELBOUNDS, model, mins, maxs );
}
void trap_UpdateScreen( void ) {
	Q_syscall( UI_UPDATESCREEN );
}
int trap_CM_LerpTag( orientation_t *tag, clipHandle_t mod, int startFrame, int endFrame, float frac, const char *tagName ) {
	return Q_syscall( UI_CM_LERPTAG, tag, mod, startFrame, endFrame, PASSFLOAT(frac), tagName );
}
void trap_S_StartLocalSound( sfxHandle_t sfx, int channelNum ) {
	Q_syscall( UI_S_STARTLOCALSOUND, sfx, channelNum );
}
sfxHandle_t	trap_S_RegisterSound( const char *sample ) {
	return Q_syscall( UI_S_REGISTERSOUND, sample );
}
void trap_Key_KeynumToStringBuf( int keynum, char *buf, int buflen ) {
	Q_syscall( UI_KEY_KEYNUMTOSTRINGBUF, keynum, buf, buflen );
}
void trap_Key_GetBindingBuf( int keynum, char *buf, int buflen ) {
	Q_syscall( UI_KEY_GETBINDINGBUF, keynum, buf, buflen );
}
void trap_Key_SetBinding( int keynum, const char *binding ) {
	Q_syscall( UI_KEY_SETBINDING, keynum, binding );
}
qboolean trap_Key_IsDown( int keynum ) {
	return Q_syscall( UI_KEY_ISDOWN, keynum );
}
qboolean trap_Key_GetOverstrikeMode( void ) {
	return Q_syscall( UI_KEY_GETOVERSTRIKEMODE );
}
void trap_Key_SetOverstrikeMode( qboolean state ) {
	Q_syscall( UI_KEY_SETOVERSTRIKEMODE, state );
}
void trap_Key_ClearStates( void ) {
	Q_syscall( UI_KEY_CLEARSTATES );
}
int trap_Key_GetCatcher( void ) {
	return Q_syscall( UI_KEY_GETCATCHER );
}
void trap_Key_SetCatcher( int catcher ) {
	Q_syscall( UI_KEY_SETCATCHER, catcher );
}
void trap_GetClipboardData( char *buf, int bufsize ) {
	Q_syscall( UI_GETCLIPBOARDDATA, buf, bufsize );
}
void trap_GetClientState( uiClientState_t *state ) {
	Q_syscall( UI_GETCLIENTSTATE, state );
}
void trap_GetGlconfig( glconfig_t *glconfig ) {
	Q_syscall( UI_GETGLCONFIG, glconfig );
}
int trap_GetConfigString( int index, char* buff, int buffsize ) {
	return Q_syscall( UI_GETCONFIGSTRING, index, buff, buffsize );
}
int	trap_LAN_GetServerCount( int source ) {
	return Q_syscall( UI_LAN_GETSERVERCOUNT, source );
}
void trap_LAN_GetServerAddressString( int source, int n, char *buf, int buflen ) {
	Q_syscall( UI_LAN_GETSERVERADDRESSSTRING, source, n, buf, buflen );
}
void trap_LAN_GetServerInfo( int source, int n, char *buf, int buflen ) {
	Q_syscall( UI_LAN_GETSERVERINFO, source, n, buf, buflen );
}
int trap_LAN_GetServerPing( int source, int n ) {
	return Q_syscall( UI_LAN_GETSERVERPING, source, n );
}
int trap_LAN_GetPingQueueCount( void ) {
	return Q_syscall( UI_LAN_GETPINGQUEUECOUNT );
}
int trap_LAN_ServerStatus( const char *serverAddress, char *serverStatus, int maxLen ) {
	return Q_syscall( UI_LAN_SERVERSTATUS, serverAddress, serverStatus, maxLen );
}
void trap_LAN_SaveCachedServers( void ) {
	Q_syscall( UI_LAN_SAVECACHEDSERVERS );
}
void trap_LAN_LoadCachedServers( void ) {
	Q_syscall( UI_LAN_LOADCACHEDSERVERS );
}
void trap_LAN_ResetPings(int n) {
	Q_syscall( UI_LAN_RESETPINGS, n );
}
void trap_LAN_ClearPing( int n ) {
	Q_syscall( UI_LAN_CLEARPING, n );
}
void trap_LAN_GetPing( int n, char *buf, int buflen, int *pingtime ) {
	Q_syscall( UI_LAN_GETPING, n, buf, buflen, pingtime );
}
void trap_LAN_GetPingInfo( int n, char *buf, int buflen ) {
	Q_syscall( UI_LAN_GETPINGINFO, n, buf, buflen );
}
void trap_LAN_MarkServerVisible( int source, int n, qboolean visible ) {
	Q_syscall( UI_LAN_MARKSERVERVISIBLE, source, n, visible );
}
int trap_LAN_ServerIsVisible( int source, int n) {
	return Q_syscall( UI_LAN_SERVERISVISIBLE, source, n );
}
qboolean trap_LAN_UpdateVisiblePings( int source ) {
	return Q_syscall( UI_LAN_UPDATEVISIBLEPINGS, source );
}
int trap_LAN_AddServer(int source, const char *name, const char *addr) {
	return Q_syscall( UI_LAN_ADDSERVER, source, name, addr );
}
void trap_LAN_RemoveServer(int source, const char *addr) {
	Q_syscall( UI_LAN_REMOVESERVER, source, addr );
}
int trap_LAN_CompareServers( int source, int sortKey, int sortDir, int s1, int s2 ) {
	return Q_syscall( UI_LAN_COMPARESERVERS, source, sortKey, sortDir, s1, s2 );
}
int trap_MemoryRemaining( void ) {
	return Q_syscall( UI_MEMORY_REMAINING );
}
int trap_PC_AddGlobalDefine( char *define ) {
	return Q_syscall( UI_PC_ADD_GLOBAL_DEFINE, define );
}
int trap_PC_LoadSource( const char *filename ) {
	return Q_syscall( UI_PC_LOAD_SOURCE, filename );
}
int trap_PC_FreeSource( int handle ) {
	return Q_syscall( UI_PC_FREE_SOURCE, handle );
}
int trap_PC_ReadToken( int handle, pc_token_t *pc_token ) {
	return Q_syscall( UI_PC_READ_TOKEN, handle, pc_token );
}
int trap_PC_SourceFileAndLine( int handle, char *filename, int *line ) {
	return Q_syscall( UI_PC_SOURCE_FILE_AND_LINE, handle, filename, line );
}
int trap_PC_LoadGlobalDefines ( const char* filename ) {
	return Q_syscall ( UI_PC_LOAD_GLOBAL_DEFINES, filename );
}
void trap_PC_RemoveAllGlobalDefines ( void ) {
	Q_syscall ( UI_PC_REMOVE_ALL_GLOBAL_DEFINES );
}
void trap_S_StopBackgroundTrack( void ) {
	Q_syscall( UI_S_STOPBACKGROUNDTRACK );
}
void trap_S_StartBackgroundTrack( const char *intro, const char *loop, qboolean bReturnWithoutStarting) {
	Q_syscall( UI_S_STARTBACKGROUNDTRACK, intro, loop, bReturnWithoutStarting );
}
int trap_RealTime(qtime_t *qtime) {
	return Q_syscall( UI_REAL_TIME, qtime );
}
int trap_CIN_PlayCinematic( const char *arg0, int xpos, int ypos, int width, int height, int bits) {
  return Q_syscall(UI_CIN_PLAYCINEMATIC, arg0, xpos, ypos, width, height, bits);
}
e_status trap_CIN_StopCinematic(int handle) {
  return Q_syscall(UI_CIN_STOPCINEMATIC, handle);
}
e_status trap_CIN_RunCinematic (int handle) {
  return Q_syscall(UI_CIN_RUNCINEMATIC, handle);
}
void trap_CIN_DrawCinematic (int handle) {
  Q_syscall(UI_CIN_DRAWCINEMATIC, handle);
}
void trap_CIN_SetExtents (int handle, int x, int y, int w, int h) {
  Q_syscall(UI_CIN_SETEXTENTS, handle, x, y, w, h);
}
void trap_R_RemapShader( const char *oldShader, const char *newShader, const char *timeOffset ) {
	Q_syscall( UI_R_REMAP_SHADER, oldShader, newShader, timeOffset );
}
int trap_SP_GetNumLanguages( void ) {
	return Q_syscall( UI_SP_GETNUMLANGUAGES );
}
void trap_GetLanguageName( const int languageIndex, char *buffer ) {
	Q_syscall( UI_SP_GETLANGUAGENAME, languageIndex, buffer);
}
qboolean trap_SP_GetStringTextString(const char *text, char *buffer, int bufferLength) {
	return Q_syscall( UI_SP_GETSTRINGTEXTSTRING, text, buffer, bufferLength );
}
void trap_G2_ListModelSurfaces(void *ghlInfo) {
	Q_syscall( UI_G2_LISTSURFACES, ghlInfo);
}
void trap_G2_ListModelBones(void *ghlInfo, int frame) {
	Q_syscall( UI_G2_LISTBONES, ghlInfo, frame);
}
void trap_G2_SetGhoul2ModelIndexes(void *ghoul2, qhandle_t *modelList, qhandle_t *skinList) {
	Q_syscall( UI_G2_SETMODELS, ghoul2, modelList, skinList);
}
qboolean trap_G2_HaveWeGhoul2Models(void *ghoul2) {
	return (qboolean)(Q_syscall(UI_G2_HAVEWEGHOULMODELS, ghoul2));
}
qboolean trap_G2API_GetBoltMatrix(void *ghoul2, const int modelIndex, const int boltIndex, mdxaBone_t *matrix, const vec3_t angles, const vec3_t position, const int frameNum, qhandle_t *modelList, vec3_t scale) {
	return (qboolean)(Q_syscall(UI_G2_GETBOLT, ghoul2, modelIndex, boltIndex, matrix, angles, position, frameNum, modelList, scale));
}
qboolean trap_G2API_GetBoltMatrix_NoReconstruct( void *ghoul2, const int modelIndex, const int boltIndex, mdxaBone_t *matrix, const vec3_t angles, const vec3_t position, const int frameNum, qhandle_t *modelList, vec3_t scale ) {
	return (qboolean)(Q_syscall(UI_G2_GETBOLT_NOREC, ghoul2, modelIndex, boltIndex, matrix, angles, position, frameNum, modelList, scale));
}
qboolean trap_G2API_GetBoltMatrix_NoRecNoRot(void *ghoul2, const int modelIndex, const int boltIndex, mdxaBone_t *matrix, const vec3_t angles, const vec3_t position, const int frameNum, qhandle_t *modelList, vec3_t scale) {
	return (qboolean)(Q_syscall(UI_G2_GETBOLT_NOREC_NOROT, ghoul2, modelIndex, boltIndex, matrix, angles, position, frameNum, modelList, scale));
}
int trap_G2API_InitGhoul2Model(void **ghoul2Ptr, const char *fileName, int modelIndex, qhandle_t customSkin, qhandle_t customShader, int modelFlags, int lodBias) {
	return Q_syscall(UI_G2_INITGHOUL2MODEL, ghoul2Ptr, fileName, modelIndex, customSkin, customShader, modelFlags, lodBias);
}
qboolean trap_G2API_SetSkin(void *ghoul2, int modelIndex, qhandle_t customSkin, qhandle_t renderSkin) {
	return Q_syscall(UI_G2_SETSKIN, ghoul2, modelIndex, customSkin, renderSkin);
}
void trap_G2API_CollisionDetect( CollisionRecord_t *collRecMap, void* ghoul2, const vec3_t angles, const vec3_t position, int frameNumber, int entNum, const vec3_t rayStart, const vec3_t rayEnd, const vec3_t scale, int traceFlags, int useLod, float fRadius ) {
	Q_syscall ( UI_G2_COLLISIONDETECT, collRecMap, ghoul2, angles, position, frameNumber, entNum, rayStart, rayEnd, scale, traceFlags, useLod, PASSFLOAT(fRadius) );
}
void trap_G2API_CollisionDetectCache( CollisionRecord_t *collRecMap, void* ghoul2, const vec3_t angles, const vec3_t position, int frameNumber, int entNum, const vec3_t rayStart, const vec3_t rayEnd, const vec3_t scale, int traceFlags, int useLod, float fRadius ) {
	Q_syscall ( UI_G2_COLLISIONDETECTCACHE, collRecMap, ghoul2, angles, position, frameNumber, entNum, rayStart, rayEnd, scale, traceFlags, useLod, PASSFLOAT(fRadius) );
}
void trap_G2API_CleanGhoul2Models(void **ghoul2Ptr) {
	Q_syscall(UI_G2_CLEANMODELS, ghoul2Ptr);
}
qboolean trap_G2API_SetBoneAngles(void *ghoul2, int modelIndex, const char *boneName, const vec3_t angles, const int flags, const int up, const int right, const int forward, qhandle_t *modelList, int blendTime, int currentTime ) {
	return (Q_syscall(UI_G2_ANGLEOVERRIDE, ghoul2, modelIndex, boneName, angles, flags, up, right, forward, modelList, blendTime, currentTime));
}
qboolean trap_G2API_SetBoneAnim(void *ghoul2, const int modelIndex, const char *boneName, const int startFrame, const int endFrame, const int flags, const float animSpeed, const int currentTime, const float setFrame, const int blendTime ) {
	return Q_syscall(UI_G2_PLAYANIM, ghoul2, modelIndex, boneName, startFrame, endFrame, flags, PASSFLOAT(animSpeed), currentTime, PASSFLOAT(setFrame), blendTime);
}
qboolean trap_G2API_GetBoneAnim(void *ghoul2, const char *boneName, const int currentTime, float *currentFrame, int *startFrame, int *endFrame, int *flags, float *animSpeed, int *modelList, const int modelIndex) {
	return Q_syscall(UI_G2_GETBONEANIM, ghoul2, boneName, currentTime, currentFrame, startFrame, endFrame, flags, animSpeed, modelList, modelIndex);
}
qboolean trap_G2API_GetBoneFrame(void *ghoul2, const char *boneName, const int currentTime, float *currentFrame, int *modelList, const int modelIndex) {
	return Q_syscall(UI_G2_GETBONEFRAME, ghoul2, boneName, currentTime, currentFrame, modelList, modelIndex);
}
void trap_G2API_GetGLAName(void *ghoul2, int modelIndex, char *fillBuf) {
	Q_syscall(UI_G2_GETGLANAME, ghoul2, modelIndex, fillBuf);
}
int trap_G2API_CopyGhoul2Instance(void *g2From, void *g2To, int modelIndex) {
	return Q_syscall(UI_G2_COPYGHOUL2INSTANCE, g2From, g2To, modelIndex);
}
void trap_G2API_CopySpecificGhoul2Model(void *g2From, int modelFrom, void *g2To, int modelTo) {
	Q_syscall(UI_G2_COPYSPECIFICGHOUL2MODEL, g2From, modelFrom, g2To, modelTo);
}
void trap_G2API_DuplicateGhoul2Instance(void *g2From, void **g2To) {
	Q_syscall(UI_G2_DUPLICATEGHOUL2INSTANCE, g2From, g2To);
}
qboolean trap_G2API_HasGhoul2ModelOnIndex(void *ghlInfo, int modelIndex) {
	return Q_syscall(UI_G2_HASGHOUL2MODELONINDEX, ghlInfo, modelIndex);
}
qboolean trap_G2API_RemoveGhoul2Model(void *ghlInfo, int modelIndex) {
	return Q_syscall(UI_G2_REMOVEGHOUL2MODEL, ghlInfo, modelIndex);
}
int	trap_G2API_AddBolt(void *ghoul2, int modelIndex, const char *boneName) {
	return Q_syscall(UI_G2_ADDBOLT, ghoul2, modelIndex, boneName);
}
void trap_G2API_SetBoltInfo(void *ghoul2, int modelIndex, int boltInfo) {
	Q_syscall(UI_G2_SETBOLTON, ghoul2, modelIndex, boltInfo);
}
qboolean trap_G2API_SetRootSurface(void *ghoul2, const int modelIndex, const char *surfaceName) {
	return Q_syscall(UI_G2_SETROOTSURFACE, ghoul2, modelIndex, surfaceName);
}
qboolean trap_G2API_SetSurfaceOnOff(void *ghoul2, const char *surfaceName, const int flags) {
	return Q_syscall(UI_G2_SETSURFACEONOFF, ghoul2, surfaceName, flags);
}
qboolean trap_G2API_SetNewOrigin(void *ghoul2, const int boltIndex) {
	return Q_syscall(UI_G2_SETNEWORIGIN, ghoul2, boltIndex);
}
int trap_G2API_GetTime(void) {
	return Q_syscall(UI_G2_GETTIME);
}
void trap_G2API_SetTime(int time, int clock) {
	Q_syscall(UI_G2_SETTIME, time, clock);
}
void trap_G2API_SetRagDoll(void *ghoul2, sharedRagDollParams_t *params) {
	Q_syscall(UI_G2_SETRAGDOLL, ghoul2, params);
}
void trap_G2API_AnimateG2Models(void *ghoul2, int time, sharedRagDollUpdateParams_t *params) {
	Q_syscall(UI_G2_ANIMATEG2MODELS, ghoul2, time, params);
}
qboolean trap_G2API_SetBoneIKState(void *ghoul2, int time, const char *boneName, int ikState, sharedSetBoneIKStateParams_t *params) {
	return Q_syscall(UI_G2_SETBONEIKSTATE, ghoul2, time, boneName, ikState, params);
}
qboolean trap_G2API_IKMove(void *ghoul2, int time, sharedIKMoveParams_t *params) {
	return Q_syscall(UI_G2_IKMOVE, ghoul2, time, params);
}
void trap_G2API_GetSurfaceName(void *ghoul2, int surfNumber, int modelIndex, char *fillBuf) {
	Q_syscall(UI_G2_GETSURFACENAME, ghoul2, surfNumber, modelIndex, fillBuf);
}
qboolean trap_G2API_AttachG2Model(void *ghoul2From, int modelIndexFrom, void *ghoul2To, int toBoltIndex, int toModel) {
	return Q_syscall(UI_G2_ATTACHG2MODEL, ghoul2From, modelIndexFrom, ghoul2To, toBoltIndex, toModel);
}

#ifdef OJK_NEW_VM_API
int UISyscall_FS_Read( void *buffer, int len, fileHandle_t f ) { trap_FS_Read( buffer, len, f ); return 0; }
int UISyscall_FS_Write( const void *buffer, int len, fileHandle_t f ) { trap_FS_Write( buffer, len, f ); return 0; }
void UISyscall_R_AddPolysToScene( qhandle_t hShader, int numVerts, const polyVert_t *verts, int num ) { trap_R_AddPolyToScene( hShader, numVerts, verts ); }
void UISyscall_G2API_CollisionDetect( CollisionRecord_t *collRecMap, void* ghoul2, const vec3_t angles, const vec3_t position, int frameNumber, int entNum, vec3_t rayStart, vec3_t rayEnd, vec3_t scale, int traceFlags, int useLod, float fRadius ) { trap_G2API_CollisionDetect( collRecMap, ghoul2, angles, position, frameNumber, entNum, rayStart, rayEnd, scale, traceFlags, useLod, fRadius ); }

static void TranslateSyscalls( void ) {
	uii.Print								= Com_Printf;
	uii.Error								= Com_Error;
	uii.Milliseconds						= trap_Milliseconds;
	uii.RealTime							= trap_RealTime;
	uii.MemoryRemaining						= trap_MemoryRemaining;

	uii.Cvar_Create							= trap_Cvar_Create;
	uii.Cvar_InfoStringBuffer				= trap_Cvar_InfoStringBuffer;
	uii.Cvar_Register						= trap_Cvar_Register;
	uii.Cvar_Reset							= trap_Cvar_Reset;
	uii.Cvar_Set							= trap_Cvar_Set;
	uii.Cvar_SetValue						= trap_Cvar_SetValue;
	uii.Cvar_Update							= trap_Cvar_Update;
	uii.Cvar_VariableStringBuffer			= trap_Cvar_VariableStringBuffer;
	uii.Cvar_VariableValue					= trap_Cvar_VariableValue;

	uii.Cmd_Argc							= trap_Argc;
	uii.Cmd_Argv							= trap_Argv;
	uii.Cmd_ExecuteText						= trap_Cmd_ExecuteText;

	uii.FS_Close							= trap_FS_FCloseFile;
	uii.FS_GetFileList						= trap_FS_GetFileList;
	uii.FS_Open								= trap_FS_FOpenFile;
	uii.FS_Read								= UISyscall_FS_Read;
	uii.FS_Write							= UISyscall_FS_Write;

	uii.GetClientState						= trap_GetClientState;
	uii.GetClipboardData					= trap_GetClipboardData;
	uii.GetConfigString						= trap_GetConfigString;
	uii.GetGlconfig							= trap_GetGlconfig;
	uii.UpdateScreen						= trap_UpdateScreen;

	uii.Key_ClearStates						= trap_Key_ClearStates;
	uii.Key_GetBindingBuf					= trap_Key_GetBindingBuf;
	uii.Key_IsDown							= trap_Key_IsDown;
	uii.Key_KeynumToStringBuf				= trap_Key_KeynumToStringBuf;
	uii.Key_SetBinding						= trap_Key_SetBinding;
	uii.Key_GetCatcher						= trap_Key_GetCatcher;
	uii.Key_GetOverstrikeMode				= trap_Key_GetOverstrikeMode;
	uii.Key_SetCatcher						= trap_Key_SetCatcher;
	uii.Key_SetOverstrikeMode				= trap_Key_SetOverstrikeMode;

	uii.PC_AddGlobalDefine					= trap_PC_AddGlobalDefine;
	uii.PC_FreeSource						= trap_PC_FreeSource;
	uii.PC_LoadGlobalDefines				= trap_PC_LoadGlobalDefines;
	uii.PC_LoadSource						= trap_PC_LoadSource;
	uii.PC_ReadToken						= trap_PC_ReadToken;
	uii.PC_RemoveAllGlobalDefines			= trap_PC_RemoveAllGlobalDefines;
	uii.PC_SourceFileAndLine				= trap_PC_SourceFileAndLine;

	uii.CIN_DrawCinematic					= trap_CIN_DrawCinematic;
	uii.CIN_PlayCinematic					= trap_CIN_PlayCinematic;
	uii.CIN_RunCinematic					= trap_CIN_RunCinematic;
	uii.CIN_SetExtents						= trap_CIN_SetExtents;
	uii.CIN_StopCinematic					= trap_CIN_StopCinematic;

	uii.LAN_AddServer						= trap_LAN_AddServer;
	uii.LAN_ClearPing						= trap_LAN_ClearPing;
	uii.LAN_CompareServers					= trap_LAN_CompareServers;
	uii.LAN_GetPing							= trap_LAN_GetPing;
	uii.LAN_GetPingInfo						= trap_LAN_GetPingInfo;
	uii.LAN_GetPingQueueCount				= trap_LAN_GetPingQueueCount;
	uii.LAN_GetServerAddressString			= trap_LAN_GetServerAddressString;
	uii.LAN_GetServerCount					= trap_LAN_GetServerCount;
	uii.LAN_GetServerInfo					= trap_LAN_GetServerInfo;
	uii.LAN_GetServerPing					= trap_LAN_GetServerPing;
	uii.LAN_LoadCachedServers				= trap_LAN_LoadCachedServers;
	uii.LAN_MarkServerVisible				= trap_LAN_MarkServerVisible;
	uii.LAN_RemoveServer					= trap_LAN_RemoveServer;
	uii.LAN_ResetPings						= trap_LAN_ResetPings;
	uii.LAN_SaveCachedServers				= trap_LAN_SaveCachedServers;
	uii.LAN_ServerIsVisible					= trap_LAN_ServerIsVisible;
	uii.LAN_ServerStatus					= trap_LAN_ServerStatus;
	uii.LAN_UpdateVisiblePings				= trap_LAN_UpdateVisiblePings;

	uii.S_StartBackgroundTrack				= trap_S_StartBackgroundTrack;
	uii.S_StartLocalSound					= trap_S_StartLocalSound;
	uii.S_StopBackgroundTrack				= trap_S_StopBackgroundTrack;
	uii.S_RegisterSound						= trap_S_RegisterSound;

	uii.SE_GetLanguageName					= trap_GetLanguageName;
	uii.SE_GetNumLanguages					= trap_SP_GetNumLanguages;
	uii.SE_GetStringTextString				= trap_SP_GetStringTextString;

	uii.R_Language_IsAsian					= trap_Language_IsAsian;
	uii.R_Language_UsesSpaces				= trap_Language_UsesSpaces;
	uii.R_AnyLanguage_ReadCharFromString	= trap_AnyLanguage_ReadCharFromString;

	uii.R_AddLightToScene					= trap_R_AddLightToScene;
	uii.R_AddPolysToScene					= UISyscall_R_AddPolysToScene;
	uii.R_AddRefEntityToScene				= trap_R_AddRefEntityToScene;
	uii.R_ClearScene						= trap_R_ClearScene;
	uii.R_DrawStretchPic					= trap_R_DrawStretchPic;
	uii.R_Font_DrawString					= trap_R_Font_DrawString;
	uii.R_Font_HeightPixels					= trap_R_Font_HeightPixels;
	uii.R_Font_StrLenChars					= trap_R_Font_StrLenChars;
	uii.R_Font_StrLenPixels					= trap_R_Font_StrLenPixels;
	uii.R_LerpTag							= trap_CM_LerpTag;
	uii.R_ModelBounds						= trap_R_ModelBounds;
	uii.R_RegisterFont						= trap_R_RegisterFont;
	uii.R_RegisterModel						= trap_R_RegisterModel;
	uii.R_RegisterShaderNoMip				= trap_R_RegisterShaderNoMip;
	uii.R_RegisterSkin						= trap_R_RegisterSkin;
	uii.R_RemapShader						= trap_R_RemapShader;
	uii.R_RenderScene						= trap_R_RenderScene;
	uii.R_SetColor							= trap_R_SetColor;
	uii.R_ShaderNameFromIndex				= trap_R_ShaderNameFromIndex;

	uii.G2_ListModelSurfaces				= trap_G2_ListModelSurfaces;
	uii.G2_ListModelBones					= trap_G2_ListModelBones;
	uii.G2_SetGhoul2ModelIndexes			= trap_G2_SetGhoul2ModelIndexes;
	uii.G2_HaveWeGhoul2Models				= trap_G2_HaveWeGhoul2Models;
	uii.G2API_GetBoltMatrix					= trap_G2API_GetBoltMatrix;
	uii.G2API_GetBoltMatrix_NoReconstruct	= trap_G2API_GetBoltMatrix_NoReconstruct;
	uii.G2API_GetBoltMatrix_NoRecNoRot		= trap_G2API_GetBoltMatrix_NoRecNoRot;
	uii.G2API_InitGhoul2Model				= trap_G2API_InitGhoul2Model;
	uii.G2API_SetSkin						= trap_G2API_SetSkin;
	uii.G2API_CollisionDetect				= UISyscall_G2API_CollisionDetect;
	uii.G2API_CollisionDetectCache			= UISyscall_G2API_CollisionDetect;
	uii.G2API_CleanGhoul2Models				= trap_G2API_CleanGhoul2Models;
	uii.G2API_SetBoneAngles					= trap_G2API_SetBoneAngles;
	uii.G2API_SetBoneAnim					= trap_G2API_SetBoneAnim;
	uii.G2API_GetBoneAnim					= trap_G2API_GetBoneAnim;
	uii.G2API_GetBoneFrame					= trap_G2API_GetBoneFrame;
	uii.G2API_GetGLAName					= trap_G2API_GetGLAName;
	uii.G2API_CopyGhoul2Instance			= trap_G2API_CopyGhoul2Instance;
	uii.G2API_CopySpecificGhoul2Model		= trap_G2API_CopySpecificGhoul2Model;
	uii.G2API_DuplicateGhoul2Instance		= trap_G2API_DuplicateGhoul2Instance;
	uii.G2API_HasGhoul2ModelOnIndex			= trap_G2API_HasGhoul2ModelOnIndex;
	uii.G2API_RemoveGhoul2Model				= trap_G2API_RemoveGhoul2Model;
	uii.G2API_AddBolt						= trap_G2API_AddBolt;
	uii.G2API_SetBoltInfo					= trap_G2API_SetBoltInfo;
	uii.G2API_SetRootSurface				= trap_G2API_SetRootSurface;
	uii.G2API_SetSurfaceOnOff				= trap_G2API_SetSurfaceOnOff;
	uii.G2API_SetNewOrigin					= trap_G2API_SetNewOrigin;
	uii.G2API_GetTime						= trap_G2API_GetTime;
	uii.G2API_SetTime						= trap_G2API_SetTime;
	uii.G2API_SetRagDoll					= trap_G2API_SetRagDoll;
	uii.G2API_AnimateG2Models				= trap_G2API_AnimateG2Models;
	uii.G2API_SetBoneIKState				= trap_G2API_SetBoneIKState;
	uii.G2API_IKMove						= trap_G2API_IKMove;
	uii.G2API_GetSurfaceName				= trap_G2API_GetSurfaceName;
	uii.G2API_AttachG2Model					= trap_G2API_AttachG2Model;
}
#endif
