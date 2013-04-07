// this line must stay at top so the whole PCH thing works...
#include "cg_headers.h"

//#include "cg_local.h"

// this file is only included when building a dll




#ifdef _IMMERSION
#include "../ff/ff.h"
#else
/////////////////////  this is a bit kludgy, but it only gives access to one
//							enum table because of the #define. May get changed.
#define CGAME_ONLY
#include "../client/fffx.h"
//
/////////////////////
#endif // _IMMERSION




//prototypes
extern void CG_PreInit();

int (*syscall)( int arg, ... ) = (int (*)( int, ...))-1;


void dllEntry( int (*syscallptr)( int arg,... ) ) {
	syscall = syscallptr;
	CG_PreInit();
}


inline int PASSFLOAT( float x ) {
	float	floatTemp;
	floatTemp = x;
	return *(int *)&floatTemp;
}

void	cgi_Printf( const char *fmt ) {
	syscall( CG_PRINT, fmt );
}

void	cgi_Error( const char *fmt ) {
	syscall( CG_ERROR, fmt );
}

int		cgi_Milliseconds( void ) {
	return syscall( CG_MILLISECONDS ); 
}

void	cgi_Cvar_Register( vmCvar_t *vmCvar, const char *varName, const char *defaultValue, int flags ) {
	syscall( CG_CVAR_REGISTER, vmCvar, varName, defaultValue, flags );
}

void	cgi_Cvar_Update( vmCvar_t *vmCvar ) {
	syscall( CG_CVAR_UPDATE, vmCvar );
}

void	cgi_Cvar_Set( const char *var_name, const char *value ) {
	syscall( CG_CVAR_SET, var_name, value );
}

int		cgi_Argc( void ) {
	return syscall( CG_ARGC );
}

void	cgi_Argv( int n, char *buffer, int bufferLength ) {
	syscall( CG_ARGV, n, buffer, bufferLength );
}

void	cgi_Args( char *buffer, int bufferLength ) {
	syscall( CG_ARGS, buffer, bufferLength );
}

int		cgi_FS_FOpenFile( const char *qpath, fileHandle_t *f, fsMode_t mode ) {
	return syscall( CG_FS_FOPENFILE, qpath, f, mode );
}

int	cgi_FS_Read( void *buffer, int len, fileHandle_t f ) {
	return syscall( CG_FS_READ, buffer, len, f );
}

int	cgi_FS_Write( const void *buffer, int len, fileHandle_t f ) {
	return syscall( CG_FS_WRITE, buffer, len, f );
}

void	cgi_FS_FCloseFile( fileHandle_t f ) {
	syscall( CG_FS_FCLOSEFILE, f );
}

void	cgi_SendConsoleCommand( const char *text ) {
	syscall( CG_SENDCONSOLECOMMAND, text );
}

void	cgi_AddCommand( const char *cmdName ) {
	syscall( CG_ADDCOMMAND, cmdName );
}

void	cgi_SendClientCommand( const char *s ) {
	syscall( CG_SENDCLIENTCOMMAND, s );
}

void	cgi_UpdateScreen( void ) {
	syscall( CG_UPDATESCREEN );
}

void	cgi_CM_LoadMap( const char *mapname ) {
	syscall( CG_CM_LOADMAP, mapname );
}

int		cgi_CM_NumInlineModels( void ) {
	return syscall( CG_CM_NUMINLINEMODELS );
}

clipHandle_t cgi_CM_InlineModel( int index ) {
	return syscall( CG_CM_INLINEMODEL, index );
}

clipHandle_t cgi_CM_TempBoxModel( const vec3_t mins, const vec3_t maxs ) {//, const int contents ) {
	return syscall( CG_CM_TEMPBOXMODEL, mins, maxs );//, contents );
}

int		cgi_CM_PointContents( const vec3_t p, clipHandle_t model ) {
	return syscall( CG_CM_POINTCONTENTS, p, model );
}

int		cgi_CM_TransformedPointContents( const vec3_t p, clipHandle_t model, const vec3_t origin, const vec3_t angles ) {
	return syscall( CG_CM_TRANSFORMEDPOINTCONTENTS, p, model, origin, angles );
}

void	cgi_CM_BoxTrace( trace_t *results, const vec3_t start, const vec3_t end,
						  const vec3_t mins, const vec3_t maxs,
						  clipHandle_t model, int brushmask ) {
	syscall( CG_CM_BOXTRACE, results, start, end, mins, maxs, model, brushmask );
}

void	cgi_CM_TransformedBoxTrace( trace_t *results, const vec3_t start, const vec3_t end,
						  const vec3_t mins, const vec3_t maxs,
						  clipHandle_t model, int brushmask,
						  const vec3_t origin, const vec3_t angles ) {
	syscall( CG_CM_TRANSFORMEDBOXTRACE, results, start, end, mins, maxs, model, brushmask, origin, angles );
}

int		cgi_CM_MarkFragments( int numPoints, const vec3_t *points, 
				const vec3_t projection,
				int maxPoints, vec3_t pointBuffer,
				int maxFragments, markFragment_t *fragmentBuffer ) {
	return syscall( CG_CM_MARKFRAGMENTS, numPoints, points, projection, maxPoints, pointBuffer, maxFragments, fragmentBuffer );
}

void cgi_CM_SnapPVS(vec3_t origin,byte *buffer)
{
	syscall(CG_CM_SNAPPVS,origin,buffer);
}

void	cgi_S_StartSound( vec3_t origin, int entityNum, int entchannel, sfxHandle_t sfx ) {
	syscall( CG_S_STARTSOUND, origin, entityNum, entchannel, sfx );
}

void	cgi_AS_ParseSets( void ) {
	syscall( CG_AS_PARSESETS );
}

void	cgi_AS_AddPrecacheEntry( const char *name ) {
	syscall( CG_AS_ADDENTRY, name );
}

void	cgi_S_UpdateAmbientSet( const char *name, vec3_t origin ) {
	syscall( CG_S_UPDATEAMBIENTSET, name, origin );
}

int		cgi_S_AddLocalSet( const char *name, vec3_t listener_origin, vec3_t origin, int entID, int time ) {
	return syscall( CG_S_ADDLOCALSET, name, listener_origin, origin, entID, time );
}

sfxHandle_t cgi_AS_GetBModelSound( const char *name, int stage ) {
	return syscall( CG_AS_GETBMODELSOUND, name, stage );
}

void	cgi_S_StartLocalSound( sfxHandle_t sfx, int channelNum ) {
	syscall( CG_S_STARTLOCALSOUND, sfx, channelNum );
}

void	cgi_S_ClearLoopingSounds( void ) {
	syscall( CG_S_CLEARLOOPINGSOUNDS );
}

void	cgi_S_AddLoopingSound( int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx ) {
	syscall( CG_S_ADDLOOPINGSOUND, entityNum, origin, velocity, sfx );
}

void	cgi_S_UpdateEntityPosition( int entityNum, const vec3_t origin ) {
	syscall( CG_S_UPDATEENTITYPOSITION, entityNum, origin );
}

void	cgi_S_Respatialize( int entityNum, const vec3_t origin, vec3_t axis[3], qboolean inwater ) {
	syscall( CG_S_RESPATIALIZE, entityNum, origin, axis, inwater );
}

sfxHandle_t	cgi_S_RegisterSound( const char *sample ) {
	return syscall( CG_S_REGISTERSOUND, sample );
}

void	cgi_S_StartBackgroundTrack( const char *intro, const char *loop, qboolean bForceStart ) {
	syscall( CG_S_STARTBACKGROUNDTRACK, intro, loop, bForceStart );
}

float	cgi_S_GetSampleLength( sfxHandle_t sfx ) {
	return syscall( CG_S_GETSAMPLELENGTH, sfx);
}

#ifdef _IMMERSION

void	cgi_FF_Start( ffHandle_t ff, int clientNum ){
	syscall( CG_FF_START, ff, clientNum );
}

void	cgi_FF_Stop( ffHandle_t ff, int clientNum ){
	syscall( CG_FF_STOP, ff, clientNum );
}

void	cgi_FF_StopAll( void ){
	syscall( CG_FF_STOPALL );
}

void	cgi_FF_Shake( int intensity, int duration ){
	syscall( CG_FF_SHAKE, intensity, duration );
}

ffHandle_t cgi_FF_Register( const char *name, int channel ){
	return syscall( CG_FF_REGISTER, name, channel );
}

void	cgi_FF_AddLoopingForce( ffHandle_t handle, int entNum ){
	syscall( CG_FF_ADDLOOPINGFORCE, handle, entNum );
}

#else

void	cgi_FF_StartFX( int iFX ){
	syscall( CG_FF_STARTFX, iFX );
}

void	cgi_FF_EnsureFX( int iFX ){
	syscall( CG_FF_ENSUREFX, iFX );
}

void	cgi_FF_StopFX( int iFX ){
	syscall( CG_FF_STOPFX, iFX );
}

void	cgi_FF_StopAllFX( void ){
	syscall( CG_FF_STOPALLFX );
}

#endif // _IMMERSION
void	cgi_R_LoadWorldMap( const char *mapname ) {
	syscall( CG_R_LOADWORLDMAP, mapname );
}

qhandle_t cgi_R_RegisterModel( const char *name ) {
	return syscall( CG_R_REGISTERMODEL, name );
}

qhandle_t cgi_R_RegisterSkin( const char *name ) {
	return syscall( CG_R_REGISTERSKIN, name );
}

qhandle_t cgi_R_RegisterShader( const char *name ) {
	qhandle_t hShader = syscall( CG_R_REGISTERSHADER, name );
	assert (hShader);
	return  hShader;
}

qhandle_t cgi_R_RegisterShaderNoMip( const char *name ) {
	return syscall( CG_R_REGISTERSHADERNOMIP, name );
}

qhandle_t cgi_R_RegisterFont( const char *name ) {
	return syscall( CG_R_REGISTERFONT, name );
}

int cgi_R_Font_StrLenPixels(const char *text, const int iFontIndex, const float scale /*= 1.0f*/) {
	return syscall( CG_R_FONTSTRLENPIXELS, text, iFontIndex, PASSFLOAT(scale) ) ;
}

int cgi_R_Font_StrLenChars(const char *text) {
	return syscall( CG_R_FONTSTRLENCHARS, text ) ;
}

int cgi_R_Font_HeightPixels(const int iFontIndex, const float scale /*= 1.0f*/) {
	return syscall( CG_R_FONTHEIGHTPIXELS, iFontIndex, PASSFLOAT(scale) );
}

qboolean cgi_Language_IsAsian( void )
{
	return syscall( CG_LANGUAGE_ISASIAN );
}

qboolean cgi_Language_UsesSpaces(void)
{
	return syscall( CG_LANGUAGE_USESSPACES );
}

unsigned int cgi_AnyLanguage_ReadCharFromString( const char **ppText, qboolean *pbIsTrailingPunctuation /* = NULL */ )
{
	return syscall( CG_ANYLANGUAGE_READFROMSTRING, ppText, pbIsTrailingPunctuation );
}

void cgi_R_Font_DrawString(int ox, int oy, const char *text, const float *rgba, const int setIndex, int iMaxPixelWidth, const float scale /*= 1.0f*/) {
	syscall (CG_R_FONTDRAWSTRING, ox, oy, text, rgba, setIndex, iMaxPixelWidth, PASSFLOAT(scale) );
}

void	cgi_R_ClearScene( void ) {
	syscall( CG_R_CLEARSCENE );
}

void	cgi_R_AddRefEntityToScene( const refEntity_t *re ) {
	syscall( CG_R_ADDREFENTITYTOSCENE, re );
}

void	cgi_R_GetLighting( const vec3_t origin, vec3_t ambientLight, vec3_t directedLight, vec3_t ligthDir ) {
	syscall( CG_R_GETLIGHTING, origin, ambientLight, directedLight, ligthDir );
}

void	cgi_R_AddPolyToScene( qhandle_t hShader , int numVerts, const polyVert_t *verts ) {
	syscall( CG_R_ADDPOLYTOSCENE, hShader, numVerts, verts );
}

void	cgi_R_AddLightToScene( const vec3_t org, float intensity, float r, float g, float b ) {
	syscall( CG_R_ADDLIGHTTOSCENE, org, PASSFLOAT(intensity), PASSFLOAT(r), PASSFLOAT(g), PASSFLOAT(b) );
}

void	cgi_R_RenderScene( const refdef_t *fd ) {
	syscall( CG_R_RENDERSCENE, fd );
}

void	cgi_R_SetColor( const float *rgba ) {
	syscall( CG_R_SETCOLOR, rgba );
}

void	cgi_R_DrawStretchPic( float x, float y, float w, float h, 
							   float s1, float t1, float s2, float t2, qhandle_t hShader ) {
	syscall( CG_R_DRAWSTRETCHPIC, PASSFLOAT(x), PASSFLOAT(y), PASSFLOAT(w), PASSFLOAT(h), PASSFLOAT(s1), PASSFLOAT(t1), PASSFLOAT(s2), PASSFLOAT(t2), hShader );
}

void	cgi_R_DrawScreenShot( float x, float y, float w, float h){
	syscall( CG_R_DRAWSCREENSHOT, PASSFLOAT(x), PASSFLOAT(y), PASSFLOAT(w), PASSFLOAT(h) );
}

void	cgi_R_ModelBounds( qhandle_t model, vec3_t mins, vec3_t maxs ) {
	syscall( CG_R_MODELBOUNDS, model, mins, maxs );
}

void	cgi_R_LerpTag( orientation_t *tag, qhandle_t mod, int startFrame, int endFrame, 
					   float frac, const char *tagName ) {
	syscall( CG_R_LERPTAG, tag, mod, startFrame, endFrame, PASSFLOAT(frac), tagName );
}

void	cgi_R_DrawRotatePic( float x, float y, float w, float h, 
				   float s1, float t1, float s2, float t2,float a, qhandle_t hShader ) 
{
	syscall( CG_R_DRAWROTATEPIC, PASSFLOAT(x), PASSFLOAT(y), PASSFLOAT(w), PASSFLOAT(h), PASSFLOAT(s1), PASSFLOAT(t1), PASSFLOAT(s2), PASSFLOAT(t2), PASSFLOAT(a), hShader );
}

void	cgi_R_DrawRotatePic2( float x, float y, float w, float h, 
				   float s1, float t1, float s2, float t2,float a, qhandle_t hShader ) 
{
	syscall( CG_R_DRAWROTATEPIC2, PASSFLOAT(x), PASSFLOAT(y), PASSFLOAT(w), PASSFLOAT(h), PASSFLOAT(s1), PASSFLOAT(t1), PASSFLOAT(s2), PASSFLOAT(t2), PASSFLOAT(a), hShader );
}

void	cgi_R_LAGoggles( void )
{
	syscall( CG_R_LA_GOGGLES );
}

void	cgi_R_Scissor( float x, float y, float w, float h) 
{
	syscall( CG_R_SCISSOR, PASSFLOAT(x), PASSFLOAT(y), PASSFLOAT(w), PASSFLOAT(h));
}

void		cgi_GetGlconfig( glconfig_t *glconfig ) {
	syscall( CG_GETGLCONFIG, glconfig );
}

void		cgi_GetGameState( gameState_t *gamestate ) {
	syscall( CG_GETGAMESTATE, gamestate );
}

void		cgi_GetCurrentSnapshotNumber( int *snapshotNumber, int *serverTime ) {
	syscall( CG_GETCURRENTSNAPSHOTNUMBER, snapshotNumber, serverTime );
}

qboolean	cgi_GetSnapshot( int snapshotNumber, snapshot_t *snapshot ) {
	return syscall( CG_GETSNAPSHOT, snapshotNumber, snapshot );
}

qboolean	cgi_GetServerCommand( int serverCommandNumber ) {
	return syscall( CG_GETSERVERCOMMAND, serverCommandNumber );
}

int			cgi_GetCurrentCmdNumber( void ) {
	return syscall( CG_GETCURRENTCMDNUMBER );
}

qboolean	cgi_GetUserCmd( int cmdNumber, usercmd_t *ucmd ) {
	return syscall( CG_GETUSERCMD, cmdNumber, ucmd );
}

void		cgi_SetUserCmdValue( int stateValue, float sensitivityScale, float mPitchOverride, float mYawOverride ) {
	syscall( CG_SETUSERCMDVALUE, stateValue, PASSFLOAT(sensitivityScale), PASSFLOAT(mPitchOverride), PASSFLOAT(mYawOverride) );
}

void		cgi_SetUserCmdAngles( float pitchOverride, float yawOverride, float rollOverride ) {
	syscall( CG_SETUSERCMDANGLES, PASSFLOAT(pitchOverride), PASSFLOAT(yawOverride), PASSFLOAT(rollOverride) );
}
/*
Ghoul2 Insert Start
*/
// CG Specific API calls
void		trap_G2_SetGhoul2ModelIndexes(CGhoul2Info_v &ghoul2, qhandle_t *modelList, qhandle_t *skinList)
{
	syscall( CG_G2_SETMODELS, &ghoul2, modelList, skinList);
}
/*
Ghoul2 Insert End
*/

void	trap_Com_SetOrgAngles(vec3_t org,vec3_t angles)
{
	syscall(COM_SETORGANGLES,org,angles);
}

void	trap_R_GetLightStyle(int style, color4ub_t color)
{
	syscall(CG_R_GET_LIGHT_STYLE, style, color);
}

void	trap_R_SetLightStyle(int style, int color)
{
	syscall(CG_R_SET_LIGHT_STYLE, style, color);
}

void	cgi_R_GetBModelVerts(int bmodelIndex, vec3_t *verts, vec3_t normal )
{
	syscall( CG_R_GET_BMODEL_VERTS, bmodelIndex, verts, normal );
}

void	cgi_R_WorldEffectCommand( const char *command )
{
	syscall( CG_R_WORLD_EFFECT_COMMAND, command );
}

// this returns a handle.  arg0 is the name in the format "idlogo.roq", set arg1 to NULL, alteredstates to qfalse (do not alter gamestate)
int trap_CIN_PlayCinematic( const char *arg0, int xpos, int ypos, int width, int height, int bits, const char *psAudioFile /* = NULL */) {
  return syscall(CG_CIN_PLAYCINEMATIC, arg0, xpos, ypos, width, height, bits, psAudioFile);
}
 
// stops playing the cinematic and ends it.  should always return FMV_EOF
// cinematics must be stopped in reverse order of when they are started
e_status trap_CIN_StopCinematic(int handle) {
  return (e_status) syscall(CG_CIN_STOPCINEMATIC, handle);
}


// will run a frame of the cinematic but will not draw it.  Will return FMV_EOF if the end of the cinematic has been reached.
e_status trap_CIN_RunCinematic (int handle) {
  return (e_status) syscall(CG_CIN_RUNCINEMATIC, handle);
}
 

// draws the current frame
void trap_CIN_DrawCinematic (int handle) {
  syscall(CG_CIN_DRAWCINEMATIC, handle);
}
 

// allows you to resize the animation dynamically
void trap_CIN_SetExtents (int handle, int x, int y, int w, int h) {
  syscall(CG_CIN_SETEXTENTS, handle, x, y, w, h);
}

void *cgi_Z_Malloc( int size, int tag )
{
	return (void *)syscall(CG_Z_MALLOC,size,tag);
}

void cgi_Z_Free( void *ptr )
{
	syscall(CG_Z_FREE,ptr);
}

void cgi_UI_Menu_Reset(void)
{
	syscall(CG_UI_MENU_RESET);
}

void cgi_UI_Menu_New(char *buf)
{
	syscall(CG_UI_MENU_NEW,buf);
}

void cgi_UI_Parse_Int(int *value)
{
	syscall(CG_UI_PARSE_INT,value);
}

void cgi_UI_Parse_String(char *buf)
{
	syscall(CG_UI_PARSE_STRING,buf);
}

void cgi_UI_Parse_Float(float *value)
{
	syscall(CG_UI_PARSE_FLOAT,value);
}

int cgi_UI_StartParseSession(char *menuFile,char **buf)
{
	return(int) syscall(CG_UI_STARTPARSESESSION,menuFile,buf);
}

void cgi_UI_EndParseSession(char *buf)
{
	syscall(CG_UI_ENDPARSESESSION,buf);
}

void cgi_UI_ParseExt(char **token)
{
	syscall(CG_UI_PARSEEXT,token);
}

void cgi_UI_MenuPaintAll(void)
{
	syscall(CG_UI_MENUPAINT_ALL);
}

void cgi_UI_String_Init(void)
{
	syscall(CG_UI_STRING_INIT);
}

int cgi_UI_GetMenuInfo(char *menuFile,int *x,int *y)
{
	return(int) syscall(CG_UI_GETMENUINFO,menuFile,x,y);
}

int cgi_UI_GetItemText(char *menuFile,char *itemName, char* text)
{
	return(int) syscall(CG_UI_GETITEMTEXT,menuFile,itemName,text);
}

int cgi_SP_Register(const char *text, qboolean persist )
{
	return syscall( CG_SP_REGISTER, text, persist );
}

int cgi_SP_GetStringTextString(const char *text, char *buffer, int bufferLength)
{
	return syscall( CG_SP_GETSTRINGTEXTSTRING, text, buffer, bufferLength );
}

int cgi_SP_GetStringText(int ID, char *buffer, int bufferLength)
{
	return syscall( CG_SP_GETSTRINGTEXT, ID, buffer, bufferLength );
}

int cgi_EndGame(void)
{
//extern void CMD_CGCam_Disable( void );
	//CMD_CGCam_Disable();	//can't do it here because it will draw the hud when we're out of camera
	return syscall( CG_SENDCONSOLECOMMAND, "cam_disable; set nextmap disconnect; cinematic outcast\n" );
}
