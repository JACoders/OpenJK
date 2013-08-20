// Filename:-	sv_savegame.cpp
//
// leave this as first line for PCH reasons...
//
#include "../server/exe_headers.h"

// a little naughty, since these are in the renderer, but I need access to them for savegames, so...
//
extern void Decompress_JPG( const char *filename, byte *pJPGData, unsigned char **pic, int *width, int *height );
extern byte *Compress_JPG(int *pOutputSize, int quality, int image_width, int image_height, byte *image_buffer, qboolean bInvertDuringCompression);

#define JPEG_IMAGE_QUALITY 95


//#define USE_LAST_SAVE_FROM_THIS_MAP	// enable this if you want to use the last explicity-loaded savegame from this map
				 						//	when respawning after dying, else it'll just load "auto" regardless 
										//	(EF1 behaviour). I should maybe time/date check them though?

#include "server.h"
#include "../game/statindex.h"
#include "../game/weapons.h"
#include "../game/g_items.h"

#pragma warning(disable : 4786)  // identifier was truncated (STL crap)
#pragma warning(disable : 4710)  // function was not inlined (STL crap)
#pragma warning(disable : 4512)  // yet more STL drivel...

#include <map>

using namespace std;

static char	saveGameComment[iSG_COMMENT_SIZE];

//#define SG_PROFILE	// enable for debug save stats if you want

#ifdef _DEBUG
#include <windows.h>
#define DEBUGOUT(blah) OutputDebugString(blah);
#else
#define DEBUGOUT(blah)
#endif

int giSaveGameVersion;	// filled in when a savegame file is opened
fileHandle_t fhSaveGame = 0;
SavedGameJustLoaded_e eSavedGameJustLoaded = eNO;
qboolean qbSGReadIsTestOnly = qfalse;	// this MUST be left in this state
char sLastSaveFileLoaded[MAX_QPATH]={0};

#define SG_MAGIC 0x1234abcd
//#define iSG_COMMENT_SIZE 64
#define iSG_MAPCMD_SIZE MAX_TOKEN_CHARS

#ifndef LPCSTR
typedef const char * LPCSTR;
#endif


static char *SG_GetSaveGameMapName(const char *psPathlessBaseName);
static void CompressMem_FreeScratchBuffer(void);


#ifdef SG_PROFILE

class CChid
{
private:
	int		m_iCount;
	int		m_iSize;
public:
	CChid()
	{
		m_iCount = 0;
		m_iSize = 0;
	}
	void Add(int iLength)
	{
		m_iCount++;
		m_iSize += iLength;
	}
	int GetCount()
	{
		return m_iCount;
	}
	int GetSize()
	{
		return m_iSize;
	}
};

typedef map<unsigned long, CChid> CChidInfo_t;
CChidInfo_t	save_info;
#endif

LPCSTR SG_GetChidText(unsigned long chid)
{
	static char	chidtext[5];

	*(unsigned long *)chidtext = BigLong(chid);
	chidtext[4] = 0;

	return chidtext;
}


static const char *GetString_FailedToOpenSaveGame(const char *psFilename, qboolean bOpen)
{
	static char sTemp[256];

	strcpy(sTemp,S_COLOR_RED);
	
	const char *psReference = bOpen ? "MENUS3_FAILED_TO_OPEN_SAVEGAME" : "MENUS3_FAILED_TO_CREATE_SAVEGAME";
	Q_strncpyz(sTemp + strlen(sTemp), va( SP_GetStringTextString(psReference), psFilename),sizeof(sTemp));
	strcat(sTemp,"\n");
	return sTemp;
}

// (copes with up to 8 ptr returns at once)
//
static LPCSTR SG_AddSavePath( LPCSTR psPathlessBaseName )
{
	static char sSaveName[8][MAX_OSPATH]; 
	static int  i=0;

	i=++i&7;

	Com_sprintf( sSaveName[i], MAX_OSPATH, "saves/%s.sav", psPathlessBaseName );
	return sSaveName[i];
}

void SG_WipeSavegame( LPCSTR psPathlessBaseName )
{
	LPCSTR psLocalFilename  = SG_AddSavePath( psPathlessBaseName );
	
	FS_DeleteUserGenFile( psLocalFilename );
}

static qboolean SG_Copy( LPCSTR psPathlessBaseName_Src, LPCSTR psPathlessBaseName_Dst )
{
	LPCSTR psLocalFilename_Src = SG_AddSavePath( psPathlessBaseName_Src );
	LPCSTR psLocalFilename_Dst = SG_AddSavePath( psPathlessBaseName_Dst );

	qboolean qbCopyWentOk = FS_CopyUserGenFile( psLocalFilename_Src, psLocalFilename_Dst );

	if (!qbCopyWentOk)
	{
		Com_Printf(S_COLOR_RED "Error during savegame-rename. Check \"%s\" for write-protect or disk full!\n", psLocalFilename_Dst );
		return qfalse;
	}

	return qtrue;
}

qboolean gbSGWriteFailed = qfalse;
static qboolean SG_Create( LPCSTR psPathlessBaseName )
{
	gbSGWriteFailed = qfalse;
	SG_WipeSavegame( psPathlessBaseName );

	LPCSTR psLocalFilename = SG_AddSavePath( psPathlessBaseName );		

	fhSaveGame = FS_FOpenFileWrite( psLocalFilename );

	if(!fhSaveGame)
	{
		Com_Printf(GetString_FailedToOpenSaveGame(psLocalFilename,qfalse));//S_COLOR_RED "Failed to create new savegame file \"%s\"\n", psLocalFilename );
		return qfalse;
	}

#ifdef SG_PROFILE
	assert( save_info.empty() );
#endif

	giSaveGameVersion = iSAVEGAME_VERSION;
	SG_Append('_VER', &giSaveGameVersion, sizeof(giSaveGameVersion));

	return qtrue;
}

// called from the ERR_DROP stuff just in case the error occured during loading of a saved game, because if
//	we didn't do this then we'd run out of quake file handles after the 8th load fail...
//
void SG_Shutdown()
{
	if (fhSaveGame )
	{
		FS_FCloseFile( fhSaveGame );
		fhSaveGame = NULL;
	}

	eSavedGameJustLoaded = eNO;	// important to do this if we ERR_DROP during loading, else next map you load after
								//	a bad save-file you'll arrive at dead :-)

	// and this bit stops people messing up the laoder by repeatedly stabbing at the load key during loads...
	//
	extern qboolean gbAlreadyDoingLoad;
					gbAlreadyDoingLoad = qfalse;
}

qboolean SG_Close()
{
	assert( fhSaveGame );	
	FS_FCloseFile( fhSaveGame );
	fhSaveGame = NULL;

#ifdef SG_PROFILE
	if (!sv_testsave->value)
	{
		CChidInfo_t::iterator it;
		int iCount = 0, iSize = 0;

		Com_DPrintf(S_COLOR_CYAN "================================\n");
		Com_DPrintf(S_COLOR_WHITE "CHID   Count      Size\n\n");
		for(it = save_info.begin(); it != save_info.end(); ++it)
		{
			Com_DPrintf("%s   %5d  %8d\n", SG_GetChidText((*it).first), (*it).second.GetCount(), (*it).second.GetSize());
			iCount += (*it).second.GetCount();
			iSize  += (*it).second.GetSize();
		}
		Com_DPrintf("\n" S_COLOR_WHITE "%d chunks making %d bytes\n", iCount, iSize);
		Com_DPrintf(S_COLOR_CYAN "================================\n");
		save_info.clear();
	}
#endif

	CompressMem_FreeScratchBuffer();
	return qtrue;
}


qboolean SG_Open( LPCSTR psPathlessBaseName )
{	
//	if ( fhSaveGame )		// hmmm...
//	{						//
//		SG_Close();			//
//	}						//
	assert( !fhSaveGame);	// I'd rather know about this

	LPCSTR psLocalFilename = SG_AddSavePath( psPathlessBaseName );	

	FS_FOpenFileRead( psLocalFilename, &fhSaveGame, qtrue );	//qtrue = dup handle, so I can close it ok later
	
	if (!fhSaveGame)
	{
//		Com_Printf(S_COLOR_RED "Failed to open savegame file %s\n", psLocalFilename);
		Com_DPrintf(GetString_FailedToOpenSaveGame(psLocalFilename, qtrue));

		return qfalse;
	}
		
	SG_Read('_VER', &giSaveGameVersion, sizeof(giSaveGameVersion));
	if (giSaveGameVersion != iSAVEGAME_VERSION)
	{
		SG_Close();
		Com_Printf (S_COLOR_RED "File \"%s\" has version # %d (expecting %d)\n",giSaveGameVersion, iSAVEGAME_VERSION);
		return qfalse;
	}

	return qtrue;
}

// you should only call this when you know you've successfully opened a savegame, and you want to query for
//	whether it's an old (street-copy) version, or a new (expansion-pack) version
//
int SG_Version(void)
{
	return giSaveGameVersion;
}

void SV_WipeGame_f(void)
{
	if (Cmd_Argc() != 2)
	{
		Com_Printf (S_COLOR_RED "USAGE: wipe <name>\n");
		return;
	}
	if (!stricmp (Cmd_Argv(1), "auto") )
	{
		Com_Printf (S_COLOR_RED "Can't wipe 'auto'\n");
		return;
	}
	SG_WipeSavegame(Cmd_Argv(1));
//	Com_Printf("%s has been wiped\n", Cmd_Argv(1));	// wurde gel�scht in german, but we've only got one string
	Com_Printf("Ok\n");
}

/*
// Store given string in saveGameComment for later use when game is 
// actually saved
*/
void SG_StoreSaveGameComment(const char *sComment)
{	
	memmove(saveGameComment,sComment,iSG_COMMENT_SIZE);
}

qboolean SV_TryLoadTransition( const char *mapname )
{
	char *psFilename = va( "hub/%s", mapname );

	Com_Printf (S_COLOR_CYAN "Restoring game \"%s\"...\n", psFilename);

	if ( !SG_ReadSavegame( psFilename ) )
	{//couldn't load a savegame
		return qfalse;
	}
	Com_Printf (S_COLOR_CYAN "Done.\n");

	return qtrue;
}

qboolean gbAlreadyDoingLoad = qfalse;
void SV_LoadGame_f(void)
{
	if (gbAlreadyDoingLoad)
	{
		Com_DPrintf ("( Already loading, ignoring extra 'load' commands... )\n");
		return;
	}

//	// check server is running
//	//
//	if ( !com_sv_running->integer )
//	{
//		Com_Printf( "Server is not running\n" );
//		return;
//	}

	if (Cmd_Argc() != 2)
	{
		Com_Printf ("USAGE: load <filename>\n");
		return;
	}

	const char *psFilename = Cmd_Argv(1);
	if (strstr (psFilename, "..") || strstr (psFilename, "/") || strstr (psFilename, "\\") )
	{
		Com_Printf (S_COLOR_RED "Bad loadgame name.\n");
		return;
	}

	if (!stricmp (psFilename, "current"))
	{
		Com_Printf (S_COLOR_RED "Can't load from \"current\"\n");
		return;
	}

	// special case, if doing a respawn then check that the available auto-save (if any) is from the same map
	//	as we're currently on (if in a map at all), if so, load that "auto", else re-load the last-loaded file...
	//
	if (!stricmp(psFilename, "*respawn"))
	{
		psFilename = "auto";	// default to standard respawn behaviour

		// see if there's a last-loaded file to even check against as regards loading...
		//
		if ( sLastSaveFileLoaded[0] )
		{
			LPCSTR psServerInfo = sv.configstrings[CS_SERVERINFO];
			LPCSTR psMapName    = Info_ValueForKey( psServerInfo, "mapname" );

			char *psMapNameOfAutoSave = SG_GetSaveGameMapName("auto");

			if ( !Q_stricmp(psMapName,"_brig") )
			{//if you're in the brig and there is no autosave, load the last loaded savegame
				if ( !psMapNameOfAutoSave )
				{
					psFilename = sLastSaveFileLoaded;
				}				
			}
			else
			{
#ifdef USE_LAST_SAVE_FROM_THIS_MAP
				// if the map name within the name of the last save file we explicitly loaded is the same
				//	as the current map, then use that...
				//
				char *psMapNameOfLastSaveFileLoaded = SG_GetSaveGameMapName(sLastSaveFileLoaded);

				if (!Q_stricmp(psMapName,psMapNameOfLastSaveFileLoaded)))
				{
					psFilename = sLastSaveFileLoaded;
				}							
				else 
#endif
				if (!(psMapName && psMapNameOfAutoSave && !Q_stricmp(psMapName,psMapNameOfAutoSave)))
				{
					// either there's no auto file, or it's from a different map to the one we've just died on...
					//
					psFilename = sLastSaveFileLoaded;
				}
			}
		}
		//default will continue to load auto
	}
		
	Com_Printf (S_COLOR_CYAN "Loading game \"%s\"...\n", psFilename);

	gbAlreadyDoingLoad = qtrue;
	if (!SG_ReadSavegame(psFilename)) {
		gbAlreadyDoingLoad = qfalse; //	do NOT do this here now, need to wait until client spawn, unless the load failed.
	} else
	{
		Com_Printf (S_COLOR_CYAN "Done.\n");	
	}
}

qboolean SG_GameAllowedToSaveHere(qboolean inCamera);

void SV_SaveGame_f(void)
{
	// check server is running
	//
	if ( !com_sv_running->integer )
	{
		Com_Printf( S_COLOR_RED "Server is not running\n" );
		return;
	}

	if (sv.state != SS_GAME)
	{
		Com_Printf (S_COLOR_RED "You must be in a game to save.\n");
		return;
	}

	// check args...
	//
	if ( Cmd_Argc() != 2 ) 
	{
		Com_Printf( "USAGE: \"save <filename>\"\n" );
		return;
	}


	if (svs.clients[0].frames[svs.clients[0].netchan.outgoingSequence & PACKET_MASK].ps.stats[STAT_HEALTH] <= 0)
	{
		Com_Printf (S_COLOR_RED "\nCan't savegame while dead!\n");
		return;
	}

	//this check catches deaths even the instant you die, like during a slo-mo death!
	gentity_t			*svent;
	svent = SV_GentityNum(0);
	if (svent->client->stats[STAT_HEALTH]<=0)
	{
		Com_Printf (S_COLOR_RED "\nCan't savegame while dead!\n");
		return;
	}

	char *psFilename = Cmd_Argv(1);

	if (!stricmp (psFilename, "current"))
	{
		Com_Printf (S_COLOR_RED "Can't save to 'current'\n");
		return;
	}

	if (strstr (psFilename, "..") || strstr (psFilename, "/") || strstr (psFilename, "\\") )
	{
		Com_Printf (S_COLOR_RED "Bad savegame name.\n");
		return;
	}

	if (!SG_GameAllowedToSaveHere(qfalse))	//full check
		return;	// this prevents people saving via quick-save now during cinematics, and skips the screenshot below!

	if (!stricmp (psFilename, "quik*") || !stricmp (psFilename, "auto*") )
	{
extern void	SCR_PrecacheScreenshot();  //scr_scrn.cpp
		SCR_PrecacheScreenshot();
		if (psFilename[4]=='*'){
			psFilename[4]=0;	//remove the *
		}
		SG_StoreSaveGameComment("");	// clear previous comment/description, which will force time/date comment.
	}

	Com_Printf (S_COLOR_CYAN "Saving game \"%s\"...\n", psFilename);
	if (SG_WriteSavegame(psFilename, qfalse))
	{
		Com_Printf (S_COLOR_CYAN "Done.\n");
	}
	else
	{
		Com_Printf (S_COLOR_RED "Failed.\n");
	}
}



//---------------
static void WriteGame(qboolean autosave)
{
	SG_Append('GAME', &autosave, sizeof(autosave));	

	if (autosave)
	{
		// write out player ammo level, health, etc...
		//
		extern void SV_Player_EndOfLevelSave(void);
		SV_Player_EndOfLevelSave();	// this sets up the various cvars needed, so we can then write them to disk
		//
		char s[MAX_STRING_CHARS];
		
		// write health/armour etc...
		//
		memset(s,0,sizeof(s));
		Cvar_VariableStringBuffer( sCVARNAME_PLAYERSAVE, s, sizeof(s) );
		SG_Append('CVSV', &s, sizeof(s));	

		// write ammo...
		//
		memset(s,0,sizeof(s));
		Cvar_VariableStringBuffer( "playerammo", s, sizeof(s) );
		SG_Append('AMMO', &s, sizeof(s));

		// write inventory...
		//
		memset(s,0,sizeof(s));
		Cvar_VariableStringBuffer( "playerinv", s, sizeof(s) );
		SG_Append('IVTY', &s, sizeof(s));
		
		// the new JK2 stuff - force powers, etc...
		//
		memset(s,0,sizeof(s));
		Cvar_VariableStringBuffer( "playerfplvl", s, sizeof(s) );
		SG_Append('FPLV', &s, sizeof(s));
	}
}

static qboolean ReadGame (void)
{
	qboolean qbAutoSave;
	SG_Read('GAME', (void *)&qbAutoSave, sizeof(qbAutoSave));

	if (qbAutoSave)
	{
		char s[MAX_STRING_CHARS]={0};

		// read health/armour etc...
		//
		memset(s,0,sizeof(s));
		SG_Read('CVSV', (void *)&s, sizeof(s));
		Cvar_Set( sCVARNAME_PLAYERSAVE, s );

		// read ammo...
		//
		memset(s,0,sizeof(s));			
		SG_Read('AMMO', (void *)&s, sizeof(s));
		Cvar_Set( "playerammo", s);

		// read inventory...
		//
		memset(s,0,sizeof(s));			
		SG_Read('IVTY', (void *)&s, sizeof(s));
		Cvar_Set( "playerinv", s);

		// read force powers...
		//
		memset(s,0,sizeof(s));
		SG_Read('FPLV', (void *)&s, sizeof(s));
		Cvar_Set( "playerfplvl", s );
	}

	return qbAutoSave;
}

//---------------


// write all CVAR_USERINFO cvars
// these will be things like model, name, ...
//
extern  cvar_t	*cvar_vars;	// I know this is really unpleasant, but I need access for scanning/writing latched cvars during save games

void SG_WriteCvars(void)
{
	cvar_t	*var;
	int		iCount = 0;

	// count the latched cvars...
	//	
	for (var = cvar_vars; var; var = var->next)
	{
		if (!(var->flags & CVAR_USERINFO))
		{
			continue;
		}
		iCount++;
	}

	// store count...
	//
	SG_Append('CVCN', &iCount, sizeof(iCount));

	// write 'em...
	//
	for (var = cvar_vars; var; var = var->next)
	{
		if (!(var->flags & CVAR_USERINFO))
		{
			continue;
		}
		SG_Append('CVAR', var->name,   strlen(var->name) + 1);
		SG_Append('VALU', var->string, strlen(var->string) + 1);
	}
}

void SG_ReadCvars(void)
{
	int		iCount;
	char	*psName;
	char	*psValue;

	SG_Read('CVCN', &iCount, sizeof(iCount));

	for (int i = 0; i < iCount; i++)
	{
		SG_Read('CVAR', NULL, 0, (void **)&psName);
		SG_Read('VALU', NULL, 0, (void **)&psValue);

		Cvar_Set (psName, psValue);

		Z_Free( psName );
		Z_Free( psValue );
	}
}

void SG_WriteServerConfigStrings( void )
{
	int iCount = 0;
	int i;	// not in FOR statement in case compiler goes weird by reg-optimising it then failing to get the address later

	// count how many non-blank server strings there are...
	//
	for ( i=0; i<MAX_CONFIGSTRINGS; i++)
	{
		if (i!=CS_SYSTEMINFO)
		{
			if (sv.configstrings[i] && strlen(sv.configstrings[i]))		// paranoia... <g>
			{
				iCount++;
			}
		}
	}

	SG_Append('CSCN', &iCount, sizeof(iCount));

	// now write 'em...
	//
	for (i=0; i<MAX_CONFIGSTRINGS; i++)
	{
		if (i!=CS_SYSTEMINFO)
		{
			if (sv.configstrings[i]	&& strlen(sv.configstrings[i]))
			{
				SG_Append('CSIN', &i, sizeof(i));
				SG_Append('CSDA', sv.configstrings[i], strlen(sv.configstrings[i])+1);
			}
		}
	}	
}

void SG_ReadServerConfigStrings( void )
{
	// trash the whole table...
	//
	for (int i=0; i<MAX_CONFIGSTRINGS; i++)
	{
		if (i!=CS_SYSTEMINFO)
		{
			if ( sv.configstrings[i] ) 
			{
				Z_Free( sv.configstrings[i] );			
			}
			sv.configstrings[i] = CopyString("");
		}
	}

	// now read the replacement ones...
	//
	int iCount;		

	SG_Read('CSCN', &iCount, sizeof(iCount));

	Com_DPrintf( "Reading %d configstrings...\n",iCount);

	for (i=0; i<iCount; i++)
	{
		int iIndex;
		char *psName;

		SG_Read('CSIN', &iIndex, sizeof(iIndex));
		SG_Read('CSDA', NULL, 0, (void **)&psName);

		Com_DPrintf( "Cfg str %d = %s\n",iIndex, psName);

		//sv.configstrings[iIndex] = psName;
		SV_SetConfigstring(iIndex, psName);
		Z_Free(psName);
	}
}



static void SG_WriteComment(qboolean qbAutosave, LPCSTR psMapName)
{
	char	sComment[iSG_COMMENT_SIZE];

	if ( !qbAutosave )
	{
		if (!*saveGameComment)
		{
			Com_sprintf( sComment, sizeof(sComment), "---> %s", psMapName );
		}
		else
		{
			strcpy(sComment,saveGameComment);
		}
	}
	else
	{	
		Com_sprintf( sComment, sizeof(sComment), "---> %s", psMapName );
	}

	SG_Append('COMM', sComment, sizeof(sComment));

	// Add Date/Time/Map stamp
	time_t now;
	time(&now);
	SG_Append('CMTM', &now, sizeof(time_t));

	Com_DPrintf("Saving: current (%s)\n", sComment);
}


// Test to see if the given file name is in the save game directory 
// then grab the comment if it's there
//
int SG_GetSaveGameComment(const char *psPathlessBaseName, char *sComment, char *sMapName)
{
	int ret = 0;
	time_t tFileTime;
	int iScreenShotLength;
	
	if ( !SG_Open( psPathlessBaseName ))
	{
		return 0;
	}							

	qbSGReadIsTestOnly = qtrue;	// do NOT leave this in this state

	if (SG_Read( 'COMM', sComment, iSG_COMMENT_SIZE ))
	{	
		if (SG_Read( 'CMTM', &tFileTime, sizeof( time_t )))	//read
		{	
			if (SG_Read('SHLN', &iScreenShotLength, sizeof(iScreenShotLength)))	// read
			{
				if (SG_Read('SHOT', NULL, iScreenShotLength, NULL))	// skip
				{
					if (SG_Read('MPCM', sMapName, iSG_MAPCMD_SIZE ))	// read
					{
						ret = tFileTime;
					}
				}
			}
		}
	}
	qbSGReadIsTestOnly = qfalse;

	if (!SG_Close())
	{
		return 0;
	}			
	return ret;
}


// read the mapname field from the supplied savegame file
//
// returns NULL if not found
//
static char *SG_GetSaveGameMapName(const char *psPathlessBaseName)
{
	static char sMapName[iSG_MAPCMD_SIZE]={0};
	char *psReturn = NULL;	

	if (SG_GetSaveGameComment(psPathlessBaseName, NULL, sMapName))
	{
		psReturn = sMapName;
	}

	return psReturn;
}


// pass in qtrue to set as loading screen, else pass in pvDest to read it into there...
//
static qboolean SG_ReadScreenshot(qboolean qbSetAsLoadingScreen, void *pvDest = NULL);
static qboolean SG_ReadScreenshot(qboolean qbSetAsLoadingScreen, void *pvDest/*=NULL*/)
{
	qboolean bReturn = qfalse;

	// get JPG screenshot data length...
	//
	int iScreenShotLength = 0;
	SG_Read('SHLN', &iScreenShotLength, sizeof(iScreenShotLength));
	//
	// alloc enough space plus extra 4K for sloppy JPG-decode reader to not do memory access violation...
	//
	byte *pJPGData = (byte *) Z_Malloc(iScreenShotLength + 4096,TAG_TEMP_SAVEGAME_WORKSPACE, qfalse);
	//
	// now read the JPG data...
	//
	SG_Read('SHOT', pJPGData, iScreenShotLength/*SG_SCR_WIDTH * SG_SCR_HEIGHT * 4*/, 0);	
	//
	// decompress JPG data...
	//
	byte *pDecompressedPic = NULL;
	int iWidth, iHeight;
	Decompress_JPG( "[savegame]", pJPGData, &pDecompressedPic, &iWidth, &iHeight );
	//
	// if the loaded image is the same size as the game is expecting, then copy it to supplied arg (if present)...
	//
	if (iWidth == SG_SCR_WIDTH && iHeight == SG_SCR_HEIGHT)
	{			
		bReturn = qtrue;
		
		if (pvDest)
		{
			memcpy(pvDest, pDecompressedPic, SG_SCR_WIDTH * SG_SCR_HEIGHT * 4);
		}		

		if (qbSetAsLoadingScreen)
		{
			SCR_SetScreenshot((byte *)pDecompressedPic, SG_SCR_WIDTH, SG_SCR_HEIGHT);
		}
	}

	Z_Free( pJPGData );
	Z_Free( pDecompressedPic );

	return bReturn;
}

// Gets the savegame screenshot
//
qboolean SG_GetSaveImage( const char *psPathlessBaseName, void *pvAddress )
{
	if (!SG_Open(psPathlessBaseName))
	{
		return qfalse;
	}
	
	SG_Read('COMM', NULL, 0, NULL);	// skip
	SG_Read('CMTM', NULL, sizeof( time_t ));

	qboolean bGotSaveImage = SG_ReadScreenshot(qfalse, pvAddress);

	SG_Close();
	return bGotSaveImage;
}


static void SG_WriteScreenshot(qboolean qbAutosave, LPCSTR psMapName)
{	
	byte *pbRawScreenShot = NULL;

	if( qbAutosave )
	{
		// try to read a levelshot (any valid TGA/JPG etc named the same as the map)...
		//
		int iWidth = SG_SCR_WIDTH;
		int iHeight= SG_SCR_HEIGHT;
		byte	byBlank[SG_SCR_WIDTH * SG_SCR_HEIGHT * 4] = {0};

		pbRawScreenShot = SCR_TempRawImage_ReadFromFile(va("levelshots/%s.tga",psMapName), &iWidth, &iHeight, byBlank, qtrue);	// qtrue = vert flip
	}

	if (!pbRawScreenShot)
	{
		pbRawScreenShot = SCR_GetScreenshot(0);
	}


	int iJPGDataSize = 0;
	byte *pJPGData = Compress_JPG(&iJPGDataSize, JPEG_IMAGE_QUALITY, SG_SCR_WIDTH, SG_SCR_HEIGHT, pbRawScreenShot, qfalse);
	SG_Append('SHLN', &iJPGDataSize, sizeof(iJPGDataSize));
	SG_Append('SHOT', pJPGData, iJPGDataSize);
	Z_Free(pJPGData);
	SCR_TempRawImage_CleanUp();
}

qboolean SG_GameAllowedToSaveHere(qboolean inCamera)
{
	if (!inCamera) {
		if ( !com_sv_running || !com_sv_running->integer )
		{
			return qfalse;	//		Com_Printf( S_COLOR_RED "Server is not running\n" );		
		}
		
		if (CL_IsRunningInGameCinematic())
		{
			return qfalse;	//nope, not during a video
		}

		if (sv.state != SS_GAME)
		{
			return qfalse;	//		Com_Printf (S_COLOR_RED "You must be in a game to save.\n");
		}
		
		//No savegames from "_" maps
		if ( !sv_mapname || (sv_mapname->string != NULL && sv_mapname->string[0] == '_') )
		{
			return qfalse;	//		Com_Printf (S_COLOR_RED "Cannot save on holodeck or brig.\n");
		}
		
		if (svs.clients[0].frames[svs.clients[0].netchan.outgoingSequence & PACKET_MASK].ps.stats[STAT_HEALTH] <= 0)
		{
			return qfalse;	//		Com_Printf (S_COLOR_RED "\nCan't savegame while dead!\n");
		}
	}
	if (!ge)
		return inCamera;	// only happens when called to test if inCamera

	return ge->GameAllowedToSaveHere();
}

qboolean SG_WriteSavegame(const char *psPathlessBaseName, qboolean qbAutosave)
{	
	if (!qbAutosave && !SG_GameAllowedToSaveHere(qfalse))	//full check
		return qfalse;	// this prevents people saving via quick-save now during cinematics

	float fPrevTestSave = sv_testsave->value;
	sv_testsave->value = 0;

	if(!SG_Create( "current" ))
	{
		Com_Printf (GetString_FailedToOpenSaveGame("current",qfalse));//S_COLOR_RED "Failed to create savegame\n");
		SG_WipeSavegame( "current" );
		sv_testsave->value = fPrevTestSave;
		return qfalse;
	}

	// Write out server data...
	//
	
	char   sMapCmd[iSG_MAPCMD_SIZE]={0};
	LPCSTR psServerInfo = sv.configstrings[CS_SERVERINFO];
	LPCSTR psMapName    = Info_ValueForKey( psServerInfo, "mapname" );

	strcpy( sMapCmd,psMapName);	// need as array rather than ptr because const strlen needed for MPCM chunk

	SG_WriteComment(qbAutosave, sMapCmd);
	SG_WriteScreenshot(qbAutosave, sMapCmd);
	SG_Append('MPCM', sMapCmd, sizeof(sMapCmd));
	SG_WriteCvars();

	WriteGame (qbAutosave);

	// Write out all the level data...
	//
	if (!qbAutosave)
	{
		SG_Append('TIME', (void *)&sv.time, sizeof(sv.time));
		SG_Append('TIMR', (void *)&sv.timeResidual, sizeof(sv.timeResidual));
		CM_WritePortalState();
		SG_WriteServerConfigStrings();		
	}
	ge->WriteLevel(qbAutosave);	// always done now, but ent saver only does player if auto

	SG_Close();
	if (gbSGWriteFailed)
	{
		Com_Printf (GetString_FailedToOpenSaveGame("current",qfalse));//S_COLOR_RED "Failed to write savegame!\n");
		SG_WipeSavegame( "current" );
		sv_testsave->value = fPrevTestSave;
		return qfalse;
	}

	SG_Copy( "current", psPathlessBaseName );

	sv_testsave->value = fPrevTestSave;
	return qtrue;
}

qboolean SG_ReadSavegame(const char *psPathlessBaseName)
{
	char		sComment[iSG_COMMENT_SIZE];
	char		sMapCmd [iSG_MAPCMD_SIZE];
	qboolean	qbAutosave;

	float fPrevTestSave = sv_testsave->value;
	sv_testsave->value = 0;

	Cvar_Set( "cg_missionstatusscreen", "0" );//reset if loading a game

	if (!SG_Open( psPathlessBaseName ))
	{
		Com_Printf (GetString_FailedToOpenSaveGame(psPathlessBaseName, qtrue));//S_COLOR_RED "Failed to open savegame \"%s\"\n", psPathlessBaseName);
		sv_testsave->value = fPrevTestSave;
		return qfalse;
	}

	// this check isn't really necessary, but it reminds me that these two strings may actually be the same physical one.
	//
	if (psPathlessBaseName != sLastSaveFileLoaded)
	{
		Q_strncpyz(sLastSaveFileLoaded,psPathlessBaseName,sizeof(sLastSaveFileLoaded));
	}

	// Read in all the server data...
	//
	SG_Read('COMM', sComment, sizeof(sComment));
	Com_DPrintf("Reading: %s\n", sComment);
	SG_Read( 'CMTM', NULL, sizeof( time_t ));

	SG_ReadScreenshot(qtrue);	// qboolean qbSetAsLoadingScreen
	SG_Read('MPCM', sMapCmd, sizeof(sMapCmd));
	SG_ReadCvars();

	// read game state
	qbAutosave = ReadGame();
	eSavedGameJustLoaded = (qbAutosave)?eAUTO:eFULL;

	SV_SpawnServer(sMapCmd, eForceReload_NOTHING, (eSavedGameJustLoaded != eFULL) );	// note that this also trashes the whole G_Alloc pool as well (of course)		

	// read in all the level data...
	//
	if (!qbAutosave)
	{
		SG_Read('TIME', (void *)&sv.time, sizeof(sv.time));
		SG_Read('TIMR', (void *)&sv.timeResidual, sizeof(sv.timeResidual));
		CM_ReadPortalState();
		SG_ReadServerConfigStrings();		
	}
	ge->ReadLevel(qbAutosave, qbLoadTransition);	// always done now, but ent reader only does player if auto

	if(!SG_Close())
	{
		Com_Printf (GetString_FailedToOpenSaveGame(psPathlessBaseName,qfalse));//S_COLOR_RED "Failed to close savegame\n");
		sv_testsave->value = fPrevTestSave;
		return qfalse;
	}

	sv_testsave->value = fPrevTestSave;
	return qtrue;
}


int Compress_RLE(const byte *pIn, int iLength, byte *pOut)
{
	int iCount=0,iOutIndex=0;
	
	while (iCount < iLength) 
	{
		int iIndex = iCount;
		byte b = pIn[iIndex++];

		while (iIndex<iLength && iIndex-iCount<127 && pIn[iIndex]==b)
		{
			iIndex++;
		}

		if (iIndex-iCount == 1) 
		{
			while (iIndex<iLength && iIndex-iCount<127 && (pIn[iIndex]!=pIn[iIndex-1] || iIndex>1 && pIn[iIndex]!=pIn[iIndex-2])){
				iIndex++;
			}
			while (iIndex<iLength && pIn[iIndex]==pIn[iIndex-1]){
				iIndex--;
			}
			pOut[iOutIndex++] = (unsigned char)(iCount-iIndex);
			for (int i=iCount; i<iIndex; i++){
				pOut[iOutIndex++] = pIn[i];
			}
		}
		else 
		{
			pOut[iOutIndex++] = (unsigned char)(iIndex-iCount);
			pOut[iOutIndex++] = b;
		}
		iCount=iIndex;
	}
	return iOutIndex;
}

void DeCompress_RLE(byte *pOut, const byte *pIn, int iDecompressedBytesRemaining)
{
	signed char count;
	
	while (iDecompressedBytesRemaining > 0) 
	{
		count = (signed char) *pIn++;
		if (count>0) 
		{
			memset(pOut,*pIn++,count);
		} 
		else 
		if (count<0)
		{
			count = (signed char) -count;
			memcpy(pOut,pIn,count);
			pIn += count;
		}
		pOut += count;
		iDecompressedBytesRemaining -= count;
	}
}

// simulate decompression over original data (but don't actually do it), to test de/compress validity...
//
qboolean Verify_RLE(const byte *pOut, const byte *pIn, int iDecompressedBytesRemaining)
{
	signed char count;
	const byte *pOutEnd = &pOut[iDecompressedBytesRemaining];
	
	while (iDecompressedBytesRemaining > 0) 
	{			
		if (pOut >= pOutEnd)
			return qfalse;
		count = (signed char) *pIn++;
		if (count>0) 
		{	
			//memset(pOut,*pIn++,count);
			int iMemSetByte = *pIn++;
			for (int i=0; i<count; i++)
			{
				if (pOut[i] != iMemSetByte)
					return qfalse;
			}
		} 
		else 
		if (count<0)
		{
			count = (signed char) -count;
//			memcpy(pOut,pIn,count);
			if (memcmp(pOut,pIn,count))
				return qfalse;
			pIn += count;
		}
		pOut += count;
		iDecompressedBytesRemaining -= count;
	}

	if (pOut != pOutEnd)
		return qfalse;

	return qtrue;
}


byte *gpbCompBlock = NULL;
int   giCompBlockSize = 0;
static void CompressMem_FreeScratchBuffer(void)
{
	if ( gpbCompBlock )
	{
		Z_Free(	gpbCompBlock);
				gpbCompBlock = NULL;
	}
	giCompBlockSize = 0;
}

static byte *CompressMem_AllocScratchBuffer(int iSize)
{
	// only alloc new buffer if we need more than the existing one...
	//
	if (giCompBlockSize < iSize)
	{			
		CompressMem_FreeScratchBuffer();

		gpbCompBlock = (byte *) Z_Malloc(iSize, TAG_TEMP_SAVEGAME_WORKSPACE, qfalse);
		giCompBlockSize = iSize;
	}

	return gpbCompBlock;
}

// returns -1 for compression-not-worth-it, else compressed length...
//
int CompressMem(byte *pbData, int iLength, byte *&pbOut)
{ 	
	if (!sv_compress_saved_games->integer)
		return -1;

	// malloc enough to cope with uncompressable data (it'll never grow to 2* size, so)...
	//
	pbOut = CompressMem_AllocScratchBuffer(iLength*2);	
	//
	// compress it...
	//
	int iOutputLength = Compress_RLE(pbData, iLength, pbOut);
	//
	// worth compressing?...
	//
	if (iOutputLength >= iLength)
		return -1;
	//
	// compression code works? (I'd hope this is always the case, but for safety)...
	//
	if (!Verify_RLE(pbData, pbOut, iLength))
		return -1;

	return iOutputLength;
}


qboolean SG_Append(unsigned long chid, void *pvData, int iLength)
{
	unsigned int	uiCksum;
	unsigned int	uiMagic = SG_MAGIC;
	unsigned int	uiSaved;

#ifdef _DEBUG
	int				i;
	unsigned long	*pTest;

	pTest = (unsigned long *) pvData;
	for (i=0; i<iLength/4; i++, pTest++)
	{
		if(*pTest == 0xcdcdcdcd)
		{
			assert(*pTest != 0xcdcdcdcd);
		}
		if(*pTest == 0xdddddddd)
		{
			assert(*pTest != 0xdddddddd);
		}
	}
#endif

	Com_DPrintf("Attempting write of chunk %s length %d\n", SG_GetChidText(chid), iLength);

	// only write data out if we're not in test mode....
	//
	if (!sv_testsave->value)
	{
		uiCksum = Com_BlockChecksum (pvData, iLength);

		uiSaved  = FS_Write(&chid,		sizeof(chid),		fhSaveGame);

		byte *pbCompressedData = NULL;
		int iCompressedLength = CompressMem((byte*)pvData, iLength, pbCompressedData);
		if (iCompressedLength != -1)
		{
			// compressed...  (write length field out as -ve)
			//
			iLength = -iLength;
			uiSaved += FS_Write(&iLength,			sizeof(iLength),			fhSaveGame);
			iLength = -iLength;
			//
			// CRC...
			//
			uiSaved += FS_Write(&uiCksum,			sizeof(uiCksum),			fhSaveGame);
			//
			// [compressed length]
			//
			uiSaved += FS_Write(&iCompressedLength, sizeof(iCompressedLength),	fhSaveGame);
			//
			// compressed data...
			//
			uiSaved += FS_Write(pbCompressedData,	iCompressedLength,			fhSaveGame);
			//
			// magic...
			//
			uiSaved += FS_Write(&uiMagic,			sizeof(uiMagic),			fhSaveGame);

			if (uiSaved != sizeof(chid) + sizeof(iLength) + sizeof(uiCksum) + sizeof(iCompressedLength) + iCompressedLength + sizeof(uiMagic))
			{
				Com_Printf(S_COLOR_RED "Failed to write %s chunk\n", SG_GetChidText(chid));
				gbSGWriteFailed = qtrue;
				return qfalse;
			}
		}
		else
		{
			// uncompressed...
			//
			uiSaved += FS_Write(&iLength,	sizeof(iLength),	fhSaveGame);
			//
			// CRC...
			//
			uiSaved += FS_Write(&uiCksum,	sizeof(uiCksum),	fhSaveGame);
			//
			// uncompressed data...
			//
			uiSaved += FS_Write( pvData,	iLength,			fhSaveGame);
			//
			// magic...
			//
			uiSaved += FS_Write(&uiMagic,	sizeof(uiMagic),	fhSaveGame);

			if (uiSaved != sizeof(chid) + sizeof(iLength) + sizeof(uiCksum) + iLength + sizeof(uiMagic))
			{
				Com_Printf(S_COLOR_RED "Failed to write %s chunk\n", SG_GetChidText(chid));
				gbSGWriteFailed = qtrue;
				return qfalse;
			}
		}
		
		#ifdef SG_PROFILE
		save_info[chid].Add(iLength);
		#endif
	}

	return qtrue;
}



// Pass in pvAddress (or NULL if you want memory to be allocated)
//	if pvAddress==NULL && ppvAddressPtr == NULL then the block is discarded/skipped.
//
// If iLength==0 then it counts as a query, else it must match the size found in the file
//
// function doesn't return if error (uses ERR_DROP), unless "qbSGReadIsTestOnly == qtrue", then NZ return = success
//
static int SG_Read_Actual(unsigned long chid, void *pvAddress, int iLength, void **ppvAddressPtr, qboolean bChunkIsOptional)
{
	unsigned int	uiLoadedCksum, uiCksum;
	unsigned int	uiLoadedMagic;
	unsigned int	uiLoadedLength;
	unsigned long	ulLoadedChid;
	int	uiLoaded;
	char			sChidText1[MAX_QPATH];
	char			sChidText2[MAX_QPATH];
	qboolean		qbTransient = qfalse;

	Com_DPrintf("Attempting read of chunk %s length %d\n", SG_GetChidText(chid), iLength);

	// Load in chid and length...
	//
	uiLoaded = FS_Read( &ulLoadedChid,   sizeof(ulLoadedChid),	fhSaveGame);
	uiLoaded+= FS_Read( &uiLoadedLength, sizeof(uiLoadedLength),fhSaveGame);

	qboolean bBlockIsCompressed = ((int)uiLoadedLength < 0);
	if (	 bBlockIsCompressed)
	{
		uiLoadedLength = -((int)uiLoadedLength);
	}

	// Make sure we are loading the correct chunk...
	//
	if( ulLoadedChid != chid)
	{
		if (bChunkIsOptional)
		{
			FS_Seek( fhSaveGame, -uiLoaded, FS_SEEK_CUR );
			return 0;
		}

		strcpy(sChidText1, SG_GetChidText(ulLoadedChid));
		strcpy(sChidText2, SG_GetChidText(chid));
		if (!qbSGReadIsTestOnly)
		{
			Com_Error(ERR_DROP, "Loaded chunk ID (%s) does not match requested chunk ID (%s)", sChidText1, sChidText2);
		}
		return 0;
	}

	// Find length of chunk and make sure it matches the requested length...
	//
	if( iLength )	// .. but only if there was one specified
	{
		if(iLength != (int)uiLoadedLength)
		{
			if (!qbSGReadIsTestOnly)
			{
				Com_Error(ERR_DROP, "Loaded chunk (%s) has different length than requested", SG_GetChidText(chid));
			}
			return 0;
		}
	}
	iLength = uiLoadedLength;	// for retval

	// Get checksum...
	//
	uiLoaded += FS_Read( &uiLoadedCksum,  sizeof(uiLoadedCksum), fhSaveGame );

	// alloc?...
	//
	if ( !pvAddress )
	{
		pvAddress = Z_Malloc(iLength, TAG_SAVEGAME, qfalse);
		//
		// Pass load address back...
		//
		if( ppvAddressPtr )
		{
			*ppvAddressPtr = pvAddress;
		}
		else
		{
			qbTransient = qtrue;	// if no passback addr, mark block for skipping
		}
	}

	// Load in data and magic number...
	//
	unsigned int uiCompressedLength=0;
	if (bBlockIsCompressed)
	{
		//
		// read compressed data length...
		//
		uiLoaded += FS_Read( &uiCompressedLength, sizeof(uiCompressedLength),fhSaveGame);
		//
		// alloc space...
		//	
		byte *pTempRLEData = (byte *)Z_Malloc(uiCompressedLength, TAG_SAVEGAME, qfalse);
		//
		// read compressed data...
		//
		uiLoaded += FS_Read( pTempRLEData,  uiCompressedLength, fhSaveGame );
		//
		// decompress it...
		//
		DeCompress_RLE((byte *)pvAddress, pTempRLEData, iLength);
		//
		// free workspace...
		//
		Z_Free( pTempRLEData );
	}
	else
	{
		uiLoaded += FS_Read(  pvAddress, iLength, fhSaveGame );
	}
	uiLoaded += FS_Read( &uiLoadedMagic,  sizeof(uiLoadedMagic), fhSaveGame );

	// Make sure the checksums match...
	//
	uiCksum = Com_BlockChecksum( pvAddress, iLength );
	if ( uiLoadedCksum != uiCksum)
	{
		if (!qbSGReadIsTestOnly)
		{
			Com_Error(ERR_DROP, "Failed checksum check for chunk", SG_GetChidText(chid));
		}
		else
		{
			if ( qbTransient )
			{
				Z_Free( pvAddress );
			}
		}
		return 0;
	}

	// Make sure the terminating magic number is there and correct...
	//
	if( uiLoadedMagic != SG_MAGIC)
	{
		if (!qbSGReadIsTestOnly)
		{
			Com_Error(ERR_DROP, "Bad savegame magic for chunk %s", SG_GetChidText(chid));
		}
		else
		{
			if ( qbTransient )
			{
				Z_Free( pvAddress );
			}
		}
		return 0;
	}

	// Make sure we didn't encounter any read errors...
	//
	if ( uiLoaded != sizeof(ulLoadedChid) + sizeof(uiLoadedLength) + sizeof(uiLoadedCksum) + (bBlockIsCompressed?sizeof(uiCompressedLength):0) + (bBlockIsCompressed?uiCompressedLength:iLength) + sizeof(uiLoadedMagic))
	{
		if (!qbSGReadIsTestOnly)
		{
			Com_Error(ERR_DROP, "Error during loading chunk %s", SG_GetChidText(chid));
		}
		else
		{
			if ( qbTransient )
			{
				Z_Free( pvAddress );
			}
		}
		return 0;
	}

	// If we are skipping the chunk, then free the memory...
	//
	if ( qbTransient )
	{
		Z_Free( pvAddress );
	}

	return iLength;
}

int SG_Read(unsigned long chid, void *pvAddress, int iLength, void **ppvAddressPtr /* = NULL */)
{
	return SG_Read_Actual(chid, pvAddress, iLength, ppvAddressPtr, qfalse );	// qboolean bChunkIsOptional
}

int SG_ReadOptional(unsigned long chid, void *pvAddress, int iLength, void **ppvAddressPtr /* = NULL */)
{
	return SG_Read_Actual(chid, pvAddress, iLength, ppvAddressPtr, qtrue);		// qboolean bChunkIsOptional
}


void SG_TestSave(void)
{
	if (sv_testsave->value && sv.state == SS_GAME)
	{
		WriteGame (false);
		ge->WriteLevel(false);
	}
}

////////////////// eof ////////////////////

