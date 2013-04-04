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
#include "..\game\statindex.h"
#include "..\game\weapons.h"
#include "..\game\g_items.h"

#ifdef _XBOX
#include <stdlib.h>
//support for mbstowcs
HANDLE sg_Handle;
#define SG_BUFFERSIZE 8192
byte sg_Buffer[SG_BUFFERSIZE];
int sg_BufferSize;
//used for save game reading
int sg_CurrentBufferPos;

#define filepathlength 120

struct XValidationHeader
{
    // Length of the file, including header, in bytes
    DWORD dwFileLength;

    // File signature (secure hash of file data)
    XCALCSIG_SIGNATURE Signature;
};

//validation header going into file and coming out of file
XValidationHeader sg_validationHeader;
//validation header calculated on file read to test against file
XValidationHeader sg_validationHeaderRead;

//signature handle
HANDLE sg_sigHandle;


int SG_Write(const void * chid, const int bytesize, fileHandle_t fhSG);
qboolean SG_Close();
int SG_Seek( fileHandle_t fhSaveGame, long offset, int origin );



#endif

#pragma warning(disable : 4786)  // identifier was truncated (STL crap)
#pragma warning(disable : 4710)  // function was not inlined (STL crap)
#pragma warning(disable : 4512)  // yet more STL drivel...

#include <map>

using namespace std;

static char	saveGameComment[iSG_COMMENT_SIZE];

//#define SG_PROFILE	// enable for debug save stats if you want

int giSaveGameVersion;	// filled in when a savegame file is opened
fileHandle_t fhSaveGame = 0;
SavedGameJustLoaded_e eSavedGameJustLoaded = eNO;
qboolean qbSGReadIsTestOnly = qfalse;	// this MUST be left in this state
char sLastSaveFileLoaded[MAX_QPATH]={0};

#define iSG_MAPCMD_SIZE MAX_QPATH

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
	
	const char *psReference = bOpen ? "MENUS_FAILED_TO_OPEN_SAVEGAME" : "MENUS3_FAILED_TO_CREATE_SAVEGAME";
	Q_strncpyz(sTemp + strlen(sTemp), va( SE_GetString(psReference), psFilename),sizeof(sTemp));
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

	if(psPathlessBaseName)
	{
		char *p = strchr(psPathlessBaseName,'/');
		if (p)
		{
			while (p)
			{
				*p = '_';
				p = strchr(p,'/');
			}
		}
	}
	Com_sprintf( sSaveName[i], MAX_OSPATH, "saves/%s.sav", psPathlessBaseName );
	return sSaveName[i];
}

void SG_WipeSavegame( LPCSTR psPathlessBaseName )
{
#ifndef _XBOX
	LPCSTR psLocalFilename  = SG_AddSavePath( psPathlessBaseName );
	
	FS_DeleteUserGenFile( psLocalFilename );
#else
	unsigned short namebuffer[filepathlength];
	mbstowcs(namebuffer, psPathlessBaseName,filepathlength);
	//kill the whole directory
	//remove it
	XDeleteSaveGame( "U:\\", namebuffer);
#endif
}

static qboolean SG_Move( LPCSTR psPathlessBaseName_Src, LPCSTR psPathlessBaseName_Dst )
{


#ifndef _XBOX
	LPCSTR psLocalFilename_Src = SG_AddSavePath( psPathlessBaseName_Src );
	LPCSTR psLocalFilename_Dst = SG_AddSavePath( psPathlessBaseName_Dst );

	qboolean qbCopyWentOk = FS_MoveUserGenFile( psLocalFilename_Src, psLocalFilename_Dst );

	if (!qbCopyWentOk)
	{
		Com_Printf(S_COLOR_RED "Error during savegame-rename. Check \"%s\" for write-protect or disk full!\n", psLocalFilename_Dst );
		return qfalse;
	}

	return qtrue;
#else
	char psLocalFilenameSrc[filepathlength];
	char psLocalFilenameDest[filepathlength];
	unsigned short widecharstring[filepathlength];
	mbstowcs(widecharstring, psPathlessBaseName_Dst, filepathlength);

	if ( ERROR_SUCCESS != XCreateSaveGame("U:\\", widecharstring, OPEN_ALWAYS, 0, psLocalFilenameDest, filepathlength))
		return qfalse;
	mbstowcs(widecharstring, psPathlessBaseName_Src, filepathlength);
	if ( ERROR_SUCCESS != XCreateSaveGame("U:\\", widecharstring, OPEN_ALWAYS, 0, psLocalFilenameSrc, filepathlength))
	{
		return qfalse;
	}


	Q_strcat(psLocalFilenameDest, filepathlength, "JK3SG.xsv");
	Q_strcat(psLocalFilenameSrc, filepathlength, "JK3SG.xsv");

	CopyFile( psLocalFilenameSrc, psLocalFilenameDest,false);

	
	return qtrue;
	
#endif
}


/* JLFSAVEGAME used to find if there is a file on the xbox */
#ifdef _XBOX
qboolean SG_Exists(LPCSTR psPathlessBaseName)
{
	char psLocalFilename[filepathlength];
	unsigned short widecharstring[filepathlength];
	mbstowcs(widecharstring, psPathlessBaseName, filepathlength);
	if ( ERROR_SUCCESS != XCreateSaveGame("U:\\", widecharstring, CREATE_NEW, 0, psLocalFilename, filepathlength))
	{
		return qtrue;
	}
	if ( ERROR_SUCCESS == XDeleteSaveGame("U:\\", widecharstring))
	{
		return qfalse;
	}
	assert(0);
	return qfalse;
}
#endif


qboolean gbSGWriteFailed = qfalse;

static qboolean SG_Create( LPCSTR psPathlessBaseName )
{
	gbSGWriteFailed = qfalse;

#ifdef _XBOX
	char psLocalFilename[filepathlength];
	char psScreenshotFilename[filepathlength];
	unsigned short widecharstring[filepathlength];
	mbstowcs(widecharstring, psPathlessBaseName, filepathlength);
	if ( ERROR_SUCCESS != XCreateSaveGame("U:\\", widecharstring, OPEN_ALWAYS, 0, psLocalFilename, filepathlength))
		return qfalse;

	// create the path for the screenshot file
	strcpy(psScreenshotFilename, psLocalFilename);
	Q_strcat(psScreenshotFilename, filepathlength,"saveimage.xbx");

	// create the path for the savegame
	Q_strcat(psLocalFilename, filepathlength, "JK3SG.xsv");

	sg_Handle = CreateFile(psLocalFilename, GENERIC_WRITE, FILE_SHARE_READ, 0, 
		OPEN_ALWAYS,	FILE_ATTRIBUTE_NORMAL, 0);
	//clear the buffer
	sg_BufferSize = 0;

	DWORD bytesWritten;
// save spot for validation
	WriteFile(sg_Handle,            // handle to file
			  &sg_validationHeader,                // data buffer
				sizeof (sg_validationHeader),     // number of bytes to write
				&bytesWritten,  // number of bytes written
				NULL        // overlapped buffer
				);
	//start the validation key creation
	// Start the signature hash
    sg_sigHandle = XCalculateSignatureBegin( 0 );
    if( sg_sigHandle == INVALID_HANDLE_VALUE )
        return FALSE;

	// attempt to copy the last screenshot to the save game directory	
	if( !CopyFile("u:\\saveimage.xbx", psScreenshotFilename, FALSE) )
	{
		CopyFile("d:\\base\\media\\defaultsaveimage.xbx", psScreenshotFilename, FALSE);
	}

#else
	SG_WipeSavegame( psPathlessBaseName );
	LPCSTR psLocalFilename = SG_AddSavePath( psPathlessBaseName );		
	fhSaveGame = FS_FOpenFileWrite( psLocalFilename );
#endif

#ifdef _XBOX
	if (!sg_Handle)
#else
	if(!fhSaveGame)
#endif
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

#ifdef _XBOX

qboolean SG_CloseWrite()
{
	DWORD bytesWritten;
	//clear the buffer to the file
	if (!WriteFile(sg_Handle,                    // handle to file
					sg_Buffer,                // data buffer
					sg_BufferSize,     // number of bytes to write
					&bytesWritten,  // number of bytes written
					NULL        // overlapped buffer
					))
				return qfalse;
	//get the length of the file
	unsigned int filelength =GetFileSize (sg_Handle, NULL);
	// create the validation code
	sg_validationHeader.dwFileLength = filelength;
	// Release signature resources
    DWORD dwSuccess =XCalculateSignatureEnd( sg_sigHandle, &sg_validationHeader.Signature );
	assert( dwSuccess == ERROR_SUCCESS );
	//seek to the first of the file
	SG_Seek(NULL,0,FS_SEEK_SET);
	//SetFilePointer(sg_Handle,0,0,FILE_BEGIN);
	//write the validation codes
	WriteFile (sg_Handle, &sg_validationHeader,sizeof (sg_validationHeader),&bytesWritten, NULL);
	return SG_Close();
}
#endif






qboolean SG_Close()
{
#ifdef _XBOX
	CloseHandle(sg_Handle);
	sg_Handle = NULL;

#else
	assert( fhSaveGame );	
	FS_FCloseFile( fhSaveGame );
#endif
	fhSaveGame = NULL;

#ifdef SG_PROFILE
	if (!sv_testsave->integer)
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
	if(!psPathlessBaseName)
	{
		return qfalse;
	}
//JLFSAVEGAME

#ifdef _XBOX
	unsigned short saveGameName[filepathlength];
	char directoryInfo[filepathlength];
	char psLocalFilename[filepathlength];
	DWORD bytesRead;
	
	mbstowcs(saveGameName, psPathlessBaseName,filepathlength);
	
	XCreateSaveGame("U:\\", saveGameName, OPEN_ALWAYS, 0,directoryInfo, filepathlength);

	strcpy (psLocalFilename , directoryInfo);
	strcat (psLocalFilename , "JK3SG.xsv");

	sg_Handle = NULL;
	sg_Handle = CreateFile(psLocalFilename, GENERIC_READ, FILE_SHARE_READ, 0, 
		OPEN_EXISTING,	FILE_ATTRIBUTE_NORMAL, 0);

	if (!sg_Handle)
#else

	LPCSTR psLocalFilename = SG_AddSavePath( psPathlessBaseName );	
	FS_FOpenFileRead( psLocalFilename, &fhSaveGame, qtrue );	//qtrue = dup handle, so I can close it ok later
	if (!fhSaveGame)
#endif

	{
//		Com_Printf(S_COLOR_RED "Failed to open savegame file %s\n", psLocalFilename);
		Com_DPrintf(GetString_FailedToOpenSaveGame(psLocalFilename, qtrue));

		return qfalse;
	}
#ifdef _XBOX
	//read the validation header
	if (!ReadFile( sg_Handle, &sg_validationHeader, sizeof(sg_validationHeader), &bytesRead,  NULL ))
	{
		SG_Close();
		Com_Printf (S_COLOR_RED "File \"%s\" has no sig");
		return qfalse;
	}
	//initialize buffer data
	sg_BufferSize = 0;
	sg_CurrentBufferPos =0;

#endif
	giSaveGameVersion=-1;//jic
	SG_Read('_VER', &giSaveGameVersion, sizeof(giSaveGameVersion));
	if (giSaveGameVersion != iSAVEGAME_VERSION)
	{
		SG_Close();
		Com_Printf (S_COLOR_RED "File \"%s\" has version # %d (expecting %d)\n",psPathlessBaseName, giSaveGameVersion, iSAVEGAME_VERSION);
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
//	Com_Printf("%s has been wiped\n", Cmd_Argv(1));	// wurde gelöscht in german, but we've only got one string
//	Com_Printf("Ok\n"); // no localization of this
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
	Com_Printf (S_COLOR_CYAN "%s.\n",SE_GetString("MENUS_DONE"));

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

#ifndef _XBOX	// VVFIXME : Part of super-bootleg SG hackery
	if (!stricmp (psFilename, "current"))
	{
		Com_Printf (S_COLOR_RED "Can't load from \"current\"\n");
		return;
	}
#endif

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
	Com_Printf (S_COLOR_CYAN "%s\n",va(SE_GetString("MENUS_LOADING_MAPNAME"), psFilename));

	gbAlreadyDoingLoad = qtrue;
	if (!SG_ReadSavegame(psFilename)) {
		gbAlreadyDoingLoad = qfalse; //	do NOT do this here now, need to wait until client spawn, unless the load failed.
	} else
	{
		Com_Printf (S_COLOR_CYAN "%s.\n",SE_GetString("MENUS_DONE"));
	}
}

qboolean SG_GameAllowedToSaveHere(qboolean inCamera);


//JLF notes
//	save game will be in charge of creating a new directory
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
		Com_Printf (S_COLOR_RED "\n%s\n", SE_GetString("SP_INGAME_CANT_SAVE_DEAD"));
		return;
	}

	//this check catches deaths even the instant you die, like during a slo-mo death!
	gentity_t			*svent;
	svent = SV_GentityNum(0);
	if (svent->client->stats[STAT_HEALTH]<=0)
	{
		Com_Printf (S_COLOR_RED "\n%s\n", SE_GetString("SP_INGAME_CANT_SAVE_DEAD"));
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
		return;	// this prevents people saving via quick-save now during cinematics.

	if ( !stricmp (psFilename, "auto") )
	{
		
#ifdef _XBOX
		extern void	SCR_PrecacheScreenshot();  //scr_scrn.cpp
		SCR_PrecacheScreenshot();
#endif
		SG_StoreSaveGameComment("");	// clear previous comment/description, which will force time/date comment.
	}

	Com_Printf (S_COLOR_CYAN "%s \"%s\"...\n", SE_GetString("CON_TEXT_SAVING_GAME"), psFilename);

	if (SG_WriteSavegame(psFilename, qfalse))
	{
		Com_Printf (S_COLOR_CYAN "%s.\n",SE_GetString("MENUS_DONE"));
	}
	else
	{
		Com_Printf (S_COLOR_RED "%s.\n",SE_GetString("MENUS_FAILED_TO_OPEN_SAVEGAME"));
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


// write all CVAR_SAVEGAME cvars
// these will be things like model, name, ...
//
extern  cvar_t	*cvar_vars;	// I know this is really unpleasant, but I need access for scanning/writing latched cvars during save games

void SG_WriteCvars(void)
{
	cvar_t	*var;
	int		iCount = 0;

	// count the cvars...
	//	
	for (var = cvar_vars; var; var = var->next)
	{
		if (!(var->flags & CVAR_SAVEGAME))
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
		if (!(var->flags & CVAR_SAVEGAME))
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

	if ( qbAutosave || !*saveGameComment)
	{
		Com_sprintf( sComment, sizeof(sComment), "---> %s", psMapName );
	}
	else
	{
		strcpy(sComment,saveGameComment);
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

	qbSGReadIsTestOnly = qtrue;	// do NOT leave this in this state

	if ( !SG_Open( psPathlessBaseName ))
	{
		qbSGReadIsTestOnly = qfalse;
		return 0;
	}							

	if (SG_Read( 'COMM', sComment, iSG_COMMENT_SIZE ))
	{	
		if (SG_Read( 'CMTM', &tFileTime, sizeof( time_t )))	//read
		{	
			if (SG_Read('MPCM', sMapName, iSG_MAPCMD_SIZE ))	// read
			{
				ret = tFileTime;
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
/*
static qboolean SG_ReadScreenshot(qboolean qbSetAsLoadingScreen, void *pvDest = NULL);
static qboolean SG_ReadScreenshot(qboolean qbSetAsLoadingScreen, void *pvDest)
{
#ifdef _XBOX
	return qfalse;
#else
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
	SG_Read('SHOT', pJPGData, iScreenShotLength, 0);	
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
#endif
}
// Gets the savegame screenshot
//
qboolean SG_GetSaveImage( const char *psPathlessBaseName, void *pvAddress )
{
	if(!psPathlessBaseName)
	{
		return qfalse;
	}
//JLFSAVEGAME
#if 0
	unsigned short saveGameName[filepathlength];
	char directoryInfo[filepathlength];
	char psLocalFilename[filepathlength];
	DWORD bytesRead;
	
	mbstowcs(saveGameName, psPathlessBaseName,filepathlength);
	
	XCreateSaveGame("U:\\", saveGameName, OPEN_ALWAYS, 0,directoryInfo, filepathlength);

	strcpy (psLocalFilename , directoryInfo);
	strcat (psLocalFilename , "saveimage.xbx");


	sg_Handle = NULL;
	sg_Handle = CreateFile(psLocalFilename, GENERIC_READ, FILE_SHARE_READ, 0, 
		OPEN_EXISTING,	FILE_ATTRIBUTE_NORMAL, 0);

	if (!sg_Handle)
		return qfalse;



#else

	if (!SG_Open(psPathlessBaseName))
	{
		return qfalse;
	}
	
	SG_Read('COMM', NULL, 0, NULL);	// skip
	SG_Read('CMTM', NULL, sizeof( time_t ));

	qboolean bGotSaveImage = SG_ReadScreenshot(qfalse, pvAddress);

	SG_Close();
#endif
	return bGotSaveImage;
}


static void SG_WriteScreenshot(qboolean qbAutosave, LPCSTR psMapName)
{
#ifndef _XBOX
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
#endif
}
*/

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

	int iPrevTestSave = sv_testsave->integer;
	sv_testsave->integer = 0;

	// Write out server data...
	//
	LPCSTR psServerInfo = sv.configstrings[CS_SERVERINFO];
	LPCSTR psMapName    = Info_ValueForKey( psServerInfo, "mapname" );
//JLF
#ifdef _XBOX
	char mapname[filepathlength];
	char numberedmapname[filepathlength];
	int mapnumber =0;
	char numberbuffer[10];
	if ( !strcmp("JKSG3",psPathlessBaseName))
	{
		//strcpy(mapname, psMapName);
		strcpy (mapname, psPathlessBaseName);
		strcpy( numberedmapname, mapname);
	}
	else
	{
		strcpy(mapname, psMapName);
		strcpy(numberedmapname, psPathlessBaseName);
	}
	while (!qbAutosave && SG_Exists( numberedmapname))
	{
		strcpy( numberedmapname, mapname);
		
		Com_sprintf(numberbuffer,sizeof(numberbuffer),"_%02i",mapnumber);
		strcat ( numberedmapname,numberbuffer);
		mapnumber++;
	}
	SG_Create( numberedmapname);

#else
	if ( !strcmp("quick",psPathlessBaseName))
	{
		SG_StoreSaveGameComment(va("--> %s <--",psMapName));
	}

	if(!SG_Create( "current" ))
			{
		Com_Printf (GetString_FailedToOpenSaveGame("current",qfalse));//S_COLOR_RED "Failed to create savegame\n");
		SG_WipeSavegame( "current" );
		sv_testsave->integer = iPrevTestSave;
		return qfalse;
	}
#endif
//END JLF

	char   sMapCmd[iSG_MAPCMD_SIZE]={0};
	strcpy( sMapCmd,psMapName);	// need as array rather than ptr because const strlen needed for MPCM chunk

	SG_WriteComment(qbAutosave, sMapCmd);
//	SG_WriteScreenshot(qbAutosave, sMapCmd);
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
#ifdef _XBOX
	SG_CloseWrite();
#else
	SG_Close();
#endif
	if (gbSGWriteFailed)
	{
		Com_Printf (GetString_FailedToOpenSaveGame("current",qfalse));//S_COLOR_RED "Failed to write savegame!\n");
		SG_WipeSavegame( "current" );
		sv_testsave->integer = iPrevTestSave;
		return qfalse;
	}

	SG_Move( "current", psPathlessBaseName );


	sv_testsave->integer = iPrevTestSave;
	return qtrue;
}

qboolean SG_ReadSavegame(const char *psPathlessBaseName)
{
	char		sComment[iSG_COMMENT_SIZE];
	char		sMapCmd [iSG_MAPCMD_SIZE];
	qboolean	qbAutosave;

	int iPrevTestSave = sv_testsave->integer;
	sv_testsave->integer = 0;

	if (!SG_Open( psPathlessBaseName ))
	{
		Com_Printf (GetString_FailedToOpenSaveGame(psPathlessBaseName, qtrue));//S_COLOR_RED "Failed to open savegame \"%s\"\n", psPathlessBaseName);
		sv_testsave->integer = iPrevTestSave;
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

//	SG_ReadScreenshot(qtrue);	// qboolean qbSetAsLoadingScreen
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
		sv_testsave->integer = iPrevTestSave;
		return qfalse;
	}

	sv_testsave->integer = iPrevTestSave;
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

		gpbCompBlock = (byte *) Z_Malloc(iSize, TAG_TEMP_WORKSPACE, qfalse);
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


#ifdef _XBOX// function for xbox
/*
int SG_Write(const void * chid, const int bytesize, fileHandle_t fhSG)
{
	DWORD bytesWritten;
	int currentsize;

	
	if (!WriteFile(sg_Handle,                    // handle to file
							chid,                // data buffer
							bytesize,     // number of bytes to write
							&bytesWritten,  // number of bytes written
							NULL        // overlapped buffer
							))
		{
			return 0;
		}

		return bytesWritten;
}
*/


int SG_Write(const void * chid, const int bytesize, fileHandle_t fhSG)
{
	DWORD bytesWritten;
	int currentsize;

	if (sg_BufferSize + bytesize>= SG_BUFFERSIZE)
	{	
		if (!WriteFile(sg_Handle,                    // handle to file
							sg_Buffer,                // data buffer
							sg_BufferSize,     // number of bytes to write
							&bytesWritten,  // number of bytes written
							NULL        // overlapped buffer
							))
	//	return bytesWritten;
//	else
		{
			return 0;
		}
		
		DWORD dwSuccess = XCalculateSignatureUpdate( sg_sigHandle, (BYTE*)(sg_Buffer),
                                                     sg_BufferSize);
		sg_BufferSize = 0;


	}
	if (bytesize >= SG_BUFFERSIZE)
	{
		if (!WriteFile(sg_Handle,                    // handle to file
							chid,                // data buffer
							bytesize,     // number of bytes to write
							&bytesWritten,  // number of bytes written
							NULL        // overlapped buffer
							))
		{
			return 0;
		}
		DWORD dwSuccess = XCalculateSignatureUpdate( sg_sigHandle, (BYTE*)(chid),
                                                     bytesize);
		sg_BufferSize = 0;
	}
	else
	{

		byte * tempptr = &(sg_Buffer[sg_BufferSize]);
		memcpy(tempptr, chid, bytesize);
		sg_BufferSize += bytesize;
	}
		return bytesize;
}

#else
//pass through function
int SG_Write(const void * chid, const int bytesize, fileHandle_t fhSaveGame)
{
		return FS_Write( chid, bytesize, fhSaveGame);
}

#endif



qboolean SG_Append(unsigned long chid, const void *pvData, int iLength)
{	
	unsigned int	uiCksum;
	unsigned int	uiSaved;
	
#ifdef _DEBUG
	int				i;
	unsigned long	*pTest;

	pTest = (unsigned long *) pvData;
	for (i=0; i<iLength/4; i++, pTest++)
	{
		assert(*pTest != 0xfeeefeee);
		assert(*pTest != 0xcdcdcdcd);
		assert(*pTest != 0xdddddddd);
	}
#endif

	Com_DPrintf("Attempting write of chunk %s length %d\n", SG_GetChidText(chid), iLength);

	// only write data out if we're not in test mode....
	//
	if (!sv_testsave->integer)
	{
		uiCksum = Com_BlockChecksum (pvData, iLength);

		uiSaved  = SG_Write(&chid,		sizeof(chid),		fhSaveGame);

		byte *pbCompressedData = NULL;
		int iCompressedLength = CompressMem((byte*)pvData, iLength, pbCompressedData);
		if (iCompressedLength != -1)
		{
			// compressed...  (write length field out as -ve)
			//
			iLength = -iLength;
			uiSaved += SG_Write(&iLength,			sizeof(iLength),			fhSaveGame);
			iLength = -iLength;
			//
			// [compressed length]
			//
			uiSaved += SG_Write(&iCompressedLength, sizeof(iCompressedLength),	fhSaveGame);
			//
			// compressed data...
			//
			uiSaved += SG_Write(pbCompressedData,	iCompressedLength,			fhSaveGame);
			//
			// CRC...
			//
			uiSaved += SG_Write(&uiCksum,			sizeof(uiCksum),			fhSaveGame);

			if (uiSaved != sizeof(chid) + sizeof(iLength) + sizeof(uiCksum) + sizeof(iCompressedLength) + iCompressedLength)
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
			uiSaved += SG_Write(&iLength,	sizeof(iLength),	fhSaveGame);
			//
			// uncompressed data...
			//
			uiSaved += SG_Write( pvData,	iLength,			fhSaveGame);
			//
			// CRC...
			//
			uiSaved += SG_Write(&uiCksum,	sizeof(uiCksum),	fhSaveGame);

			if (uiSaved != sizeof(chid) + sizeof(iLength) + sizeof(uiCksum) + iLength)
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


#ifdef _XBOX// function for xbox
//SG_ReadBytes replaces FS_Read. I was going to use SG_Read but it is already in use
/*
int SG_ReadBytes(void * chid, int bytesize, fileHandle_t fhSG)
{  
	byte* bufferptr;
	unsigned char* destptr;
	DWORD retBytesRead=0;
	DWORD bytesRead =0;
	int segmentLength;
	

	//bufferptr = (byte*)chid;
	//destptr = NULL;
	
	if (ReadFile(sg_Handle,                    // handle to file
				chid,                // data buffer
				bytesize,     // number of bytes to write
				&bytesRead,  // number of bytes written
				NULL        // overlapped buffer
				))
		{	return bytesRead;
		}
		else
		{
			return 0;
		}
		
	return retBytesRead;
}
*/

int SG_ReadBytes(void * chid, int bytesize, fileHandle_t fhSG)
{  
	byte* bufferptr;
	unsigned char* destptr;
	DWORD retBytesRead=0;
	DWORD bytesRead =0;
	int segmentLength;
	

	//bufferptr = (byte*)chid;
	//destptr = NULL;
	
	if ( bytesize < (sg_BufferSize - sg_CurrentBufferPos))
	{
		bufferptr = &(sg_Buffer[sg_CurrentBufferPos]);
		memcpy(chid,bufferptr, bytesize);
		sg_CurrentBufferPos+= bytesize;
		retBytesRead = bytesize;
	}
	else
	{
		destptr = (byte*)((void*)chid);

 		while ( bytesize >0)
		{
			bufferptr = &(sg_Buffer[sg_CurrentBufferPos]);
			segmentLength = sg_BufferSize - sg_CurrentBufferPos;
			if (segmentLength <= bytesize)
			{
				memcpy(destptr, bufferptr, segmentLength);
				destptr += segmentLength;
				retBytesRead += segmentLength;
				bytesize -= segmentLength;
				sg_CurrentBufferPos += segmentLength;
			}
			else
			{
				memcpy(destptr, bufferptr, bytesize);
				destptr += bytesize;
				retBytesRead += bytesize;
				sg_CurrentBufferPos += bytesize;
				bytesize -= bytesize;
				
			}
	
			if (sg_BufferSize - sg_CurrentBufferPos <= 0 && bytesize >0)
			{
				if (ReadFile(sg_Handle,                    // handle to file
							sg_Buffer,                // data buffer
							SG_BUFFERSIZE,     // number of bytes to write
							&bytesRead,  // number of bytes written
							NULL        // overlapped buffer
							))
				{
					sg_BufferSize = bytesRead;
					sg_CurrentBufferPos = 0;
					bufferptr = sg_Buffer;
					//sig processing
				}
				else
				{
					return 0;
				}
			}

		}
	}
	return retBytesRead;
}



// handle offset origin
//fhSaveGame not used (use global variable)
int SG_Seek( fileHandle_t fhSaveGame, long offset, int origin )
{
	switch (origin)
	{
	case FS_SEEK_CUR:
		return SetFilePointer(
					sg_Handle,                // handle to file
					offset,        // bytes to move pointer
					NULL,		   // bytes to move pointer
					FILE_CURRENT   // starting point
				);
		break;
	case FS_SEEK_END:
		return SetFilePointer(
					sg_Handle,                // handle to file
					offset,        // bytes to move pointer
					NULL,		   // bytes to move pointer
					FILE_END   // starting point
				);
		break;
	default:
        //FS_SEEK_SET:
		return SetFilePointer(
					sg_Handle,                // handle to file
					offset,        // bytes to move pointer
					NULL,		   // bytes to move pointer
					FILE_BEGIN   // starting point
				);
	}
	return 0;
}

#else
//pass through function
int SG_ReadBytes(void * chid, int bytesize, fileHandle_t fhSaveGame)
{
	return FS_Read( chid, bytesize, fhSaveGame);
}


int SG_Seek( fileHandle_t fhSaveGame, long offset, int origin )
{
	return FS_Seek(fhSaveGame, offset, origin);
}

#endif



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
	unsigned int	uiLoadedLength;
	unsigned long	ulLoadedChid;
	unsigned int	uiLoaded;
	char			sChidText1[MAX_QPATH];
	char			sChidText2[MAX_QPATH];
	qboolean		qbTransient = qfalse;

	Com_DPrintf("Attempting read of chunk %s length %d\n", SG_GetChidText(chid), iLength);

	// Load in chid and length...
	//
	uiLoaded = SG_ReadBytes( &ulLoadedChid,   sizeof(ulLoadedChid),	fhSaveGame);
	uiLoaded+= SG_ReadBytes( &uiLoadedLength, sizeof(uiLoadedLength),fhSaveGame);

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
			SG_Seek( fhSaveGame, -(int)uiLoaded, FS_SEEK_CUR );
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
		uiLoaded += SG_ReadBytes( &uiCompressedLength, sizeof(uiCompressedLength),fhSaveGame);
		//
		// alloc space...
		//	
		byte *pTempRLEData = (byte *)Z_Malloc(uiCompressedLength, TAG_SAVEGAME, qfalse);
		//
		// read compressed data...
		//
		uiLoaded += SG_ReadBytes( pTempRLEData,  uiCompressedLength, fhSaveGame );
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
		uiLoaded += SG_ReadBytes(  pvAddress, iLength, fhSaveGame );
	}
	// Get checksum...
	//
	uiLoaded += SG_ReadBytes( &uiLoadedCksum,  sizeof(uiLoadedCksum), fhSaveGame );

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

	// Make sure we didn't encounter any read errors...
	//size_t
	if ( uiLoaded != sizeof(ulLoadedChid) + sizeof(uiLoadedLength) + sizeof(uiLoadedCksum) + (bBlockIsCompressed?sizeof(uiCompressedLength):0) + (bBlockIsCompressed?uiCompressedLength:iLength))
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
	if (sv_testsave->integer && sv.state == SS_GAME)
	{
		WriteGame (false);
		ge->WriteLevel(false);
	}
}

////////////////// eof ////////////////////

