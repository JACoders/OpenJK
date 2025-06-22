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

// Filename:-	cg_credits.cpp
//
// module for end credits code

#include "cg_local.h"
#include "cg_media.h"


#define fCARD_FADESECONDS		1.0f	// fade up time, also fade down time
#define fCARD_SUSTAINSECONDS	2.0f	// hold time before fade down
#define fLINE_SECONDTOSCROLLUP	15.0f	// how long one line takes to scroll up the screen


#define MAX_LINE_BYTES 2048

qhandle_t ghFontHandle = 0;
float gfFontScale = 1.0f;
vec4_t gv4Color = {0};

struct StringAndSize_t
{
	int iStrLenPixels;
	std::string str;

	StringAndSize_t()
	{
		iStrLenPixels = -1;
		str = "";
	}
	StringAndSize_t(const char *psString)
	{
		iStrLenPixels = -1;
		str = psString;
	}
	StringAndSize_t & operator = (const char *psString)
	{
		iStrLenPixels = -1;
		str = psString;
		return *this;
	}

	const char *c_str(void)
	{
		return str.c_str();
	}

	int GetPixelLength(void)
	{
		if (iStrLenPixels == -1)
		{
			iStrLenPixels = cgi_R_Font_StrLenPixels(str.c_str(), ghFontHandle, gfFontScale);
		}

		return iStrLenPixels;
	}

	bool IsEmpty(void)
	{
		return str.empty();
	}
};

struct CreditCard_t
{
	int						iTime;
	StringAndSize_t			strTitle;
	std::vector<StringAndSize_t> vstrText;

	CreditCard_t()
	{
		iTime = -1;	// flag "not set yet"
	}
};

struct CreditLine_t
{
	int						iLine;
	StringAndSize_t			strText;
	std::vector<StringAndSize_t> vstrText;
	bool					bDotted;
};

typedef std::list <CreditLine_t>		CreditLines_t;
typedef std::list <CreditCard_t>		CreditCards_t;

struct CreditData_t
{
	int			iStartTime;

	CreditCards_t CreditCards;
	CreditLines_t CreditLines;

	qboolean Running(void)
	{
		return (qboolean)(CreditCards.size() || CreditLines.size());
	}
};

CreditData_t CreditData;


static const char *Capitalize( const char *psTest )
{
	static char sTemp[MAX_LINE_BYTES];

	Q_strncpyz(sTemp, psTest, sizeof(sTemp));

	if (!cgi_Language_IsAsian())
	{
		Q_strupr(sTemp);	// capitalise titles (if not asian!!!!)
	}

	return sTemp;
}

static bool CountsAsWhiteSpaceForCaps( char c )
{
	return !!(isspace(c) || c == '-' || c == '.' || c == '(' || c == ')');
}
static const char *UpperCaseFirstLettersOnly( const char *psTest )
{
	static char sTemp[MAX_LINE_BYTES];

	Q_strncpyz(sTemp, psTest, sizeof(sTemp));

	if (!cgi_Language_IsAsian())
	{
		Q_strlwr(sTemp);

		char *p = sTemp;
		while (*p)
		{
			while (*p && CountsAsWhiteSpaceForCaps(*p)) p++;//(isspace(*p) || *p == '-' || *p == '.')) p++;	// also copes with hyphenated names (awkward gits)
			if (*p)
			{
				*p = toupper(*p);
				while (*p && !CountsAsWhiteSpaceForCaps(*p)) p++;	// cope with hyphenated names and initials (awkward gits)
			}
		}
	}

	// now restore any weird stuff...
	//
	char *p = strstr(sTemp," Mc");	// eg "Mcfarrell" should be "McFarrell"
	if (p && isalpha(p[3]))
	{
		p[3] = toupper(p[3]);
	}
	p = strstr(sTemp," O'");	// eg "O'flaherty" should be "O'Flaherty"
	if (p && isalpha(p[3]))
	{
		p[3] = toupper(p[3]);
	}
	p = strstr(sTemp,"Lucasarts");
	if (p)
	{
		p[5] = 'A';	// capitalise the 'A' in LucasArts (jeez...)
	}

	return sTemp;
}

static const char *GetSubString(std::string &strResult)
{
	static char sTemp[MAX_LINE_BYTES];

	if (!strlen(strResult.c_str()))
		return NULL;

	Q_strncpyz(sTemp,strResult.c_str(),sizeof(sTemp));

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

	return sTemp;
}

// sort entries by their last name (starts at back of string and moves forward until start or just before whitespace)
// ...
static bool SortBySurname(const StringAndSize_t &str1, const StringAndSize_t &str2)
{
	std::string::const_reverse_iterator rstart1 = std::find_if(str1.str.rbegin(), str1.str.rend(), isspace);
	std::string::const_reverse_iterator rstart2 = std::find_if(str2.str.rbegin(), str2.str.rend(), isspace);


	return Q_stricmp(&*rstart1.base(), &*rstart2.base()) < 0;
}



void CG_Credits_Init( const char *psStripReference, vec4_t *pv4Color )
{
	// could make these into parameters later, but for now...
	//
	ghFontHandle = cgs.media.qhFontMedium;
	gfFontScale = 1.0f;

	memcpy(gv4Color,pv4Color,sizeof(gv4Color));	// memcpy so we can poke into alpha channel

	// first, ask the strlen of the final string...
	//
	int iStrLen = cgi_SP_GetStringTextString( psStripReference, NULL, 0 );
	if (!iStrLen)
	{
#ifndef FINAL_BUILD
		Com_Printf("WARNING: CG_Credits_Init(): invalid text key :'%s'\n", psStripReference);
#endif
		return;
	}
	//
	// malloc space to hold it...
	//
	char *psMallocText = (char *) cgi_Z_Malloc( iStrLen+1, TAG_TEMP_WORKSPACE );
	//
	// now get the string...
	//
	iStrLen = cgi_SP_GetStringTextString( psStripReference, psMallocText, iStrLen+1 );
	//ensure we found a match
	if (!iStrLen)
	{
		assert(0);	// should never get here now, but wtf?
		cgi_Z_Free(psMallocText);
#ifndef FINAL_BUILD
		Com_Printf("WARNING: CG_Credits_Init(): invalid text key :'%s'\n", psStripReference);
#endif
		return;
	}

	// read whole string in and process as cards, lines etc...
	//
	typedef enum
	{
		eNothing = 0,
		eLine,
		eDotEntry,
		eTitle,
		eCard,
		eFinished,
	} Mode_e;
	Mode_e eMode = eNothing;

	qboolean bCardsFinished = qfalse;
	int iLineNumber = 0;
	const char *psTextParse = psMallocText;
	while (*psTextParse != '\0')
	{
		// read a line...
		//
		char sLine[MAX_LINE_BYTES];
			 sLine[0]='\0';
		qboolean bWasCommand = qtrue;
		while (1)
		{
			qboolean bIsTrailingPunctuation;
			unsigned int uiLetter = cgi_AnyLanguage_ReadCharFromString(&psTextParse, &bIsTrailingPunctuation);

			// concat onto string so far...
			//
			if (uiLetter == 32 && sLine[0] == '\0')
			{
				continue;	// unless it's a space at the start of a line, in which case ignore it.
			}

			if (uiLetter == '\n' || uiLetter == '\0' )
			{
				// have we got a command word?...
				//
				if (!Q_stricmpn(sLine,"(#",2))
				{
					// yep...
					//
					if (!Q_stricmp(sLine, "(#CARD)"))
					{
						if (!bCardsFinished)
						{
							eMode = eCard;
						}
						else
						{
							#ifndef FINAL_BUILD
							Com_Printf( S_COLOR_YELLOW "CG_Credits_Init(): No current support for cards after scroll!\n" );
							#endif
							eMode = eNothing;
						}
						break;
					}
					else
					if (!Q_stricmp(sLine, "(#TITLE)"))
					{
						eMode = eTitle;
						bCardsFinished = qtrue;
						break;
					}
					else
					if (!Q_stricmp(sLine, "(#LINE)"))
					{
						eMode = eLine;
						bCardsFinished = qtrue;
						break;
					}
					else
					if (!Q_stricmp(sLine, "(#DOTENTRY)"))
					{
						eMode = eDotEntry;
						bCardsFinished = qtrue;
						break;
					}
					else
					{
						#ifndef FINAL_BUILD
						Com_Printf( S_COLOR_YELLOW "CG_Credits_Init(): bad keyword \"%s\"!\n", sLine );
						#endif
						eMode = eNothing;
					}
				}
				else
				{
					// I guess not...
					//
					bWasCommand = qfalse;
					break;
				}
			}
			else
			{
				// must be a letter...
				//
				if (uiLetter > 255)
				{
					Q_strcat(sLine, sizeof(sLine), va("%c%c",uiLetter >> 8, uiLetter & 0xFF));
				}
				else
				{
					Q_strcat(sLine, sizeof(sLine), va("%c",uiLetter & 0xFF));
				}
			}
		}

		// command?...
		//
		if (bWasCommand)
		{
			// this'll just be a mode change, so ignore...
			//
		}
		else
		{
			// else we've got some text to display...
			//
			switch (eMode)
			{
				case eNothing:	break;
				case eLine:
				{
					CreditLine_t	CreditLine;
									CreditLine.iLine	= iLineNumber++;
									CreditLine.strText	= sLine;

					CreditData.CreditLines.push_back( CreditLine );
				}
				break;

				case eDotEntry:
				{
					CreditLine_t	CreditLine;
									CreditLine.iLine	= iLineNumber;
									CreditLine.bDotted	= true;

					std::string strResult(sLine);
					const char *p;
					while ((p=GetSubString(strResult)) != NULL)
					{
						if (CreditLine.strText.IsEmpty())
						{
							CreditLine.strText = p;
						}
						else
						{
							CreditLine.vstrText.push_back( UpperCaseFirstLettersOnly(p) );
						}
					}

					if (!CreditLine.strText.IsEmpty() && CreditLine.vstrText.size())
					{
						// sort entries RHS dotted entries by alpha...
						//
						std::sort( CreditLine.vstrText.begin(), CreditLine.vstrText.end(), SortBySurname );

						CreditData.CreditLines.push_back( CreditLine );
						iLineNumber += CreditLine.vstrText.size();
					}
				}
				break;

				case eTitle:
				{
					iLineNumber++;	// leading blank line

					CreditLine_t	CreditLine;
									CreditLine.iLine	= iLineNumber++;
									CreditLine.strText	= Capitalize(sLine);

					CreditData.CreditLines.push_back( CreditLine );

					iLineNumber++;	// trailing blank line
					break;
				}
				case eCard:
				{
					CreditCard_t CreditCard;

					std::string strResult(sLine);
					const char *p;
					while ((p=GetSubString(strResult)) != NULL)
					{
						if (CreditCard.strTitle.IsEmpty())
						{
							CreditCard.strTitle = Capitalize( p );
						}
						else
						{
							CreditCard.vstrText.push_back( UpperCaseFirstLettersOnly( p ) );
						}
					}

					if (!CreditCard.strTitle.IsEmpty())
					{
						// sort entries by alpha...
						//
						std::sort( CreditCard.vstrText.begin(), CreditCard.vstrText.end(), SortBySurname );

						CreditData.CreditCards.push_back(CreditCard);
					}
				}
				break;
				default:
				break;
			}
		}
	}

	cgi_Z_Free(psMallocText);
	CreditData.iStartTime = cg.time;
}

qboolean CG_Credits_Running( void )
{
	return CreditData.Running();
}

// returns qtrue if still drawing...
//
qboolean CG_Credits_Draw( void )
{
	if ( CG_Credits_Running() )
	{
		const int iFontHeight = (int) (1.5f * (float) cgi_R_Font_HeightPixels(ghFontHandle, gfFontScale));	// taiwanese & japanese need 1.5 fontheight spacing

//		cgi_R_SetColor( *gpv4Color );

		// display cards first...
		//
		if (CreditData.CreditCards.size())
		{
			// grab first card off the list (we know there's at least one here, so...)
			//
			CreditCard_t &CreditCard = (*CreditData.CreditCards.begin());

			if (CreditCard.iTime == -1)
			{
				// onceonly time init...
				//
				CreditCard.iTime = cg.time;
			}

			// play with the alpha channel for fade up/down...
			//
			const float fMilliSecondsElapsed = cg.time - CreditCard.iTime;
			const float fSecondsElapsed		 = fMilliSecondsElapsed / 1000.0f;
			if (fSecondsElapsed < fCARD_FADESECONDS)
			{
				// fading up...
				//
				gv4Color[3] = fSecondsElapsed / fCARD_FADESECONDS;
//				OutputDebugString(va("fade up: %f\n",gv4Color[3]));
			}
			else
			if (fSecondsElapsed > fCARD_FADESECONDS + fCARD_SUSTAINSECONDS)
			{
				// fading down...
				//
				const float fFadeDownSeconds = fSecondsElapsed - (fCARD_FADESECONDS + fCARD_SUSTAINSECONDS);
				gv4Color[3] = 1.0f - (fFadeDownSeconds / fCARD_FADESECONDS);
//				OutputDebugString(va("fade dw: %f\n",gv4Color[3]));
			}
			else
			{
				gv4Color[3] = 1.0f;
//				OutputDebugString(va("normal: %f\n",gv4Color[3]));
			}
			if (gv4Color[3] < 0.0f)
				gv4Color[3] = 0.0f;	// ... otherwise numbers that have dipped slightly -ve flash up fullbright after fade down

			//
			// how many lines is it?
			//
			int iLines = CreditCard.vstrText.size() + 2;	// +2 for title itself & one seperator line
			//
			int iYpos = (SCREEN_HEIGHT - (iLines * iFontHeight))/2;
			//
			// draw it, title first...
			//
			int iWidth = CreditCard.strTitle.GetPixelLength();
			int iXpos  = (SCREEN_WIDTH - iWidth)/2;
			cgi_R_Font_DrawString(iXpos, iYpos, CreditCard.strTitle.c_str(), gv4Color, ghFontHandle, -1, gfFontScale);
			//
			iYpos += iFontHeight*2;	// skip blank line then move to main pos
			//
			for (size_t i=0; i<CreditCard.vstrText.size(); i++)
			{
				StringAndSize_t &StringAndSize = CreditCard.vstrText[i];
				iWidth = StringAndSize.GetPixelLength();
				iXpos  = (SCREEN_WIDTH - iWidth)/2;
				cgi_R_Font_DrawString(iXpos, iYpos, StringAndSize.c_str(), gv4Color, ghFontHandle, -1, gfFontScale);
				iYpos += iFontHeight;
			}

			// next card?...
			//
			if (fSecondsElapsed > fCARD_FADESECONDS + fCARD_SUSTAINSECONDS + fCARD_FADESECONDS)
			{
				// yep, so erase the first entry (which will trigger the next one to be initialised on re-entry)...
				//
				CreditData.CreditCards.erase( CreditData.CreditCards.begin() );

				if (!CreditData.CreditCards.size())
				{
					// all cards gone, so re-init timer for lines...
					//
					CreditData.iStartTime = cg.time;
				}
			}
			//
			return qtrue;
		}
		else
		{
			// doing scroll text...
			//
			if (CreditData.CreditLines.size())
			{
				// process all lines...
				//
				const float fMilliSecondsElapsed = cg.time - CreditData.iStartTime;
				const float fSecondsElapsed		 = fMilliSecondsElapsed / 1000.0f;

				bool bEraseOccured = false;
				for (CreditLines_t::iterator it = CreditData.CreditLines.begin(); it != CreditData.CreditLines.end(); bEraseOccured ? it : ++it)
				{
					CreditLine_t &CreditLine = (*it);
					bEraseOccured = false;

					static const float fPixelsPerSecond = ((float)SCREEN_HEIGHT / fLINE_SECONDTOSCROLLUP);

					int iYpos = SCREEN_HEIGHT + (CreditLine.iLine * iFontHeight);
						iYpos-= (int) (fPixelsPerSecond * fSecondsElapsed);

					int iTextLinesThisItem = Q_max( (int)CreditLine.vstrText.size(), 1);
					if (iYpos + (iTextLinesThisItem * iFontHeight) < 0)
					{
						// scrolled off top of screen, so erase it...
						//
						it = CreditData.CreditLines.erase( it );
						bEraseOccured = true;
					}
					else
					if (iYpos < SCREEN_HEIGHT)
					{
						// onscreen, so print it...
						//
						bool bIsDotted = !!CreditLine.vstrText.size();	// eg "STUNTS ...................... MR ED"

						int iWidth = CreditLine.strText.GetPixelLength();
						int iXpos  = bIsDotted ? 4 : (SCREEN_WIDTH - iWidth)/2;

						gv4Color[3] = 1.0f;

						cgi_R_Font_DrawString(iXpos, iYpos, CreditLine.strText.c_str(), gv4Color, ghFontHandle, -1, gfFontScale);

						// now print any dotted members...
						//
						for (size_t i=0; i<CreditLine.vstrText.size(); i++)
						{
							StringAndSize_t &StringAndSize = CreditLine.vstrText[i];
							iWidth = StringAndSize.GetPixelLength();
							iXpos  = (SCREEN_WIDTH-4 - iWidth);
							cgi_R_Font_DrawString(iXpos, iYpos, StringAndSize.c_str(), gv4Color, ghFontHandle, -1, gfFontScale);
							iYpos += iFontHeight;
						}
					}
				}

				return qtrue;
			}
		}
	}

	return qfalse;
}



////////////////////// eof /////////////////////

