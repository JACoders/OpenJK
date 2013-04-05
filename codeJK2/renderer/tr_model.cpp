// tr_models.c -- model loading and caching

// leave this as first line for PCH reasons...
//
#include "../server/exe_headers.h"

#include "tr_local.h"
#include "MatComp.h"
#include "../qcommon/sstring.h"

#define	LL(x) x=LittleLong(x)

static qboolean R_LoadMD3 (model_t *mod, int lod, void *buffer, const char *name, qboolean bAlreadyCached );
static qboolean R_LoadMD4 (model_t *mod,		  void *buffer, const char *name, qboolean bAlreadyCached );

/*
Ghoul2 Insert Start
*/

typedef	struct modelHash_s
{
	char		name[MAX_QPATH];
	qhandle_t	handle;
	struct		modelHash_s	*next;

}modelHash_t;

#define FILE_HASH_SIZE		1024
static	modelHash_t 		*mhHashTable[FILE_HASH_SIZE];


/*
Ghoul2 Insert End
*/



// This stuff looks a bit messy, but it's kept here as black box, and nothing appears in any .H files for other 
//	modules to worry about. I may make another module for this sometime.
//
typedef pair<int,int> StringOffsetAndShaderIndexDest_t;
typedef vector <StringOffsetAndShaderIndexDest_t> ShaderRegisterData_t;
struct CachedEndianedModelBinary_s
{
	void	*pModelDiskImage;
	int		iAllocSize;		// may be useful for mem-query, but I don't actually need it
	ShaderRegisterData_t ShaderRegisterData;

	int		iLastLevelUsedOn;

	CachedEndianedModelBinary_s()
	{
		pModelDiskImage = 0;
		iLastLevelUsedOn    = -1;
		iAllocSize = 0;
		ShaderRegisterData.clear();
	}
};
typedef struct CachedEndianedModelBinary_s CachedEndianedModelBinary_t;
typedef map <sstring_t,CachedEndianedModelBinary_t>	CachedModels_t;
													CachedModels_t CachedModels;	// the important cache item.

void RE_RegisterModels_StoreShaderRequest(const char *psModelFileName, const char *psShaderName, const int *piShaderIndexPoke)
{
	char sModelName[MAX_QPATH];

	Q_strncpyz(sModelName,psModelFileName,sizeof(sModelName));
	Q_strlwr  (sModelName);

	CachedEndianedModelBinary_t &ModelBin = CachedModels[sModelName];

	if (ModelBin.pModelDiskImage == NULL)
	{	
		assert(0);	// should never happen, means that we're being called on a model that wasn't loaded
	}
	else
	{
		const int iNameOffset =		  psShaderName		- (char *)ModelBin.pModelDiskImage;
		const int iPokeOffset = (char*) piShaderIndexPoke	- (char *)ModelBin.pModelDiskImage;

		ModelBin.ShaderRegisterData.push_back( StringOffsetAndShaderIndexDest_t( iNameOffset,iPokeOffset) );
	}
}


static const byte FakeGLAFile[] = 
{
0x32, 0x4C, 0x47, 0x41, 0x06, 0x00, 0x00, 0x00, 0x2A, 0x64, 0x65, 0x66, 0x61, 0x75, 0x6C, 0x74,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x01, 0x00, 0x00, 0x00,
0x14, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x18, 0x01, 0x00, 0x00, 0x68, 0x00, 0x00, 0x00,
0x26, 0x01, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x4D, 0x6F, 0x64, 0x56, 0x69, 0x65, 0x77, 0x20,
0x69, 0x6E, 0x74, 0x65, 0x72, 0x6E, 0x61, 0x6C, 0x20, 0x64, 0x65, 0x66, 0x61, 0x75, 0x6C, 0x74,
0x00, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD,
0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD,
0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFD, 0xBF, 0xFE, 0x7F, 0xFE, 0x7F, 0xFE, 0x7F,
0x00, 0x80, 0x00, 0x80, 0x00, 0x80
};


// returns qtrue if loaded, and sets the supplied qbool to true if it was from cache (instead of disk)
//   (which we need to know to avoid LittleLong()ing everything again (well, the Mac needs to know anyway)...
//
qboolean RE_RegisterModels_GetDiskFile( const char *psModelFileName, void **ppvBuffer, qboolean *pqbAlreadyCached)
{
	char sModelName[MAX_QPATH];

	Q_strncpyz(sModelName,psModelFileName,sizeof(sModelName));
	Q_strlwr  (sModelName);

	CachedEndianedModelBinary_t &ModelBin = CachedModels[sModelName];

	if (ModelBin.pModelDiskImage == NULL)
	{
		// didn't have it cached, so try the disk...
		//

			// special case intercept first...
			//
			if (!strcmp(sDEFAULT_GLA_NAME ".gla" , psModelFileName))
			{
				// return fake params as though it was found on disk...
				//
				void *pvFakeGLAFile = ri.Malloc( sizeof(FakeGLAFile), TAG_FILESYS, qfalse );
				memcpy(pvFakeGLAFile, &FakeGLAFile[0],  sizeof(FakeGLAFile));
				*ppvBuffer = pvFakeGLAFile;
				*pqbAlreadyCached = qfalse;	// faking it like this should mean that it works fine on the Mac as well
				return qtrue;	
			}

		ri.FS_ReadFile( sModelName, ppvBuffer );
		*pqbAlreadyCached = qfalse;

		bool bSuccess = !!(*ppvBuffer);

		if (bSuccess)
		{				
			ri.Printf( PRINT_DEVELOPER, "RE_RegisterModels_GetDiskFile(): Disk-loading \"%s\"\n",psModelFileName);
		}

		return bSuccess;
	}
	else
	{
		*ppvBuffer = ModelBin.pModelDiskImage;
		*pqbAlreadyCached = qtrue;
		return qtrue;
	}
}


// if return == true, no further action needed by the caller...
//
void *RE_RegisterModels_Malloc(int iSize, const char *psModelFileName, qboolean *pqbAlreadyFound, memtag_t eTag)
{
	char sModelName[MAX_QPATH];

	Q_strncpyz(sModelName,psModelFileName,sizeof(sModelName));
	Q_strlwr  (sModelName);

	CachedEndianedModelBinary_t &ModelBin = CachedModels[sModelName];

	if (ModelBin.pModelDiskImage == NULL)
	{
		// ... then this entry has only just been created, ie we need to load it fully...
		//
		void *pMalloc = ri.Malloc(iSize,eTag, qfalse );

		if (!pMalloc)	// not needed anymore, but wtf?
		{
			ri.Error( ERR_FATAL, "RE_RegisterModels_Malloc(): Failed to alloc %d bytes for \"%s\"!", iSize, psModelFileName );
		}

		ModelBin.pModelDiskImage= pMalloc;
		ModelBin.iAllocSize		= iSize;
		*pqbAlreadyFound		= qfalse;
	}
	else
	{
		// if we already had this model entry, then re-register all the shaders it wanted...
		//
		const int iEntries = ModelBin.ShaderRegisterData.size();
		for (int i=0; i<iEntries; i++)
		{
			int iShaderNameOffset	= ModelBin.ShaderRegisterData[i].first;
			int iShaderPokeOffset	= ModelBin.ShaderRegisterData[i].second;

			const char *const psShaderName	 =		   &((char*)ModelBin.pModelDiskImage)[iShaderNameOffset];
				  int  *const piShaderPokePtr= (int *) &((char*)ModelBin.pModelDiskImage)[iShaderPokeOffset];

			shader_t *sh = R_FindShader( psShaderName, lightmapsNone, stylesDefault, qtrue );
	            
			if ( sh->defaultShader ) 
			{
				*piShaderPokePtr = 0;
			} else {
				*piShaderPokePtr = sh->index;
			}
		}
		*pqbAlreadyFound = qtrue;	// tell caller not to re-Endian or re-Shader this binary		
	}

	ModelBin.iLastLevelUsedOn = RE_RegisterMedia_GetLevel();

	return ModelBin.pModelDiskImage;
}


// dump any models not being used by this level if we're running low on memory...
//
static int GetModelDataAllocSize(void)
{
	return	Z_MemSize( TAG_MODEL_MD3) +
			Z_MemSize( TAG_MODEL_MD4) +
			Z_MemSize( TAG_MODEL_GLM) +
			Z_MemSize( TAG_MODEL_GLA);
}
extern cvar_t *r_modelpoolmegs;
//
// return qtrue if at least one cached model was freed (which tells z_malloc()-fail recoveryt code to try again)
//
extern qboolean gbInsideRegisterModel;
qboolean RE_RegisterModels_LevelLoadEnd(qboolean bDeleteEverythingNotUsedThisLevel /* = qfalse */)
{	
	qboolean bAtLeastoneModelFreed = qfalse;

	ri.Printf( PRINT_DEVELOPER, "RE_RegisterModels_LevelLoadEnd():\n");

	if (gbInsideRegisterModel)
	{
		Com_DPrintf( "(Inside RE_RegisterModel (z_malloc recovery?), exiting...\n");
	}
	else
	{
		int iLoadedModelBytes	=	GetModelDataAllocSize();
		const int iMaxModelBytes=	r_modelpoolmegs->integer * 1024 * 1024;

		qboolean bEraseOccured = qfalse;
		for (CachedModels_t::iterator itModel = CachedModels.begin(); itModel != CachedModels.end() && ( bDeleteEverythingNotUsedThisLevel || iLoadedModelBytes > iMaxModelBytes ); bEraseOccured?itModel:++itModel)
		{			
			bEraseOccured = qfalse;

			CachedEndianedModelBinary_t &CachedModel = (*itModel).second;

			qboolean bDeleteThis = qfalse;

			if (bDeleteEverythingNotUsedThisLevel)
			{
				bDeleteThis = (CachedModel.iLastLevelUsedOn != RE_RegisterMedia_GetLevel());
			}
			else
			{
				bDeleteThis = (CachedModel.iLastLevelUsedOn < RE_RegisterMedia_GetLevel());
			}

			// if it wasn't used on this level, dump it...
			//
			if (bDeleteThis)
			{
				LPCSTR psModelName = (*itModel).first.c_str();
				ri.Printf( PRINT_DEVELOPER, "Dumping \"%s\"", psModelName);

	#ifdef _DEBUG
				ri.Printf( PRINT_DEVELOPER, ", used on lvl %d\n",CachedModel.iLastLevelUsedOn);
	#endif				

				if (CachedModel.pModelDiskImage) {
					ri.Free(CachedModel.pModelDiskImage);	
					//CachedModel.pModelDiskImage = NULL;	// REM for reference, erase() call below negates the need for it.
					bAtLeastoneModelFreed = qtrue;
				}

				itModel = CachedModels.erase(itModel);
				bEraseOccured = qtrue;

				iLoadedModelBytes = GetModelDataAllocSize();				
			}
		}
	}

	ri.Printf( PRINT_DEVELOPER, "RE_RegisterModels_LevelLoadEnd(): Ok\n");	

	return bAtLeastoneModelFreed;	
}

void RE_RegisterModels_Info_f( void )
{	
	int iModels = CachedModels.size();
	int iModel  = 0;
	int iTotalBytes = 0;
	for (CachedModels_t::iterator itModel = CachedModels.begin(); itModel != CachedModels.end(); ++itModel,iModel++)
	{	
		CachedEndianedModelBinary_t &CachedModel = (*itModel).second;

		ri.Printf( PRINT_ALL, "%d/%d: \"%s\" (%d bytes)",iModel,iModels,(*itModel).first.c_str(),CachedModel.iAllocSize );

		#ifdef _DEBUG
		ri.Printf( PRINT_ALL, ", lvl %d\n",CachedModel.iLastLevelUsedOn);
		#endif

		iTotalBytes += CachedModel.iAllocSize;
	}
	ri.Printf( PRINT_ALL, "%d bytes total (%.2fMB)\n",iTotalBytes, (float)iTotalBytes / 1024.0f / 1024.0f);
}


static void RE_RegisterModels_DeleteAll(void)
{
	for (CachedModels_t::iterator itModel = CachedModels.begin(); itModel != CachedModels.end(); )
	{
		CachedEndianedModelBinary_t &CachedModel = (*itModel).second;

		if (CachedModel.pModelDiskImage) {
			Z_Free(CachedModel.pModelDiskImage);					
		}

		itModel = CachedModels.erase(itModel);			
	}

	extern void RE_AnimationCFGs_DeleteAll(void);
	RE_AnimationCFGs_DeleteAll();
}


static int giRegisterMedia_CurrentLevel=0;
static qboolean gbAllowScreenDissolve = qtrue;
//
// param "bAllowScreenDissolve" is just a convenient way of getting hold of a bool which can be checked by the code that
//	issues the InitDissolve command later in RE_RegisterMedia_LevelLoadEnd()
//
void RE_RegisterMedia_LevelLoadBegin(const char *psMapName, ForceReload_e eForceReload, qboolean bAllowScreenDissolve)
{
	gbAllowScreenDissolve = bAllowScreenDissolve;

	// for development purposes we may want to ditch certain media just before loading a map...
	//
	switch (eForceReload)
	{
		case eForceReload_BSP:

			CM_DeleteCachedMap(qtrue);
			R_Images_DeleteLightMaps();
			break;

		case eForceReload_MODELS:

			RE_RegisterModels_DeleteAll();
			break;

		case eForceReload_ALL:

			// BSP...
			//
			CM_DeleteCachedMap(qtrue);
			R_Images_DeleteLightMaps();
			//
			// models...
			//
			RE_RegisterModels_DeleteAll();
			break;
	}

	// at some stage I'll probably want to put some special logic here, like not incrementing the level number
	//	when going into a map like "brig" or something, so returning to the previous level doesn't require an 
	//	asset reload etc, but for now...
	//
	// only bump level number if we're not on the same level. 
	//	Note that this will hide uncached models, which is perhaps a bad thing?...
	//
	static char sPrevMapName[MAX_QPATH]={0};
	if (Q_stricmp( psMapName,sPrevMapName ))
	{
		Q_strncpyz( sPrevMapName, psMapName, sizeof(sPrevMapName) );
		giRegisterMedia_CurrentLevel++;
	}
}

int RE_RegisterMedia_GetLevel(void)
{
	return giRegisterMedia_CurrentLevel;
}

extern qboolean SND_RegisterAudio_LevelLoadEnd(qboolean bDeleteEverythingNotUsedThisLevel);

void RE_RegisterMedia_LevelLoadEnd(void)
{
	RE_RegisterModels_LevelLoadEnd(qfalse);
	RE_RegisterImages_LevelLoadEnd();
	SND_RegisterAudio_LevelLoadEnd(qfalse);

	if (gbAllowScreenDissolve)
	{
		RE_InitDissolve(qfalse);
	}

	S_RestartMusic();
	
	extern qboolean gbAlreadyDoingLoad;
					gbAlreadyDoingLoad = qfalse;
}




/*
** R_GetModelByHandle
*/
model_t	*R_GetModelByHandle( qhandle_t index ) {
	model_t		*mod;

	// out of range gets the defualt model
	if ( index < 1 || index >= tr.numModels ) {
		return tr.models[0];
	}

	mod = tr.models[index];

	return mod;
}

//===============================================================================

/*
** R_AllocModel
*/
model_t *R_AllocModel( void ) {
	model_t		*mod;

	if ( tr.numModels == MAX_MOD_KNOWN ) {
		return NULL;
	}

	mod = (model_t*) ri.Hunk_Alloc( sizeof( *tr.models[tr.numModels] ), qtrue );
	mod->index= tr.numModels;
	tr.models[tr.numModels] = mod;
	tr.numModels++;

	return mod;
}

/*
Ghoul2 Insert Start
*/

/*
================
return a hash value for the filename
================
*/
static long generateHashValue( const char *fname, const int size ) {
	int		i;
	long	hash;
	char	letter;

	hash = 0;
	i = 0;
	while (fname[i] != '\0') {
		letter = tolower(fname[i]);
		if (letter =='.') break;				// don't include extension
		if (letter =='\\') letter = '/';		// damn path names
		hash+=(long)(letter)*(i+119);
		i++;
	}
	hash &= (size-1);
	return hash;
}

void RE_InsertModelIntoHash(const char *name, model_t *mod)
{
	int			hash;
	modelHash_t	*mh;

	hash = generateHashValue(name, FILE_HASH_SIZE);

	// insert this file into the hash table so we can look it up faster later
	mh = (modelHash_t*)ri.Hunk_Alloc( sizeof( modelHash_t ), qtrue );

	mh->next = mhHashTable[hash];
	mh->handle = mod->index;
	strcpy(mh->name, name);
	mhHashTable[hash] = mh;
}
/*
Ghoul2 Insert End
*/

#ifdef _NPATCH
// this is now replaced by code that looks for "path/path/modelname.npatch" file existance to enable npatching
/*
// List of all n-patchable models
// Just comment out models that shouldn't be n-patched...
char *npatchableModels[] =
{
	"models/players/bespin_cop/model.glm",
	"models/players/chiss/model.glm",
	"models/players/desann/model.glm",
	"models/players/droids/model.glm",
	"models/players/galak/model.glm",
	"models/players/galak_mech/model.glm",
	"models/players/gonk/model.glm",
	"models/players/gran/model.glm",
	"models/players/imperial/model.glm",
	"models/players/imperial_worker/model.glm",
	"models/players/interrogator/model.glm",
	"models/players/jan/model.glm",
	"models/players/jedi/model.glm",
	"models/players/jeditrainer/model.glm",
	"models/players/kyle/model.glm",
	"models/players/lando/model.glm",
	"models/players/luke/model.glm",
	"models/players/mark1/model.glm",
	"models/players/mark2/model.glm",
	"models/players/minemonster/model.glm",
	"models/players/monmothma/model.glm",
	"models/players/morgan/model.glm",
	"models/players/mouse/model.glm",
	"models/players/prisoner/model.glm",
	"models/players/probe/model.glm",
	"models/players/protocol/model.glm",
	"models/players/r2d2/model.glm",
	"models/players/r5d2/model.glm",
	"models/players/rebel/model.glm",
	"models/players/reborn/model.glm",
	"models/players/reelo/model.glm",
	"models/players/remote/model.glm",
	"models/players/rodian/model.glm",
	"models/players/sentry/model.glm",
	"models/players/shadowtrooper/model.glm",
	"models/players/stormpilot/model.glm",
	"models/players/stormtrooper/model.glm",
	"models/players/swamptrooper/model.glm",
	"models/players/tavion/model.glm",
	"models/players/trandoshan/model.glm",
	"models/players/ugnaught/model.glm",
	"models/players/weequay/model.glm",
	NULL
};
*/
#endif // _NPATCH

/*
====================
RE_RegisterModel

Loads in a model for the given name

Zero will be returned if the model fails to load.
An entry will be retained for failed models as an
optimization to prevent disk rescanning if they are
asked for again.
====================
*/
static qhandle_t RE_RegisterModel_Actual( const char *name ) 
{
	model_t		*mod;
	unsigned	*buf;
	int			lod;
	int			ident;
	qboolean	loaded;
//	qhandle_t	hModel;
	int			numLoaded;
/*
Ghoul2 Insert Start
*/
	int			hash;
	modelHash_t	*mh;
/*
Ghoul2 Insert End
*/

	if ( !name || !name[0] ) {
		ri.Printf( PRINT_WARNING, "RE_RegisterModel: NULL name\n" );
		return 0;
	}

	if ( strlen( name ) >= MAX_QPATH ) {
		ri.Printf( PRINT_DEVELOPER, "Model name exceeds MAX_QPATH\n" );
		return 0;
	}

/*
Ghoul2 Insert Start
*/
//	if (!tr.registered) {
//		ri.Printf( PRINT_WARNING, "RE_RegisterModel (%s) called before ready!\n",name );
//		return 0;
//	}
	//
	// search the currently loaded models
	//

	hash = generateHashValue(name, FILE_HASH_SIZE);

	//
	// see if the model is already loaded
	//
	for (mh=mhHashTable[hash]; mh; mh=mh->next) {
		if (Q_stricmp(mh->name, name) == 0) {
			if (tr.models[mh->handle]->type == MOD_BAD)
			{
				return 0;
			}
			return mh->handle;
		}
	}

//	for ( hModel = 1 ; hModel < tr.numModels; hModel++ ) {
//		mod = tr.models[hModel];
//		if ( !strcmp( mod->name, name ) ) {
//			if( mod->type == MOD_BAD ) {
//				return 0;
//			}
//			return hModel;
//		}
//	}

/*
Ghoul2 Insert End
*/

	// allocate a new model_t

	if ( ( mod = R_AllocModel() ) == NULL ) {
		ri.Printf( PRINT_WARNING, "RE_RegisterModel: R_AllocModel() failed for '%s'\n", name);
		return 0;
	}

	// only set the name after the model has been successfully loaded
	Q_strncpyz( mod->name, name, sizeof( mod->name ) );

#ifdef _NPATCH
	// Make sure we don't n-patch unless explicitly specified
/*
	mod->npatchable = qfalse;
	for ( int i = 0; npatchableModels[i]; i++ )
	{
		if ( stricmp(name, npatchableModels[i]) == 0 )
		{
			mod->npatchable = qtrue;
			break;
		}
	}
*/
	// new version, npatching is no controlled by the existance of a "*.npatch" file in the same place as *.glm

	extern cvar_t *r_ATI_NPATCH_available;	
	if (r_ATI_NPATCH_available && r_ATI_NPATCH_available->integer && strstr(name,".glm"))
	{
		extern char *Filename_WithoutExt(const char *psFilename);
		fileHandle_t f;
		FS_FOpenFileRead( va("%s.npatch",Filename_WithoutExt(name)),&f, qtrue ); // qtrue = dup handle, so I can close it ok later
		mod->npatchable = !!f;
		if (f)
		{
			FS_FCloseFile(f);
		}
	}

#endif // _NPATCH

	// make sure the render thread is stopped
	R_SyncRenderThread();

	int iLODStart = 0;
	if (strstr (name, ".md3")) {
		iLODStart = MD3_MAX_LODS-1;	//this loads the md3s in reverse so they can be biased
	}
	mod->numLods = 0;

	//
	// load the files
	//
	numLoaded = 0;

	for ( lod = iLODStart; lod >= 0 ; lod-- ) {
		char filename[1024];

		strcpy( filename, name );

		if ( lod != 0 ) {
			char namebuf[80];

			if ( strrchr( filename, '.' ) ) {
				*strrchr( filename, '.' ) = 0;
			}
			sprintf( namebuf, "_%d.md3", lod );
			strcat( filename, namebuf );
		}

		qboolean bAlreadyCached = qfalse;		
		if (!RE_RegisterModels_GetDiskFile(filename, (void **)&buf, &bAlreadyCached))
		{
			continue;
		}
		
		//loadmodel = mod;	// this seems to be fairly pointless

		// important that from now on we pass 'filename' instead of 'name' to all model load functions,
		//	because 'filename' accounts for any LOD mangling etc so guarantees unique lookups for yet more
		//	internal caching...
		//		
		ident = *(unsigned *)buf;
		if (!bAlreadyCached)
		{
			ident = LittleLong(ident);
		}

		switch (ident)
		{
			// if you add any new types of model load in this switch-case, tell me, 
			//	or copy what I've done with the cache scheme (-ste).
			//
			case MD4_IDENT: 

				loaded = R_LoadMD4( mod, buf, filename, bAlreadyCached );
				break;

			case MDXA_IDENT:

				loaded = R_LoadMDXA( mod, buf, filename, bAlreadyCached );
				break;
		
			case MDXM_IDENT:
				
				loaded = R_LoadMDXM( mod, buf, filename, bAlreadyCached );
				break;

			case MD3_IDENT:

				loaded = R_LoadMD3( mod, lod, buf, filename, bAlreadyCached );
				break;

			default:

				ri.Printf (PRINT_WARNING,"RE_RegisterModel: unknown fileid for %s\n", filename);
				goto fail;
		}
		
		if (!bAlreadyCached){	// important to check!!
			ri.FS_FreeFile (buf);
		}

		if ( !loaded ) {
			if ( lod == 0 ) {
				goto fail;
			} else {
				break;
			}
		} else {
			mod->numLods++;
			numLoaded++;
			// if we have a valid model and are biased
			// so that we won't see any higher detail ones,
			// stop loading them
			if ( lod <= r_lodbias->integer ) {
				break;
			}
		}
	}

	if ( numLoaded ) {
		// duplicate into higher lod spots that weren't
		// loaded, in case the user changes r_lodbias on the fly
		for ( lod-- ; lod >= 0 ; lod-- ) {
			mod->numLods++;
			mod->md3[lod] = mod->md3[lod+1];
		}
/*
Ghoul2 Insert Start
*/

	RE_InsertModelIntoHash(name, mod);
	return mod->index;
/*
Ghoul2 Insert End
*/
	
	}


fail:
	// we still keep the model_t around, so if the model name is asked for
	// again, we won't bother scanning the filesystem
	mod->type = MOD_BAD;
	RE_InsertModelIntoHash(name, mod);
	return 0;
}




// wrapper function needed to avoid problems with mid-function returns so I can safely use this bool to tell the
//	z_malloc-fail recovery code whether it's safe to ditch any model caches...
//
qboolean gbInsideRegisterModel = qfalse;
qhandle_t RE_RegisterModel( const char *name )
{
	gbInsideRegisterModel = qtrue;	// !!!!!!!!!!!!!!

		qhandle_t q = RE_RegisterModel_Actual( name );

if (stricmp(&name[strlen(name)-4],".gla")){
	gbInsideRegisterModel = qfalse;		// GLA files recursively call this, so don't turn off half way. A reference count would be nice, but if any ERR_DROP ever occurs within the load then the refcount will be knackered from then on
}

	return q;
}


/*
=================
R_LoadMD3
=================
*/
static qboolean R_LoadMD3 (model_t *mod, int lod, void *buffer, const char *mod_name, qboolean bAlreadyCached ) {
	int					i, j;
	md3Header_t			*pinmodel;
	md3Surface_t		*surf;
	md3Shader_t			*shader;
	int					version;
	int					size;

#ifndef _M_IX86
	md3Frame_t			*frame;
	md3Triangle_t		*tri;
	md3St_t				*st;
	md3XyzNormal_t		*xyz;
	md3Tag_t			*tag;
#endif


	pinmodel= (md3Header_t *)buffer;
	//
	// read some fields from the binary, but only LittleLong() them when we know this wasn't an already-cached model...
	//
	version = pinmodel->version;
	size	= pinmodel->ofsEnd;

	if (!bAlreadyCached)
	{
		version = LittleLong(version);
		size	= LittleLong(size);
	}
	
	if (version != MD3_VERSION) {
		ri.Printf( PRINT_WARNING, "R_LoadMD3: %s has wrong version (%i should be %i)\n",
				 mod_name, version, MD3_VERSION);
		return qfalse;
	}

	mod->type      = MOD_MESH;	
	mod->dataSize += size;

	qboolean bAlreadyFound = qfalse;
	mod->md3[lod] = (md3Header_t *) //ri.Hunk_Alloc( size );
										RE_RegisterModels_Malloc(size, mod_name, &bAlreadyFound, TAG_MODEL_MD3);

	assert(bAlreadyCached == bAlreadyFound);	// I should probably eliminate 'bAlreadyFound', but wtf?

	if (!bAlreadyFound)
	{	
		memcpy (mod->md3[lod], buffer, size );

		LL(mod->md3[lod]->ident);
		LL(mod->md3[lod]->version);
		LL(mod->md3[lod]->numFrames);
		LL(mod->md3[lod]->numTags);
		LL(mod->md3[lod]->numSurfaces);
		LL(mod->md3[lod]->ofsFrames);
		LL(mod->md3[lod]->ofsTags);
		LL(mod->md3[lod]->ofsSurfaces);
		LL(mod->md3[lod]->ofsEnd);
	}

	if ( mod->md3[lod]->numFrames < 1 ) {
		ri.Printf( PRINT_WARNING, "R_LoadMD3: %s has no frames\n", mod_name );
		return qfalse;
	}

	if (bAlreadyFound)
	{
		return qtrue;	// All done. Stop, go no further, do not pass Go...
	}

#ifndef _M_IX86
	//
	// optimisation, we don't bother doing this for standard intel case since our data's already in that format...
	//

	// swap all the frames
    frame = (md3Frame_t *) ( (byte *)mod->md3[lod] + mod->md3[lod]->ofsFrames );
    for ( i = 0 ; i < mod->md3[lod]->numFrames ; i++, frame++) {
    	frame->radius = LittleFloat( frame->radius );
        for ( j = 0 ; j < 3 ; j++ ) {
            frame->bounds[0][j] = LittleFloat( frame->bounds[0][j] );
            frame->bounds[1][j] = LittleFloat( frame->bounds[1][j] );
	    	frame->localOrigin[j] = LittleFloat( frame->localOrigin[j] );
        }
	}

	// swap all the tags
    tag = (md3Tag_t *) ( (byte *)mod->md3[lod] + mod->md3[lod]->ofsTags );
    for ( i = 0 ; i < mod->md3[lod]->numTags * mod->md3[lod]->numFrames ; i++, tag++) {
        for ( j = 0 ; j < 3 ; j++ ) {
			tag->origin[j] = LittleFloat( tag->origin[j] );
			tag->axis[0][j] = LittleFloat( tag->axis[0][j] );
			tag->axis[1][j] = LittleFloat( tag->axis[1][j] );
			tag->axis[2][j] = LittleFloat( tag->axis[2][j] );
        }
	}
#endif

	// swap all the surfaces
	surf = (md3Surface_t *) ( (byte *)mod->md3[lod] + mod->md3[lod]->ofsSurfaces );
	for ( i = 0 ; i < mod->md3[lod]->numSurfaces ; i++) {
        LL(surf->flags);
        LL(surf->numFrames);
        LL(surf->numShaders);
        LL(surf->numTriangles);
        LL(surf->ofsTriangles);
        LL(surf->numVerts);
        LL(surf->ofsShaders);
        LL(surf->ofsSt);
        LL(surf->ofsXyzNormals);
        LL(surf->ofsEnd);
		
		if ( surf->numVerts > SHADER_MAX_VERTEXES ) {
			ri.Error (ERR_DROP, "R_LoadMD3: %s has more than %i verts on a surface (%i)",
				mod_name, SHADER_MAX_VERTEXES, surf->numVerts );
		}
		if ( surf->numTriangles*3 > SHADER_MAX_INDEXES ) {
			ri.Error (ERR_DROP, "R_LoadMD3: %s has more than %i triangles on a surface (%i)",
				mod_name, SHADER_MAX_INDEXES / 3, surf->numTriangles );
		}
	
		// change to surface identifier
		surf->ident = SF_MD3;

		// lowercase the surface name so skin compares are faster
		Q_strlwr( surf->name );

		// strip off a trailing _1 or _2
		// this is a crutch for q3data being a mess
		j = strlen( surf->name );
		if ( j > 2 && surf->name[j-2] == '_' ) {
			surf->name[j-2] = 0;
		}

        // register the shaders
        shader = (md3Shader_t *) ( (byte *)surf + surf->ofsShaders );
        for ( j = 0 ; j < surf->numShaders ; j++, shader++ ) {
            shader_t	*sh;

            sh = R_FindShader( shader->name, lightmapsNone, stylesDefault, qtrue );
			if ( sh->defaultShader ) {
				shader->shaderIndex = 0;
			} else {
				shader->shaderIndex = sh->index;
			}
			RE_RegisterModels_StoreShaderRequest(mod_name, &shader->name[0], &shader->shaderIndex);
        }


#ifndef _M_IX86
//
// optimisation, we don't bother doing this for standard intel case since our data's already in that format...
//

		// swap all the triangles
		tri = (md3Triangle_t *) ( (byte *)surf + surf->ofsTriangles );
		for ( j = 0 ; j < surf->numTriangles ; j++, tri++ ) {
			LL(tri->indexes[0]);
			LL(tri->indexes[1]);
			LL(tri->indexes[2]);
		}

		// swap all the ST
        st = (md3St_t *) ( (byte *)surf + surf->ofsSt );
        for ( j = 0 ; j < surf->numVerts ; j++, st++ ) {
            st->st[0] = LittleFloat( st->st[0] );
            st->st[1] = LittleFloat( st->st[1] );
        }

		// swap all the XyzNormals
        xyz = (md3XyzNormal_t *) ( (byte *)surf + surf->ofsXyzNormals );
        for ( j = 0 ; j < surf->numVerts * surf->numFrames ; j++, xyz++ ) 
		{
            xyz->xyz[0] = LittleShort( xyz->xyz[0] );
            xyz->xyz[1] = LittleShort( xyz->xyz[1] );
            xyz->xyz[2] = LittleShort( xyz->xyz[2] );

            xyz->normal = LittleShort( xyz->normal );
        }
#endif

		// find the next surface
		surf = (md3Surface_t *)( (byte *)surf + surf->ofsEnd );
	}
    
	return qtrue;
}

/*
=================
R_LoadMD4
=================
*/
static qboolean R_LoadMD4( model_t *mod, void *buffer, const char *mod_name, qboolean bAlreadyCached ) {
	int					i,l;
	md4Header_t			*pinmodel, *md4;
	md4LOD_t			*lod;
	md4Surface_t		*surf;
	int					version;
	int					size;
	shader_t			*sh;

#ifndef _M_IX86
	int					j, k;
	int					frameSize;
	md4Tag_t			*tag;
	md4Triangle_t		*tri;
	md4Vertex_t			*v;
	md4Frame_t			*frame;
	md4CompFrame_t		*cframe;
#endif 

	pinmodel = (md4Header_t *)buffer;
	//
	// read some fields from the binary, but only LittleLong() them when we know this wasn't an already-cached model...
	//	
	version = (pinmodel->version);
	size	= (pinmodel->ofsEnd);

	if (!bAlreadyCached)
	{
		version = LittleLong(version);
		size	= LittleLong(size);
	}

	if (version != MD4_VERSION) {
		ri.Printf( PRINT_WARNING, "R_LoadMD4: %s has wrong version (%i should be %i)\n",
				 mod_name, version, MD4_VERSION);
		return qfalse;
	}

	mod->type	   = MOD_MD4;	
	mod->dataSize += size;

	qboolean bAlreadyFound = qfalse;
	md4 = mod->md4 = (md4Header_t*) //ri.Hunk_Alloc( size );
									RE_RegisterModels_Malloc(size, mod_name, &bAlreadyFound, TAG_MODEL_MD4);

	assert(bAlreadyCached == bAlreadyFound);	// I should probably eliminate 'bAlreadyFound', but wtf?

	if (!bAlreadyFound)
	{
		memcpy( md4, buffer, size );

		LL(md4->ident);
		LL(md4->version);
		LL(md4->numFrames);
		LL(md4->numBones);
		LL(md4->numLODs);
		LL(md4->ofsFrames);
		LL(md4->ofsLODs);
		LL(md4->numTags);
		LL(md4->ofsTags);
		LL(md4->ofsEnd);
	}

	mod->numLods = md4->numLODs -1 ;	//copy this up to the model for ease of use - it wil get inced after this.

	if ( md4->numFrames < 1 ) {
		ri.Printf( PRINT_WARNING, "R_LoadMD4: %s has no frames\n", mod_name );
		return qfalse;
	}

	if (bAlreadyFound)
	{
		return qtrue;	// All done. Stop, go no further, do not LittleLong(), do not pass Go...
	}


#ifndef _M_IX86
	//
	// optimisation, we don't bother doing this for standard intel case since our data's already in that format...
	//
	if (md4->ofsFrames<0) // Compressed .
	{
		// swap all the frames
		frameSize = (int)( &((md4CompFrame_t *)0)->bones[ md4->numBones ] );
		for ( i = 0 ; i < md4->numFrames ; i++) {
			cframe = (md4CompFrame_t *) ( (byte *)md4 - md4->ofsFrames + i * frameSize );
    		cframe->radius = LittleFloat( cframe->radius );
			for ( j = 0 ; j < 3 ; j++ ) {
				cframe->bounds[0][j] = LittleFloat( cframe->bounds[0][j] );
				cframe->bounds[1][j] = LittleFloat( cframe->bounds[1][j] );
	    		cframe->localOrigin[j] = LittleFloat( cframe->localOrigin[j] );
			}
			for ( j = 0 ; j < md4->numBones * sizeof( md4CompBone_t ) / 2 ; j++ ) {
				((short *)cframe->bones)[j] = LittleShort( ((short *)cframe->bones)[j] );
			}
		}
	}
	else
	{
		// swap all the frames
		frameSize = (int)( &((md4Frame_t *)0)->bones[ md4->numBones ] );
		for ( i = 0 ; i < md4->numFrames ; i++) {
			frame = (md4Frame_t *) ( (byte *)md4 + md4->ofsFrames + i * frameSize );
    		frame->radius = LittleFloat( frame->radius );
			for ( j = 0 ; j < 3 ; j++ ) {
				frame->bounds[0][j] = LittleFloat( frame->bounds[0][j] );
				frame->bounds[1][j] = LittleFloat( frame->bounds[1][j] );
	    		frame->localOrigin[j] = LittleFloat( frame->localOrigin[j] );
			}
			for ( j = 0 ; j < md4->numBones * sizeof( md4Bone_t ) / 4 ; j++ ) {
				((float *)frame->bones)[j] = LittleFloat( ((float *)frame->bones)[j] );
			}
		}
	}
#endif


	// swap all the LOD's	(we need to do the middle part of this even for intel, because of shader reg and err-check)
	lod = (md4LOD_t *) ( (byte *)md4 + md4->ofsLODs );
	for ( l = 0 ; l < md4->numLODs ; l++) {
		LL(lod->ofsSurfaces);
		LL(lod->numSurfaces);
		LL(lod->ofsEnd);
		// swap all the surfaces
		surf = (md4Surface_t *) ( (byte *)lod + lod->ofsSurfaces );
		for ( i = 0 ; i < lod->numSurfaces ; i++) {
			LL(surf->numTriangles);
			LL(surf->ofsTriangles);
			LL(surf->numVerts);
			LL(surf->ofsVerts);
			LL(surf->ofsEnd);
			LL(surf->ofsHeader);
			
			if ( surf->numVerts > SHADER_MAX_VERTEXES ) {
				ri.Error (ERR_DROP, "R_LoadMD4: %s has more than %i verts on a surface (%i)",
					mod_name, SHADER_MAX_VERTEXES, surf->numVerts );
			}
			if ( surf->numTriangles*3 > SHADER_MAX_INDEXES ) {
				ri.Error (ERR_DROP, "R_LoadMD4: %s has more than %i triangles on a surface (%i)",
					mod_name, SHADER_MAX_INDEXES / 3, surf->numTriangles );
			}
		
			// change to surface identifier
			surf->ident = SF_MD4;

			// register the shaders
			sh = R_FindShader( surf->shader, lightmapsNone, stylesDefault, qtrue );
			if ( sh->defaultShader ) {
				surf->shaderIndex = 0;
			} else {
				surf->shaderIndex = sh->index;
			}
			RE_RegisterModels_StoreShaderRequest(mod_name, &surf->shader[0], &surf->shaderIndex);

#ifndef _M_IX86
//
// optimisation, we don't bother doing this for standard intel case since our data's already in that format...
//

			// swap all the triangles
			tri = (md4Triangle_t *) ( (byte *)surf + surf->ofsTriangles );
			for ( j = 0 ; j < surf->numTriangles ; j++, tri++ ) {
				LL(tri->indexes[0]);
				LL(tri->indexes[1]);
				LL(tri->indexes[2]);
			}

			// swap all the vertexes
			v = (md4Vertex_t *) ( (byte *)surf + surf->ofsVerts );
			for ( j = 0 ; j < surf->numVerts ; j++ ) {
				v->normal[0] = LittleFloat( v->normal[0] );
				v->normal[1] = LittleFloat( v->normal[1] );
				v->normal[2] = LittleFloat( v->normal[2] );

				v->texCoords[0] = LittleFloat( v->texCoords[0] );
				v->texCoords[1] = LittleFloat( v->texCoords[1] );

				v->numWeights = LittleLong( v->numWeights );

				for ( k = 0 ; k < v->numWeights ; k++ ) {
					v->weights[k].boneIndex = LittleLong( v->weights[k].boneIndex );
					v->weights[k].boneWeight = LittleFloat( v->weights[k].boneWeight );
				   v->weights[k].offset[0] = LittleFloat( v->weights[k].offset[0] );
				   v->weights[k].offset[1] = LittleFloat( v->weights[k].offset[1] );
				   v->weights[k].offset[2] = LittleFloat( v->weights[k].offset[2] );
				}
				v = (md4Vertex_t *)&v->weights[v->numWeights];
			}
#endif

			// find the next surface
			surf = (md4Surface_t *)( (byte *)surf + surf->ofsEnd );
		}

		// find the next LOD
		lod = (md4LOD_t *)( (byte *)lod + lod->ofsEnd );
	}

#ifndef _M_IX86
//
// optimisation, we don't bother doing this for standard intel case since our data's already in that format...
//
	tag = (md4Tag_t *) ( (byte *)md4 + md4->ofsTags );
	for ( i = 0 ; i < md4->numTags ; i++) {
		LL(tag->boneIndex);
		tag++;
	}
#endif

	return qtrue;
}


//=============================================================================

/*
** RE_BeginRegistration
*/
void RE_BeginRegistration( glconfig_t *glconfigOut ) {
	ri.Hunk_Clear();

	R_Init();
	*glconfigOut = glConfig;

	R_SyncRenderThread();

	tr.viewCluster = -1;		// force markleafs to regenerate
//	R_ClearFlares();
	RE_ClearScene();

	tr.registered = qtrue;
}

//=============================================================================

/*
===============
R_ModelInit
===============
*/
void R_ModelInit( void ) {
	model_t		*mod;

	// leave a space for NULL model
	tr.numModels = 0;

	mod = R_AllocModel();
	mod->type = MOD_BAD;
/*
Ghoul2 Insert Start
*/

	memset(mhHashTable, 0, sizeof(mhHashTable));
/*
Ghoul2 Insert End
*/

}


/*
================
R_Modellist_f
================
*/
void R_Modellist_f( void ) {
	int		i, j;
	model_t	*mod;
	int		total;
	int		lods;

	total = 0;
	for ( i = 1 ; i < tr.numModels; i++ ) {
		mod = tr.models[i];
		switch (mod->type)
		{
			default:
				assert(0);
				ri.Printf( PRINT_ALL, "UNKNOWN  :      %s\n", mod->name );
				break;

			case MOD_BAD:
				ri.Printf( PRINT_ALL, "MOD_BAD  :      %s\n", mod->name );
				break;

			case MOD_BRUSH:
				ri.Printf( PRINT_ALL, "%8i : (%i) %s\n", mod->dataSize, mod->numLods, mod->name );
				break;

			case MOD_MD4: 
				if (mod->md4 && mod->md4->ofsFrames<0) // Compressed .
					ri.Printf( PRINT_ALL, "%8i : (%i) %s\n",				mod->dataSize, mod->numLods, mod->name );
				else
					ri.Printf( PRINT_ALL, "%8i : (%i) %s (uncompressed)\n",	mod->dataSize, mod->numLods, mod->name );				
				break;

			case MOD_MDXA:

				ri.Printf( PRINT_ALL, "%8i : (%i) %s\n", mod->dataSize, mod->numLods, mod->name );								
				break;
		
			case MOD_MDXM:
				
				ri.Printf( PRINT_ALL, "%8i : (%i) %s\n", mod->dataSize, mod->numLods, mod->name );								
				break;

			case MOD_MESH:

				lods = 1;
				for ( j = 1 ; j < MD3_MAX_LODS ; j++ ) {
					if ( mod->md3[j] && mod->md3[j] != mod->md3[j-1] ) {
						lods++;
					}
				}				
				ri.Printf( PRINT_ALL, "%8i : (%i) %s\n",mod->dataSize, lods, mod->name );
				break;		
		}
		total += mod->dataSize;
	}
	ri.Printf( PRINT_ALL, "%8i : Total models\n", total );

/*	this doesn't work with the new hunks
	if ( tr.world ) {
		ri.Printf( PRINT_ALL, "%8i : %s\n", tr.world->dataSize, tr.world->name );
	} */
}

//=============================================================================


/*
================
R_GetTag for MD3s
================
*/
static md3Tag_t *R_GetTag( md3Header_t *mod, int frame, const char *tagName ) {
	md3Tag_t		*tag;
	int				i;

	if ( frame >= mod->numFrames ) {
		// it is possible to have a bad frame while changing models, so don't error
		frame = mod->numFrames - 1;
	}

	tag = (md3Tag_t *)((byte *)mod + mod->ofsTags) + frame * mod->numTags;
	for ( i = 0 ; i < mod->numTags ; i++, tag++ ) {
		if ( !strcmp( tag->name, tagName ) ) {
			return tag;	// found it
		}
	}

	return NULL;
}

/*
================
R_GetAnimTag for MD4s or MDRs...
================
*/
static void R_GetAnimTag( md4Header_t *mod, int framenum, const char *tagName ,md3Tag_t * dest) {
	int				i;
	int				frameSize;
	md4Frame_t		*frame;
	md4CompFrame_t	*cframe;
	md4Tag_t		*tag;
	md4Bone_t		tbone;

	if ( framenum >= mod->numFrames ) {
		// it is possible to have a bad frame while changing models, so don't error
		framenum = mod->numFrames - 1;
	}

	tag = (md4Tag_t *)((byte *)mod + mod->ofsTags);
	for ( i = 0 ; i < mod->numTags ; i++, tag++ ) {
		if ( !strcmp( tag->name, tagName ) ) 
		{
			if (mod->ofsFrames<0)	//compressed model
			{
				frameSize = (int)( &((md4CompFrame_t *)0)->bones[ mod->numBones ] );
				cframe = (md4CompFrame_t *)((byte *)mod - mod->ofsFrames + framenum * frameSize );
				MC_UnCompress(tbone.matrix,cframe->bones[tag->boneIndex].Comp);
				{
					int j,k;
					for (j=0;j<3;j++)
					{
						for (k=0;k<3;k++)
							dest->axis[j][k]=tbone.matrix[k][j];
					}
				}
				dest->origin[0]=tbone.matrix[0][3];
				dest->origin[1]=tbone.matrix[1][3];
				dest->origin[2]=tbone.matrix[2][3];
			}
			else
			{
				frameSize = (int)( &((md4Frame_t *)0)->bones[ mod->numBones ] );
				frame = (md4Frame_t *)((byte *)mod + mod->ofsFrames + framenum * frameSize );
				{
					int j,k;
					for (j=0;j<3;j++)
					{
						for (k=0;k<3;k++)
							dest->axis[j][k]=frame->bones[tag->boneIndex].matrix[k][j];
					}
				}
				dest->origin[0]=frame->bones[tag->boneIndex].matrix[0][3];
				dest->origin[1]=frame->bones[tag->boneIndex].matrix[1][3];
				dest->origin[2]=frame->bones[tag->boneIndex].matrix[2][3];
			}
			return;
		}
	}
	AxisClear( dest->axis );
	VectorClear( dest->origin );
}

/*
================
R_LerpTag
================
*/
void	R_LerpTag( orientation_t *tag, qhandle_t handle, int startFrame, int endFrame, 
					 float frac, const char *tagName ) {
	md3Tag_t	*start, *finish;
	int		i;
	float		frontLerp, backLerp;
	model_t		*model;

	model = R_GetModelByHandle( handle );
	if ( model->md3[0] ) 
	{
		start = R_GetTag( model->md3[0], startFrame, tagName );
		finish = R_GetTag( model->md3[0], endFrame, tagName );
	}
	else if ( model->md4 ) 
	{
		md3Tag_t	tstart, tfinish;
		start = &tstart;
		R_GetAnimTag( model->md4, startFrame, tagName, start );
		finish = &tfinish;
		R_GetAnimTag( model->md4, endFrame, tagName, finish );
	}
	else
	{
		AxisClear( tag->axis );
		VectorClear( tag->origin );
		return;
	}

	if ( !start || !finish ) {
		AxisClear( tag->axis );
		VectorClear( tag->origin );
		return;
	}

	frontLerp = frac;
	backLerp = 1.0 - frac;

	for ( i = 0 ; i < 3 ; i++ ) {
		tag->origin[i] = start->origin[i] * backLerp +  finish->origin[i] * frontLerp;
		tag->axis[0][i] = start->axis[0][i] * backLerp +  finish->axis[0][i] * frontLerp;
		tag->axis[1][i] = start->axis[1][i] * backLerp +  finish->axis[1][i] * frontLerp;
		tag->axis[2][i] = start->axis[2][i] * backLerp +  finish->axis[2][i] * frontLerp;
	}
	VectorNormalize( tag->axis[0] );
	VectorNormalize( tag->axis[1] );
	VectorNormalize( tag->axis[2] );
}


/*
====================
R_ModelBounds
====================
*/
void R_ModelBounds( qhandle_t handle, vec3_t mins, vec3_t maxs ) {
	model_t		*model;

	model = R_GetModelByHandle( handle );

	if ( model->bmodel ) {
		VectorCopy( model->bmodel->bounds[0], mins );
		VectorCopy( model->bmodel->bounds[1], maxs );
		return;
	}

	if ( model->md3[0] ) {
		md3Header_t	*header;
		md3Frame_t	*frame;
		header = model->md3[0];

		frame = (md3Frame_t *)( (byte *)header + header->ofsFrames );

		VectorCopy( frame->bounds[0], mins );
		VectorCopy( frame->bounds[1], maxs );
	}
	else if ( model->md4 ) {
		md4Frame_t		*frame;
		if (model->md4->ofsFrames<0)
			// Hack, this point to an MD4CompFrame, but works here.
			frame = (md4Frame_t *)((byte *)model->md4 - model->md4->ofsFrames);
		else
			frame = (md4Frame_t *)((byte *)model->md4 + model->md4->ofsFrames);
		VectorCopy( frame->bounds[0], mins );
		VectorCopy( frame->bounds[1], maxs );		
	}
	else
	{
		VectorClear( mins );
		VectorClear( maxs );
		return;
	}
}
