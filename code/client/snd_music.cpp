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

//Anything above this #include will be ignored by the compiler
#include "../server/exe_headers.h"

#include "../qcommon/sstring.h"
#include <algorithm>
#include <string>
#include <sstream>

#include "snd_local.h"
#include "cl_mp3.h"

//
#include "snd_music.h"

#include "../game/genericparser2.h"

extern qboolean S_FileExists( const char *psFilename );

#define sKEY_MUSICFILES	CSTRING_VIEW( "musicfiles" )
#define sKEY_ENTRY		CSTRING_VIEW( "entry" )
#define sKEY_EXIT		CSTRING_VIEW( "exit" )
#define sKEY_MARKER		CSTRING_VIEW( "marker" )
#define sKEY_TIME		CSTRING_VIEW( "time" )
#define sKEY_NEXTFILE	CSTRING_VIEW( "nextfile" )
#define sKEY_NEXTMARK	CSTRING_VIEW( "nextmark" )
#define sKEY_LEVELMUSIC	CSTRING_VIEW( "levelmusic" )
#define sKEY_EXPLORE	CSTRING_VIEW( "explore" )
#define sKEY_ACTION		CSTRING_VIEW( "action" )
#define sKEY_BOSS		CSTRING_VIEW( "boss" )
#define sKEY_DEATH		CSTRING_VIEW( "death" )
#define sKEY_USES		CSTRING_VIEW( "uses" )
#define sKEY_USEBOSS	CSTRING_VIEW( "useboss" )

#define sKEY_PLACEHOLDER "placeholder"	// ignore these

#define sFILENAME_DMS	"ext_data/dms.dat"

typedef struct
{
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

typedef struct
{
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
	if (MusicData)
	{
		MusicData->clear();
	}
	MusicData = NULL;
}

namespace detail
{
	static void build_string( std::ostream& stream )
	{
	}

	template< typename T, typename... Tail >
	static void build_string( std::ostream& stream, const T& head, Tail... tail )
	{
		stream << head;
		build_string( stream, tail... );
	}
}

template< typename... Tail >
static std::string build_string( Tail... tail )
{
	std::ostringstream os;
	detail::build_string( os, tail... );
	return os.str();
}

// some sort of error in the music data...
// only use during parse, not run-time use, and bear in mid that data is zapped after error message, so exit any loops immediately
//
static void Music_Parse_Error( gsl::czstring filename, const std::string& error )
{
	std::string message = build_string(
		S_COLOR_RED "Error parsing music data (in \"", filename, "\"):\n",
		error , "\n"
		);
	Com_Printf( "%s", message.c_str() );
	MusicData->clear();
}

// something to just mention if interested...
//
static void Music_Parse_Warning( const std::string& error )
{
	extern cvar_t *s_debugdynamic;
	if( s_debugdynamic && s_debugdynamic->integer )
	{
		Com_Printf( S_COLOR_YELLOW "%s", error.c_str() );
	}
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

static qboolean Music_ParseMusic( gsl::czstring filename, const CGenericParser2& Parser, MusicData_t* MusicData, const CGPGroup& pgMusicFiles, const gsl::cstring_span& psMusicName, const gsl::cstring_span& psMusicNameKey, MusicState_e eMusicState )
{
	bool bReturn = false;
	MusicFile_t MusicFile;

	const CGPGroup* const pgMusicFile = pgMusicFiles.FindSubGroup( psMusicName );
	if( pgMusicFile )
	{
		// read subgroups...
		//
		bool bEntryFound = false;
		bool bExitFound = false;
		//
		// (read entry points first, so I can check exit points aren't too close in time)
		//
		const CGPGroup* pEntryGroup = pgMusicFile->FindSubGroup( sKEY_ENTRY );
		if( pEntryGroup )
		{
			// read entry points...
			//
			for( auto& prop : pEntryGroup->GetProperties() )
			{
				//if( Q::substr( prop.GetName(), 0, sKEY_MARKER.size() ) == sKEY_MARKER )	// for now, assume anything is a marker
				{
					MusicFile.MusicEntryTimes[ prop.GetName() ] = Q::svtoi( prop.GetTopValue() );
					bEntryFound = true;
				}
			}
		}

		for( auto& group : pgMusicFile->GetSubGroups() )
		{
			auto& groupName = group.GetName();

			if( groupName == sKEY_ENTRY )
			{
				// skip entry points, I've already read them in above
				//
			}
			else if( groupName == sKEY_EXIT )
			{
				int iThisExitPointIndex = MusicFile.MusicExitPoints.size();	// must eval this first, so unaffected by push_back etc
				//
				// read this set of exit points...
				//
				MusicExitPoint_t MusicExitPoint;
				for( auto& prop : group.GetProperties() )
				{
					auto& key = prop.GetName();
					auto& value = prop.GetTopValue();

					if( key == sKEY_NEXTFILE )
					{
						MusicExitPoint.sNextFile = value;
						bExitFound = true;	// harmless to keep setting
					}
					else if( key == sKEY_NEXTMARK )
					{
						MusicExitPoint.sNextMark = value;
					}
					else if( Q::substr( key, 0, sKEY_TIME.size() ) == sKEY_TIME )
					{
						MusicExitTime_t MusicExitTime;
						MusicExitTime.fTime = Q::svtof( value );
						MusicExitTime.iExitPoint = iThisExitPointIndex;

						// new check, don't keep this this exit point if it's within 1.5 seconds either way of an entry point...
						//
						bool bTooCloseToEntryPoint = false;
						for( auto& item : MusicFile.MusicEntryTimes )
						{
							float fThisEntryTime = item.second;

							if( Q_fabs( fThisEntryTime - MusicExitTime.fTime ) < 1.5f )
							{
								//								bTooCloseToEntryPoint = true;	// not sure about this, ignore for now
								break;
							}
						}
						if( !bTooCloseToEntryPoint )
						{
							MusicFile.MusicExitTimes.push_back( MusicExitTime );
						}
					}
				}

				MusicFile.MusicExitPoints.push_back( MusicExitPoint );
				int iNumExitPoints = MusicFile.MusicExitPoints.size();

				// error checking...
				//
				switch( eMusicState )
				{
				case eBGRNDTRACK_EXPLORE:
					if( iNumExitPoints > iMAX_EXPLORE_TRANSITIONS )
					{
						Music_Parse_Error( filename, build_string( "\"", psMusicName, "\" has > ", iMAX_EXPLORE_TRANSITIONS, " ", psMusicNameKey, " transitions defined!\n" ) );
						return qfalse;
					}
					break;

				case eBGRNDTRACK_ACTION:
					if( iNumExitPoints > iMAX_ACTION_TRANSITIONS )
					{
						Music_Parse_Error( filename, build_string( "\"", psMusicName, "\" has > ", iMAX_ACTION_TRANSITIONS, " ", psMusicNameKey, " transitions defined!\n" ) );
						return qfalse;
					}
					break;

				case eBGRNDTRACK_BOSS:
				case eBGRNDTRACK_DEATH:

					Music_Parse_Error( filename, build_string( "\"", psMusicName, "\" has ", psMusicNameKey, " transitions defined, this is not allowed!\n" ) );
					return qfalse;
				default:
					break;
				}
			}
		}

		// for now, assume everything was ok unless some obvious things are missing...
		//
		bReturn = true;

		// boss & death pieces can omit entry/exit stuff
		if( eMusicState != eBGRNDTRACK_BOSS && eMusicState != eBGRNDTRACK_DEATH )
		{
			if( !bEntryFound )
			{
				Music_Parse_Error( filename, build_string( "Unable to find subgroup \"", sKEY_ENTRY, "\" in group \"", psMusicName, "\"\n" ) );
				bReturn = false;
			}
			if( !bExitFound )
			{
				Music_Parse_Error( filename, build_string( "Unable to find subgroup \"", sKEY_EXIT, "\" in group \"", psMusicName, "\"\n" ) );
				bReturn = false;
			}
		}
	}
	else
	{
		Music_Parse_Error( filename, build_string( "Unable to find musicfiles entry \"", psMusicName, "\"\n" ) );
	}

	if( bReturn )
	{
		MusicFile.sFileNameBase = psMusicName;
		( *MusicData )[ psMusicNameKey ] = MusicFile;
	}

	return (qboolean)bReturn;
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

static qboolean Music_ParseLeveldata( gsl::czstring psLevelName )
{
	qboolean bReturn = qfalse;

	if (MusicData == NULL)
	{
		// sorry vv, false leaks make it hard to find true leaks
		static MusicData_t singleton;
		MusicData = &singleton;
	}

	// already got this data?
	//
	if (MusicData->size() && !Q_stricmp(psLevelName,gsLevelNameForCompare.c_str()))
	{
		return qtrue;
	}

	MusicData->clear();

	// shorten level name to MAX_QPATH so sstring's assignment assertion is satisfied.
	char sLevelName[MAX_QPATH];
	Q_strncpyz(sLevelName,psLevelName,sizeof(sLevelName));

	gsLevelNameForLoad		= sLevelName;	// harmless to init here even if we fail to parse dms.dat file
	gsLevelNameForCompare	= sLevelName;	// harmless to init here even if we fail to parse dms.dat file
	gsLevelNameForBossLoad	= sLevelName;	// harmless to init here even if we fail to parse dms.dat file

	gsl::czstring filename = sFILENAME_DMS;
	CGenericParser2 Parser;
	if( !Parser.Parse( filename ) )
	{
		Music_Parse_Error( filename, "Error using GP to parse file\n" );
	}
	else
	{
		const CGPGroup& pFileGroup = Parser.GetBaseParseGroup();
		const CGPGroup* pgMusicFiles = pFileGroup.FindSubGroup( sKEY_MUSICFILES );
		if( !pgMusicFiles )
		{
			Music_Parse_Error(filename, build_string( "Unable to find subgroup \"", sKEY_MUSICFILES ,"\"\n" ) );
		}
		else
		{
			const CGPGroup* pgLevelMusic = pFileGroup.FindSubGroup( sKEY_LEVELMUSIC );

			if( !pgLevelMusic )
			{
				Music_Parse_Error( filename, build_string( "Unable to find subgroup \"", sKEY_MUSICFILES, "\"\n" ) );
			}
			else
			{
				const CGPGroup *pgThisLevelMusic = nullptr;
				//
				// check for new USE keyword...
				//
				int steps = 0;
				gsl::cstring_span searchName{ &sLevelName[ 0 ], &sLevelName[ strlen( &sLevelName[ 0 ] ) ] };

				const int sanityLimit = 10;
				while( !searchName.empty() && steps < sanityLimit )
				{
					gsLevelNameForLoad = searchName;
					gsLevelNameForBossLoad = gsLevelNameForLoad;
					pgThisLevelMusic = pgLevelMusic->FindSubGroup( searchName );

					if( pgThisLevelMusic )
					{
						const CGPProperty* pValue = pgThisLevelMusic->FindProperty( sKEY_USES );
						if( pValue )
						{
							// re-search using the USE param...
							//
							searchName = pValue->GetTopValue();
							steps++;
							//									Com_DPrintf("Using \"%s\"\n",sSearchName.c_str());
						}
						else
						{
							// no new USE keyword found...
							//
							searchName = {};
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
				if( !pgThisLevelMusic || steps >= sanityLimit )
				{
					Music_Parse_Warning( build_string( "Unable to find entry for \"", sLevelName, "\" in \"", filename, "\"\n" ) );
				}
				else
				{
					// these are optional fields, so see which ones we find...
					//
					gsl::cstring_span psName_Explore;
					gsl::cstring_span psName_Action;
					gsl::cstring_span psName_Boss;
					gsl::cstring_span psName_UseBoss;

					for( auto& prop : pgThisLevelMusic->GetProperties() )
					{
						auto& key = prop.GetName();
						auto& value = prop.GetTopValue();

						if( Q::stricmp( value, sKEY_PLACEHOLDER ) == Q::Ordering::EQ )
						{
							// ignore "placeholder" items
							continue;
						}

						if( Q::stricmp( key, sKEY_EXPLORE ) == Q::Ordering::EQ )
						{
							psName_Explore = value;
						}
						else if( Q::stricmp( key, sKEY_ACTION ) == Q::Ordering::EQ )
						{
							psName_Action = value;
						}
						else if( Q::stricmp( key, sKEY_USEBOSS ) == Q::Ordering::EQ )
						{
							psName_UseBoss = value;
						}
						else if( Q::stricmp( key, sKEY_BOSS ) == Q::Ordering::EQ )
						{
							psName_Boss = value;
						}
					}

					bReturn = qtrue;	// defualt to ON now, so I can turn it off if "useboss" fails

					if( !psName_UseBoss.empty() )
					{
						const CGPGroup *pgLevelMusicOfBoss = pgLevelMusic->FindSubGroup( psName_UseBoss );
						if( !pgLevelMusicOfBoss )
						{
							Music_Parse_Error( filename, build_string( "Unable to find 'useboss' entry \"", psName_UseBoss, "\"\n", psName_UseBoss ) );
							bReturn = qfalse;
						}
						else
						{
							const CGPProperty *pValueBoss = pgLevelMusicOfBoss->FindProperty( sKEY_BOSS );
							if( !pValueBoss )
							{
								Music_Parse_Error( filename, build_string( "'useboss' \"", psName_UseBoss, "\" has no \"boss\" entry!\n" ) );
								bReturn = qfalse;
							}
							else
							{
								psName_Boss = pValueBoss->GetTopValue();
								gsLevelNameForBossLoad = psName_UseBoss;
							}
						}
					}


					// done this way in case I want to conditionally pass any bools depending on music type...
					//
					if( bReturn && psName_Explore.length() )
					{
						bReturn = Music_ParseMusic( filename, Parser, MusicData, *pgMusicFiles, psName_Explore, sKEY_EXPLORE, eBGRNDTRACK_EXPLORE );
					}
					if( bReturn && psName_Action.length() )
					{
						bReturn = Music_ParseMusic( filename, Parser, MusicData, *pgMusicFiles, psName_Action, sKEY_ACTION, eBGRNDTRACK_ACTION );
					}
					if( bReturn && psName_Boss.length() )
					{
						bReturn = Music_ParseMusic( filename, Parser, MusicData, *pgMusicFiles, psName_Boss, sKEY_BOSS, eBGRNDTRACK_BOSS );
					}
					if( bReturn /*&& psName_Death*/ )	// LAST MINUTE HACK!!, always force in some death music!!!!
					{
						//bReturn = Music_ParseMusic(Parser, MusicData, pgMusicFiles, psName_Death,	sKEY_DEATH,   eBGRNDTRACK_DEATH);

						MusicFile_t m;
						m.sFileNameBase = "death_music";
						( *MusicData )[ sKEY_DEATH ] = m;
					}
				}
			}
		}
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
				Music_Parse_Error( filename, build_string( "Music file \"", psMusicFileName, "\" not found!\n" ) );
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
					Music_Parse_Error( filename, build_string( "Transition file \"", psTransitionFileName, "\" (entry \"", MusicExitPoint.sNextFile.c_str(), "\" ) not found!\n" ) );
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
							Music_Parse_Error( filename, build_string( "Unable to find entry point \"", psNextMark, "\" in description for \"", MusicFile_Explore.sFileNameBase.c_str(), "\"\n" ) );
							return qfalse;		// have to return, because music data destroyed now
						}
					}
					else
					{
						Music_Parse_Error( filename, build_string( "Unable to find ", Music_BaseStateToString( eBGRNDTRACK_EXPLORE ), " piece to match \"", MusicFile.sFileNameBase.c_str(), "\"\n" ) );
						return qfalse;		// have to return, because music data destroyed now
					}
				}
			}
		}
	}

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
	Q_strlwr(sLevelName);

	if (strlen(sLevelName))	// avoid error messages when there's no music waiting to be played and we try and restart it...
	{
		if (Music_ParseLeveldata(sLevelName))
		{
			return (qboolean)(Music_GetBaseMusicFile(eBGRNDTRACK_EXPLORE) &&
								Music_GetBaseMusicFile(eBGRNDTRACK_ACTION));
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
				size_t iTransNum = eMusicState - eBGRNDTRACK_ACTIONTRANS0;
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
				size_t iTransNum = eMusicState - eBGRNDTRACK_EXPLORETRANS0;
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
	return (qboolean)(eMusicState >= eBGRNDTRACK_FIRSTTRANSITION &&
						eMusicState <= eBGRNDTRACK_LASTTRANSITION);
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
				size_t iExitPoint = pExitTime->iExitPoint;
				//
				// the two params to give back...
				//
				MusicState_e	eFeedBackTransition			= eBGRNDTRACK_EXPLORETRANS0;	// any old default
				float			fFeedBackNewTrackEntryTime	= 0.0f;
				//
				// check legality in case of crap data...
				//
				if (iExitPoint < pMusicFile->MusicExitPoints.size())
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

//			OutputDebugString(va("Music_GetRandomEntryTime(): Entry %d\n",iRandomEntryNum));

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

