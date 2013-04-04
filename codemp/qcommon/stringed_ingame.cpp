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
#include "../server/server.h"
#include "../game/q_shared.h"
#include "qcommon.h"
#include "../qcommon/fixedmap.h"
#include "../zlib/zlib.h"

//
//////////////////////////////////////////////////


#pragma warning ( disable : 4511 )			// copy constructor could not be generated
#pragma warning ( disable : 4512 )			// assignment operator could not be generated
#pragma warning ( disable : 4663 )			// C++ language change: blah blah template crap blah blah
#include "stringed_ingame.h"
#include "stringed_interface.h"

// Needed for DWORD and XC_LANGUAGE defines:
#include <xtl.h>

///////////////////////////////////////////////

// some STL stuff...
#include <string>
using namespace std;

///////////////////////////////////////////////

cvar_t	*se_language = NULL;

// Yeah, it's hardcoded. I don't give a shit.
#define MAX_STRING_ENTRIES	4096


typedef struct SE_Entry_s
{
	string		m_strString;
} SE_Entry_t;


//typedef map <string, SE_Entry_t> mapStringEntries_t;

class CStringEdPackage
{
private:

	SE_BOOL				m_bEndMarkerFound_ParseOnly;
	string				m_strCurrentEntryRef_ParseOnly;
	string				m_strCurrentEntryEnglish_ParseOnly;
	string				m_strCurrentFileRef_ParseOnly;
	string				m_strLoadingLanguage_ParseOnly;	// eg "german"
	SE_BOOL				m_bLoadingEnglish_ParseOnly;

public:

	CStringEdPackage()
	{
		Z_PushNewDeleteTag( TAG_STRINGED );
		m_Strings = new VVFixedMap< char *, unsigned long >( MAX_STRING_ENTRIES );
		Z_PopNewDeleteTag();

		Clear( SE_FALSE );
	}

	~CStringEdPackage()
	{
		Clear( SE_FALSE );
	}

	// Text entries, indexed by crc32 of reference:
	VVFixedMap< char *, unsigned long > *m_Strings;

//	mapStringEntries_t	m_StringEntries;	// needs to be in public space now

	void	Clear( SE_BOOL bChangingLanguages );
	void	SetupNewFileParse( LPCSTR psFileName );
	SE_BOOL	ReadLine( LPCSTR &psParsePos, char *psDest );
	LPCSTR	ParseLine( LPCSTR psLine );
	LPCSTR	ExtractLanguageFromPath( LPCSTR psFileName );
	SE_BOOL	EndMarkerFoundDuringParse( void )
	{
		return m_bEndMarkerFound_ParseOnly;
	}

private:

	void	AddEntry( LPCSTR psLocalReference );
	int		GetNumStrings(void);
	void	SetString( LPCSTR psLocalReference, LPCSTR psNewString, SE_BOOL bEnglishDebug );
	SE_BOOL	SetReference( int iIndex, LPCSTR psNewString );
	LPCSTR	GetCurrentFileName(void);
	LPCSTR	GetCurrentReference_ParseOnly( void );
	SE_BOOL	CheckLineForKeyword( LPCSTR psKeyword, LPCSTR &psLine);
	LPCSTR	InsideQuotes( LPCSTR psLine );
	LPCSTR	ConvertCRLiterals_Read( LPCSTR psString );
	void	REMKill( char *psBuffer );
	char	*Filename_PathOnly( LPCSTR psFilename );
	char	*Filename_WithoutPath(LPCSTR psFilename);
	char	*Filename_WithoutExt(LPCSTR psFilename);
};

CStringEdPackage TheStringPackage;


void CStringEdPackage::Clear( SE_BOOL bChangingLanguages )
{
//	m_StringEntries.clear();

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
// (normally I'd call another function for this, but this is supposed to be engine-independant,
//	 so a certain amount of re-invention of the wheel is to be expected...)
//
char *CStringEdPackage::Filename_PathOnly(LPCSTR psFilename)
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
// (normally I'd call another function for this, but this is supposed to be engine-independant,
//	 so a certain amount of re-invention of the wheel is to be expected...)
//
char *CStringEdPackage::Filename_WithoutExt(LPCSTR psFilename)
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
// (normally I'd call another function for this, but this is supposed to be engine-independant,
//	 so a certain amount of re-invention of the wheel is to be expected...)
//
char *CStringEdPackage::Filename_WithoutPath(LPCSTR psFilename)
{
	static char sString[ iSE_MAX_FILENAME_LENGTH ];

	LPCSTR psCopyPos = psFilename;
	
	while (*psFilename)
	{
		if (*psFilename == '/' || *psFilename == '\\')
			psCopyPos = psFilename+1;
		psFilename++;
	}

	strcpy(sString,psCopyPos);

	return sString;
}


LPCSTR CStringEdPackage::ExtractLanguageFromPath( LPCSTR psFileName )
{
	return Filename_WithoutPath( Filename_PathOnly( psFileName ) );
}


void CStringEdPackage::SetupNewFileParse( LPCSTR psFileName )
{
	char sString[ iSE_MAX_FILENAME_LENGTH ];

	strcpy(sString, Filename_WithoutPath( Filename_WithoutExt( psFileName ) ));
	Q_strupr(sString);

	m_strCurrentFileRef_ParseOnly = sString;	// eg "OBJECTIVES"
	m_strLoadingLanguage_ParseOnly = ExtractLanguageFromPath( psFileName );
	m_bLoadingEnglish_ParseOnly = (!stricmp( m_strLoadingLanguage_ParseOnly.c_str(), "english" )) ? SE_TRUE : SE_FALSE;
}


// returns SE_TRUE if supplied keyword found at line start (and advances supplied ptr past any whitespace to next arg (or line end if none),
//
//	else returns SE_FALSE...
//
SE_BOOL CStringEdPackage::CheckLineForKeyword( LPCSTR psKeyword, LPCSTR &psLine)
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
LPCSTR CStringEdPackage::ConvertCRLiterals_Read( LPCSTR psString )
{
	static string str;
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
SE_BOOL CStringEdPackage::ReadLine( LPCSTR &psParsePos, char *psDest )
{
	if (psParsePos[0])
	{	
		LPCSTR psLineEnd = strchr(psParsePos, '\n');
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
LPCSTR CStringEdPackage::InsideQuotes( LPCSTR psLine )
{
	// I *could* replace this string object with a declared array, but wasn't sure how big to leave it, and it'd have to
	//	be static as well, hence permanent. (problem on consoles?)
	//
	static	string	str;
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



// this copes with both foreigners using hi-char values (eg the french using 0x92 instead of 0x27
//  for a "'" char), as well as the fact that our buggy fontgen program writes out zeroed glyph info for
//	some fonts anyway (though not all, just as a gotcha).
//
// New bit, instead of static buffer (since XBox guys are desperately short of mem) I return a malloc'd buffer now,
//	so remember to free it!
//
static char *CopeWithDumbStringData( LPCSTR psSentence, LPCSTR psThisLanguage )
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
	if (!stricmp(psThisLanguage,"ENGLISH") ||
		!stricmp(psThisLanguage,"FRENCH") ||
		!stricmp(psThisLanguage,"GERMAN") ||
		!stricmp(psThisLanguage,"ITALIAN") ||
		!stricmp(psThisLanguage,"SPANISH") ||
		!stricmp(psThisLanguage,"POLISH") ||
		!stricmp(psThisLanguage,"RUSSIAN")
		)
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
LPCSTR CStringEdPackage::ParseLine( LPCSTR psLine )
{
	LPCSTR psErrorMessage = NULL;

	if (psLine)
	{
		if (CheckLineForKeyword( sSE_KEYWORD_VERSION, psLine ))
		{
			// VERSION 	"1"
			//
			LPCSTR psVersionNumber = InsideQuotes( psLine );
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
			LPCSTR psLocalReference = InsideQuotes( psLine );
			AddEntry( psLocalReference );
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
			LPCSTR psReference = GetCurrentReference_ParseOnly();
			if ( psReference[0] )
			{
				psLine += strlen(sSE_KEYWORD_LANG);

				// what language is this?...
				//
				LPCSTR psWordEnd = psLine;
				while (*psWordEnd && *psWordEnd != ' ' && *psWordEnd != '\t')
				{
					psWordEnd++;
				}
				char sThisLanguage[1024]={0};
				int iCharsToCopy = psWordEnd - psLine;
				if (iCharsToCopy > sizeof(sThisLanguage)-1)
				{
					iCharsToCopy = sizeof(sThisLanguage)-1;
				}
				strncpy(sThisLanguage, psLine, iCharsToCopy);	// already declared as {0} so no need to zero-cap dest buffer

				psLine += strlen(sThisLanguage);
				LPCSTR _psSentence = ConvertCRLiterals_Read( InsideQuotes( psLine ) );

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
					SE_BOOL bSentenceIsEnglish = (!stricmp(sThisLanguage,"english")) ? SE_TRUE: SE_FALSE;	// see whether this is the english master or not

					// this check can be omitted, I'm just being extra careful here...
					//
					if ( !bSentenceIsEnglish )
					{
						// basically this is just checking that an .STE file override is the same language as the .STR...
						//
						if (stricmp( m_strLoadingLanguage_ParseOnly.c_str(), sThisLanguage ))
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
LPCSTR CStringEdPackage::GetCurrentReference_ParseOnly( void )
{
	return m_strCurrentEntryRef_ParseOnly.c_str();
}
	
// add new string entry (during parse)
//
void CStringEdPackage::AddEntry( LPCSTR psLocalReference )
{
	// the reason I don't just assign it anyway is because the optional .STE override files don't contain flags, 
	//	and therefore would wipe out the parsed flags of the .STR file...
	//
/*
	mapStringEntries_t::iterator itEntry = m_StringEntries.find( va("%s_%s",m_strCurrentFileRef_ParseOnly.c_str(), psLocalReference) );
	if (itEntry == m_StringEntries.end())
	{
		SE_Entry_t SE_Entry;
		m_StringEntries[ va("%s_%s", m_strCurrentFileRef_ParseOnly.c_str(), psLocalReference) ] = SE_Entry;
	}
*/
	m_strCurrentEntryRef_ParseOnly = psLocalReference;

}


void CStringEdPackage::SetString( LPCSTR psLocalReference, LPCSTR psNewString, SE_BOOL bEnglishDebug )
{
	const char *ref = va("%s_%s", m_strCurrentFileRef_ParseOnly.c_str(), psLocalReference);
	unsigned long refCrc = crc32( 0, (const Bytef *)ref, strlen(ref) );

	if ( bEnglishDebug )
	{	
		// This is the leading english text of a foreign sentence pair (so it's the debug-key text):
		// don't store, just make a note in-case #same shows up:
		m_strCurrentEntryEnglish_ParseOnly = psNewString;
	}
	else if ( m_bLoadingEnglish_ParseOnly )
	{
		// It's the english text, and we're loading english. Add it!
		int len = strlen( psNewString );
		char *strData = (char *) Z_Malloc( len + 1, TAG_STRINGED, qfalse );
		strcpy( strData, psNewString );

		m_Strings->Insert( strData, refCrc );
	}
	else
	{				
		// It's foreign text, we're going to add it, but we need to check for #same
		if (!stricmp(psNewString, sSE_EXPORT_SAME))
		{
			// If it's #same, then copy the stored english version:
			int len = m_strCurrentEntryEnglish_ParseOnly.length();
			char *strData = (char *) Z_Malloc( len + 1, TAG_STRINGED, qfalse );
			strcpy( strData, m_strCurrentEntryEnglish_ParseOnly.c_str() );

			m_Strings->Insert( strData, refCrc );
		}
		else
		{
			// Explicit foreign text. Add it!
			int len = strlen( psNewString );
			char *strData = (char *) Z_Malloc( len + 1, TAG_STRINGED, qfalse );
			strcpy( strData, psNewString );

			m_Strings->Insert( strData, refCrc );
		}
	}
}



// filename is local here, eg:	"strings/german/obj.str"
//
// return is either NULL for good else error message to display...
//
static LPCSTR SE_Load_Actual( LPCSTR psFileName, SE_BOOL bSpeculativeLoad )
{
	LPCSTR psErrorMessage = NULL;
	
	unsigned char *psLoadedData = SE_LoadFileData( psFileName );
	if ( psLoadedData )
	{
		// now parse the data...
		//
		char *psParsePos = (char *) psLoadedData;

		TheStringPackage.SetupNewFileParse( psFileName );

		char sLineBuffer[16384];	// should be enough for one line of text (some of them can be BIG though)
		while ( !psErrorMessage && TheStringPackage.ReadLine((LPCSTR &) psParsePos, sLineBuffer ) )
		{
			if (strlen(sLineBuffer))
			{
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

static LPCSTR SE_GetFoundFile( string &strResult )
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
LPCSTR SE_Load( LPCSTR psFileName, SE_BOOL bFailIsCritical = SE_TRUE  )
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


	LPCSTR psErrorMessage = SE_Load_Actual( psFileName, SE_FALSE );

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
		
			psErrorMessage = SE_Load_Actual( sFileName, SE_TRUE );
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
LPCSTR SE_GetString( LPCSTR psPackageReference, LPCSTR psStringReference)
{
	char sReference[256];	// will always be enough, I've never seen one more than about 30 chars long

	sprintf(sReference,"%s_%s", psPackageReference, psStringReference);

	return SE_GetString( Q_strupr(sReference) );
}


LPCSTR SE_GetString( LPCSTR psPackageAndStringReference )
{
	int len = strlen( psPackageAndStringReference );
	char sReference[256];	// will always be enough, I've never seen one more than about 30 chars long
	assert( len < sizeof(sReference) );

	Q_strncpyz( sReference, psPackageAndStringReference, sizeof(sReference) );
	Q_strupr( sReference );

	unsigned long refCrc = crc32( 0, (const Bytef *)sReference, len );
	char **strData = TheStringPackage.m_Strings->Find( refCrc );

	if( !strData )
		return "";
	else
		return *strData;
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
	Z_PushNewDeleteTag( TAG_STRINGED );

	TheStringPackage.Clear( SE_FALSE );

//	se_language = Cvar_Get("se_language", "english", CVAR_ARCHIVE | CVAR_NORESTART);
	extern DWORD g_dwLanguage;
	switch( g_dwLanguage )
	{
		case XC_LANGUAGE_FRENCH:
			se_language = Cvar_Get("se_language", "french", CVAR_NORESTART);
			break;
		case XC_LANGUAGE_GERMAN:
			se_language = Cvar_Get("se_language", "german", CVAR_NORESTART);
			break;
		case XC_LANGUAGE_ENGLISH:
		default:
			se_language = Cvar_Get("se_language", "english", CVAR_NORESTART);
			break;
	}

	// Rather than calling SE_LoadLanguage directly, do this. Otherwise,
	// se_langauge->modified doesn't get cleared, and we parse the string files
	// twice. Gah.
	SE_CheckForLanguageUpdates();

	Z_PopNewDeleteTag();
}

// returns error message else NULL for ok.
//
// Any errors that result from this should probably be treated as game-fatal, since an asset file is fuxored.
//
LPCSTR SE_LoadLanguage( LPCSTR psLanguage )
{
	LPCSTR psErrorMessage = NULL;

	if (psLanguage && psLanguage[0])
	{
		SE_NewLanguage();

		string strResults;
		/*int iFilesFound = */SE_BuildFileList( 
												#ifdef _STRINGED
													va("C:\\Source\\Tools\\StringEd\\test_data\\%s",sSE_STRINGS_DIR)
												#else
													sSE_STRINGS_DIR
												#endif
												, strResults 
											);

		LPCSTR p;
		while ( (p=SE_GetFoundFile (strResults)) != NULL && !psErrorMessage )
		{
			LPCSTR psThisLang = TheStringPackage.ExtractLanguageFromPath( p );

			if ( !stricmp( psLanguage, psThisLang ) )
			{
				psErrorMessage = SE_Load( p );
			}
		}
	}
	else
	{
		assert( 0 && "SE_LoadLanguage(): Bad language name!" );
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
		LPCSTR psErrorMessage = SE_LoadLanguage( se_language->string );
		if ( psErrorMessage )
		{
			Com_Error( ERR_DROP, psErrorMessage );
		}
		TheStringPackage.m_Strings->Sort();
		se_language->modified = SE_FALSE;
	}
}


///////////////////////// eof //////////////////////////
