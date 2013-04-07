// Filename:-	parser.cpp
//
#include "stdafx.h"
#include "includes.h"
#include "stl.h"
//
#include "parser.h"


// Very simple parser, I just read the "alias" part of a raven-generic file, and store them into
//	a string map. 
//
// Example file:
//
/*
Alias
{
	"srcarples"	"boltpoint_righthand"
}

Alias
{
	"slcarples"	"boltpoint_lefthand"
}
*/
//  (possibly more than one per "Alias" brace? I'll code for it.
// 
// return = success / fail...
//
bool Parser_Load(LPCSTR psFullPathedFilename, MappedString_t &ParsedAliases)
{
	bool bReturn = false;

	ParsedAliases.clear();

	FILE *fhHandle = fopen(psFullPathedFilename,"rt");
	if (fhHandle)
	{			
		bool bParsingBlock = false;
		bool bSkippingBlock= false;
		char sLine[1024];

		while (fgets(sLine,sizeof(sLine)-1,fhHandle)!=NULL)
		{
			sLine[sizeof(sLine)-1]='\0';

			// :-)
			CString str(sLine);
					str.TrimLeft();
					str.TrimRight();

			strcpy(sLine,str);

			if (!bSkippingBlock)
			{
				if (!bParsingBlock)
				{
					if (strlen(sLine))	// found any kind of header?
					{
						if (!stricmp(sLine,"Alias"))
						{							
							bParsingBlock = true;						
						}
						else							
						{
							// not a recognised header, so...
							//
							bSkippingBlock = true;
						}
					}
					continue;
				}
				else
				{
					if (!stricmp(sLine,"{"))
						continue;

					if (!stricmp(sLine,"}"))
					{
						bParsingBlock = false;
						continue;
					}

					if (strlen(sLine))
					{
						// must be a value pair, so...
						//
						// first, find the whitespace that seperates them...
						//
						CString strPair(sLine);
						int iLoc = strPair.FindOneOf(" \t");
						if (iLoc == -1)
						{
							assert(0);
							ErrorBox(va("Parser_Load(): Couldn't find whitespace-seperator in line:\n\n\"%s\"\n\n( File: \"%s\" )",(LPCSTR) strPair,psFullPathedFilename));
							bReturn = false;
							break;
						}

						// stl & MFC rule!...
						//
						CString strArg_Left(strPair.Left(iLoc));	// real name
								strArg_Left.TrimRight();
								strArg_Left.Replace("\"","");

						CString strArg_Right(strPair.Mid (iLoc));	// alias name
								strArg_Right.TrimLeft();
								strArg_Right.Replace("\"","");

						ParsedAliases[(LPCSTR)strArg_Left] = (LPCSTR)strArg_Right;

						bReturn = true;
						continue;
					}
				}
			}
			else
			{
				// skip to close brace...
				//
				if (stricmp(sLine,"}"))
					continue;

				bSkippingBlock = false;
			}
		}

		fclose(fhHandle);
	}
	// DT EDIT
	/*
	else
	{
		ErrorBox( va("Couldn't open file: %s\n", psFullPathedFilename));
		return false;
	}
	*/

	return bReturn;
}


/////////////// eof /////////////

