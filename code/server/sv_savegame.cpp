/*
===========================================================================
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

// Filename:-	sv_savegame.cpp
#include "../server/exe_headers.h"

#define JPEG_IMAGE_QUALITY 95

//#define USE_LAST_SAVE_FROM_THIS_MAP	// enable this if you want to use the last explicity-loaded savegame from this map
				 						//	when respawning after dying, else it'll just load "auto" regardless 
										//	(EF1 behaviour). I should maybe time/date check them though?

#include "server.h"
#include "../qcommon/stringed_ingame.h"
#include "../game/statindex.h"
#include "../game/weapons.h"
#include "../game/g_items.h"

#include <map>

static char	saveGameComment[iSG_COMMENT_SIZE];

//#define SG_PROFILE	// enable for debug save stats if you want

int giSaveGameVersion;	// filled in when a savegame file is opened
fileHandle_t fhSaveGame = 0;
SavedGameJustLoaded_e eSavedGameJustLoaded = eNO;
qboolean qbSGReadIsTestOnly = qfalse;	// this MUST be left in this state
char sLastSaveFileLoaded[MAX_QPATH]={0};

#define iSG_MAPCMD_SIZE MAX_QPATH

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

typedef map<unsigned int, CChid> CChidInfo_t;
CChidInfo_t	save_info;
#endif

const char *SG_GetChidText(unsigned int chid)
{
	static union { char c[5]; int i; } chidtext;

	chidtext.i = BigLong(chid);

	return chidtext.c;
}


static const char *GetString_FailedToOpenSaveGame(const char *psFilename, qboolean bOpen)
{
	static char sTemp[256];

	strcpy(sTemp,S_COLOR_RED);

#ifdef JK2_MODE
	const char *psReference = bOpen ? "MENUS3_FAILED_TO_OPEN_SAVEGAME" : "MENUS3_FAILED_TO_CREATE_SAVEGAME";
#else
	const char *psReference = bOpen ? "MENUS_FAILED_TO_OPEN_SAVEGAME" : "MENUS3_FAILED_TO_CREATE_SAVEGAME";
#endif
	Q_strncpyz(sTemp + strlen(sTemp), va( SE_GetString(psReference), psFilename),sizeof(sTemp));
	strcat(sTemp,"\n");
	return sTemp;
}

// (copes with up to 8 ptr returns at once)
//
static const char *SG_AddSavePath( const char *psPathlessBaseName )
{
	static char sSaveName[8][MAX_OSPATH]; 
	static int  i=0;

	int next = i = (i + 1) & 7;
	i = next;

	if(psPathlessBaseName)
	{
		char *p = const_cast<char*>(strchr(psPathlessBaseName,'/'));
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

void SG_WipeSavegame( const char *psPathlessBaseName )
{
	const char *psLocalFilename  = SG_AddSavePath( psPathlessBaseName );
	
	FS_DeleteUserGenFile( psLocalFilename );
}

static qboolean SG_Move( const char *psPathlessBaseName_Src, const char *psPathlessBaseName_Dst )
{
	const char *psLocalFilename_Src = SG_AddSavePath( psPathlessBaseName_Src );
	const char *psLocalFilename_Dst = SG_AddSavePath( psPathlessBaseName_Dst );

	qboolean qbCopyWentOk = FS_MoveUserGenFile( psLocalFilename_Src, psLocalFilename_Dst );

	if (!qbCopyWentOk)
	{
		Com_Printf(S_COLOR_RED "Error during savegame-rename. Check \"%s\" for write-protect or disk full!\n", psLocalFilename_Dst );
		return qfalse;
	}

	return qtrue;
}


qboolean gbSGWriteFailed = qfalse;

static qboolean SG_Create( const char *psPathlessBaseName )
{
	gbSGWriteFailed = qfalse;

	SG_WipeSavegame( psPathlessBaseName );
	const char *psLocalFilename = SG_AddSavePath( psPathlessBaseName );		
	fhSaveGame = FS_FOpenFileWrite( psLocalFilename );

	if(!fhSaveGame)
	{
		Com_Printf(GetString_FailedToOpenSaveGame(psLocalFilename,qfalse));//S_COLOR_RED "Failed to create new savegame file \"%s\"\n", psLocalFilename );
		return qfalse;
	}

	giSaveGameVersion = iSAVEGAME_VERSION;
	SG_Append(INT_ID('_','V','E','R'), &giSaveGameVersion, sizeof(giSaveGameVersion));

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
		fhSaveGame = NULL_FILE;
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
	fhSaveGame = NULL_FILE;

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


qboolean SG_Open( const char *psPathlessBaseName )
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

	const char *psLocalFilename = SG_AddSavePath( psPathlessBaseName );	
	FS_FOpenFileRead( psLocalFilename, &fhSaveGame, qtrue );	//qtrue = dup handle, so I can close it ok later
	if (!fhSaveGame)
	{
//		Com_Printf(S_COLOR_RED "Failed to open savegame file %s\n", psLocalFilename);
		Com_DPrintf(GetString_FailedToOpenSaveGame(psLocalFilename, qtrue));

		return qfalse;
	}
	giSaveGameVersion=-1;//jic
	SG_Read(INT_ID('_','V','E','R'), &giSaveGameVersion, sizeof(giSaveGameVersion));
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
	if (!Q_stricmp (Cmd_Argv(1), "auto") )
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
#ifdef JK2_MODE
	Com_Printf (S_COLOR_CYAN "Done.\n");
#else
	Com_Printf (S_COLOR_CYAN "%s.\n",SE_GetString("MENUS_DONE"));
#endif

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

	if (!Q_stricmp (psFilename, "current"))
	{
		Com_Printf (S_COLOR_RED "Can't load from \"current\"\n");
		return;
	}

	// special case, if doing a respawn then check that the available auto-save (if any) is from the same map
	//	as we're currently on (if in a map at all), if so, load that "auto", else re-load the last-loaded file...
	//
	if (!Q_stricmp(psFilename, "*respawn"))
	{
		psFilename = "auto";	// default to standard respawn behaviour

		// see if there's a last-loaded file to even check against as regards loading...
		//
		if ( sLastSaveFileLoaded[0] )
		{
			const char *psServerInfo = sv.configstrings[CS_SERVERINFO];
			const char *psMapName    = Info_ValueForKey( psServerInfo, "mapname" );

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
#ifdef JK2_MODE
	Com_Printf (S_COLOR_CYAN "Loading game \"%s\"...\n", psFilename);
#else
	Com_Printf (S_COLOR_CYAN "%s\n",va(SE_GetString("MENUS_LOADING_MAPNAME"), psFilename));
#endif

	gbAlreadyDoingLoad = qtrue;
	if (!SG_ReadSavegame(psFilename)) {
		gbAlreadyDoingLoad = qfalse; //	do NOT do this here now, need to wait until client spawn, unless the load failed.
	} else
	{
#ifdef JK2_MODE
		Com_Printf (S_COLOR_CYAN "Done.\n");
#else
		Com_Printf (S_COLOR_CYAN "%s.\n",SE_GetString("MENUS_DONE"));
#endif
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
		Com_Printf( "USAGE: save <filename>\n" );
		return;
	}


	if (svs.clients[0].frames[svs.clients[0].netchan.outgoingSequence & PACKET_MASK].ps.stats[STAT_HEALTH] <= 0)
	{
#ifdef JK2_MODE
		Com_Printf (S_COLOR_RED "\nCan't savegame while dead!\n");
#else
		Com_Printf (S_COLOR_RED "\n%s\n", SE_GetString("SP_INGAME_CANT_SAVE_DEAD"));
#endif
		return;
	}

	//this check catches deaths even the instant you die, like during a slo-mo death!
	gentity_t			*svent;
	svent = SV_GentityNum(0);
	if (svent->client->stats[STAT_HEALTH]<=0)
	{
#ifdef JK2_MODE
		Com_Printf (S_COLOR_RED "\nCan't savegame while dead!\n");
#else
		Com_Printf (S_COLOR_RED "\n%s\n", SE_GetString("SP_INGAME_CANT_SAVE_DEAD"));
#endif
		return;
	}

	const char *psFilename = Cmd_Argv(1);
	char filename[MAX_QPATH] = {0};

	Q_strncpyz(filename, psFilename, sizeof(filename));

	if (!Q_stricmp (filename, "current"))
	{
		Com_Printf (S_COLOR_RED "Can't save to 'current'\n");
		return;
	}

	if (strstr (filename, "..") || strstr (filename, "/") || strstr (filename, "\\") )
	{
		Com_Printf (S_COLOR_RED "Bad savegame name.\n");
		return;
	}

	if (!SG_GameAllowedToSaveHere(qfalse))	//full check
		return;	// this prevents people saving via quick-save now during cinematics.

#ifdef JK2_MODE
	if ( !Q_stricmp (filename, "quik*") || !Q_stricmp (filename, "auto*") )
	{
		if ( filename[4]=='*' )
			filename[4]=0;	//remove the *
		SG_StoreSaveGameComment("");	// clear previous comment/description, which will force time/date comment.
	}
#else
	if ( !Q_stricmp (filename, "auto") )
	{
		SG_StoreSaveGameComment("");	// clear previous comment/description, which will force time/date comment.
	}
#endif

#ifdef JK2_MODE
	Com_Printf (S_COLOR_CYAN "Saving game \"%s\"...\n", filename);
#else
	Com_Printf (S_COLOR_CYAN "%s \"%s\"...\n", SE_GetString("CON_TEXT_SAVING_GAME"), filename);
#endif

	if (SG_WriteSavegame(filename, qfalse))
	{
#ifdef JK2_MODE
		Com_Printf (S_COLOR_CYAN "Done.\n");
#else
		Com_Printf (S_COLOR_CYAN "%s.\n",SE_GetString("MENUS_DONE"));
#endif
	}
	else
	{
#ifdef JK2_MODE
		Com_Printf (S_COLOR_RED "Failed.\n");
#else
		Com_Printf (S_COLOR_RED "%s.\n",SE_GetString("MENUS_FAILED_TO_OPEN_SAVEGAME"));
#endif
	}
}



//---------------
static void WriteGame(qboolean autosave)
{
	SG_Append(INT_ID('G','A','M','E'), &autosave, sizeof(autosave));	

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
		SG_Append(INT_ID('C','V','S','V'), &s, sizeof(s));	

		// write ammo...
		//
		memset(s,0,sizeof(s));
		Cvar_VariableStringBuffer( "playerammo", s, sizeof(s) );
		SG_Append(INT_ID('A','M','M','O'), &s, sizeof(s));

		// write inventory...
		//
		memset(s,0,sizeof(s));
		Cvar_VariableStringBuffer( "playerinv", s, sizeof(s) );
		SG_Append(INT_ID('I','V','T','Y'), &s, sizeof(s));
		
		// the new JK2 stuff - force powers, etc...
		//
		memset(s,0,sizeof(s));
		Cvar_VariableStringBuffer( "playerfplvl", s, sizeof(s) );
		SG_Append(INT_ID('F','P','L','V'), &s, sizeof(s));
	}
}

static qboolean ReadGame (void)
{
	qboolean qbAutoSave;
	SG_Read(INT_ID('G','A','M','E'), (void *)&qbAutoSave, sizeof(qbAutoSave));

	if (qbAutoSave)
	{
		char s[MAX_STRING_CHARS]={0};

		// read health/armour etc...
		//
		memset(s,0,sizeof(s));
		SG_Read(INT_ID('C','V','S','V'), (void *)&s, sizeof(s));
		Cvar_Set( sCVARNAME_PLAYERSAVE, s );

		// read ammo...
		//
		memset(s,0,sizeof(s));			
		SG_Read(INT_ID('A','M','M','O'), (void *)&s, sizeof(s));
		Cvar_Set( "playerammo", s);

		// read inventory...
		//
		memset(s,0,sizeof(s));			
		SG_Read(INT_ID('I','V','T','Y'), (void *)&s, sizeof(s));
		Cvar_Set( "playerinv", s);

		// read force powers...
		//
		memset(s,0,sizeof(s));
		SG_Read(INT_ID('F','P','L','V'), (void *)&s, sizeof(s));
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
#ifdef JK2_MODE
		if (!(var->flags & (CVAR_SAVEGAME|CVAR_USERINFO)))
#else
		if (!(var->flags & CVAR_SAVEGAME))
#endif
		{
			continue;
		}
		iCount++;
	}

	// store count...
	//
	SG_Append(INT_ID('C','V','C','N'), &iCount, sizeof(iCount));

	// write 'em...
	//
	for (var = cvar_vars; var; var = var->next)
	{
#ifdef JK2_MODE
		if (!(var->flags & (CVAR_SAVEGAME|CVAR_USERINFO)))
#else
		if (!(var->flags & CVAR_SAVEGAME))
#endif
		{
			continue;
		}
		SG_Append(INT_ID('C','V','A','R'), var->name,   strlen(var->name) + 1);
		SG_Append(INT_ID('V','A','L','U'), var->string, strlen(var->string) + 1);
	}
}

void SG_ReadCvars(void)
{
	int		iCount;
	char	*psName;
	char	*psValue;

	SG_Read(INT_ID('C','V','C','N'), &iCount, sizeof(iCount));

	for (int i = 0; i < iCount; i++)
	{
		SG_Read(INT_ID('C','V','A','R'), NULL, 0, (void **)&psName);
		SG_Read(INT_ID('V','A','L','U'), NULL, 0, (void **)&psValue);

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

	SG_Append(INT_ID('C','S','C','N'), &iCount, sizeof(iCount));

	// now write 'em...
	//
	for (i=0; i<MAX_CONFIGSTRINGS; i++)
	{
		if (i!=CS_SYSTEMINFO)
		{
			if (sv.configstrings[i]	&& strlen(sv.configstrings[i]))
			{
				SG_Append(INT_ID('C','S','I','N'), &i, sizeof(i));
				SG_Append(INT_ID('C','S','D','A'), sv.configstrings[i], strlen(sv.configstrings[i])+1);
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

	SG_Read(INT_ID('C','S','C','N'), &iCount, sizeof(iCount));

	Com_DPrintf( "Reading %d configstrings...\n",iCount);

	for (int i = 0; i<iCount; i++)
	{
		int iIndex;
		char *psName;

		SG_Read(INT_ID('C','S','I','N'), &iIndex, sizeof(iIndex));
		SG_Read(INT_ID('C','S','D','A'), NULL, 0, (void **)&psName);

		Com_DPrintf( "Cfg str %d = %s\n",iIndex, psName);

		//sv.configstrings[iIndex] = psName;
		SV_SetConfigstring(iIndex, psName);
		Z_Free(psName);
	}
}

static unsigned int SG_UnixTimestamp ( const time_t& t )
{
	return static_cast<unsigned int>(t);
}

static void SG_WriteComment(qboolean qbAutosave, const char *psMapName)
{
	char	sComment[iSG_COMMENT_SIZE];

	if ( qbAutosave || !*saveGameComment)
	{
		Com_sprintf( sComment, sizeof(sComment), "---> %s", psMapName );
	}
	else
	{
		Q_strncpyz(sComment,saveGameComment, sizeof(sComment));
	}

	SG_Append(INT_ID('C','O','M','M'), sComment, sizeof(sComment));

	// Add Date/Time/Map stamp
	unsigned int timestamp = SG_UnixTimestamp (time (NULL));
	SG_Append(INT_ID('C','M','T','M'), &timestamp, sizeof (timestamp));

	Com_DPrintf("Saving: current (%s)\n", sComment);
}

static time_t SG_GetTime ( unsigned int timestamp )
{
	return static_cast<time_t>(timestamp);
}

// Test to see if the given file name is in the save game directory 
// then grab the comment if it's there
//
int SG_GetSaveGameComment(const char *psPathlessBaseName, char *sComment, char *sMapName)
{
	int ret = 0;
	time_t tFileTime;
#ifdef JK2_MODE
	size_t iScreenShotLength;
#endif

	qbSGReadIsTestOnly = qtrue;	// do NOT leave this in this state

	if ( !SG_Open( psPathlessBaseName ))
	{
		qbSGReadIsTestOnly = qfalse;
		return 0;
	}							

	if (SG_Read( INT_ID('C','O','M','M'), sComment, iSG_COMMENT_SIZE ))
	{	
		unsigned int fileTime = 0;
		if (SG_Read( INT_ID('C','M','T','M'), &fileTime, sizeof(fileTime)))	//read
		{
			tFileTime = SG_GetTime (fileTime);
#ifdef JK2_MODE
			if (SG_Read(INT_ID('S','H','L','N'), &iScreenShotLength, sizeof(iScreenShotLength)))
			{
				if (SG_Read(INT_ID('S','H','O','T'), NULL, iScreenShotLength, NULL))
				{
#endif
			if (SG_Read(INT_ID('M','P','C','M'), sMapName, iSG_MAPCMD_SIZE ))	// read
			{
				ret = tFileTime;
			}
#ifdef JK2_MODE
				}
			}
#endif
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
#ifdef JK2_MODE
static qboolean SG_ReadScreenshot(qboolean qbSetAsLoadingScreen, void *pvDest = NULL);
static qboolean SG_ReadScreenshot(qboolean qbSetAsLoadingScreen, void *pvDest)
{
	qboolean bReturn = qfalse;

	// get JPG screenshot data length...
	//
	size_t iScreenShotLength = 0;
	SG_Read(INT_ID('S','H','L','N'), &iScreenShotLength, sizeof(iScreenShotLength));
	//
	// alloc enough space plus extra 4K for sloppy JPG-decode reader to not do memory access violation...
	//
	byte *pJPGData = (byte *) Z_Malloc(iScreenShotLength + 4096,TAG_TEMP_WORKSPACE, qfalse);
	//
	// now read the JPG data...
	//
	SG_Read(INT_ID('S','H','O','T'), pJPGData, iScreenShotLength, 0);	
	//
	// decompress JPG data...
	//
	byte *pDecompressedPic = NULL;
	int iWidth, iHeight;
	re.LoadJPGFromBuffer(pJPGData, iScreenShotLength, &pDecompressedPic, &iWidth, &iHeight);
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
	if(!psPathlessBaseName)
	{
		return qfalse;
	}

	if (!SG_Open(psPathlessBaseName))
	{
		return qfalse;
	}
	
	SG_Read(INT_ID('C','O','M','M'), NULL, 0, NULL);	// skip
	SG_Read(INT_ID('C','M','T','M'), NULL, sizeof( unsigned int ));

	qboolean bGotSaveImage = SG_ReadScreenshot(qfalse, pvAddress);

	SG_Close();
	return bGotSaveImage;
}


static void SG_WriteScreenshot(qboolean qbAutosave, const char *psMapName)
{
	byte *pbRawScreenShot = NULL;
	byte *byBlank = NULL;

	if( qbAutosave )
	{
		// try to read a levelshot (any valid TGA/JPG etc named the same as the map)...
		//
		int iWidth = SG_SCR_WIDTH;
		int iHeight= SG_SCR_HEIGHT;
		const size_t	bySize = SG_SCR_WIDTH * SG_SCR_HEIGHT * 4;
		byte *src, *dst;

		byBlank = new byte[bySize];
		pbRawScreenShot = SCR_TempRawImage_ReadFromFile(va("levelshots/%s.tga",psMapName), &iWidth, &iHeight, byBlank, qtrue);	// qtrue = vert flip
		
		if (pbRawScreenShot)
		{
			for (int y = 0; y < iHeight; y++)
			{
				for (int x = 0; x < iWidth; x++)
				{
					src = pbRawScreenShot + 4 * (y * iWidth + x);
					dst = pbRawScreenShot + 3 * (y * iWidth + x);
					dst[0] = src[0];
					dst[1] = src[1];
					dst[2] = src[2];
				}
			}
		}
	}

	if (!pbRawScreenShot)
	{
		pbRawScreenShot = SCR_GetScreenshot(0);
	}


	size_t iJPGDataSize = 0;
	size_t bufSize = SG_SCR_WIDTH * SG_SCR_HEIGHT * 3;
	byte *pJPGData = (byte *)Z_Malloc( bufSize, TAG_TEMP_WORKSPACE, qfalse, 4 );
	iJPGDataSize = re.SaveJPGToBuffer(pJPGData, bufSize, JPEG_IMAGE_QUALITY, SG_SCR_WIDTH, SG_SCR_HEIGHT, pbRawScreenShot, 0 );
	if ( qbAutosave )
		delete[] byBlank;
	SG_Append(INT_ID('S','H','L','N'), &iJPGDataSize, sizeof(iJPGDataSize));
	SG_Append(INT_ID('S','H','O','T'), pJPGData, iJPGDataSize);
	Z_Free(pJPGData);
	SCR_TempRawImage_CleanUp();
}
#endif


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
	const char *psServerInfo = sv.configstrings[CS_SERVERINFO];
	const char *psMapName    = Info_ValueForKey( psServerInfo, "mapname" );
//JLF
#ifdef JK2_MODE
	if ( !strcmp("quik",psPathlessBaseName))
#else
	if ( !strcmp("quick",psPathlessBaseName))
#endif
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
//END JLF

	char   sMapCmd[iSG_MAPCMD_SIZE]={0};
	Q_strncpyz( sMapCmd,psMapName, sizeof(sMapCmd));	// need as array rather than ptr because const strlen needed for MPCM chunk

	SG_WriteComment(qbAutosave, sMapCmd);
#ifdef JK2_MODE
	SG_WriteScreenshot(qbAutosave, sMapCmd);
#endif
	SG_Append(INT_ID('M','P','C','M'), sMapCmd, sizeof(sMapCmd));
	SG_WriteCvars();

	WriteGame (qbAutosave);

	// Write out all the level data...
	//
	if (!qbAutosave)
	{
		SG_Append(INT_ID('T','I','M','E'), (void *)&sv.time, sizeof(sv.time));
		SG_Append(INT_ID('T','I','M','R'), (void *)&sv.timeResidual, sizeof(sv.timeResidual));
		CM_WritePortalState();
		SG_WriteServerConfigStrings();		
	}
	ge->WriteLevel(qbAutosave);	// always done now, but ent saver only does player if auto
	SG_Close();
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

#ifdef JK2_MODE
	Cvar_Set( "cg_missionstatusscreen", "0" );//reset if loading a game
#endif

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
	SG_Read(INT_ID('C','O','M','M'), sComment, sizeof(sComment));
	Com_DPrintf("Reading: %s\n", sComment);
	SG_Read( INT_ID('C','M','T','M'), NULL, sizeof( unsigned int ));

#ifdef JK2_MODE
	SG_ReadScreenshot(qtrue);	// qboolean qbSetAsLoadingScreen
#endif
	SG_Read(INT_ID('M','P','C','M'), sMapCmd, sizeof(sMapCmd));
	SG_ReadCvars();

	// read game state
	qbAutosave = ReadGame();
	eSavedGameJustLoaded = (qbAutosave)?eAUTO:eFULL;

	SV_SpawnServer(sMapCmd, eForceReload_NOTHING, (eSavedGameJustLoaded != eFULL) );	// note that this also trashes the whole G_Alloc pool as well (of course)		

	// read in all the level data...
	//
	if (!qbAutosave)
	{
		SG_Read(INT_ID('T','I','M','E'), (void *)&sv.time, sizeof(sv.time));
		SG_Read(INT_ID('T','I','M','R'), (void *)&sv.timeResidual, sizeof(sv.timeResidual));
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
			while (iIndex<iLength && iIndex-iCount<127 && (pIn[iIndex]!=pIn[iIndex-1] || (iIndex>1 && pIn[iIndex]!=pIn[iIndex-2]))){
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

//pass through function
int SG_Write(const void * chid, const int bytesize, fileHandle_t fhSaveGame)
{
		return FS_Write( chid, bytesize, fhSaveGame);
}



qboolean SG_Append(unsigned int chid, const void *pvData, int iLength)
{	
	unsigned int	uiCksum;
	unsigned int	uiSaved;
	
#ifdef _DEBUG
	int				i;
	unsigned int	*pTest;

	pTest = (unsigned int *) pvData;
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

//pass through function
int SG_ReadBytes(void * chid, int bytesize, fileHandle_t fhSaveGame)
{
	return FS_Read( chid, bytesize, fhSaveGame);
}


int SG_Seek( fileHandle_t fhSaveGame, long offset, int origin )
{
	return FS_Seek(fhSaveGame, offset, origin);
}



// Pass in pvAddress (or NULL if you want memory to be allocated)
//	if pvAddress==NULL && ppvAddressPtr == NULL then the block is discarded/skipped.
//
// If iLength==0 then it counts as a query, else it must match the size found in the file
//
// function doesn't return if error (uses ERR_DROP), unless "qbSGReadIsTestOnly == qtrue", then NZ return = success
//
static int SG_Read_Actual(unsigned int chid, void *pvAddress, int iLength, void **ppvAddressPtr, qboolean bChunkIsOptional)
{
	unsigned int	uiLoadedCksum, uiCksum;
	unsigned int	uiLoadedLength;
	unsigned int	ulLoadedChid;
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

int SG_Read(unsigned int chid, void *pvAddress, int iLength, void **ppvAddressPtr /* = NULL */)
{
	return SG_Read_Actual(chid, pvAddress, iLength, ppvAddressPtr, qfalse );	// qboolean bChunkIsOptional
}

int SG_ReadOptional(unsigned int chid, void *pvAddress, int iLength, void **ppvAddressPtr /* = NULL */)
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

