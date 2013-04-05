// Filename:-	ui_debug.cpp
//
// an entire temp module just for doing some evil menu hackery during development...
//


#include "../server/exe_headers.h"


#if 0	// this entire module was special code to StripEd-ify *menu, it isn't needed now, but I'll keep the source around for a while	-ste


#ifdef _DEBUG
#include <set>
#include "../qcommon/sstring.h"
typedef sstring<4096> sstringBIG_t;
typedef set<sstring_t>	StringSet_t;
						StringSet_t		MenusUsed;
typedef map <sstring_t,	StringSet_t>	ReferencesAndPackages_t;
										ReferencesAndPackages_t ReferencesAndPackage;


struct Reference_t
{
	sstringBIG_t	sString;
	sstring_t		sReference;
	sstring_t		sMenu;

	Reference_t()
	{
		sString		= "";
		sReference	= "";
		sMenu		= "";
	}

	// sort by menu entry, then by reference within menu...
	//
	bool operator < (const Reference_t& _X) const 
	{
		int i = stricmp(sMenu.c_str(),_X.sMenu.c_str());
		if (i)
			return i<0;

		return !!(stricmp(sReference.c_str(),_X.sReference.c_str()) < 0);
	}
};
#include <list>
typedef list <Reference_t>	References_t;
							References_t BadReferences;

sstring_t sCurrentMenu;
void UI_Debug_AddMenuFilePath(LPCSTR psMenuFile)	// eg "ui/blah.menu"
{
	sCurrentMenu = psMenuFile;

	OutputDebugString(va("Current menu: \"%s\"\n",psMenuFile));
}


typedef struct
{
	// to correct...
	//
	sstring_t		sMenuFile;
	sstringBIG_t	sTextToFind;		// will be either @REFERENCE or "text"
	sstringBIG_t	sTextToReplaceWith;	// will be @NEWREF
	//
	// to generate new data...
	//
	sstring_t		sStripEdReference;	// when NZ, this will create a new StripEd entry...
	sstringBIG_t	sStripEdText;
	sstring_t		sStripEdFileRef;	// ... in this file reference (eg "SPMENUS%d"), where 0.255 of each all have this in them (for ease of coding)

} CorrectionDataItem_t;
typedef list<CorrectionDataItem_t>	CorrectionData_t;
									CorrectionData_t CorrectionData;


static LPCSTR CreateUniqueReference(LPCSTR psText)
{
	static set <sstringBIG_t> ReferencesSoFar;
	
	static sstringBIG_t NewReference;

	LPCSTR psTextScanPos = psText;

	while(*psTextScanPos)
	{
		while (isspace(*psTextScanPos)) psTextScanPos++;

		NewReference = psTextScanPos;
				
		// cap off text at an approx length...
		//
		const int iApproxReferenceLength = 20;
		char *p;		
		if (iApproxReferenceLength<strlen(NewReference.c_str()))
		{
			p = NewReference.c_str();
			p+=iApproxReferenceLength;
			while ( *p && !isspace(*p))
				p++;

			*p = '\0';
		}

		// now replace everything except digits and letters with underscores...
		//
		p = NewReference.c_str();
		for (int i=0; i<strlen(p); i++)
		{
			if (!isalpha(p[i]) && !isdigit(p[i]))
			{
				p[i] = '_';
			}
		}
		strupr(p);

		// remove any trailing underscores...
		//
		while ( strlen(NewReference.c_str()) && NewReference.c_str()[strlen(NewReference.c_str())-1]=='_')
		{
			NewReference.c_str()[strlen(NewReference.c_str())-1]='\0';
		}

		// remove any multiple underscores...
		//
		while ( (p=strstr(NewReference.c_str(),"__")) != NULL)
		{
			memmove(p,p+1,strlen(p+1)+1);
		}

		// do we already have this reference?...
		//
		if (!strlen(NewReference.c_str()))
		{
			break;	// empty, shit.
		}
		if (ReferencesSoFar.find(NewReference.c_str()) == ReferencesSoFar.end())
		{
			// no, so add it in then return...
			//
			ReferencesSoFar.insert(NewReference.c_str());
			return NewReference.c_str();
		}

		// skip past the first word in the reference and try again...
		//
		while (*psTextScanPos && !isspace(*psTextScanPos)) psTextScanPos++;
	}

	// if we get here then we're getting desperate, so...
	//		  
	// (special case check first)...
	//	
	for (LPCSTR p2 = psText; *p2 && isspace(*p2); p2++){}
	if (!*p2)
	{
		psText = "BLANK";
	}
	
	int iReScanDigit = 1;
	while (1)
	{
		NewReference = va("%s_%d",psText,iReScanDigit++);

		// now replace everything except digits and letters with underscores...
		//
		char *p = NewReference.c_str();
		for (int i=0; i<strlen(p); i++)
		{
			if (!isalpha(p[i]) && !isdigit(p[i]))
			{
				p[i] = '_';
			}
		}
		strupr(p);

		// remove any trailing underscores...
		//
		while ( strlen(NewReference.c_str()) && NewReference.c_str()[strlen(NewReference.c_str())-1]=='_')
		{
			NewReference.c_str()[strlen(NewReference.c_str())-1]='\0';
		}

		// remove any multiple underscores...
		//
		while ( (p=strstr(NewReference.c_str(),"__")) != NULL)
		{
			memmove(p,p+1,strlen(p+1)+1);
		}


		if (ReferencesSoFar.find(NewReference.c_str()) == ReferencesSoFar.end())
		{
			// no, so add it in then return...
			//
			ReferencesSoFar.insert(NewReference.c_str());
			return NewReference.c_str();
		}
	}

	// should never get here...
	//
	assert(0);
	return NULL;
}


static LPCSTR EnterBadRef(LPCSTR ps4LetterType, LPCSTR psBad)
{
	Reference_t	BadReference;
				BadReference.sString = psBad;
				BadReference.sMenu	 = sCurrentMenu;

	BadReferences.push_back( BadReference );

	LPCSTR	p = NULL;
//			p = CreateUniqueReference(psBad);
//	OutputDebugString(va("NEWREF: \"%s\"\n",p));
	return	p;
}

static void EnterRef(LPCSTR psReference, LPCSTR psText, LPCSTR psMenuFile)
{
	// special hack, StripEd text at this point will have had any "\n" LITERALS replaced by the 0x0D 0x0A pair,
	//	so we need to put them back to "\n" text ready for saving out into StripEd files again...
	//
	string	strNewText( psText );
	//
	// not sure whether just 0x0A or 0x0D/0x0A pairs (sigh), so first, eliminate 0x0Ds...
	//
	int iLoc;
	while ( (iLoc = strNewText.find(0x0D,0)) != -1)
	{
		strNewText.replace(iLoc, 1, "");
	}
	// now replace any 0x0As with literal "\n" strings...
	//
	while ( (iLoc = strNewText.find(0x0A,0)) != -1)
	{
		strNewText.replace(iLoc, 1, "\\n");
	}
	psText = strNewText.c_str();

	// curiousity...
	//
	static int iLongestText = 0;
	if (iLongestText < strlen(psText))
	{
		iLongestText = strlen(psText);
		OutputDebugString(va("Longest StripEd text: %d\n",iLongestText));
	}



	typedef map <sstringBIG_t, sstring_t> TextConsolidationTable_t;	// string and ref
	static TextConsolidationTable_t TextConsolidationTable, RefrConsolidationTable;
	static int iIndex = 0;	// INC'd every time a new StripEd entry is synthesised

	TextConsolidationTable_t::iterator it = TextConsolidationTable.find(psText);
	if (it == TextConsolidationTable.end())
	{
		// new entry...
		//
		LPCSTR psNewReference = CreateUniqueReference( (strlen(psReference) > strlen(psText))?psReference:psText  );

		CorrectionDataItem_t	CorrectionDataItem;
								CorrectionDataItem.sMenuFile			= psMenuFile;
								CorrectionDataItem.sTextToFind			= strlen(psReference) ? ( /* !stricmp(psReference,psNewReference) ? "" :*/ va("@%s",psReference) )
																			: va("\"%s\"",psText);
								CorrectionDataItem.sTextToReplaceWith	= /* !stricmp(psReference,psNewReference) ? "" : */va("@%s",psNewReference);
								//
								CorrectionDataItem.sStripEdReference	= psNewReference;
								CorrectionDataItem.sStripEdText			= psText;

//		qboolean bIsMulti = !!strstr(psMenuFile,"jk2mp");
//								CorrectionDataItem.sStripEdFileRef		= va("%sMENUS%d",bIsMulti?"MP":"SP",iIndex/256);
								CorrectionDataItem.sStripEdFileRef		= va(  "MENUS%d",iIndex/256);
		iIndex++;

		CorrectionData.push_back( CorrectionDataItem );

		TextConsolidationTable[ psText ] = psNewReference;
		RefrConsolidationTable[ psText ] = CorrectionDataItem.sStripEdFileRef.c_str();
	}
	else
	{
		// text already entered, so do a little duplicate-resolving...
		//
		// need to find the reference for the existing version...
		//
		LPCSTR psNewReference = (*it).second.c_str();
		LPCSTR psPackageRef	  = (*RefrConsolidationTable.find(psText)).second.c_str();	// yeuch, hack-city

		// only enter correction data if references are different...
		//
//		if (stricmp(psReference,psNewReference))
		{
			CorrectionDataItem_t	CorrectionDataItem;
									CorrectionDataItem.sMenuFile			= psMenuFile;
									CorrectionDataItem.sTextToFind			= strlen(psReference) ? va("@%s",psReference) : va("\"%s\"",psText);
									CorrectionDataItem.sTextToReplaceWith	= va("@%s",psNewReference);
									//
									CorrectionDataItem.sStripEdReference	= "";
									CorrectionDataItem.sStripEdText			= "";
									CorrectionDataItem.sStripEdFileRef		= psPackageRef;


			CorrectionData.push_back( CorrectionDataItem );
		}
	}
}

static void EnterGoodRef(LPCSTR ps4LetterType, LPCSTR psReference, LPCSTR psPackageReference, LPCSTR psText)
{
	EnterRef(psReference, psText, sCurrentMenu.c_str());

	ReferencesAndPackage[psReference].insert(psPackageReference);
	MenusUsed.insert(psPackageReference);
}

static bool SendFileToNotepad(LPCSTR psFilename)
{
	bool bReturn = false;

	char sExecString[MAX_PATH];

	sprintf(sExecString,"notepad %s",psFilename);

	if (WinExec(sExecString,	//  LPCSTR lpCmdLine,  // address of command line
				SW_SHOWNORMAL	//  UINT uCmdShow      // window style for new application
				)
		>31	// don't ask me, Windoze just uses >31 as OK in this call.
		)
	{
		// ok...
		//
		bReturn = true;
	}
	else
	{
		assert(0);//ErrorBox("Unable to locate/run NOTEPAD on this machine!\n\n(let me know about this -Ste)");		
	}

	return bReturn;
}

// creates as temp file, then spawns notepad with it...
//
static bool SendStringToNotepad(LPCSTR psWhatever, LPCSTR psLocalFileName)
{
	bool bReturn = false;

	LPCSTR psOutputFileName = va("c:\\%s",psLocalFileName);

	FILE *handle = fopen(psOutputFileName,"wt");
	if (handle)
	{
		fprintf(handle,"%s",psWhatever);	// NOT fprintf(handle,psWhatever), which will try and process built-in %s stuff
		fclose(handle);

		bReturn = SendFileToNotepad(psOutputFileName);
	}
	else
	{
		assert(0);//ErrorBox(va("Unable to create file \"%s\" for notepad to use!",psOutputFileName));
	}

	return bReturn;
}


static qboolean DoFileFindReplace( LPCSTR psMenuFile, LPCSTR psFind, LPCSTR psReplace )
{
	char *buffer;

	OutputDebugString(va("Loading: \"%s\"\n",psMenuFile));

	int iLen = FS_ReadFile( psMenuFile,(void **) &buffer);
	if (iLen<1) 
	{
		OutputDebugString("Failed!\n");
		assert(0);
		return qfalse;
	}
	

	// find/rep...
	//
	string	str(buffer);
			str += "\r\n";	// safety for *(char+1) stuff


	FS_FreeFile( buffer );	// let go of the buffer

	// originally this kept looping for replacements, but now it only does one (since the find/replace args are repeated
	//	and this is called again if there are >1 replacements of the same strings to be made...
	//
//	int iReplacedCount = 0;
	char *pFound;
	int iSearchPos = 0;
	while ( (pFound = strstr(str.c_str()+iSearchPos,psFind)) != NULL)
	{
		// special check, the next char must be whitespace or carriage return etc, or we're not on a whole-word position...
		//
		int iFoundLoc = pFound - str.c_str();
		char cAfterFind = pFound[strlen(psFind)];
		if (cAfterFind > 32)
		{
			// ... then this string was part of a larger one, so ignore it...
			//
			iSearchPos = iFoundLoc+1;
			continue;
		}		

		str.replace(iFoundLoc, strlen(psFind), psReplace);
//		iSearchPos = iFoundLoc+1;
//		iReplacedCount++;
		break;
	}

//	assert(iReplacedCount);
//	if (iReplacedCount>1)
//	{
//		int z=1;
//	}
	FS_WriteFile( psMenuFile, str.c_str(), strlen(str.c_str()));

	OutputDebugString("Ok\n");

	return qtrue;
}

void UI_Dump_f(void)
{
	string sFinalOutput;
	vector <string> vStripEdFiles;

#define OUTPUT sFinalOutput+=
#define OUTPUTSTRIP vStripEdFiles[vStripEdFiles.size()-1] +=

	OUTPUT("### UI_Dump(): Top\n");

	for (ReferencesAndPackages_t::iterator it = ReferencesAndPackage.begin(); it!=ReferencesAndPackage.end(); ++it)
	{	
		if ( (*it).second.size()>1)
		{
			OUTPUT(va("!!!DUP:  Ref \"%s\" exists in:\n",(*it).first.c_str()));
			StringSet_t &Set = (*it).second;
			for (StringSet_t::iterator itS = Set.begin(); itS!=Set.end(); ++itS)
			{
				OUTPUT(va("%s\n",(*itS).c_str()));
			}
		}
	}

	OUTPUT("\nSP Package Reference list:\n");

	for (StringSet_t::iterator itS = MenusUsed.begin(); itS!=MenusUsed.end(); ++itS)
	{
		OUTPUT(va("%s\n",(*itS).c_str()));
	}

	OUTPUT("\nBad Text list:\n");

	for (References_t::iterator itBad=BadReferences.begin(); itBad!=BadReferences.end();++itBad)
	{
		Reference_t &BadReference = (*itBad);

		OUTPUT(va("File: %30s  \"%s\"\n",BadReference.sMenu.c_str(), BadReference.sString.c_str()));
	}

	OUTPUT("\nAdding bad references to final correction list...\n");

	for (itBad=BadReferences.begin(); itBad!=BadReferences.end();++itBad)
	{	
		Reference_t &BadReference = (*itBad);

		EnterRef("", BadReference.sString.c_str(), BadReference.sMenu.c_str() );
	}	


	OUTPUT("\nFinal correction list:\n");

//	qboolean bIsMulti = !!strstr((*CorrectionData.begin()).sMenuFile.c_str(),"jk2mp");

	// actually do the find/replace...
	//
	for (CorrectionData_t::iterator itCorrectionData = CorrectionData.begin(); itCorrectionData != CorrectionData.end(); ++itCorrectionData)
	{
		CorrectionDataItem_t &CorrectionDataItem = (*itCorrectionData);

		if (CorrectionDataItem.sTextToFind.c_str()[0] && CorrectionDataItem.sTextToReplaceWith.c_str()[0])
		{
			OUTPUT( va("Load File: \"%s\", find \"%s\", replace with \"%s\"\n", 
												CorrectionDataItem.sMenuFile.c_str(),
															CorrectionDataItem.sTextToFind.c_str(),
																				CorrectionDataItem.sTextToReplaceWith.c_str()
						)
					);


//			if (strstr(CorrectionDataItem.sTextToReplaceWith.c_str(),"START_A_NEW_GAME"))
//			{
//				int z=1;
//			}
			assert( CorrectionDataItem.sTextToReplaceWith.c_str()[0] );
			string	sReplace(	CorrectionDataItem.sTextToReplaceWith.c_str() );
					sReplace.insert(1,"_");
					sReplace.insert(1,CorrectionDataItem.sStripEdFileRef.c_str());								
				
			DoFileFindReplace(	CorrectionDataItem.sMenuFile.c_str(),
								CorrectionDataItem.sTextToFind.c_str(),
								sReplace.c_str()//CorrectionDataItem.sTextToReplaceWith.c_str()
								);
		}
	}


	// scan in all SP files into one huge string, so I can pick out any foreign translations to add in when generating
	//	new StripEd files...
	//
	char **ppsFiles;
	char *buffers[1000];	// max # SP files, well-OTT.
	int iNumFiles;
	int i;
	string sStripFiles;

	// scan for shader files
	ppsFiles = FS_ListFiles( "strip", ".sp", &iNumFiles );
	if ( !ppsFiles || !iNumFiles )
	{
		assert(0);
	}
	else
	{
		// load files...
		//
		for (i=0; i<iNumFiles; i++)
		{
			char sFilename[MAX_QPATH];

			Com_sprintf( sFilename, sizeof( sFilename ), "strip/%s", ppsFiles[i] );
			OutputDebugString( va("...loading '%s'\n", sFilename ) );
			int iLen = FS_ReadFile( sFilename, (void **)&buffers[i] );
			if ( iLen<1 ) {
				assert(0);//Com_Error( ERR_DROP, "Couldn't load %s", filename );
			}
		}

		// free up memory...
		//
		FS_FreeFileList( ppsFiles );

		// build single large buffer and free up buffers as we go...
		//		
		// ( free in reverse order, so the temp files are all dumped )
		for ( i=iNumFiles-1; i>=0; i-- ) 
		{
			sStripFiles += buffers[i];
			sStripFiles += "\r\n";

			FS_FreeFile( buffers[i] );
		}
	}

	int iIndex=0;
	for (itCorrectionData = CorrectionData.begin(); itCorrectionData != CorrectionData.end(); ++itCorrectionData)
	{
		CorrectionDataItem_t &CorrectionDataItem = (*itCorrectionData);

		if (CorrectionDataItem.sStripEdReference.c_str()[0]	// skip over duplicate-resolving entries
//			&& CorrectionDataItem.sStripEdText.c_str()[0]	//
			)
		{	
			string strAnyForeignStringsFound;	// will be entire line plus CR
			string strNotes;					// will be just the bit within quotes

			LPCSTR psFoundExisting;
			int iInitialSearchPos = 0;
			while (iInitialSearchPos < sStripFiles.size() &&
					(strAnyForeignStringsFound.empty() || strNotes.empty())
					)
			{
				if ( (psFoundExisting = strstr( sStripFiles.c_str()+iInitialSearchPos, va("\"%s\"",CorrectionDataItem.sStripEdText.c_str()))) != NULL )
				{
					// see if we can find any NOTES entry above this...
					//
					LPCSTR p;

					if (strNotes.empty())
					{
						p = psFoundExisting;
						while (p > sStripFiles.c_str() && *p!='{')
						{
							if (!strnicmp(p,"NOTES",5) && isspace(p[-1]) && isspace(p[5]))
							{
								p = strchr(p,'"');
								if (!p++) 
									break;
								while (*p != '"')
									strNotes += *p++;
								break;
							}
							p--;
						}
					}

					// now search for any foreign versions we already have translated...
					//
					if (strAnyForeignStringsFound.empty())
					{
						p = psFoundExisting;
						LPCSTR psNextBrace = strchr(p,'}');
						assert(psNextBrace);
						if (psNextBrace)
						{
							for (int i=2; i<10; i++)
							{							
								LPCSTR psForeign = strstr(p,va("TEXT_LANGUAGE%d",i));
								if (psForeign && psForeign < psNextBrace)
								{
									strAnyForeignStringsFound += "   ";
									while (*psForeign != '\n' && *psForeign != '\0')
									{
										strAnyForeignStringsFound += *psForeign++;
									}
									strAnyForeignStringsFound += "\n";
								}
							}
						}
					}

					iInitialSearchPos = psFoundExisting - sStripFiles.c_str();
					iInitialSearchPos++;	// one past, so we don't re-find ourselves
				}
				else
				{
					break;
				}
			}

			if (!strNotes.empty())
			{
				strNotes = va("   NOTES \"%s\"\n",strNotes.c_str());
			}

			// now do output...
			//
			if (!(iIndex%256))
			{
				string s;
				vStripEdFiles.push_back(s);

				OUTPUTSTRIP(	va(	"VERSION 1\n"
								"CONFIG W:\\bin\\striped.cfg\n"
								"ID %d\n"
								"REFERENCE MENUS%d\n"
								"DESCRIPTION \"menu text\"\n"
								"COUNT 256\n",		// count will need correcting for last one
								250 + (iIndex/256),	// 250 range seems to be unused
								iIndex/256
								)
							);

//				OUTPUTSTRIP( va("REFERENCE %s\n", va("%sMENUS%d",bIsMulti?"MP":"SP",iIndex/256)) );
//				OUTPUTSTRIP( va("REFERENCE %s\n", va(  "MENUS%d",iIndex/256)) );
			}

			OUTPUTSTRIP( va(	"INDEX %d\n"
								"{\n"
								"   REFERENCE %s\n"
								"%s"
								"   TEXT_LANGUAGE1 \"%s\"\n"
								"%s"
								"}\n",
								iIndex%256,
								CorrectionDataItem.sStripEdReference.c_str(),
								(strNotes.empty()?"":strNotes.c_str()),
								CorrectionDataItem.sStripEdText.c_str(),
								strAnyForeignStringsFound.c_str()
								)
							);

			iIndex++;
		}
	}

	OUTPUT("### UI_Dump(): Bottom\n");

	SendStringToNotepad(sFinalOutput.c_str(), "temp.txt");

	// output the SP files...
	//
	for (i=0; i<vStripEdFiles.size(); i++)
	{
		// need to make local string, because ingame va() is crippled to 2 depths...
		//
		char sName[MAX_PATH];
		sprintf(sName,"Source\\StarWars\\code\\base\\strip\\MENUS%d.sp",i);
		SendStringToNotepad(vStripEdFiles[i].c_str(), sName);
	}
}


void UI_Debug_EnterReference(LPCSTR ps4LetterType, LPCSTR psItemString)
{
	if ((int)psItemString < 0)	//string package ID
	{
		LPCSTR psPackageName, psPackageReference, psText;
		psItemString = SP_GetReferenceText(-(int)psItemString, psPackageName, psPackageReference, psText);
		if (psItemString && psItemString[0]) {
//			OutputDebugString(va("    TEXT: \"%s\" Package: %s\n",psItemString,psPackageReference));
			EnterGoodRef(ps4LetterType,psItemString,psPackageReference,psText);
		}
		else {
			assert(0);
		}
	}
	else
	{	
		if (psItemString && psItemString[0]) 
		{
			if ( !stricmp(psItemString,"english") ||
				 !stricmp(psItemString,"francais") ||
				 !stricmp(psItemString,"deutsch")
				 )
			{
				// then don't localise it!
			}
			else
			{
				//OutputDebugString(va("BAD TEXT: \"%s\"  REF: \"%s\"\n",psItemString, EnterBadText(psItemString)));
				EnterBadRef(ps4LetterType,psItemString);
			}
		}		
		else {
			assert(0);
		}
	}
}

#endif

#endif

