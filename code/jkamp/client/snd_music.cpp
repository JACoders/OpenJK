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

// Filename:-	snd_music.cpp
//
//  Stuff to parse in special x-fade music format and handle blending etc

#include <algorithm>
#include <string>

#include "qcommon/q_shared.h"
#include "qcommon/sstring.h"
#include "qcommon/GenericParser2.h"

#include "snd_local.h"
#include "snd_music.h"
#include "snd_ambient.h"


extern qboolean S_FileExists( const char *psFilename );

#define sKEY_MUSICFILES	"musicfiles"
#define sKEY_ENTRY		"entry"
#define sKEY_EXIT		"exit"
#define sKEY_MARKER		"marker"
#define sKEY_TIME		"time"
#define sKEY_NEXTFILE	"nextfile"
#define sKEY_NEXTMARK	"nextmark"
#define sKEY_LEVELMUSIC	"levelmusic"
#define sKEY_EXPLORE	"explore"
#define sKEY_ACTION		"action"
#define sKEY_BOSS		"boss"
#define sKEY_DEATH		"death"
#define sKEY_USES		"uses"
#define sKEY_USEBOSS	"useboss"

#define sKEY_PLACEHOLDER "placeholder"	// ignore these

#define sFILENAME_DMS	"ext_data/dms.dat"


#define MUSIC_PARSE_ERROR(_string)		Music_Parse_Error(_string)	// only use during parse, not run-time use, and bear in mid that data is zapped after error message, so exit any loops immediately
#define MUSIC_PARSE_WARNING(_string)	Music_Parse_Warning(_string)

typedef struct MusicExitPoint_s {
	sstring_t	sNextFile;
	sstring_t	sNextMark;		// blank if used for an explore piece, name of marker point to enter new file at

} MusicExitPoint_t;

struct MusicExitTime_t	// need to declare this way for operator < below
{
	float		fTime;
	int			iExitPoint;

	// I'm defining this '<' operator so STL's sort algorithm will work
	//
	bool operator < (const MusicExitTime_t& X) const {return (fTime < X.fTime);}
};

// it's possible for all 3 of these to be empty if it's boss or death music
//
typedef std::vector	<MusicExitPoint_t>	MusicExitPoints_t;
typedef std::vector	<MusicExitTime_t>	MusicExitTimes_t;
typedef std::map	<sstring_t, float>	MusicEntryTimes_t;	// key eg "marker1"

typedef struct MusicFile_s {
	sstring_t			sFileNameBase;
	MusicEntryTimes_t	MusicEntryTimes;
	MusicExitPoints_t	MusicExitPoints;
	MusicExitTimes_t	MusicExitTimes;

} MusicFile_t;

typedef std::map <sstring_t, MusicFile_t>	MusicData_t;			// string is "explore", "action", "boss" etc
										MusicData_t* MusicData = NULL;
// there are now 2 of these, because of the new "uses" keyword...
//
sstring_t	gsLevelNameForLoad;		// eg "kejim_base", formed from literal BSP name, but also used as dir name for music paths
sstring_t	gsLevelNameForCompare;	// eg "kejim_base", formed from literal BSP name, but also used as dir name for music paths
sstring_t	gsLevelNameForBossLoad;	// eg "kejim_base', special case for enabling boss music to come from a different dir - sigh....

void Music_Free(void)
{
	delete MusicData;
	MusicData = NULL;
}

// some sort of error in the music data...
//
static void Music_Parse_Error(const char *psError)
{
#ifdef FINAL_BUILD
	extern cvar_t *s_debugdynamic;
	if (s_debugdynamic && s_debugdynamic->integer)
	{
#endif
		Com_Printf(S_COLOR_RED "Error parsing music data ( in \"%s\" ):\n%s\n",sFILENAME_DMS,psError);
#ifdef FINAL_BUILD
	}
#endif
	MusicData->clear();
}

// something to just mention if interested...
//
static void Music_Parse_Warning(const char *psError)
{
#ifdef FINAL_BUILD
	extern cvar_t *s_debugdynamic;
	if (s_debugdynamic && s_debugdynamic->integer)
	{
#endif
		Com_Printf(S_COLOR_YELLOW "%s", psError);
#ifdef FINAL_BUILD
	}
#endif
}

// the 2nd param here is pretty kludgy (sigh), and only used for testing for the "boss" type.
// Unfortunately two of the places that calls this doesn't have much other access to the state other than
//	a string, not an enum, so for those cases they only pass in BOSS or EXPLORE, so don't rely on it totally.
//
static const char *Music_BuildFileName(const char *psFileNameBase, MusicState_e eMusicState )
{
	static sstring_t sFileName;

	//HACK!
	if (eMusicState == eBGRNDTRACK_DEATH)
	{
		return "music/death_music.mp3";
	}

	const char *psDirName = (eMusicState == eBGRNDTRACK_BOSS) ? gsLevelNameForBossLoad.c_str() : gsLevelNameForLoad.c_str();

	sFileName = va("music/%s/%s.mp3",psDirName,psFileNameBase);
	return sFileName.c_str();
}

// this MUST return NULL for non-base states unless doing debug-query
const char *Music_BaseStateToString( MusicState_e eMusicState, qboolean bDebugPrintQuery /* = qfalse */ )
{
	switch (eMusicState)
	{
		case eBGRNDTRACK_EXPLORE:	return "explore";
		case eBGRNDTRACK_ACTION:	return "action";
		case eBGRNDTRACK_BOSS:		return "boss";
		case eBGRNDTRACK_SILENCE:	return "silence";	// not used in this module, but snd_dma uses it now it's de-static'd
		case eBGRNDTRACK_DEATH:		return "death";

		// info only, not map<> lookup keys (unlike above)...
		//
		case eBGRNDTRACK_ACTIONTRANS0:		if (bDebugPrintQuery) return "action_tr0";
		case eBGRNDTRACK_ACTIONTRANS1:		if (bDebugPrintQuery) return "action_tr1";
		case eBGRNDTRACK_ACTIONTRANS2:		if (bDebugPrintQuery) return "action_tr2";
		case eBGRNDTRACK_ACTIONTRANS3:		if (bDebugPrintQuery) return "action_tr3";
		case eBGRNDTRACK_EXPLORETRANS0:		if (bDebugPrintQuery) return "explore_tr0";
		case eBGRNDTRACK_EXPLORETRANS1:		if (bDebugPrintQuery) return "explore_tr1";
		case eBGRNDTRACK_EXPLORETRANS2:		if (bDebugPrintQuery) return "explore_tr2";
		case eBGRNDTRACK_EXPLORETRANS3:		if (bDebugPrintQuery) return "explore_tr3";
		case eBGRNDTRACK_FADE:				if (bDebugPrintQuery) return "fade";
		default: break;
	}

	return NULL;
}

static qboolean Music_ParseMusic(CGenericParser2 &Parser, MusicData_t *MusicData, CGPGroup *pgMusicFiles, const char *psMusicName, const char *psMusicNameKey, MusicState_e eMusicState)
{
	qboolean bReturn = qfalse;

	MusicFile_t MusicFile;

	CGPGroup *pgMusicFile = pgMusicFiles->FindSubGroup(psMusicName);
	if (pgMusicFile)
	{
		// read subgroups...
		//
		qboolean bEntryFound = qfalse;
		qboolean bExitFound  = qfalse;
		//
		// (read entry points first, so I can check exit points aren't too close in time)
		//
		CGPGroup *pEntryGroup = pgMusicFile->FindSubGroup(sKEY_ENTRY);
		if (pEntryGroup)
		{
			// read entry points...
			//
			for (CGPValue *pValue = pEntryGroup->GetPairs(); pValue; pValue = pValue->GetNext())
			{
				const char *psKey	= pValue->GetName();
				const char *psValue	= pValue->GetTopValue();

				//if (!Q_strncmp(psKey,sKEY_MARKER,strlen(sKEY_MARKER)))	// for now, assume anything is a marker
				{
					MusicFile.MusicEntryTimes[psKey] = atof(psValue);
					bEntryFound = qtrue;						// harmless to keep setting
				}
			}
		}

		for (CGPGroup *pGroup = pgMusicFile->GetSubGroups(); pGroup; pGroup = pGroup->GetNext())
		{
			const char *psGroupName = pGroup->GetName();

			if (!strcmp(psGroupName,sKEY_ENTRY))
			{
				// skip entry points, I've already read them in above
				//
			}
			else
			if (!strcmp(psGroupName,sKEY_EXIT))
			{
				int iThisExitPointIndex = MusicFile.MusicExitPoints.size();	// must eval this first, so unaffected by push_back etc
				//
				// read this set of exit points...
				//
				MusicExitPoint_t MusicExitPoint;
				for (CGPValue *pValue = pGroup->GetPairs(); pValue; pValue = pValue->GetNext())
				{
					const char *psKey	= pValue->GetName();
					const char *psValue	= pValue->GetTopValue();

					if (!strcmp(psKey,sKEY_NEXTFILE))
					{
						MusicExitPoint.sNextFile = psValue;
						bExitFound  = qtrue;	// harmless to keep setting
					}
					else
					if (!strcmp(psKey,sKEY_NEXTMARK))
					{
						MusicExitPoint.sNextMark = psValue;
					}
					else
					if (!Q_strncmp(psKey,sKEY_TIME,strlen(sKEY_TIME)))
					{
						MusicExitTime_t MusicExitTime;
										MusicExitTime.fTime		= atof(psValue);
										MusicExitTime.iExitPoint= iThisExitPointIndex;

						// new check, don't keep this this exit point if it's within 1.5 seconds either way of an entry point...
						//
						qboolean bTooCloseToEntryPoint = qfalse;
						for (MusicEntryTimes_t::iterator itEntryTimes = MusicFile.MusicEntryTimes.begin(); itEntryTimes != MusicFile.MusicEntryTimes.end(); ++itEntryTimes)
						{
							float fThisEntryTime = (*itEntryTimes).second;

							if (Q_fabs(fThisEntryTime - MusicExitTime.fTime) < 1.5f)
							{
//								bTooCloseToEntryPoint = qtrue;	// not sure about this, ignore for now
								break;
							}
						}
						if (!bTooCloseToEntryPoint)
						{
							MusicFile.MusicExitTimes.push_back(MusicExitTime);
						}
					}
				}

				MusicFile.MusicExitPoints.push_back(MusicExitPoint);
				int iNumExitPoints = MusicFile.MusicExitPoints.size();

				// error checking...
				//
				switch (eMusicState)
				{
					case eBGRNDTRACK_EXPLORE:
						if (iNumExitPoints > iMAX_EXPLORE_TRANSITIONS)
						{
							MUSIC_PARSE_ERROR( va("\"%s\" has > %d %s transitions defined!\n",psMusicName,iMAX_EXPLORE_TRANSITIONS,psMusicNameKey) );
							return qfalse;
						}
						break;

					case eBGRNDTRACK_ACTION:
						if (iNumExitPoints > iMAX_ACTION_TRANSITIONS)
						{
							MUSIC_PARSE_ERROR( va("\"%s\" has > %d %s transitions defined!\n",psMusicName,iMAX_ACTION_TRANSITIONS,psMusicNameKey) );
							return qfalse;
						}
						break;

					case eBGRNDTRACK_BOSS:
					case eBGRNDTRACK_DEATH:

						MUSIC_PARSE_ERROR( va("\"%s\" has %s transitions defined, this is not allowed!\n",psMusicName,psMusicNameKey) );
						break;

					default:
						break;
				}
			}
		}

		// for now, assume everything was ok unless some obvious things are missing...
		//
		bReturn = qtrue;

		if (eMusicState != eBGRNDTRACK_BOSS && eMusicState != eBGRNDTRACK_DEATH)	// boss & death pieces can omit entry/exit stuff
		{
			if (!bEntryFound)
			{
				MUSIC_PARSE_ERROR(va("Unable to find subgroup \"%s\" in group \"%s\"\n",sKEY_ENTRY,psMusicName));
				bReturn = qfalse;
			}
			if (!bExitFound)
			{
				MUSIC_PARSE_ERROR(va("Unable to find subgroup \"%s\" in group \"%s\"\n",sKEY_EXIT,psMusicName));
				bReturn = qfalse;
			}
		}
	}
	else
	{
		MUSIC_PARSE_ERROR(va("Unable to find musicfiles entry \"%s\"\n",psMusicName));
	}

	if (bReturn)
	{
		MusicFile.sFileNameBase  = psMusicName;
		(*MusicData)[ psMusicNameKey ] = MusicFile;
	}

	return bReturn;
}




// I only need this because GP2 can't cope with trailing whitespace (for !@#$%^'s sake!!!!)...
//
// (output buffer will always be just '\n' seperated, regardless of possible "\r\n" pairs)
//
// (remember to Z_Free() the returned char * when done with it!!!)
//
static char *StripTrailingWhiteSpaceOnEveryLine(char *pText)
{
	std::string strNewText;

	while (*pText)
	{
		char sOneLine[1024];	// BTO: was 16k

		// find end of line...
		//
		char *pThisLineEnd = pText;
		while (*pThisLineEnd && *pThisLineEnd != '\r' && ((unsigned)(pThisLineEnd-pText) < sizeof(sOneLine)-1))
		{
			pThisLineEnd++;
		}

		unsigned int iCharsToCopy = pThisLineEnd - pText;
		strncpy(sOneLine, pText, iCharsToCopy);
				sOneLine[iCharsToCopy]='\0';
		pText += iCharsToCopy;
		while (*pText == '\n' || *pText == '\r') pText++;

		// trim trailing...
		//
		qboolean bTrimmed = qfalse;
		do
		{
			bTrimmed = qfalse;
			int iStrLen = strlen(sOneLine);

			if (iStrLen)
			{
				if (sOneLine[iStrLen-1] == '\t' || sOneLine[iStrLen-1] == ' ')
				{
					sOneLine[iStrLen-1] = '\0';
					bTrimmed = qtrue;
				}
			}
		}
		while (bTrimmed);

		strNewText += sOneLine;
		strNewText += "\n";
	}

	char  *pNewText = (char *) Z_Malloc( strlen(strNewText.c_str())+1, TAG_TEMP_WORKSPACE, qfalse);
	strcpy(pNewText, strNewText.c_str());
	return pNewText;
}


// called from SV_SpawnServer, but before map load and music start etc.
//
// This just initialises the Lucas music structs so the background music player can interrogate them...
//
sstring_t gsLevelNameFromServer;
void Music_SetLevelName(const char *psLevelName)
{
	gsLevelNameFromServer = psLevelName;
}

static qboolean Music_ParseLeveldata(const char *psLevelName)
{
	qboolean bReturn = qfalse;

	if (MusicData == NULL)
	{
		MusicData = new MusicData_t;
	}

		// already got this data?
	//
	if (MusicData->size() && !Q_stricmp(psLevelName,gsLevelNameForCompare.c_str()))
	{
		return qtrue;
	}

	MusicData->clear();

	char sLevelName[MAX_QPATH];
	Q_strncpyz(sLevelName,psLevelName,sizeof(sLevelName));

	gsLevelNameForLoad		= sLevelName;	// harmless to init here even if we fail to parse dms.dat file
	gsLevelNameForCompare	= sLevelName;	// harmless to init here even if we fail to parse dms.dat file
	gsLevelNameForBossLoad	= sLevelName;	// harmless to init here even if we fail to parse dms.dat file

	char *pText = NULL;
	/*int iTotalBytesLoaded = */FS_ReadFile(sFILENAME_DMS, (void **)&pText );
	if (pText)
	{
		char *psStrippedText = StripTrailingWhiteSpaceOnEveryLine(pText);
		CGenericParser2 Parser;
		char *psDataPtr = psStrippedText;	// because ptr gets advanced, so we supply a clone that GP can alter
		if (Parser.Parse(&psDataPtr, true))
		{
			CGPGroup *pFileGroup = Parser.GetBaseParseGroup();
			if (pFileGroup)
			{
				CGPGroup *pgMusicFiles = pFileGroup->FindSubGroup(sKEY_MUSICFILES);
				if (pgMusicFiles)
				{
					CGPGroup *pgLevelMusic = pFileGroup->FindSubGroup(sKEY_LEVELMUSIC);

					if (pgLevelMusic)
					{
						CGPGroup *pgThisLevelMusic = NULL;
						//
						// check for new USE keyword...
						//
						int iSanityLimit = 0;
						sstring_t sSearchName(sLevelName);

						while (sSearchName.c_str()[0] && iSanityLimit < 10)
						{
							gsLevelNameForLoad		= sSearchName;
							gsLevelNameForBossLoad	= sSearchName;
							pgThisLevelMusic = pgLevelMusic->FindSubGroup(sSearchName.c_str());

							if (pgThisLevelMusic)
							{
								CGPValue *pValue = pgThisLevelMusic->FindPair(sKEY_USES);
								if (pValue)
								{
									// re-search using the USE param...
									//
									sSearchName = pValue->GetTopValue();
									iSanityLimit++;
//									Com_DPrintf("Using \"%s\"\n",sSearchName.c_str());
								}
								else
								{
									// no new USE keyword found...
									//
									sSearchName = "";
								}
							}
							else
							{
								// level entry not found...
								//
								break;
							}
						}

						// now go ahead and use the final music set we've decided on...
						//
						if (pgThisLevelMusic && iSanityLimit < 10)
						{
							// these are optional fields, so see which ones we find...
							//
							const char *psName_Explore = NULL;
							const char *psName_Action  = NULL;
							const char *psName_Boss	  = NULL;
							//const char *psName_Death	  = NULL;
							//
							const char *psName_UseBoss = NULL;

							for (CGPValue *pValue = pgThisLevelMusic->GetPairs(); pValue; pValue = pValue->GetNext())
							{
								const char *psKey	= pValue->GetName();
								const char *psValue	= pValue->GetTopValue();

								if (Q_stricmp(psValue,sKEY_PLACEHOLDER))	// ignore "placeholder" items
								{
									if (!Q_stricmp(psKey,sKEY_EXPLORE))
									{
										psName_Explore = psValue;
									}
									else
									if (!Q_stricmp(psKey,sKEY_ACTION))
									{
										psName_Action  = psValue;
									}
									else
									if (!Q_stricmp(psKey,sKEY_USEBOSS))
									{
										psName_UseBoss = psValue;
									}
									else
									if (!Q_stricmp(psKey,sKEY_BOSS))
									{
										psName_Boss = psValue;
									}
									/*else
									if (!Q_stricmp(psKey,sKEY_DEATH))
									{
										psName_Death = psValue;
									}*/
								}
							}

							bReturn = qtrue;	// defualt to ON now, so I can turn it off if "useboss" fails

							if (psName_UseBoss)
							{
								CGPGroup *pgLevelMusicOfBoss = pgLevelMusic->FindSubGroup(psName_UseBoss);
								if (pgLevelMusicOfBoss)
								{
									CGPValue *pValueBoss = pgLevelMusicOfBoss->FindPair(sKEY_BOSS);
									if (pValueBoss)
									{
										psName_Boss = pValueBoss->GetTopValue();
										gsLevelNameForBossLoad = psName_UseBoss;
									}
									else
									{
										MUSIC_PARSE_ERROR(va("'useboss' \"%s\" has no \"boss\" entry!\n",psName_UseBoss));
										bReturn = qfalse;
									}
								}
								else
								{
									MUSIC_PARSE_ERROR(va("Unable to find 'useboss' entry \"%s\"\n",psName_UseBoss));
									bReturn = qfalse;
								}
							}


							// done this way in case I want to conditionally pass any bools depending on music type...
							//
							if (bReturn && psName_Explore)
							{
								bReturn = Music_ParseMusic(Parser, MusicData, pgMusicFiles, psName_Explore,	sKEY_EXPLORE, eBGRNDTRACK_EXPLORE);
							}
							if (bReturn && psName_Action)
							{
								bReturn = Music_ParseMusic(Parser, MusicData, pgMusicFiles, psName_Action,	sKEY_ACTION,  eBGRNDTRACK_ACTION);
							}
							if (bReturn && psName_Boss)
							{
								bReturn = Music_ParseMusic(Parser, MusicData, pgMusicFiles, psName_Boss,	sKEY_BOSS,    eBGRNDTRACK_BOSS);
							}
							if (bReturn /*&& psName_Death*/)	// LAST MINUTE HACK!!, always force in some death music!!!!
							{
								//bReturn = Music_ParseMusic(Parser, MusicData, pgMusicFiles, psName_Death,	sKEY_DEATH,   eBGRNDTRACK_DEATH);

								MusicFile_t m;
											m.sFileNameBase = "death_music";
								(*MusicData)[ sKEY_DEATH ] = m;
							}
						}
						else
						{
							MUSIC_PARSE_WARNING(va("Unable to find entry for \"%s\" in \"%s\"\n",sLevelName,sFILENAME_DMS));
						}
					}
					else
					{
						MUSIC_PARSE_ERROR(va("Unable to find subgroup \"%s\"\n",sKEY_LEVELMUSIC));
					}
				}
				else
				{
					MUSIC_PARSE_ERROR(va("Unable to find subgroup \"%s\"\n",sKEY_MUSICFILES));
				}
			}
			else
			{
				MUSIC_PARSE_ERROR( "Error calling GP2.GetBaseParseGroup()\n" );
			}
		}
		else
		{
			MUSIC_PARSE_ERROR( "Error using GP to parse file\n" );
		}

		Z_Free(psStrippedText);
		FS_FreeFile( pText );
	}
	else
	{
		MUSIC_PARSE_ERROR( "Unable to even read main file\n" );	// file name specified in error message
	}

	if (bReturn)
	{
		// sort exit points, and do some error checking...
		//
		for (MusicData_t::iterator itMusicData = MusicData->begin(); itMusicData != MusicData->end(); ++itMusicData)
		{
			const char *psMusicStateType	= (*itMusicData).first.c_str();
			MusicFile_t &MusicFile	= (*itMusicData).second;

			// kludge up an enum, only interested in boss or not at the moment, so...
			//
			MusicState_e eMusicState = !Q_stricmp(psMusicStateType,"boss") ? eBGRNDTRACK_BOSS : !Q_stricmp(psMusicStateType,"death") ? eBGRNDTRACK_DEATH : eBGRNDTRACK_EXPLORE;

			if (!MusicFile.MusicExitTimes.empty())
			{
				sort(MusicFile.MusicExitTimes.begin(),MusicFile.MusicExitTimes.end());
			}

			// check music exists...
			//
			const char *psMusicFileName = Music_BuildFileName( MusicFile.sFileNameBase.c_str(), eMusicState );
			if (!S_FileExists( psMusicFileName ))
			{
				MUSIC_PARSE_ERROR(va("Music file \"%s\" not found!\n",psMusicFileName));
				return qfalse;		// have to return, because music data destroyed now
			}

			// check all transition music pieces exist, and that entry points into new pieces after transitions also exist...
			//
			for (size_t iExitPoint=0; iExitPoint < MusicFile.MusicExitPoints.size(); iExitPoint++)
			{
				MusicExitPoint_t &MusicExitPoint = MusicFile.MusicExitPoints[ iExitPoint ];

				const char *psTransitionFileName = Music_BuildFileName( MusicExitPoint.sNextFile.c_str(), eMusicState );
				if (!S_FileExists( psTransitionFileName ))
				{
					MUSIC_PARSE_ERROR(va("Transition file \"%s\" (entry \"%s\" ) not found!\n",psTransitionFileName, MusicExitPoint.sNextFile.c_str()));
					return qfalse;		// have to return, because music data destroyed now
				}

				const char *psNextMark = MusicExitPoint.sNextMark.c_str();
				if (strlen(psNextMark))	// always NZ ptr
				{
					// then this must be "action" music under current rules...
					//
					assert( !strcmp(psMusicStateType, Music_BaseStateToString(eBGRNDTRACK_ACTION) ? Music_BaseStateToString(eBGRNDTRACK_ACTION):"") );
					//
					// does this marker exist in the explore piece?
					//
					MusicData_t::iterator itExploreMusicData = MusicData->find( Music_BaseStateToString(eBGRNDTRACK_EXPLORE) );
					if (itExploreMusicData != MusicData->end())
					{
						MusicFile_t &MusicFile_Explore = (*itExploreMusicData).second;

						if (!MusicFile_Explore.MusicEntryTimes.count(psNextMark))
						{
							MUSIC_PARSE_ERROR( va("Unable to find entry point \"%s\" in description for \"%s\"\n",psNextMark,MusicFile_Explore.sFileNameBase.c_str()) );
							return qfalse;		// have to return, because music data destroyed now
						}
					}
					else
					{
						MUSIC_PARSE_ERROR( va("Unable to find %s piece to match \"%s\"\n", Music_BaseStateToString(eBGRNDTRACK_EXPLORE), MusicFile.sFileNameBase.c_str() ) );
						return qfalse;		// have to return, because music data destroyed now
					}
				}
			}
		}
	}

#ifdef _DEBUG
/*
	// dump the whole thing out to prove it was read in ok...
	//
	if (bReturn)
	{
		for (MusicData_t::iterator itMusicData = MusicData->begin(); itMusicData != MusicData->end(); ++itMusicData)
		{
			const char *psMusicState		= (*itMusicData).first.c_str();
			MusicFile_t &MusicFile	= (*itMusicData).second;

			Com_OPrintf("Music State:  \"%s\",  File: \"%s\"\n",psMusicState, MusicFile.sFileNameBase.c_str());

			// entry times...
			//
			for (MusicEntryTimes_t::iterator itEntryTimes = MusicFile.MusicEntryTimes.begin(); itEntryTimes != MusicFile.MusicEntryTimes.end(); ++itEntryTimes)
			{
				const char *psMarkerName	= (*itEntryTimes).first.c_str();
				float	fEntryTime		= (*itEntryTimes).second;

				Com_OPrintf("Entry time for \"%s\": %f\n", psMarkerName, fEntryTime);
			}

			// exit points...
			//
			for (int i=0; i<MusicFile.MusicExitPoints.size(); i++)
			{
				MusicExitPoint_t &MusicExitPoint = MusicFile.MusicExitPoints[i];

				Com_OPrintf("Exit point %d:	sNextFile: \"%s\", sNextMark: \"%s\"\n",i,MusicExitPoint.sNextFile.c_str(),MusicExitPoint.sNextMark.c_str());
			}

			// exit times...
			//
			for (i=0; i<MusicFile.MusicExitTimes.size(); i++)
			{
				MusicExitTime_t &MusicExitTime = MusicFile.MusicExitTimes[i];

				Com_OPrintf("Exit time %d:		fTime: %f, iExitPoint: %d\n",i,MusicExitTime.fTime,MusicExitTime.iExitPoint);
			}
		}
	}
*/
#endif

	return bReturn;
}


// returns ptr to music file, or NULL for error/missing...
//
static MusicFile_t *Music_GetBaseMusicFile( const char *psMusicState )	// where psMusicState is (eg) "explore", "action" or "boss"
{
	MusicData_t::iterator it = MusicData->find( psMusicState );
	if (it != MusicData->end())
	{
		MusicFile_t *pMusicFile = &(*it).second;
		return pMusicFile;
	}

	return NULL;
}

static MusicFile_t *Music_GetBaseMusicFile( MusicState_e eMusicState )
{
	const char *psMusicStateString = Music_BaseStateToString( eMusicState );
	if ( psMusicStateString )
	{
		return Music_GetBaseMusicFile( psMusicStateString );
	}

	return NULL;
}


// where label is (eg) "kejim_base"...
//
qboolean Music_DynamicDataAvailable(const char *psDynamicMusicLabel)
{
	char sLevelName[MAX_QPATH];
	Q_strncpyz(sLevelName,COM_SkipPath( const_cast<char*>( (psDynamicMusicLabel&&psDynamicMusicLabel[0])?psDynamicMusicLabel:gsLevelNameFromServer.c_str() ) ),sizeof(sLevelName));

	std::string s = sLevelName;
	std::transform(s.begin(), s.end(), s.begin(), ::tolower);

	if (strlen(sLevelName))	// avoid error messages when there's no music waiting to be played and we try and restart it...
	{
		if (Music_ParseLeveldata(sLevelName))
		{
			return (qboolean)(!!(Music_GetBaseMusicFile (eBGRNDTRACK_EXPLORE) && Music_GetBaseMusicFile (eBGRNDTRACK_ACTION)));
		}
	}

	return qfalse;
}

const char *Music_GetFileNameForState( MusicState_e eMusicState)
{
	MusicFile_t *pMusicFile = NULL;
	switch (eMusicState)
	{
		case eBGRNDTRACK_EXPLORE:
		case eBGRNDTRACK_ACTION:
		case eBGRNDTRACK_BOSS:
		case eBGRNDTRACK_DEATH:

			pMusicFile = Music_GetBaseMusicFile( eMusicState );
			if (pMusicFile)
			{
				return Music_BuildFileName( pMusicFile->sFileNameBase.c_str(), eMusicState );
			}
			break;

		case eBGRNDTRACK_ACTIONTRANS0:
		case eBGRNDTRACK_ACTIONTRANS1:
		case eBGRNDTRACK_ACTIONTRANS2:
		case eBGRNDTRACK_ACTIONTRANS3:

			pMusicFile = Music_GetBaseMusicFile( eBGRNDTRACK_ACTION );
			if (pMusicFile)
			{
				unsigned int iTransNum = eMusicState - eBGRNDTRACK_ACTIONTRANS0;
				if (iTransNum < pMusicFile->MusicExitPoints.size())
				{
					return Music_BuildFileName( pMusicFile->MusicExitPoints[iTransNum].sNextFile.c_str(), eMusicState );
				}
			}
			break;

		case eBGRNDTRACK_EXPLORETRANS0:
		case eBGRNDTRACK_EXPLORETRANS1:
		case eBGRNDTRACK_EXPLORETRANS2:
		case eBGRNDTRACK_EXPLORETRANS3:

			pMusicFile = Music_GetBaseMusicFile( eBGRNDTRACK_EXPLORE );
			if (pMusicFile)
			{
				unsigned int iTransNum = eMusicState - eBGRNDTRACK_EXPLORETRANS0;
				if (iTransNum < pMusicFile->MusicExitPoints.size())
				{
					return Music_BuildFileName( pMusicFile->MusicExitPoints[iTransNum].sNextFile.c_str(), eMusicState );
				}
			}
			break;

		default:
			#ifndef FINAL_BUILD
			assert(0);	// duh....what state are they asking for?
			Com_Printf( S_COLOR_RED "Music_GetFileNameForState( %d ) unhandled case!\n",eMusicState );
			#endif
			break;
	}

	return NULL;
}



qboolean Music_StateIsTransition( MusicState_e eMusicState )
{
	return (qboolean)(eMusicState >= eBGRNDTRACK_FIRSTTRANSITION && eMusicState <= eBGRNDTRACK_LASTTRANSITION);
}


qboolean Music_StateCanBeInterrupted( MusicState_e eMusicState, MusicState_e eProposedMusicState )
{
	// death music can interrupt anything...
	//
	if (eProposedMusicState == eBGRNDTRACK_DEATH)
		return qtrue;
	//
	// ... and can't be interrupted once started...(though it will internally-switch to silence at the end, rather than loop)
	//
	if (eMusicState == eBGRNDTRACK_DEATH)
	{
		return qfalse;
	}

	// boss music can interrupt anything (other than death, but that's already handled above)...
	//
	if (eProposedMusicState == eBGRNDTRACK_BOSS)
		return qtrue;
	//
	// ... and can't be interrupted once started...
	//
	if (eMusicState == eBGRNDTRACK_BOSS)
	{
		// ...except by silence (or death, but again, that's already handled above)
		//
		if (eProposedMusicState == eBGRNDTRACK_SILENCE)
			return qtrue;

		return qfalse;
	}

	// action music can interrupt anything (after boss & death filters above)...
	//
	if (eProposedMusicState == eBGRNDTRACK_ACTION)
		return qtrue;

	// nothing can interrupt a transition (after above filters)...
	//
	if (Music_StateIsTransition( eMusicState ))
		return qfalse;

	// current state is therefore interruptable...
	//
	return qtrue;
}



// returns qtrue if music is allowed to transition out of current state, based on current play position...
// (doesn't bother returning final state after transition (eg action->transition->explore) becuase it's fairly obvious)
//
// supply:
//
// playing point in float seconds
// enum of track being queried
//
// get:
//
// enum of transition track to switch to
// float time of entry point of new track *after* transition
//
qboolean Music_AllowedToTransition( float			fPlayingTimeElapsed,
									MusicState_e	eMusicState,
									//
									MusicState_e	*peTransition /* = NULL */,
									float			*pfNewTrackEntryTime /* = NULL */
									)
{
	const float fTimeEpsilon = 0.3f;	// arb., how close we have to be to an exit point to take it.
										//		if set too high then music change is sloppy
										//		if set too low[/precise] then we might miss an exit if client fps is poor


	MusicFile_t *pMusicFile = Music_GetBaseMusicFile( eMusicState );
	if (pMusicFile && !pMusicFile->MusicExitTimes.empty())
	{
		MusicExitTime_t	T;
						T.fTime = fPlayingTimeElapsed;

		// since a MusicExitTimes_t item is a sorted array, we can use the equal_range algorithm...
		//
		std::pair <MusicExitTimes_t::iterator, MusicExitTimes_t::iterator> itp = equal_range( pMusicFile->MusicExitTimes.begin(), pMusicFile->MusicExitTimes.end(), T);
		if (itp.first != pMusicFile->MusicExitTimes.begin())
			itp.first--;	// encompass the one before, in case we've just missed an exit point by < fTimeEpsilon
		if (itp.second!= pMusicFile->MusicExitTimes.end())
			itp.second++;	// increase range to one beyond, so we can do normal STL being/end looping below
		for (MusicExitTimes_t::iterator it = itp.first; it != itp.second; ++it)
		{
			MusicExitTimes_t::iterator pExitTime = it;

			if ( Q_fabs(pExitTime->fTime - fPlayingTimeElapsed) <= fTimeEpsilon )
			{
				// got an exit point!, work out feedback params...
				//
				int iExitPoint = pExitTime->iExitPoint;
				//
				// the two params to give back...
				//
				MusicState_e	eFeedBackTransition			= eBGRNDTRACK_EXPLORETRANS0;	// any old default
				float			fFeedBackNewTrackEntryTime	= 0.0f;
				//
				// check legality in case of crap data...
				//
				if ((unsigned)iExitPoint < pMusicFile->MusicExitPoints.size())
				{
					MusicExitPoint_t &ExitPoint = pMusicFile->MusicExitPoints[ iExitPoint ];

					switch (eMusicState)
					{
						case eBGRNDTRACK_EXPLORE:
						{
							assert(iExitPoint < iMAX_EXPLORE_TRANSITIONS);	// already been checked, but sanity
							assert(!ExitPoint.sNextMark.c_str()[0]);		// simple error checking, but harmless if tripped. explore transitions go to silence, hence no entry time for [silence] state after transition

							eFeedBackTransition = (MusicState_e) (eBGRNDTRACK_EXPLORETRANS0 + iExitPoint);
						}
						break;

						case eBGRNDTRACK_ACTION:
						{
							assert(iExitPoint < iMAX_ACTION_TRANSITIONS);	// already been checked, but sanity

							// if there's an entry marker point defined...
							//
							if (ExitPoint.sNextMark.c_str()[0])
							{
								MusicData_t::iterator itExploreMusicData = MusicData->find( Music_BaseStateToString(eBGRNDTRACK_EXPLORE) );
								//
								// find "explore" music...
								//
								if (itExploreMusicData != MusicData->end())
								{
									MusicFile_t &MusicFile_Explore = (*itExploreMusicData).second;
									//
									// find the entry marker within the music and read the time there...
									//
									MusicEntryTimes_t::iterator itEntryTime = MusicFile_Explore.MusicEntryTimes.find( ExitPoint.sNextMark.c_str() );
									if (itEntryTime != MusicFile_Explore.MusicEntryTimes.end())
									{
										fFeedBackNewTrackEntryTime = (*itEntryTime).second;
										eFeedBackTransition = (MusicState_e) (eBGRNDTRACK_ACTIONTRANS0 + iExitPoint);
									}
									else
									{
										#ifndef FINAL_BUILD
										assert(0);	// sanity, should have been caught elsewhere, but harmless to do this
										Com_Printf( S_COLOR_RED "Music_AllowedToTransition() unable to find entry marker \"%s\" in \"%s\"",ExitPoint.sNextMark.c_str(), MusicFile_Explore.sFileNameBase.c_str());
										#endif
										return qfalse;
									}
								}
								else
								{
									#ifndef FINAL_BUILD
									assert(0);	// sanity, should have been caught elsewhere, but harmless to do this
									Com_Printf( S_COLOR_RED "Music_AllowedToTransition() unable to find %s version of \"%s\"\n",Music_BaseStateToString(eBGRNDTRACK_EXPLORE), pMusicFile->sFileNameBase.c_str());
									#endif
									return qfalse;
								}
							}
							else
							{
								eFeedBackTransition			= eBGRNDTRACK_ACTIONTRANS0;
								fFeedBackNewTrackEntryTime	= 0.0f;		// already set to this, but FYI
							}
						}
						break;

						default:
						{
							#ifndef FINAL_BUILD
							assert(0);
							Com_Printf( S_COLOR_RED "Music_AllowedToTransition(): No code to transition from music type %d\n",eMusicState);
							#endif
							return qfalse;
						}
						break;
					}
				}
				else
				{
					#ifndef FINAL_BUILD
					assert(0);
					Com_Printf( S_COLOR_RED "Music_AllowedToTransition(): Illegal exit point %d, max = %d (music: \"%s\")\n",iExitPoint, pMusicFile->MusicExitPoints.size()-1, pMusicFile->sFileNameBase.c_str() );
					#endif
					return qfalse;
				}


				// feed back answers...
				//
				if ( peTransition)
				{
					*peTransition = eFeedBackTransition;
				}

				if ( pfNewTrackEntryTime )
				{
					*pfNewTrackEntryTime = fFeedBackNewTrackEntryTime;
				}

				return qtrue;
			}
		}
	}

	return qfalse;
}


// typically used to get a (predefined) random entry point for the action music, but will work on any defined type with entry points,
//	defaults safely to 0.0f if no info available...
//
float Music_GetRandomEntryTime( MusicState_e eMusicState )
{
	MusicData_t::iterator itMusicData = MusicData->find( Music_BaseStateToString( eMusicState ) );
	if (itMusicData != MusicData->end())
	{
		MusicFile_t &MusicFile = (*itMusicData).second;

		if (MusicFile.MusicEntryTimes.size())	// make sure at least one defined, else default to start
		{
			// Quake's random number generator isn't very good, so instead of this:
			//
			// int iRandomEntryNum = Q_irand(0, (MusicFile.MusicEntryTimes.size()-1) );
			//
			// ... I'll do this (ensuring we don't get the same result on two consecutive calls, but without while-loop)...
			//
			static int	iPrevRandomNumber = -1;
			static int	iCallCount = 0;
						iCallCount++;
			int iRandomEntryNum = (rand()+iCallCount) % (MusicFile.MusicEntryTimes.size());	// legal range
			if (iRandomEntryNum == iPrevRandomNumber && MusicFile.MusicEntryTimes.size()>1)
			{
				iRandomEntryNum += 1;
				iRandomEntryNum %= (MusicFile.MusicEntryTimes.size());
			}
			iPrevRandomNumber = iRandomEntryNum;

//			Com_OPrintf("Music_GetRandomEntryTime(): Entry %d\n",iRandomEntryNum);

			for (MusicEntryTimes_t::iterator itEntryTime = MusicFile.MusicEntryTimes.begin(); itEntryTime != MusicFile.MusicEntryTimes.end(); ++itEntryTime)
			{
				if (!iRandomEntryNum--)
				{
					return (*itEntryTime).second;
				}
			}
		}
	}

	return 0.0f;
}

// info only, used in "soundinfo" command...
//
const char *Music_GetLevelSetName(void)
{
	if (Q_stricmp(gsLevelNameForCompare.c_str(), gsLevelNameForLoad.c_str()))
	{
		// music remap via USES command...
		//
		return va("%s -> %s",gsLevelNameForCompare.c_str(), gsLevelNameForLoad.c_str());
	}

	return gsLevelNameForLoad.c_str();
}

///////////////// eof /////////////////////

