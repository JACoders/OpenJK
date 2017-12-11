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

// Filename:-	stringed_ingame.cpp
//
// This file is designed to be pasted into each game project that uses the StringEd package's files.
//  You can alter the way it does things by (eg) replacing STL with RATL, BUT LEAVE THE OVERALL
//	FUNCTIONALITY THE SAME, or if I ever make any funadamental changes to the way the package works
//	then you're going to be SOOL (shit out of luck ;-)...
//

//////////////////////////////////////////////////
//
// stuff common to all qcommon files...
#include "server/server.h"
#include "qcommon/q_shared.h"
#include "qcommon.h"
//
//////////////////////////////////////////////////
#include "stringed_ingame.h"
#include "stringed_interface.h"

///////////////////////////////////////////////
//
// some STL stuff...
#include <list>
#include <map>
#include <set>
#include <string>
#include <vector>

typedef std::vector<std::string>	vStrings_t;
typedef std::vector<int>		vInts_t;
//
///////////////////////////////////////////////

cvar_t	*se_language = NULL;
cvar_t	*se_debug = NULL;
cvar_t  *sp_leet = NULL;	// kept as 'sp_' for JK2 compat.

#define __DEBUGOUT(_string)	Com_OPrintf("%s",_string)
#define __ASSERT(_blah)		assert(_blah)

typedef struct SE_Entry_s
{
	std::string		m_strString;
	std::string		m_strDebug;	// english and/or "#same", used for debugging only. Also prefixed by "SE:" to show which strings go through StringEd (ie aren't hardwired)
	int			m_iFlags;

	SE_Entry_s()
	{
		m_iFlags = 0;
	}

} SE_Entry_t;


typedef std::map <std::string, SE_Entry_t> mapStringEntries_t;

class CStringEdPackage
{
private:

	SE_BOOL				m_bEndMarkerFound_ParseOnly;
	std::string			m_strCurrentEntryRef_ParseOnly;
	std::string			m_strCurrentEntryEnglish_ParseOnly;
	std::string			m_strCurrentFileRef_ParseOnly;
	std::string			m_strLoadingLanguage_ParseOnly;	// eg "german"
	SE_BOOL				m_bLoadingEnglish_ParseOnly;

public:

	CStringEdPackage()
	{
		Clear( SE_FALSE );
	}

	~CStringEdPackage()
	{
		Clear( SE_FALSE );
	}

	mapStringEntries_t	m_StringEntries;	// needs to be in public space now
	SE_BOOL				m_bLoadDebug;		// ""
	//
	// flag stuff...
	//
	std::vector <std::string>		m_vstrFlagNames;
	std::map	<std::string,int>	m_mapFlagMasks;

	void	Clear( SE_BOOL bChangingLanguages );
	void	SetupNewFileParse( const char *psFileName, SE_BOOL bLoadDebug );
	SE_BOOL	ReadLine( const char *&psParsePos, char *psDest );
	const char *ParseLine( const char *psLine );
	int		GetFlagMask( const char *psFlagName );
	const char *ExtractLanguageFromPath( const char *psFileName );
	SE_BOOL	EndMarkerFoundDuringParse( void )
	{
		return m_bEndMarkerFound_ParseOnly;
	}

private:

	void	AddEntry( const char *psLocalReference );
	int		GetNumStrings(void);
	void	SetString( const char *psLocalReference, const char *psNewString, SE_BOOL bEnglishDebug );
	SE_BOOL	SetReference( int iIndex, const char *psNewString );
	void	AddFlagReference( const char *psLocalReference, const char *psFlagName );
	const char *GetCurrentFileName(void);
	const char *GetCurrentReference_ParseOnly( void );
	SE_BOOL	CheckLineForKeyword( const char *psKeyword, const char *&psLine);
	const char *InsideQuotes( const char *psLine );
	const char *ConvertCRLiterals_Read( const char *psString );
	void	REMKill( char *psBuffer );
	char	*Filename_PathOnly( const char *psFilename );
	char	*Filename_WithoutPath(const char *psFilename);
	char	*Filename_WithoutExt(const char *psFilename);
};

CStringEdPackage TheStringPackage;


void CStringEdPackage::Clear( SE_BOOL bChangingLanguages )
{
	m_StringEntries.clear();

	if ( !bChangingLanguages )
	{
		// if we're changing languages, then I'm going to leave these alone. This is to do with any (potentially) cached
		//	flag bitmasks on the game side. It shouldn't matter since all files are written out at once using the build
		//	command in StringEd. But if ever someone changed a file by hand, or added one, or whatever, and it had a
		//	different set of flags declared, or the strings were in a different order, then the flags might also change
		//	the order I see them in, and hence their indexes and masks. This should never happen unless people mess with
		//	the .STR files by hand and delete some, but this way makes sure it'll all work just in case...
		//
		// ie. flags stay defined once they're defined, and only the destructor (at app-end) kills them.
		//
		m_vstrFlagNames.clear();
		m_mapFlagMasks.clear();
	}

	m_bEndMarkerFound_ParseOnly = SE_FALSE;
	m_strCurrentEntryRef_ParseOnly = "";
	m_strCurrentEntryEnglish_ParseOnly = "";
	//
	// the other vars are cleared in SetupNewFileParse(), and are ok to not do here.
	//
}



// loses anything after the path (if any), (eg) "dir/name.bmp" becomes "dir"
// (copes with either slash-scheme for names)
//
// (normally I'd call another function for this, but this is supposed to be engine-independent,
//	 so a certain amount of re-invention of the wheel is to be expected...)
//
char *CStringEdPackage::Filename_PathOnly(const char *psFilename)
{
	static char sString[ iSE_MAX_FILENAME_LENGTH ];

	strcpy(sString,psFilename);

	char *p1= strrchr(sString,'\\');
	char *p2= strrchr(sString,'/');
	char *p = (p1>p2)?p1:p2;
	if (p)
		*p=0;

	return sString;
}


// returns (eg) "dir/name" for "dir/name.bmp"
// (copes with either slash-scheme for names)
//
// (normally I'd call another function for this, but this is supposed to be engine-independent,
//	 so a certain amount of re-invention of the wheel is to be expected...)
//
char *CStringEdPackage::Filename_WithoutExt(const char *psFilename)
{
	static char sString[ iSE_MAX_FILENAME_LENGTH ];

	strcpy(sString,psFilename);

	char *p = strrchr(sString,'.');
	char *p2= strrchr(sString,'\\');
	char *p3= strrchr(sString,'/');

	// special check, make sure the first suffix we found from the end wasn't just a directory suffix (eg on a path'd filename with no extension anyway)
	//
	if (p &&
		(p2==0 || (p2 && p>p2)) &&
		(p3==0 || (p3 && p>p3))
		)
		*p=0;

	return sString;
}

// returns actual filename only, no path
// (copes with either slash-scheme for names)
//
// (normally I'd call another function for this, but this is supposed to be engine-independent,
//	 so a certain amount of re-invention of the wheel is to be expected...)
//
char *CStringEdPackage::Filename_WithoutPath(const char *psFilename)
{
	static char sString[ iSE_MAX_FILENAME_LENGTH ];

	const char *psCopyPos = psFilename;

	while (*psFilename)
	{
		if (*psFilename == '/' || *psFilename == '\\')
			psCopyPos = psFilename+1;
		psFilename++;
	}

	strcpy(sString,psCopyPos);

	return sString;
}


const char *CStringEdPackage::ExtractLanguageFromPath( const char *psFileName )
{
	return Filename_WithoutPath( Filename_PathOnly( psFileName ) );
}


void CStringEdPackage::SetupNewFileParse( const char *psFileName, SE_BOOL bLoadDebug )
{
	char sString[ iSE_MAX_FILENAME_LENGTH ];

	strcpy(sString, Filename_WithoutPath( Filename_WithoutExt( psFileName ) ));
	Q_strupr(sString);

	m_strCurrentFileRef_ParseOnly = sString;	// eg "OBJECTIVES"
	m_strLoadingLanguage_ParseOnly = ExtractLanguageFromPath( psFileName );
	m_bLoadingEnglish_ParseOnly = (!Q_stricmp( m_strLoadingLanguage_ParseOnly.c_str(), "english" )) ? SE_TRUE : SE_FALSE;
	m_bLoadDebug = bLoadDebug;
}


// returns SE_TRUE if supplied keyword found at line start (and advances supplied ptr past any whitespace to next arg (or line end if none),
//
//	else returns SE_FALSE...
//
SE_BOOL CStringEdPackage::CheckLineForKeyword( const char *psKeyword, const char *&psLine)
{
	if (!Q_stricmpn(psKeyword, psLine, strlen(psKeyword)) )
	{
		psLine += strlen(psKeyword);

		// skip whitespace to arrive at next item...
		//
		while ( *psLine == '\t' || *psLine == ' ' )
		{
			psLine++;
		}
		return SE_TRUE;
	}

	return SE_FALSE;
}

// change "\n" to '\n' (i.e. 2-byte char-string to 1-byte ctrl-code)...
//  (or "\r\n" in editor)
//
const char *CStringEdPackage::ConvertCRLiterals_Read( const char *psString )
{
	static std::string str;
	str = psString;
	int iLoc;
	while ( (iLoc = str.find("\\n")) != -1 )
	{
		str[iLoc  ] = '\n';
		str.erase( iLoc+1,1 );
	}

	return str.c_str();
}


// kill off any "//" onwards part in the line, but NOT if it's inside a quoted string...
//
void CStringEdPackage::REMKill( char *psBuffer )
{
	char *psScanPos = psBuffer;
	char *p;
	int iDoubleQuotesSoFar = 0;

	// scan forwards in case there are more than one (and the first is inside quotes)...
	//
	while ( (p=strstr(psScanPos,"//")) != NULL)
	{
		// count the number of double quotes before this point, if odd number, then we're inside quotes...
		//
		int iDoubleQuoteCount = iDoubleQuotesSoFar;

		for (int i=0; i<p-psScanPos; i++)
		{
			if (psScanPos[i] == '"')
			{
				iDoubleQuoteCount++;
			}
		}
		if (!(iDoubleQuoteCount&1))
		{
			// not inside quotes, so kill line here...
			//
			*p='\0';
			//
			// and remove any trailing whitespace...
			//
			if (psScanPos[0])	// any strlen? (else access violation with -1 below)
			{
				int iWhiteSpaceScanPos = strlen(psScanPos)-1;
				while (iWhiteSpaceScanPos>=0 && isspace(psScanPos[iWhiteSpaceScanPos]))
				{
					psScanPos[iWhiteSpaceScanPos--] = '\0';
				}
			}

			return;
		}
		else
		{
			// inside quotes (blast), oh well, skip past and keep scanning...
			//
			psScanPos = p+1;
			iDoubleQuotesSoFar = iDoubleQuoteCount;
		}
	}
}

// returns true while new lines available to be read...
//
SE_BOOL CStringEdPackage::ReadLine( const char *&psParsePos, char *psDest )
{
	if (psParsePos[0])
	{
		const char *psLineEnd = strchr(psParsePos, '\n');
		if (psLineEnd)
		{
			int iCharsToCopy = (psLineEnd - psParsePos);
			strncpy(psDest, psParsePos, iCharsToCopy);
					psDest[iCharsToCopy] = '\0';
			psParsePos += iCharsToCopy;
			while (*psParsePos && strchr("\r\n",*psParsePos))
			{
				psParsePos++;	// skip over CR or CR/LF pairs
			}
		}
		else
		{
			// last line...
			//
			strcpy(psDest, psParsePos);
			psParsePos += strlen(psParsePos);
		}

		// clean up the line...
		//
		if (psDest[0])
		{
			int iWhiteSpaceScanPos = strlen(psDest)-1;
			while (iWhiteSpaceScanPos>=0 && isspace(psDest[iWhiteSpaceScanPos]))
			{
				psDest[iWhiteSpaceScanPos--] = '\0';
			}

			REMKill( psDest );
		}
		return SE_TRUE;
	}

	return SE_FALSE;
}

// remove any outside quotes from this supplied line, plus any leading or trailing whitespace...
//
const char *CStringEdPackage::InsideQuotes( const char *psLine )
{
	// I *could* replace this string object with a declared array, but wasn't sure how big to leave it, and it'd have to
	//	be static as well, hence permanent. (problem on consoles?)
	//
	static	std::string	str;
					str = "";	// do NOT join to above line

	// skip any leading whitespace...
	//
	while (*psLine == ' ' || *psLine == '\t')
	{
		psLine++;
	}

	// skip any leading quote...
	//
	if (*psLine == '"')
	{
		psLine++;
	}

	// assign it...
	//
	str = psLine;

	if (psLine[0])
	{
		// lose any trailing whitespace...
		//
		while (	str.c_str()[ strlen(str.c_str()) -1 ] == ' ' ||
				str.c_str()[ strlen(str.c_str()) -1 ] == '\t'
				)
		{
			str.erase( strlen(str.c_str()) -1, 1);
		}

		// lose any trailing quote...
		//
		if (str.c_str()[ strlen(str.c_str()) -1 ] == '"')
		{
			str.erase( strlen(str.c_str()) -1, 1);
		}
	}

	// and return it...
	//
	return str.c_str();
}


// returns flag bitmask (eg 00000010b), else 0 for not found
//
int CStringEdPackage::GetFlagMask( const char *psFlagName )
{
	std::map <std::string, int>::iterator itFlag = m_mapFlagMasks.find( psFlagName );
	if ( itFlag != m_mapFlagMasks.end() )
	{
		int &iMask = (*itFlag).second;
		return iMask;
	}

	return 0;
}


void CStringEdPackage::AddFlagReference( const char *psLocalReference, const char *psFlagName )
{
	// add the flag to the list of known ones...
	//
	int iMask = GetFlagMask( psFlagName );
	if (iMask == 0)
	{
		m_vstrFlagNames.push_back( psFlagName );
		iMask = 1 << (m_vstrFlagNames.size()-1);
		m_mapFlagMasks[ psFlagName ] = iMask;
	}
	//
	// then add the reference to this flag to the currently-parsed reference...
	//
	mapStringEntries_t::iterator itEntry = m_StringEntries.find( va("%s_%s",m_strCurrentFileRef_ParseOnly.c_str(), psLocalReference) );
	if (itEntry != m_StringEntries.end())
	{
		SE_Entry_t &Entry = (*itEntry).second;
		Entry.m_iFlags |= iMask;
	}
}

// this copes with both foreigners using hi-char values (eg the french using 0x92 instead of 0x27
//  for a "'" char), as well as the fact that our buggy fontgen program writes out zeroed glyph info for
//	some fonts anyway (though not all, just as a gotcha).
//
// New bit, instead of static buffer (since XBox guys are desperately short of mem) I return a malloc'd buffer now,
//	so remember to free it!
//
static char *CopeWithDumbStringData( const char *psSentence, const char *psThisLanguage )
{
	const int iBufferSize = strlen(psSentence)*3;	// *3 to allow for expansion of anything even stupid string consisting entirely of elipsis chars
	char *psNewString = (char *) Z_Malloc(iBufferSize, TAG_TEMP_WORKSPACE, qfalse);
	Q_strncpyz(psNewString, psSentence, iBufferSize);

	// this is annoying, I have to just guess at which languages to do it for (ie NOT ASIAN/MBCS!!!) since the
	//	string system was deliberately (and correctly) designed to not know or care whether it was doing SBCS
	//	or MBCS languages, because it was never envisioned that I'd have to clean up other people's mess.
	//
	// Ok, bollocks to it, this will have to do. Any other languages that come later and have bugs in their text can
	//	get fixed by them typing it in properly in the first place...
	//
	if ( !Q_stricmp( psThisLanguage, "ENGLISH" ) ||
		 !Q_stricmp( psThisLanguage, "FRENCH" ) ||
		 !Q_stricmp( psThisLanguage, "GERMAN" ) ||
		 !Q_stricmp( psThisLanguage, "ITALIAN" ) ||
		 !Q_stricmp( psThisLanguage, "SPANISH" ) ||
		 !Q_stricmp( psThisLanguage, "POLISH" ) ||
		 !Q_stricmp( psThisLanguage, "RUSSIAN" ) )
	{
		char *p;

	//	strXLS_Speech.Replace(va("%c",0x92),va("%c",0x27));	// "'"
		while ((p=strchr(psNewString,0x92))!=NULL)  // "rich" (and illegal) apostrophe
		{
			*p = 0x27;
		}

	//	strXLS_Speech.Replace(va("%c",0x93),"\"");			// smart quotes -> '"'
		while ((p=strchr(psNewString,0x93))!=NULL)
		{
			*p = '"';
		}

	//	strXLS_Speech.Replace(va("%c",0x94),"\"");			// smart quotes -> '"'
		while ((p=strchr(psNewString,0x94))!=NULL)
		{
			*p = '"';
		}

	//	strXLS_Speech.Replace(va("%c",0x0B),".");			// full stop
		while ((p=strchr(psNewString,0x0B))!=NULL)
		{
			*p = '.';
		}

	//	strXLS_Speech.Replace(va("%c",0x85),"...");			// "..."-char ->  3-char "..."
		while ((p=strchr(psNewString,0x85))!=NULL)  // "rich" (and illegal) apostrophe
		{
			memmove(p+2,p,strlen(p));
			*p++ = '.';
			*p++ = '.';
			*p   = '.';
		}

	//	strXLS_Speech.Replace(va("%c",0x91),va("%c",0x27));	// "'"
		while ((p=strchr(psNewString,0x91))!=NULL)
		{
			*p = 0x27;
		}

	//	strXLS_Speech.Replace(va("%c",0x96),va("%c",0x2D));	// "-"
		while ((p=strchr(psNewString,0x96))!=NULL)
		{
			*p = 0x2D;
		}

	//	strXLS_Speech.Replace(va("%c",0x97),va("%c",0x2D));	// "-"
		while ((p=strchr(psNewString,0x97))!=NULL)
		{
			*p = 0x2D;
		}

		// bug fix for picky grammatical errors, replace "?." with "? "
		//
		while ((p=strstr(psNewString,"?."))!=NULL)
		{
			p[1] = ' ';
		}

		// StripEd and our print code don't support tabs...
		//
		while ((p=strchr(psNewString,0x09))!=NULL)
		{
			*p = ' ';
		}
	}

	return psNewString;
}

// return is either NULL for good else error message to display...
//
const char *CStringEdPackage::ParseLine( const char *psLine )
{
	const char *psErrorMessage = NULL;

	if (psLine)
	{
		if (CheckLineForKeyword( sSE_KEYWORD_VERSION, psLine ))
		{
			// VERSION 	"1"
			//
			const char *psVersionNumber = InsideQuotes( psLine );
			int		iVersionNumber = atoi( psVersionNumber );

			if (iVersionNumber != iSE_VERSION)
			{
				psErrorMessage = va("Unexpected version number %d, expecting %d!\n", iVersionNumber, iSE_VERSION);
			}
		}
		else
		if (	CheckLineForKeyword(sSE_KEYWORD_CONFIG, psLine)
			||	CheckLineForKeyword(sSE_KEYWORD_FILENOTES, psLine)
			||	CheckLineForKeyword(sSE_KEYWORD_NOTES, psLine)
			)
		{
			// not used ingame, but need to absorb the token
		}
		else
		if (CheckLineForKeyword(sSE_KEYWORD_REFERENCE, psLine))
		{
			// REFERENCE	GUARD_GOOD_TO_SEE_YOU
			//
			const char *psLocalReference = InsideQuotes( psLine );
			AddEntry( psLocalReference );
		}
		else
		if (CheckLineForKeyword(sSE_KEYWORD_FLAGS, psLine))
		{
			// FLAGS 	FLAG_CAPTION FLAG_TYPEMATIC
			//
			const char *psReference = GetCurrentReference_ParseOnly();
			if (psReference[0])
			{
				static const char sSeperators[] = " \t";
				char sFlags[1024]={0};	// 1024 chars should be enough to store 8 flag names
				strncpy(sFlags, psLine, sizeof(sFlags)-1);
				char *psToken = strtok( sFlags, sSeperators );
				while( psToken != NULL )
				{
					// psToken = flag name (in caps)
					//
					Q_strupr(psToken);	// jic
					AddFlagReference( psReference, psToken );

					// read next flag for this string...
					//
					psToken = strtok( NULL, sSeperators );
				}
			}
			else
			{
				psErrorMessage = "Error parsing file: Unexpected \"" sSE_KEYWORD_FLAGS "\"\n";
			}
		}
		else
		if (CheckLineForKeyword(sSE_KEYWORD_ENDMARKER, psLine))
		{
			// ENDMARKER
			//
			m_bEndMarkerFound_ParseOnly = SE_TRUE;	// the only major error checking I bother to do (for file truncation)
		}
		else
		if (!Q_stricmpn(sSE_KEYWORD_LANG, psLine, strlen(sSE_KEYWORD_LANG)))
		{
			// LANG_ENGLISH 	"GUARD:  Good to see you, sir.  Taylor is waiting for you in the clean tent.  We need to get you suited up.  "
			//
			const char *psReference = GetCurrentReference_ParseOnly();
			if ( psReference[0] )
			{
				psLine += strlen(sSE_KEYWORD_LANG);

				// what language is this?...
				//
				const char *psWordEnd = psLine;
				while (*psWordEnd && *psWordEnd != ' ' && *psWordEnd != '\t')
				{
					psWordEnd++;
				}
				char sThisLanguage[1024]={0};
				size_t iCharsToCopy = psWordEnd - psLine;
				if (iCharsToCopy > sizeof(sThisLanguage)-1)
				{
					iCharsToCopy = sizeof(sThisLanguage)-1;
				}
				strncpy(sThisLanguage, psLine, iCharsToCopy);	// already declared as {0} so no need to zero-cap dest buffer

				psLine += strlen(sThisLanguage);
				const char *_psSentence = ConvertCRLiterals_Read( InsideQuotes( psLine ) );

				// Dammit, I hate having to do crap like this just because other people mess up and put
				//	stupid data in their text, so I have to cope with it.
				//
				// note hackery with _psSentence and psSentence because of const-ness. bleurgh. Just don't ask.
				//
				char *psSentence = CopeWithDumbStringData( _psSentence, sThisLanguage );

				if ( m_bLoadingEnglish_ParseOnly )
				{
					// if loading just "english", then go ahead and store it...
					//
					SetString( psReference, psSentence, SE_FALSE );
				}
				else
				{
					// if loading a foreign language...
					//
					SE_BOOL bSentenceIsEnglish = (!Q_stricmp(sThisLanguage,"english")) ? SE_TRUE: SE_FALSE;	// see whether this is the english master or not

					// this check can be omitted, I'm just being extra careful here...
					//
					if ( !bSentenceIsEnglish )
					{
						// basically this is just checking that an .STE file override is the same language as the .STR...
						//
						if (Q_stricmp( m_strLoadingLanguage_ParseOnly.c_str(), sThisLanguage ))
						{
							psErrorMessage = va("Language \"%s\" found when expecting \"%s\"!\n", sThisLanguage, m_strLoadingLanguage_ParseOnly.c_str());
						}
					}

					if (!psErrorMessage)
					{
						SetString( psReference, psSentence, bSentenceIsEnglish );
					}
				}

				Z_Free( psSentence );
			}
			else
			{
				psErrorMessage = "Error parsing file: Unexpected \"" sSE_KEYWORD_LANG "\"\n";
			}
		}
		else
		{
			psErrorMessage = va("Unknown keyword at linestart: \"%s\"\n", psLine);
		}
	}

	return psErrorMessage;
}

// returns reference of string being parsed, else "" for none.
//
const char *CStringEdPackage::GetCurrentReference_ParseOnly( void )
{
	return m_strCurrentEntryRef_ParseOnly.c_str();
}

// add new string entry (during parse)
//
void CStringEdPackage::AddEntry( const char *psLocalReference )
{
	// the reason I don't just assign it anyway is because the optional .STE override files don't contain flags,
	//	and therefore would wipe out the parsed flags of the .STR file...
	//
	mapStringEntries_t::iterator itEntry = m_StringEntries.find( va("%s_%s",m_strCurrentFileRef_ParseOnly.c_str(), psLocalReference) );
	if (itEntry == m_StringEntries.end())
	{
		SE_Entry_t SE_Entry;
		m_StringEntries[ va("%s_%s", m_strCurrentFileRef_ParseOnly.c_str(), psLocalReference) ] = SE_Entry;
	}
	m_strCurrentEntryRef_ParseOnly = psLocalReference;
}

const char *Leetify( const char *psString )
{
	static std::string str;
	str = psString;
	if (sp_leet->integer == 42)	// very specific test, so you won't hit it accidentally
	{
		static const
		char cReplace[]={	'o','0','l','1','e','3','a','4','s','5','t','7','i','!','h','#',
							'O','0','L','1','E','3','A','4','S','5','T','7','I','!','H','#'	// laziness because of strchr()
						};

		char *p;
		for (size_t i=0; i<sizeof(cReplace); i+=2)
		{
			while ((p=(char*)strchr(str.c_str(),cReplace[i]))!=NULL)
				*p = cReplace[i+1];
		}
	}

	return str.c_str();
}


void CStringEdPackage::SetString( const char *psLocalReference, const char *psNewString, SE_BOOL bEnglishDebug )
{
	mapStringEntries_t::iterator itEntry = m_StringEntries.find( va("%s_%s",m_strCurrentFileRef_ParseOnly.c_str(), psLocalReference) );
	if (itEntry != m_StringEntries.end())
	{
		SE_Entry_t &Entry = (*itEntry).second;

		if ( bEnglishDebug || m_bLoadingEnglish_ParseOnly)
		{
			// then this is the leading english text of a foreign sentence pair (so it's the debug-key text),
			//	or it's the only text when it's english being loaded...
			//
			Entry.m_strString = Leetify( psNewString );
			if ( m_bLoadDebug )
			{
				Entry.m_strDebug = sSE_DEBUGSTR_PREFIX;
				Entry.m_strDebug+= /* m_bLoadingEnglish_ParseOnly ? "" : */ psNewString;
				Entry.m_strDebug+= sSE_DEBUGSTR_SUFFIX;
			}
			m_strCurrentEntryEnglish_ParseOnly = psNewString;	// for possible "#same" resolving in foreign later
		}
		else
		{
			// then this is foreign text (so check for "#same" resolving)...
			//
			if (!Q_stricmp(psNewString, sSE_EXPORT_SAME))
			{
				Entry.m_strString = m_strCurrentEntryEnglish_ParseOnly;	// foreign "#same" is now english
				if (m_bLoadDebug)
				{
					Entry.m_strDebug = sSE_DEBUGSTR_PREFIX;
					Entry.m_strDebug+= sSE_EXPORT_SAME;				// english (debug) is now "#same"
					Entry.m_strDebug+= sSE_DEBUGSTR_SUFFIX;
				}
			}
			else
			{
				Entry.m_strString	= psNewString;							// foreign is just foreign
			}
		}
	}
	else
	{
		__ASSERT(0);	// should never happen
	}
}



// filename is local here, eg:	"strings/german/obj.str"
//
// return is either NULL for good else error message to display...
//
static const char *SE_Load_Actual( const char *psFileName, SE_BOOL bLoadDebug, SE_BOOL bSpeculativeLoad )
{
	const char *psErrorMessage = NULL;

	unsigned char *psLoadedData = SE_LoadFileData( psFileName );
	if ( psLoadedData )
	{
		// now parse the data...
		//
		char *psParsePos = (char *) psLoadedData;

		TheStringPackage.SetupNewFileParse( psFileName, bLoadDebug );

		char sLineBuffer[16384];	// should be enough for one line of text (some of them can be BIG though)
		while ( !psErrorMessage && TheStringPackage.ReadLine((const char *&) psParsePos, sLineBuffer ) )
		{
			if (strlen(sLineBuffer))
			{
//				__DEBUGOUT( sLineBuffer );
//				__DEBUGOUT( "\n" );

				psErrorMessage = TheStringPackage.ParseLine( sLineBuffer );
			}
		}

		SE_FreeFileDataAfterLoad( psLoadedData);

		if (!psErrorMessage && !TheStringPackage.EndMarkerFoundDuringParse())
		{
			psErrorMessage = va("Truncated file, failed to find \"%s\" at file end!", sSE_KEYWORD_ENDMARKER);
		}
	}
	else
	{
		if ( bSpeculativeLoad )
		{
			// then it's ok to not find the file, so do nothing...
		}
		else
		{
			psErrorMessage = va("Unable to load \"%s\"!", psFileName);
		}
	}

	return psErrorMessage;
}

static const char *SE_GetFoundFile( std::string &strResult )
{
	static char sTemp[1024/*MAX_PATH*/];

	if (!strlen(strResult.c_str()))
		return NULL;

	strncpy(sTemp,strResult.c_str(),sizeof(sTemp)-1);
	sTemp[sizeof(sTemp)-1]='\0';

	char *psSemiColon = strchr(sTemp,';');
	if (  psSemiColon)
	{
		 *psSemiColon = '\0';

		 strResult.erase(0,(psSemiColon-sTemp)+1);
	}
	else
	{
		// no semicolon found, probably last entry? (though i think even those have them on, oh well)
		//
		strResult.erase();
	}

//	strlwr(sTemp);	// just for consistancy and set<> -> set<> erasure checking etc

	return sTemp;
}

//////////// API entry points from rest of game.... //////////////////////////////

// filename is local here, eg:	"strings/german/obj.str"
//
// return is either NULL for good else error message to display...
//
const char *SE_Load( const char *psFileName, SE_BOOL bLoadDebug = SE_TRUE, SE_BOOL bFailIsCritical = SE_TRUE  )
{
	////////////////////////////////////////////////////
	//
	// ingame here tends to pass in names without paths, but I expect them when doing a language load, so...
	//
	char sTemp[1000]={0};
	if (!strchr(psFileName,'/'))
	{
		strcpy(sTemp,sSE_STRINGS_DIR);
		strcat(sTemp,"/");
		if (se_language)
		{
			strcat(sTemp,se_language->string);
			strcat(sTemp,"/");
		}
	}
	strcat(sTemp,psFileName);
	COM_DefaultExtension( sTemp, sizeof(sTemp), sSE_INGAME_FILE_EXTENSION);
	psFileName = &sTemp[0];
	//
	////////////////////////////////////////////////////


	const char *psErrorMessage = SE_Load_Actual( psFileName, bLoadDebug, SE_FALSE );

	// check for any corresponding / overriding .STE files and load them afterwards...
	//
	if ( !psErrorMessage )
	{
		char sFileName[ iSE_MAX_FILENAME_LENGTH ];
		strncpy( sFileName, psFileName, sizeof(sFileName)-1 );
				 sFileName[ sizeof(sFileName)-1 ] = '\0';
		char *p = strrchr( sFileName, '.' );
		if (p && strlen(p) == strlen(sSE_EXPORT_FILE_EXTENSION))
		{
			strcpy( p, sSE_EXPORT_FILE_EXTENSION );

			psErrorMessage = SE_Load_Actual( sFileName, bLoadDebug, SE_TRUE );
		}
	}

	if (psErrorMessage)
	{
		if (bFailIsCritical)
		{
	//		TheStringPackage.Clear(TRUE);	// Will we want to do this?  Any errors that arise should be fixed immediately
			Com_Error( ERR_DROP, "SE_Load(): Couldn't load \"%s\"!\n\nError: \"%s\"\n", psFileName, psErrorMessage );
		}
		else
		{
			Com_DPrintf(S_COLOR_YELLOW "SE_Load(): Couldn't load \"%s\"!\n", psFileName );
		}
	}

	return psErrorMessage;
}


// convenience-function for the main GetString call...
//
const char *SE_GetString( const char *psPackageReference, const char *psStringReference)
{
	char sReference[256];	// will always be enough, I've never seen one more than about 30 chars long

	Com_sprintf(sReference,sizeof(sReference),"%s_%s", psPackageReference, psStringReference);

	return SE_GetString( Q_strupr(sReference) );
}


const char *SE_GetString( const char *psPackageAndStringReference )
{
	char sReference[256];	// will always be enough, I've never seen one more than about 30 chars long
	assert(strlen(psPackageAndStringReference) < sizeof(sReference) );
	Q_strncpyz(sReference, psPackageAndStringReference, sizeof(sReference) );
	Q_strupr(sReference);

	mapStringEntries_t::iterator itEntry = TheStringPackage.m_StringEntries.find( sReference );
	if (itEntry != TheStringPackage.m_StringEntries.end())
	{
		SE_Entry_t &Entry = (*itEntry).second;

		if ( se_debug->integer && TheStringPackage.m_bLoadDebug )
		{
			return Entry.m_strDebug.c_str();
		}
		else
		{
			return Entry.m_strString.c_str();
		}
	}

	// should never get here, but fall back anyway... (except we DO use this to see if there's a debug-friendly key bind, which may not exist)
	//
//	__ASSERT(0);
	return "";	// you may want to replace this with something based on _DEBUG or not?
}


// convenience-function for the main GetFlags call...
//
int	SE_GetFlags ( const char *psPackageReference, const char *psStringReference )
{
	char sReference[256];	// will always be enough, I've never seen one more than about 30 chars long

	Com_sprintf(sReference,sizeof(sReference),"%s_%s", psPackageReference, psStringReference);

	return SE_GetFlags( sReference );
}

int	SE_GetFlags ( const char *psPackageAndStringReference )
{
	mapStringEntries_t::iterator itEntry = TheStringPackage.m_StringEntries.find( psPackageAndStringReference );
	if (itEntry != TheStringPackage.m_StringEntries.end())
	{
		SE_Entry_t &Entry = (*itEntry).second;

		return Entry.m_iFlags;
	}

	// should never get here, but fall back anyway...
	//
	__ASSERT(0);

	return 0;
}


int SE_GetNumFlags( void )
{
	return TheStringPackage.m_vstrFlagNames.size();
}

const char *SE_GetFlagName( int iFlagIndex )
{
	if ( iFlagIndex < (int)TheStringPackage.m_vstrFlagNames.size())
	{
		return TheStringPackage.m_vstrFlagNames[ iFlagIndex ].c_str();
	}

	__ASSERT(0);
	return "";
}

// returns flag bitmask (eg 00000010b), else 0 for not found
//
int SE_GetFlagMask( const char *psFlagName )
{
	return TheStringPackage.GetFlagMask( psFlagName );
}

// I could cache the result of this since it won't change during app lifetime unless someone does a build-publish
//	while you're still ingame. Cacheing would make sense since it can take a while to scan, but I'll leave it and
//	let whoever calls it cache the results instead. I'll make it known that it's a slow process to call this, but
//	whenever anyone calls someone else's code they should assign it to an int anyway, since you've no idea what's
//	going on. Basically, don't  use this in a FOR loop as the end-condition. Duh.
//
// Groan, except for Bob. I mentioned that this was slow and only call it once, but he's calling it from
//	every level-load...  Ok, cacheing it is...
//
std::vector <std::string> gvLanguagesAvailable;
int SE_GetNumLanguages(void)
{
	if ( gvLanguagesAvailable.empty() )
	{
		std::string strResults;
		/*int iFilesFound = */SE_BuildFileList(
												#ifdef _STRINGED
													va("C:\\Source\\Tools\\StringEd\\test_data\\%s",sSE_STRINGS_DIR)
												#else
													sSE_STRINGS_DIR
												#endif
												, strResults
											);

		std::set<std::string> strUniqueStrings;	// laziness <g>
		const char *p;
		while ((p=SE_GetFoundFile (strResults)) != NULL)
		{
			const char *psLanguage = TheStringPackage.ExtractLanguageFromPath( p );

	//		__DEBUGOUT( p );
	//		__DEBUGOUT( "\n" );
	//		__DEBUGOUT( psLanguage );
	//		__DEBUGOUT( "\n" );

			if (!strUniqueStrings.count( psLanguage ))
			{
				strUniqueStrings.insert( psLanguage );

				// if english is available, it should always be first... ( I suppose )
				//
				if (!Q_stricmp(psLanguage,"english"))
				{
					gvLanguagesAvailable.insert( gvLanguagesAvailable.begin(), psLanguage );
				}
				else
				{
					gvLanguagesAvailable.push_back( psLanguage );
				}
			}
		}
	}

	return gvLanguagesAvailable.size();
}

// SE_GetNumLanguages() must have been called before this...
//
const char *SE_GetLanguageName( int iLangIndex )
{
	if ( iLangIndex < (int)gvLanguagesAvailable.size() )
	{
		return gvLanguagesAvailable[ iLangIndex ].c_str();
	}

	__ASSERT(0);
	return "";
}

// SE_GetNumLanguages() must have been called before this...
//
const char *SE_GetLanguageDir( int iLangIndex )
{
	if ( iLangIndex < (int)gvLanguagesAvailable.size() )
	{
		return va("%s/%s", sSE_STRINGS_DIR, gvLanguagesAvailable[ iLangIndex ].c_str() );
	}

	__ASSERT(0);
	return "";
}

void SE_NewLanguage(void)
{
	TheStringPackage.Clear( SE_TRUE );
}



// these two functions aren't needed other than to make Quake-type games happy and/or stop memory managers
//	complaining about leaks if they report them before the global StringEd package object calls it's own dtor.
//
// but here they are for completeness's sake I guess...
//
void SE_Init(void)
{
	TheStringPackage.Clear( SE_FALSE );

#ifdef _DEBUG
//	int iNumLanguages = SE_GetNumLanguages();
#endif

	se_language = Cvar_Get("se_language", "english", CVAR_ARCHIVE | CVAR_NORESTART);
	se_debug = Cvar_Get("se_debug", "0", 0);
	sp_leet = Cvar_Get("sp_leet", "0", CVAR_ROM );

	// if doing a buildscript, load all languages...
	//
	extern cvar_t *com_buildScript;
	if (com_buildScript->integer == 2)
	{
		int iLanguages = SE_GetNumLanguages();
		for (int iLang = 0; iLang < iLanguages; iLang++)
		{
            const char *psLanguage = SE_GetLanguageName( iLang );	// eg "german"
			Com_Printf( "com_buildScript(2): Loading language \"%s\"...\n", psLanguage );
			SE_LoadLanguage( psLanguage );
		}
	}

	const char *psErrorMessage = SE_LoadLanguage( se_language->string );
	if (psErrorMessage)
	{
		Com_Error( ERR_DROP, "SE_Init() Unable to load language: \"%s\"!\nError: \"%s\"\n", se_language->string,psErrorMessage );
	}

}

void SE_ShutDown(void)
{
	TheStringPackage.Clear( SE_FALSE );
}


// returns error message else NULL for ok.
//
// Any errors that result from this should probably be treated as game-fatal, since an asset file is fuxored.
//
const char *SE_LoadLanguage( const char *psLanguage, SE_BOOL bLoadDebug /* = SE_TRUE */ )
{
	const char *psErrorMessage = NULL;

	if (psLanguage && psLanguage[0])
	{
		SE_NewLanguage();

		std::string strResults;
		/*int iFilesFound = */SE_BuildFileList(
												#ifdef _STRINGED
													va("C:\\Source\\Tools\\StringEd\\test_data\\%s",sSE_STRINGS_DIR)
												#else
													sSE_STRINGS_DIR
												#endif
												, strResults
											);

		const char *p;
		while ( (p=SE_GetFoundFile (strResults)) != NULL && !psErrorMessage )
		{
			const char *psThisLang = TheStringPackage.ExtractLanguageFromPath( p );

			if ( !Q_stricmp( psLanguage, psThisLang ) )
			{
				psErrorMessage = SE_Load( p, bLoadDebug );
			}
		}
	}
	else
	{
		__ASSERT( 0 && "SE_LoadLanguage(): Bad language name!" );
	}

	return psErrorMessage;
}


// called in Com_Frame, so don't take up any time! (can also be called during dedicated)
//
// instead of re-loading just the files we've already loaded I'm going to load the whole language (simpler)
//
void SE_CheckForLanguageUpdates(void)
{
	if (se_language && se_language->modified)
	{
		const char *psErrorMessage = SE_LoadLanguage( se_language->string, SE_TRUE );
		if ( psErrorMessage )
		{
			Com_Error( ERR_DROP, psErrorMessage );
		}
		se_language->modified = SE_FALSE;
	}
}


///////////////////////// eof //////////////////////////
