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

#include "qcommon/sstring.h"	// stl string class won't compile in here (MS shite), so use Gil's.
#include "tr_local.h"
#include "tr_font.h"

#include "qcommon/stringed_ingame.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// This file is shared in the single and multiplayer codebases, so be CAREFUL WHAT YOU ADD/CHANGE!!!!!
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef enum
{
	eWestern,	// ( I only care about asian languages in here at the moment )
	eRussian,	//  .. but now I need to care about this, since it uses a different TP
	ePolish,	// ditto
	eKorean,
	eTaiwanese,	// 15x15 glyphs tucked against BR of 16x16 space
	eJapanese,	// 15x15 glyphs tucked against TL of 16x16 space
	eChinese,	// 15x15 glyphs tucked against TL of 16x16 space
	eThai,		// 16x16 cells with glyphs against left edge, special file (tha_widths.dat) for variable widths
} Language_e;

// this is to cut down on all the stupid string compares I've been doing, and convert asian stuff to switch-case
//
Language_e GetLanguageEnum()
{
	static int			iSE_Language_ModificationCount = -1234;	// any old silly value that won't match the cvar mod count
	static Language_e	eLanguage = eWestern;

	// only re-strcmp() when language string has changed from what we knew it as...
	//
	if (iSE_Language_ModificationCount != se_language->modificationCount )
	{
		iSE_Language_ModificationCount  = se_language->modificationCount;

				if ( Language_IsRussian()	)	eLanguage = eRussian;
		else	if ( Language_IsPolish()	)	eLanguage = ePolish;
		else	if ( Language_IsKorean()	)	eLanguage = eKorean;
		else	if ( Language_IsTaiwanese()	)	eLanguage = eTaiwanese;
		else	if ( Language_IsJapanese()	)	eLanguage = eJapanese;
		else	if ( Language_IsChinese()	)	eLanguage = eChinese;
		else	if ( Language_IsThai()		)	eLanguage = eThai;
		else	eLanguage = eWestern;
	}

	return eLanguage;
}

struct SBCSOverrideLanguages_t
{
	const char *m_psName;
	Language_e	m_eLanguage;
};

// so I can do some stuff with for-next loops when I add polish etc...
//
SBCSOverrideLanguages_t g_SBCSOverrideLanguages[]=
{
	{"russian",	eRussian},
	{"polish",	ePolish},
	{NULL,		eWestern}
};



//================================================
//

#define sFILENAME_THAI_WIDTHS	"fonts/tha_widths.dat"
#define sFILENAME_THAI_CODES	"fonts/tha_codes.dat"

struct ThaiCodes_t
{
	std::map <int, int>	m_mapValidCodes;
	std::vector<int>		m_viGlyphWidths;
	sstring_t		m_strInitFailureReason;	// so we don't have to keep retrying to work this out

	void Clear( void )
	{
		m_mapValidCodes.clear();
		m_viGlyphWidths.clear();
		m_strInitFailureReason = "";	// if blank, never failed, else says don't bother re-trying
	}

	ThaiCodes_t()
	{
		Clear();
	}

	// convert a supplied 1,2 or 3-byte multiplied-up integer into a valid 0..n index, else -1...
	//
	int GetValidIndex( int iCode )
	{
		std::map <int,int>::iterator it = m_mapValidCodes.find( iCode );
		if (it != m_mapValidCodes.end())
		{
            return (*it).second;
		}

		return -1;
	}

	int GetWidth( int iGlyphIndex )
	{
		if (iGlyphIndex < (int)m_viGlyphWidths.size())
		{
			return m_viGlyphWidths[ iGlyphIndex ];
		}

		assert(0);
		return 0;
	}

	// return is error message to display, or NULL for success
	const char *Init(void)
	{
		if (m_mapValidCodes.empty() && m_viGlyphWidths.empty())
		{
			if (m_strInitFailureReason.empty())	// never tried and failed already?
			{
				int *piData = NULL;	// note <int>, not <byte>, for []-access
				//
				// read the valid-codes table in...
				//
				int iBytesRead = ri.FS_ReadFile( sFILENAME_THAI_CODES, (void **) &piData );
				if (iBytesRead > 0 && !(iBytesRead&3))	// valid length and multiple of 4 bytes long
				{
					int iTableEntries = iBytesRead / sizeof(int);

					for (int i=0; i < iTableEntries; i++)
					{
						m_mapValidCodes[ piData[i] ] = i;	// convert MBCS code to sequential index...
					}
					ri.FS_FreeFile( piData );	// dispose of original

					// now read in the widths... (I'll keep these in a simple STL vector, so they'all disappear when the <map> entries do...
					//
					iBytesRead = ri.FS_ReadFile( sFILENAME_THAI_WIDTHS, (void **) &piData );
					if (iBytesRead > 0 && !(iBytesRead&3) && iBytesRead>>2/*sizeof(int)*/ == iTableEntries)
					{
						for (int i=0; i<iTableEntries; i++)
						{
							m_viGlyphWidths.push_back( piData[i] );
						}
						ri.FS_FreeFile( piData );	// dispose of original
					}
					else
					{
						m_strInitFailureReason = va("Error with file \"%s\", size = %d!\n", sFILENAME_THAI_WIDTHS, iBytesRead);
					}
				}
				else
				{
					m_strInitFailureReason = va("Error with file \"%s\", size = %d!\n", sFILENAME_THAI_CODES, iBytesRead);
				}
			}
		}

		return m_strInitFailureReason.c_str();
	}
};


#define GLYPH_MAX_KOREAN_SHADERS	3
#define GLYPH_MAX_TAIWANESE_SHADERS 4
#define GLYPH_MAX_JAPANESE_SHADERS	3
#define GLYPH_MAX_CHINESE_SHADERS	3
#define GLYPH_MAX_THAI_SHADERS		3
#define GLYPH_MAX_ASIAN_SHADERS		4	// this MUST equal the larger of the above defines

class CFontInfo
{
private:
	// From the fontdat file
	glyphInfo_t		mGlyphs[GLYPH_COUNT];

//	int				mAsianHack;				// unused junk from John's fontdat file format.
	// end of fontdat data

	int				mShader;   				// handle to the shader with the glyph

	int				m_hAsianShaders[GLYPH_MAX_ASIAN_SHADERS];	// shaders for Korean glyphs where applicable
	glyphInfo_t		m_AsianGlyph;			// special glyph containing asian->western scaling info for all glyphs
	int				m_iAsianGlyphsAcross;	// needed to dynamically calculate S,T coords
	int				m_iAsianPagesLoaded;
	bool			m_bAsianLastPageHalfHeight;
	int				m_iLanguageModificationCount;	// doesn't matter what this is, so long as it's comparable as being changed

	ThaiCodes_t		*m_pThaiData;

public:
	char			m_sFontName[MAX_QPATH];	// eg "fonts/lcd"	// needed for korean font-hint if we need >1 hangul set
	int				mPointSize;
	int				mHeight;
	int				mAscender;
	int				mDescender;

	bool			mbRoundCalcs;	// trying to make this !@#$%^ thing work with scaling
	int				m_iThisFont;	// handle to itself
	int				m_iAltSBCSFont;	// -1 == NULL // alternative single-byte font for languages like russian/polish etc that need to override high characters ?
	int				m_iOriginalFontWhenSBCSOverriden;
	float			m_fAltSBCSFontScaleFactor;	// -1, else amount to adjust returned values by to make them fit the master western font they're substituting for
	bool			m_bIsFakeAlienLanguage;	// ... if true, don't process as MBCS or override as SBCS etc

	CFontInfo(const char *fontName);
//	CFontInfo(int fill) { memset(this, fill, sizeof(*this)); }	// wtf?
	~CFontInfo(void) {}

	const int GetPointSize(void) const { return(mPointSize); }
	const int GetHeight(void) const { return(mHeight); }
	const int GetAscender(void) const { return(mAscender); }
	const int GetDescender(void) const { return(mDescender); }

	const glyphInfo_t *GetLetter(const unsigned int uiLetter, int *piShader = NULL);
	const int GetCollapsedAsianCode(ulong uiLetter) const;

	const int GetLetterWidth(const unsigned int uiLetter);
	const int GetLetterHorizAdvance(const unsigned int uiLetter);
	const int GetShader(void) const { return(mShader); }

	void FlagNoAsianGlyphs(void) { m_hAsianShaders[0] = 0; m_iLanguageModificationCount = -1; }	// used during constructor
	bool AsianGlyphsAvailable(void) const { return !!(m_hAsianShaders[0]); }

	void UpdateAsianIfNeeded( bool bForceReEval = false);
};

//================================================




// round float to one decimal place...
//
float RoundTenth( float fValue )
{
	return ( floorf( (fValue*10.0f) + 0.5f) ) / 10.0f;
}


int							g_iCurrentFontIndex;	// entry 0 is reserved index for missing/invalid, else ++ with each new font registered
std::vector<CFontInfo *>			g_vFontArray;
typedef std::map<sstring_t, int>	FontIndexMap_t;
							FontIndexMap_t g_mapFontIndexes;
int g_iNonScaledCharRange;	// this is used with auto-scaling of asian fonts, anything below this number is preserved in scale, anything above is scaled down by 0.75f

//paletteRGBA_c				lastcolour;

// =============================== some korean stuff =======================================

#define KSC5601_HANGUL_HIBYTE_START		0xB0	// range is...
#define KSC5601_HANGUL_HIBYTE_STOP		0xC8	// ... inclusive
#define KSC5601_HANGUL_LOBYTE_LOBOUND	0xA0	// range is...
#define KSC5601_HANGUL_LOBYTE_HIBOUND	0xFF	// ...bounding (ie only valid in between these points, but NULLs in charsets for these codes)
#define KSC5601_HANGUL_CODES_PER_ROW	96		// 2 more than the number of glyphs

extern qboolean Language_IsKorean( void );

static inline bool Korean_ValidKSC5601Hangul( byte _iHi, byte _iLo )
{
	return (_iHi >=KSC5601_HANGUL_HIBYTE_START		&&
			_iHi <=KSC5601_HANGUL_HIBYTE_STOP		&&
			_iLo > KSC5601_HANGUL_LOBYTE_LOBOUND	&&
			_iLo < KSC5601_HANGUL_LOBYTE_HIBOUND
			);
}

static inline bool Korean_ValidKSC5601Hangul( unsigned int uiCode )
{
	return Korean_ValidKSC5601Hangul( uiCode >> 8, uiCode & 0xFF );
}


// takes a KSC5601 double-byte hangul code and collapses down to a 0..n glyph index...
// Assumes rows are 96 wide (glyph slots), not 94 wide (actual glyphs), so I can ignore boundary markers
//
// (invalid hangul codes will return 0)
//
static int Korean_CollapseKSC5601HangulCode(unsigned int uiCode)
{
	if (Korean_ValidKSC5601Hangul( uiCode ))
	{
		uiCode -= (KSC5601_HANGUL_HIBYTE_START * 256) + KSC5601_HANGUL_LOBYTE_LOBOUND;	// sneaky maths on both bytes, reduce to 0x0000 onwards
		uiCode  = ((uiCode >> 8) * KSC5601_HANGUL_CODES_PER_ROW) + (uiCode & 0xFF);
		return uiCode;
	}
	return 0;
}

static int Korean_InitFields(int &iGlyphTPs, const char *&psLang)
{
	psLang		= "kor";
	iGlyphTPs	= GLYPH_MAX_KOREAN_SHADERS;
	g_iNonScaledCharRange = 255;
	return 32;	// m_iAsianGlyphsAcross
}

// ======================== some taiwanese stuff ==============================

// (all ranges inclusive for Big5)...
//
#define BIG5_HIBYTE_START0		0xA1	// (misc chars + level 1 hanzi)
#define BIG5_HIBYTE_STOP0		0xC6	//
#define BIG5_HIBYTE_START1		0xC9	// (level 2 hanzi)
#define BIG5_HIBYTE_STOP1		0xF9	//
#define BIG5_LOBYTE_LOBOUND0	0x40	//
#define BIG5_LOBYTE_HIBOUND0	0x7E	//
#define BIG5_LOBYTE_LOBOUND1	0xA1	//
#define BIG5_LOBYTE_HIBOUND1	0xFE	//
#define BIG5_CODES_PER_ROW		160		// 3 more than the number of glyphs

extern qboolean Language_IsTaiwanese( void );

static bool Taiwanese_ValidBig5Code( unsigned int uiCode )
{
	const byte _iHi = (uiCode >> 8)&0xFF;
	if (	(_iHi >= BIG5_HIBYTE_START0 && _iHi <= BIG5_HIBYTE_STOP0)
		||	(_iHi >= BIG5_HIBYTE_START1 && _iHi <= BIG5_HIBYTE_STOP1)
		)
	{
		const byte _iLo = uiCode & 0xFF;

		if ( (_iLo >= BIG5_LOBYTE_LOBOUND0 && _iLo <= BIG5_LOBYTE_HIBOUND0) ||
			 (_iLo >= BIG5_LOBYTE_LOBOUND1 && _iLo <= BIG5_LOBYTE_HIBOUND1)
			)
		{
			return true;
		}
	}

	return false;
}


// only call this when Taiwanese_ValidBig5Code() has already returned true...
//
static bool Taiwanese_IsTrailingPunctuation( unsigned int uiCode )
{
	// so far I'm just counting the first 21 chars, those seem to be all the basic punctuation...
	//
	if (	uiCode >= ((BIG5_HIBYTE_START0<<8)|BIG5_LOBYTE_LOBOUND0) &&
			uiCode <  (((BIG5_HIBYTE_START0<<8)|BIG5_LOBYTE_LOBOUND0)+20)
		)
	{
		return true;
	}

	return false;
}


// takes a BIG5 double-byte code (including level 2 hanzi) and collapses down to a 0..n glyph index...
// Assumes rows are 160 wide (glyph slots), not 157 wide (actual glyphs), so I can ignore boundary markers
//
// (invalid big5 codes will return 0)
//
static int Taiwanese_CollapseBig5Code( unsigned int uiCode )
{
	if (Taiwanese_ValidBig5Code( uiCode ))
	{
		uiCode -= (BIG5_HIBYTE_START0 * 256) + BIG5_LOBYTE_LOBOUND0;	// sneaky maths on both bytes, reduce to 0x0000 onwards
		if ( (uiCode & 0xFF) >= (BIG5_LOBYTE_LOBOUND1-1)-BIG5_LOBYTE_LOBOUND0)
		{
			uiCode -= ((BIG5_LOBYTE_LOBOUND1-1) - (BIG5_LOBYTE_HIBOUND0+1)) -1;
		}
		uiCode = ((uiCode >> 8) * BIG5_CODES_PER_ROW) + (uiCode & 0xFF);
		return uiCode;
	}
	return 0;
}

static int Taiwanese_InitFields(int &iGlyphTPs, const char *&psLang)
{
	psLang		= "tai";
	iGlyphTPs	= GLYPH_MAX_TAIWANESE_SHADERS;
	g_iNonScaledCharRange = 255;
	return 64;	// m_iAsianGlyphsAcross
}

// ======================== some Japanese stuff ==============================


// ( all ranges inclusive for Shift-JIS )
//
#define SHIFTJIS_HIBYTE_START0	0x81
#define SHIFTJIS_HIBYTE_STOP0	0x9F
#define SHIFTJIS_HIBYTE_START1	0xE0
#define SHIFTJIS_HIBYTE_STOP1	0xEF
//
#define SHIFTJIS_LOBYTE_START0	0x40
#define SHIFTJIS_LOBYTE_STOP0	0x7E
#define SHIFTJIS_LOBYTE_START1	0x80
#define SHIFTJIS_LOBYTE_STOP1	0xFC
#define SHIFTJIS_CODES_PER_ROW	(((SHIFTJIS_LOBYTE_STOP0-SHIFTJIS_LOBYTE_START0)+1)+((SHIFTJIS_LOBYTE_STOP1-SHIFTJIS_LOBYTE_START1)+1))


extern qboolean Language_IsJapanese( void );

static bool Japanese_ValidShiftJISCode( byte _iHi, byte _iLo )
{
	if (	(_iHi >= SHIFTJIS_HIBYTE_START0 && _iHi <= SHIFTJIS_HIBYTE_STOP0)
		||	(_iHi >= SHIFTJIS_HIBYTE_START1 && _iHi <= SHIFTJIS_HIBYTE_STOP1)
		)
	{
		if ( (_iLo >= SHIFTJIS_LOBYTE_START0 && _iLo <= SHIFTJIS_LOBYTE_STOP0) ||
			 (_iLo >= SHIFTJIS_LOBYTE_START1 && _iLo <= SHIFTJIS_LOBYTE_STOP1)
			)
		{
			return true;
		}
	}

	return false;
}

static inline bool Japanese_ValidShiftJISCode( unsigned int uiCode )
{
	return Japanese_ValidShiftJISCode( uiCode >> 8, uiCode & 0xFF );
}


// only call this when Japanese_ValidShiftJISCode() has already returned true...
//
static bool Japanese_IsTrailingPunctuation( unsigned int uiCode )
{
	// so far I'm just counting the first 18 chars, those seem to be all the basic punctuation...
	//
	if (	uiCode >= ((SHIFTJIS_HIBYTE_START0<<8)|SHIFTJIS_LOBYTE_START0) &&
			uiCode <  (((SHIFTJIS_HIBYTE_START0<<8)|SHIFTJIS_LOBYTE_START0)+18)
		)
	{
		return true;
	}

	return false;
}


// takes a ShiftJIS double-byte code and collapse down to a 0..n glyph index...
//
// (invalid codes will return 0)
//
static int Japanese_CollapseShiftJISCode( unsigned int uiCode )
{
	if (Japanese_ValidShiftJISCode( uiCode ))
	{
		uiCode -= ((SHIFTJIS_HIBYTE_START0<<8)|SHIFTJIS_LOBYTE_START0);	// sneaky maths on both bytes, reduce to 0x0000 onwards

		if ( (uiCode & 0xFF) >= (SHIFTJIS_LOBYTE_START1)-SHIFTJIS_LOBYTE_START0)
		{
			uiCode -= ((SHIFTJIS_LOBYTE_START1)-SHIFTJIS_LOBYTE_STOP0)-1;
		}

		if ( ((uiCode>>8)&0xFF) >= (SHIFTJIS_HIBYTE_START1)-SHIFTJIS_HIBYTE_START0)
		{
			uiCode -= (((SHIFTJIS_HIBYTE_START1)-SHIFTJIS_HIBYTE_STOP0)-1) << 8;
		}

		uiCode = ((uiCode >> 8) * SHIFTJIS_CODES_PER_ROW) + (uiCode & 0xFF);

		return uiCode;
	}
	return 0;
}


static int Japanese_InitFields(int &iGlyphTPs, const char *&psLang)
{
	psLang		= "jap";
	iGlyphTPs	= GLYPH_MAX_JAPANESE_SHADERS;
	g_iNonScaledCharRange = 255;
	return 64;	// m_iAsianGlyphsAcross
}

// ======================== some Chinese stuff ==============================

#define GB_HIBYTE_START		0xA1	// range is...
#define GB_HIBYTE_STOP		0xF7	// ... inclusive
#define GB_LOBYTE_LOBOUND	0xA0	// range is...
#define GB_LOBYTE_HIBOUND	0xFF	// ...bounding (ie only valid in between these points, but NULLs in charsets for these codes)
#define GB_CODES_PER_ROW	95		// 1 more than the number of glyphs

extern qboolean Language_IsChinese( void );

static inline bool Chinese_ValidGBCode( byte _iHi, byte _iLo )
{
	return (_iHi >=GB_HIBYTE_START		&&
			_iHi <=GB_HIBYTE_STOP		&&
			_iLo > GB_LOBYTE_LOBOUND	&&
			_iLo < GB_LOBYTE_HIBOUND
			);
}

static inline bool Chinese_ValidGBCode( unsigned int uiCode)
{
	return Chinese_ValidGBCode( uiCode >> 8, uiCode & 0xFF );
}


// only call this when Chinese_ValidGBCode() has already returned true...
//
static bool Chinese_IsTrailingPunctuation( unsigned int uiCode )
{
	// so far I'm just counting the first 13 chars, those seem to be all the basic punctuation...
	//
	if (	uiCode >  ((GB_HIBYTE_START<<8)|GB_LOBYTE_LOBOUND) &&
			uiCode <  (((GB_HIBYTE_START<<8)|GB_LOBYTE_LOBOUND)+14)
		)
	{
		return true;
	}

	return false;
}


// takes a GB double-byte code and collapses down to a 0..n glyph index...
// Assumes rows are 96 wide (glyph slots), not 94 wide (actual glyphs), so I can ignore boundary markers
//
// (invalid GB codes will return 0)
//
static int Chinese_CollapseGBCode( unsigned int uiCode )
{
	if (Chinese_ValidGBCode( uiCode ))
	{
		uiCode -= (GB_HIBYTE_START * 256) + GB_LOBYTE_LOBOUND;	// sneaky maths on both bytes, reduce to 0x0000 onwards
		uiCode  = ((uiCode >> 8) * GB_CODES_PER_ROW) + (uiCode & 0xFF);
		return uiCode;
	}

	return 0;
}

static int Chinese_InitFields(int &iGlyphTPs, const char *&psLang)
{
	psLang		= "chi";
	iGlyphTPs	= GLYPH_MAX_CHINESE_SHADERS;
	g_iNonScaledCharRange = 255;
	return 64;	// m_iAsianGlyphsAcross
}

// ======================== some Thai stuff ==============================

//TIS 620-2533

#define TIS_GLYPHS_START	160
#define TIS_SARA_AM			0xD3		// special case letter, both a new letter and a trailing accent for the prev one
ThaiCodes_t g_ThaiCodes;	// the one and only instance of this object

extern qboolean Language_IsThai( void );

/*
static int Thai_IsAccentChar( unsigned int uiCode )
{
	switch (uiCode)
	{
		case 209:
		case 212:	case 213:	case 214:	case 215:	case 216:	case 217:	case 218:
		case 231:	case 232:	case 233:	case 234:	case 235:	case 236:	case 237:	case 238:
		return true;
	}

	return false;
}
*/

// returns a valid Thai code (or 0), based on taking 1,2 or 3 bytes from the supplied byte stream
//	Fills in <iThaiBytes> with 1,2 or 3
static int Thai_ValidTISCode( const byte *psString, int &iThaiBytes )
{
	// try a 1-byte code first...
	//
	if (psString[0] >= 160)	// so western letters drop through and use normal font
	{
		// this code is heavily little-endian, so someone else will need to port for Mac etc... (not my problem ;-)
		//
		union CodeToTry_t
		{
            char sChars[4];
			unsigned int uiCode;
		};

		CodeToTry_t CodeToTry;
		CodeToTry.uiCode = 0;	// important that we clear all 4 bytes in sChars here

		// thai codes can be up to 3 bytes long, so see how high we can get...
		//
		int i;
		for (i=0; i<3; i++)
		{
			CodeToTry.sChars[i] = psString[i];

            int iIndex = g_ThaiCodes.GetValidIndex( CodeToTry.uiCode );
			if (iIndex == -1)
			{
				// failed, so return previous-longest code...
				//
				CodeToTry.sChars[i] = 0;
				break;
			}
		}
		iThaiBytes = i;
		assert(i);	// if 'i' was 0, then this may be an error, trying to get a thai accent as standalone char?
		return CodeToTry.uiCode;
	}

	return 0;
}

// special case, thai can only break on certain letters, and since the rules are complicated then
//	we tell the translators to put an underscore ('_') between each word even though in Thai they're
//	all jammed together at final output onscreen...
//
static inline bool Thai_IsTrailingPunctuation( unsigned int uiCode )
{
	return uiCode == '_';
}

// takes a TIS 1,2 or 3 byte code and collapse down to a 0..n glyph index...
//
// (invalid codes will return 0)
//
static int Thai_CollapseTISCode( unsigned int uiCode )
{
	if (uiCode >= TIS_GLYPHS_START)	// so western letters drop through as invalid
	{
		int iCollapsedIndex = g_ThaiCodes.GetValidIndex( uiCode );
		if (iCollapsedIndex != -1)
		{
			return iCollapsedIndex;
		}
	}

	return 0;
}

static int Thai_InitFields(int &iGlyphTPs, const char *&psLang)
{
	psLang		= "tha";
	iGlyphTPs	= GLYPH_MAX_THAI_SHADERS;
	g_iNonScaledCharRange = INT_MAX;	// in other words, don't scale any thai chars down
	return 32;	// m_iAsianGlyphsAcross
}


// ============================================================================

// takes char *, returns integer char at that point, and advances char * on by enough bytes to move
//	past the letter (either western 1 byte or Asian multi-byte)...
//
// looks messy, but the actual execution route is quite short, so it's fast...
//
// Note that I have to have this 3-param form instead of advancing a passed-in "const char **psText" because of VM-crap where you can only change ptr-contents, not ptrs themselves. Bleurgh. Ditto the qtrue:qfalse crap instead of just returning stuff straight through.
//
unsigned int AnyLanguage_ReadCharFromString( const char *psText, int *piAdvanceCount, qboolean *pbIsTrailingPunctuation /* = NULL */)
{
	const byte *psString = (const byte *) psText;	// avoid sign-promote bug
	unsigned int uiLetter;

	switch ( GetLanguageEnum() )
	{
		case eKorean:
		{
			if ( Korean_ValidKSC5601Hangul( psString[0], psString[1] ))
			{
				uiLetter = (psString[0] * 256) + psString[1];
				*piAdvanceCount = 2;

				// not going to bother testing for korean punctuation here, since korean already
				//	uses spaces, and I don't have the punctuation glyphs defined, only the basic 2350 hanguls
				//
				if ( pbIsTrailingPunctuation)
				{
					*pbIsTrailingPunctuation = qfalse;
				}

				return uiLetter;
			}
		}
		break;

		case eTaiwanese:
		{
			if ( Taiwanese_ValidBig5Code( (psString[0] * 256) + psString[1] ))
			{
				uiLetter = (psString[0] * 256) + psString[1];
				*piAdvanceCount = 2;

				// need to ask if this is a trailing (ie like a comma or full-stop) punctuation?...
				//
				if ( pbIsTrailingPunctuation)
				{
					*pbIsTrailingPunctuation = Taiwanese_IsTrailingPunctuation( uiLetter ) ? qtrue : qfalse;
				}

				return uiLetter;
			}
		}
		break;

		case eJapanese:
		{
			if ( Japanese_ValidShiftJISCode( psString[0], psString[1] ))
			{
				uiLetter = (psString[0] * 256) + psString[1];
				*piAdvanceCount = 2;

				// need to ask if this is a trailing (ie like a comma or full-stop) punctuation?...
				//
				if ( pbIsTrailingPunctuation)
				{
					*pbIsTrailingPunctuation = Japanese_IsTrailingPunctuation( uiLetter ) ? qtrue : qfalse;
				}

				return uiLetter;
			}
		}
		break;

		case eChinese:
		{
			if ( Chinese_ValidGBCode( (psString[0] * 256) + psString[1] ))
			{
				uiLetter = (psString[0] * 256) + psString[1];
				*piAdvanceCount = 2;

				// need to ask if this is a trailing (ie like a comma or full-stop) punctuation?...
				//
				if ( pbIsTrailingPunctuation)
				{
					*pbIsTrailingPunctuation = Chinese_IsTrailingPunctuation( uiLetter ) ? qtrue : qfalse;
				}

				return uiLetter;
			}
		}
		break;

		case eThai:
		{
			int iThaiBytes;
			uiLetter = Thai_ValidTISCode( psString, iThaiBytes );
			if ( uiLetter )
			{
				*piAdvanceCount = iThaiBytes;

				if ( pbIsTrailingPunctuation )
				{
					*pbIsTrailingPunctuation = Thai_IsTrailingPunctuation( uiLetter ) ? qtrue : qfalse;
				}

				return uiLetter;
			}
		}
		break;

		default:
		break;
	}

	// ... must not have been an MBCS code...
	//
	uiLetter = psString[0];
	*piAdvanceCount = 1;

	if (pbIsTrailingPunctuation)
	{
		*pbIsTrailingPunctuation = (uiLetter == '!' ||
									uiLetter == '?' ||
									uiLetter == ',' ||
									uiLetter == '.' ||
									uiLetter == ';' ||
									uiLetter == ':'
									) ? qtrue : qfalse;
	}

	return uiLetter;
}


// needed for subtitle printing since original code no longer worked once camera bar height was changed to 480/10
//	rather than refdef height / 10. I now need to bodge the coords to come out right.
//
qboolean Language_IsAsian(void)
{
	switch ( GetLanguageEnum() )
	{
		case eKorean:
		case eTaiwanese:
		case eJapanese:
		case eChinese:
		case eThai:	// this is asian, but the query is normally used for scaling
			return qtrue;
		default:
			break;
	}

	return qfalse;
}

qboolean Language_UsesSpaces(void)
{
	// ( korean uses spaces )
	switch ( GetLanguageEnum() )
	{
		case eTaiwanese:
		case eJapanese:
		case eChinese:
		case eThai:
			return qfalse;
		default:
			break;
	}

	return qtrue;
}

// ======================================================================
// name is (eg) "ergo" or "lcd", no extension.
//
//  If path present, it's a special language hack for SBCS override languages, eg: "lcd/russian", which means
//	  just treat the file as "russian", but with the "lcd" part ensuring we don't find a different registered russian font
//
CFontInfo::CFontInfo(const char *_fontName)
{
	int			len, i;
	void		*buff;
	dfontdat_t	*fontdat;

	// remove any special hack name insertions...
	//
	char fontName[MAX_QPATH];
	sprintf(fontName,"fonts/%s.fontdat",COM_SkipPath(const_cast<char*>(_fontName)));	// COM_SkipPath should take a const char *, but it's just possible people use it as a char * I guess, so I have to hack around like this <groan>

	// clear some general things...
	//
	m_pThaiData = NULL;
	m_iAltSBCSFont = -1;
	m_iThisFont = -1;
	m_iOriginalFontWhenSBCSOverriden = -1;
	m_fAltSBCSFontScaleFactor = -1;
	m_bIsFakeAlienLanguage = !strcmp(_fontName,"aurabesh");	// dont try and make SBCS or asian overrides for this

	len = ri.FS_ReadFile(fontName, NULL);
	if (len == sizeof(dfontdat_t))
	{
		ri.FS_ReadFile(fontName, &buff);
		fontdat = (dfontdat_t *)buff;

		for(i = 0; i < GLYPH_COUNT; i++)
		{
#ifdef Q3_BIG_ENDIAN
			mGlyphs[i].width = LittleShort(fontdat->mGlyphs[i].width);
			mGlyphs[i].height = LittleShort(fontdat->mGlyphs[i].height);
			mGlyphs[i].horizAdvance = LittleShort(fontdat->mGlyphs[i].horizAdvance);
			mGlyphs[i].horizOffset = LittleShort(fontdat->mGlyphs[i].horizOffset);
			mGlyphs[i].baseline = LittleLong(fontdat->mGlyphs[i].baseline);
			mGlyphs[i].s = LittleFloat(fontdat->mGlyphs[i].s);
			mGlyphs[i].t = LittleFloat(fontdat->mGlyphs[i].t);
			mGlyphs[i].s2 = LittleFloat(fontdat->mGlyphs[i].s2);
			mGlyphs[i].t2 = LittleFloat(fontdat->mGlyphs[i].t2);
#else
			mGlyphs[i] = fontdat->mGlyphs[i];
#endif
		}
		mPointSize = LittleShort(fontdat->mPointSize);
		mHeight = LittleShort(fontdat->mHeight);
		mAscender = LittleShort(fontdat->mAscender);
		mDescender = LittleShort(fontdat->mDescender);
//		mAsianHack = LittleShort(fontdat->mKoreanHack);	// ignore this crap, it's some junk in the fontdat file that no-one uses
		mbRoundCalcs = false /*!!strstr(fontName,"ergo")*/;

		// cope with bad fontdat headers...
		//
		if (mHeight == 0)
		{
			mHeight = mPointSize;
            mAscender = mPointSize - Round( ((float)mPointSize/10.0f)+2 );	// have to completely guess at the baseline... sigh.
            mDescender = mHeight - mAscender;
		}

		ri.FS_FreeFile(buff);
	}
	else
	{
		mHeight = 0;
		mShader = 0;
	}

	Q_strncpyz(m_sFontName, fontName, sizeof(m_sFontName));
	COM_StripExtension( m_sFontName, m_sFontName, sizeof( m_sFontName ) );	// so we get better error printing if failed to load shader (ie lose ".fontdat")
	mShader = RE_RegisterShaderNoMip(m_sFontName);

	FlagNoAsianGlyphs();
	UpdateAsianIfNeeded(true);

	// finished...
	g_vFontArray.resize(g_iCurrentFontIndex + 1);
	g_vFontArray[g_iCurrentFontIndex++] = this;


	if ( ri.Cvar_VariableIntegerValue( "com_buildScript" ) == 2)
	{
		Com_Printf( "com_buildScript(2): Registering foreign fonts...\n" );
		static qboolean bDone = qfalse;	// Do this once only (for speed)...
		if (!bDone)
		{
			bDone = qtrue;

			char sTemp[MAX_QPATH];
			int iGlyphTPs = 0;
			const char *psLang = NULL;

			// SBCS override languages...
			//
			fileHandle_t f;
			for (int i=0; g_SBCSOverrideLanguages[i].m_psName ;i++)
			{
				char sTemp[MAX_QPATH];

				sprintf(sTemp,"fonts/%s.tga", g_SBCSOverrideLanguages[i].m_psName );
				ri.FS_FOpenFileRead( sTemp, &f, qfalse );
				if (f) ri.FS_FCloseFile( f );

				sprintf(sTemp,"fonts/%s.fontdat", g_SBCSOverrideLanguages[i].m_psName );
				ri.FS_FOpenFileRead( sTemp, &f, qfalse );
				if (f) ri.FS_FCloseFile( f );
			}

			// asian MBCS override languages...
			//
			for (int iLang=0; iLang<5; iLang++)
			{
				switch (iLang)
				{
					case 0:	m_iAsianGlyphsAcross = Korean_InitFields	(iGlyphTPs, psLang);	break;
					case 1: m_iAsianGlyphsAcross = Taiwanese_InitFields	(iGlyphTPs, psLang);	break;
					case 2: m_iAsianGlyphsAcross = Japanese_InitFields	(iGlyphTPs, psLang);	break;
					case 3: m_iAsianGlyphsAcross = Chinese_InitFields	(iGlyphTPs, psLang);	break;
					case 4: m_iAsianGlyphsAcross = Thai_InitFields		(iGlyphTPs, psLang);
					{
						// additional files needed for Thai language...
						//
						ri.FS_FOpenFileRead( sFILENAME_THAI_WIDTHS , &f, qfalse );
						if (f) {
							ri.FS_FCloseFile( f );
						}

						ri.FS_FOpenFileRead( sFILENAME_THAI_CODES, &f, qfalse );
						if (f) {
							ri.FS_FCloseFile( f );
						}
					}
                    break;
				}

				for (int i=0; i<iGlyphTPs; i++)
				{
					Com_sprintf(sTemp,sizeof(sTemp), "fonts/%s_%d_1024_%d.tga", psLang, 1024/m_iAsianGlyphsAcross, i);

					// RE_RegisterShaderNoMip( sTemp );	// don't actually need to load it, so...
					ri.FS_FOpenFileRead( sTemp, &f, qfalse );
					if (f) {
						ri.FS_FCloseFile( f );
					}
				}
			}
		}
	}
}

void CFontInfo::UpdateAsianIfNeeded( bool bForceReEval /* = false */ )
{
	// if asian language, then provide an alternative glyph set and fill in relevant fields...
	//
	if (mHeight && !m_bIsFakeAlienLanguage)	// western charset exists in first place, and isn't alien rubbish?
	{
		Language_e eLanguage = GetLanguageEnum();

		if (eLanguage == eKorean || eLanguage == eTaiwanese || eLanguage == eJapanese || eLanguage == eChinese || eLanguage == eThai)
		{
			int iCappedHeight = mHeight < 16 ? 16: mHeight;	// arbitrary limit on small char sizes because Asian chars don't squash well

			if (m_iLanguageModificationCount != se_language->modificationCount || !AsianGlyphsAvailable() || bForceReEval)
			{
				m_iLanguageModificationCount  = se_language->modificationCount;

				int iGlyphTPs = 0;
				const char *psLang = NULL;

				switch ( eLanguage )
				{
					case eKorean:		m_iAsianGlyphsAcross = Korean_InitFields(iGlyphTPs, psLang);	break;
					case eTaiwanese:	m_iAsianGlyphsAcross = Taiwanese_InitFields(iGlyphTPs, psLang);	break;
					case eJapanese:		m_iAsianGlyphsAcross = Japanese_InitFields(iGlyphTPs, psLang);	break;
					case eChinese:		m_iAsianGlyphsAcross = Chinese_InitFields(iGlyphTPs, psLang);	break;
					case eThai:
					{
						m_iAsianGlyphsAcross = Thai_InitFields(iGlyphTPs, psLang);

						if (!m_pThaiData)
						{
							const char *psFailureReason = g_ThaiCodes.Init();
							if (!psFailureReason[0])
							{
								m_pThaiData = &g_ThaiCodes;
							}
							else
							{
								// failed to load a needed file, reset to English...
								//
								ri.Cvar_Set("se_language", "english");
								Com_Error( ERR_DROP, psFailureReason );
							}
						}
					}
					break;
					default: break;
				}

				// textures need loading...
				//
				if (m_sFontName[0])
				{
					// Use this sometime if we need to do logic to load alternate-height glyphs to better fit other fonts.
					// (but for now, we just use the one glyph set)
					//
				}

				for (int i = 0; i < iGlyphTPs; i++)
				{
					// (Note!!  assumption for S,T calculations: all Asian glyph textures pages are square except for last one)
					//
					char sTemp[MAX_QPATH];
					Com_sprintf(sTemp,sizeof(sTemp), "fonts/%s_%d_1024_%d", psLang, 1024/m_iAsianGlyphsAcross, i);
					//
					// returning 0 here will automatically inhibit Asian glyph calculations at runtime...
					//
					m_hAsianShaders[i] = RE_RegisterShaderNoMip( sTemp );
				}

				// for now I'm hardwiring these, but if we ever have more than one glyph set per language then they'll be changed...
				//
				m_iAsianPagesLoaded = iGlyphTPs;	// not necessarily true, but will be safe, and show up obvious if something missing
				m_bAsianLastPageHalfHeight = true;

				bForceReEval = true;
			}

			if (bForceReEval)
			{
				// now init the Asian member glyph fields to make them come out the same size as the western ones
				//	that they serve as an alternative for...
				//
				m_AsianGlyph.width			= iCappedHeight;	// square Asian chars same size as height of western set
				m_AsianGlyph.height			= iCappedHeight;	// ""
				switch (eLanguage)
				{
					default:		m_AsianGlyph.horizAdvance	= iCappedHeight;	break;
					case eKorean:	m_AsianGlyph.horizAdvance	= iCappedHeight - 1;break;	// korean has a small amount of space at the edge of the glyph

					case eTaiwanese:
					case eJapanese:
					case eChinese:	m_AsianGlyph.horizAdvance	= iCappedHeight + 3;	// need to force some spacing for these
//					case eThai:	// this is done dynamically elsewhere, since Thai glyphs are variable width
				}
				m_AsianGlyph.horizOffset	= 0;				// ""
				m_AsianGlyph.baseline		= mAscender + ((iCappedHeight - mHeight) >> 1);
			}
		}
		else
		{
			// not using Asian...
			//
			FlagNoAsianGlyphs();
		}
	}
	else
	{
		// no western glyphs available, so don't attempt to match asian...
		//
		FlagNoAsianGlyphs();
	}
}

static CFontInfo *GetFont_Actual(int index)
{
	index &= SET_MASK;
	if((index >= 1) && (index < g_iCurrentFontIndex))
	{
		CFontInfo *pFont = g_vFontArray[index];

		if (pFont)
		{
			pFont->UpdateAsianIfNeeded();
		}

		return pFont;
	}
	return(NULL);
}


// needed to add *piShader param because of multiple TPs,
//	if not passed in, then I also skip S,T calculations for re-usable static asian glyphinfo struct...
//
const glyphInfo_t *CFontInfo::GetLetter(const unsigned int uiLetter, int *piShader /* = NULL */)
{
	if ( AsianGlyphsAvailable() )
	{
		int iCollapsedAsianCode = GetCollapsedAsianCode( uiLetter );
		if (iCollapsedAsianCode)
		{
			if (piShader)
			{
				// (Note!!  assumption for S,T calculations: all asian glyph textures pages are square except for last one
				//			which may or may not be half height) - but not for Thai
				//
				int iTexturePageIndex = iCollapsedAsianCode / (m_iAsianGlyphsAcross * m_iAsianGlyphsAcross);

				if (iTexturePageIndex > m_iAsianPagesLoaded)
				{
					assert(0);				// should never happen
					iTexturePageIndex = 0;
				}

				int iOriginalCollapsedAsianCode = iCollapsedAsianCode;	// need to back this up (if Thai) for later
				iCollapsedAsianCode -= iTexturePageIndex *  (m_iAsianGlyphsAcross * m_iAsianGlyphsAcross);

				const int iColumn	= iCollapsedAsianCode % m_iAsianGlyphsAcross;
				const int iRow		= iCollapsedAsianCode / m_iAsianGlyphsAcross;
				const bool bHalfT	= (iTexturePageIndex == (m_iAsianPagesLoaded - 1) && m_bAsianLastPageHalfHeight);
				const int iAsianGlyphsDown = (bHalfT) ? m_iAsianGlyphsAcross / 2 : m_iAsianGlyphsAcross;

				switch ( GetLanguageEnum() )
				{
					case eKorean:
					default:
					{
						m_AsianGlyph.s  = (float)( iColumn    ) / (float)m_iAsianGlyphsAcross;
						m_AsianGlyph.t  = (float)( iRow       ) / (float)  iAsianGlyphsDown;
						m_AsianGlyph.s2 = (float)( iColumn + 1) / (float)m_iAsianGlyphsAcross;
						m_AsianGlyph.t2 = (float)( iRow + 1   ) / (float)  iAsianGlyphsDown;
					}
					break;

					case eTaiwanese:
					{
						m_AsianGlyph.s  = (float)(((1024 / m_iAsianGlyphsAcross) * ( iColumn    ))+1) / 1024.0f;
						m_AsianGlyph.t  = (float)(((1024 / iAsianGlyphsDown    ) * ( iRow       ))+1) / 1024.0f;
						m_AsianGlyph.s2 = (float)(((1024 / m_iAsianGlyphsAcross) * ( iColumn+1  ))  ) / 1024.0f;
						m_AsianGlyph.t2 = (float)(((1024 / iAsianGlyphsDown    ) * ( iRow+1     ))  ) / 1024.0f;
					}
					break;

					case eJapanese:
					case eChinese:
					{
						m_AsianGlyph.s  = (float)(((1024 / m_iAsianGlyphsAcross) * ( iColumn    ))  ) / 1024.0f;
						m_AsianGlyph.t  = (float)(((1024 / iAsianGlyphsDown    ) * ( iRow       ))  ) / 1024.0f;
						m_AsianGlyph.s2 = (float)(((1024 / m_iAsianGlyphsAcross) * ( iColumn+1  ))-1) / 1024.0f;
						m_AsianGlyph.t2 = (float)(((1024 / iAsianGlyphsDown    ) * ( iRow+1     ))-1) / 1024.0f;
					}
					break;

					case eThai:
					{
						int iGlyphXpos = (1024 / m_iAsianGlyphsAcross) * ( iColumn );
						int iGlyphWidth = g_ThaiCodes.GetWidth( iOriginalCollapsedAsianCode );

						// very thai-specific language-code...
						//
						if (uiLetter == TIS_SARA_AM)
						{
							iGlyphXpos += 9;	// these are pixel coords on the source TP, so don't affect scaled output
							iGlyphWidth= 20;	//
						}
						m_AsianGlyph.s  = (float)(iGlyphXpos) / 1024.0f;
						m_AsianGlyph.t  = (float)(((1024 / iAsianGlyphsDown    ) * ( iRow       ))  ) / 1024.0f;
						// technically this .s2 line should be modified to blit only the correct width, but since
						//	all Thai glyphs are up against the left edge of their cells and have blank to the cell
						//	boundary then it's better to keep these calculations simpler...

						m_AsianGlyph.s2 = (float)(iGlyphXpos+iGlyphWidth) / 1024.0f;
						m_AsianGlyph.t2 = (float)(((1024 / iAsianGlyphsDown    ) * ( iRow+1     ))-1) / 1024.0f;

						// special addition for Thai, need to bodge up the width and advance fields...
						//
                        m_AsianGlyph.width = iGlyphWidth;
						m_AsianGlyph.horizAdvance = iGlyphWidth + 1;
					}
					break;
				}
				*piShader = m_hAsianShaders[ iTexturePageIndex ];
			}
			return &m_AsianGlyph;
		}
	}

	if (piShader)
	{
		*piShader = GetShader();
	}

	const glyphInfo_t *pGlyph = &mGlyphs[ uiLetter & 0xff ];
	//
	// SBCS language substitution?...
	//
	if ( m_fAltSBCSFontScaleFactor != -1 )
	{
		// sod it, use the asian glyph, that's fine...
		//
		memcpy(&m_AsianGlyph,pGlyph,sizeof(m_AsianGlyph));	// *before* changin pGlyph!

//		CFontInfo *pOriginalFont = GetFont_Actual( this->m_iOriginalFontWhenSBCSOverriden );
//		pGlyph = &pOriginalFont->mGlyphs[ uiLetter & 0xff ];

		#define ASSIGN_WITH_ROUNDING(_dst,_src) _dst = mbRoundCalcs ? Round( m_fAltSBCSFontScaleFactor * _src ) : m_fAltSBCSFontScaleFactor * (float)_src;

		ASSIGN_WITH_ROUNDING( m_AsianGlyph.baseline,	pGlyph->baseline );
		ASSIGN_WITH_ROUNDING( m_AsianGlyph.height,		pGlyph->height );
		ASSIGN_WITH_ROUNDING( m_AsianGlyph.horizAdvance,pGlyph->horizAdvance );
//		m_AsianGlyph.horizOffset	= /*Round*/( m_fAltSBCSFontScaleFactor * pGlyph->horizOffset );
		ASSIGN_WITH_ROUNDING( m_AsianGlyph.width,		pGlyph->width );

		pGlyph = &m_AsianGlyph;
	}

	return pGlyph;
}

const int CFontInfo::GetCollapsedAsianCode(ulong uiLetter) const
{
	int iCollapsedAsianCode = 0;

	if (AsianGlyphsAvailable())
	{
		switch ( GetLanguageEnum() )
		{
			case eKorean:		iCollapsedAsianCode = Korean_CollapseKSC5601HangulCode( uiLetter );	break;
			case eTaiwanese:	iCollapsedAsianCode = Taiwanese_CollapseBig5Code( uiLetter );		break;
			case eJapanese:		iCollapsedAsianCode = Japanese_CollapseShiftJISCode( uiLetter );	break;
			case eChinese:		iCollapsedAsianCode = Chinese_CollapseGBCode( uiLetter );			break;
			case eThai:			iCollapsedAsianCode = Thai_CollapseTISCode( uiLetter );				break;
			default:			assert(0);	/* unhandled asian language */							break;
		}
	}

	return iCollapsedAsianCode;
}

const int CFontInfo::GetLetterWidth(unsigned int uiLetter)
{
	const glyphInfo_t *pGlyph = GetLetter( uiLetter );
	return pGlyph->width ? pGlyph->width : mGlyphs[(unsigned)'.'].width;
}

const int CFontInfo::GetLetterHorizAdvance(unsigned int uiLetter)
{
	const glyphInfo_t *pGlyph = GetLetter( uiLetter );
	return pGlyph->horizAdvance ? pGlyph->horizAdvance : mGlyphs[(unsigned)'.'].horizAdvance;
}

// ensure any GetFont calls that need SBCS overriding (such as when playing in Russian) have the appropriate stuff done...
//
static CFontInfo *GetFont_SBCSOverride(CFontInfo *pFont, Language_e eLanguageSBCS, const char *psLanguageNameSBCS )
{
	if ( !pFont->m_bIsFakeAlienLanguage )
	{
		if ( GetLanguageEnum() == eLanguageSBCS )
		{
			if ( pFont->m_iAltSBCSFont == -1 ) 	// no reg attempted yet?
			{
				// need to register this alternative SBCS font...
				//
				int iAltFontIndex = RE_RegisterFont( va("%s/%s",COM_SkipPath(pFont->m_sFontName),psLanguageNameSBCS) );	// ensure unique name (eg: "lcd/russian")
				CFontInfo *pAltFont = GetFont_Actual( iAltFontIndex );
				if ( pAltFont )
				{
					// work out the scaling factor for this font's glyphs...( round it to 1 decimal place to cut down on silly scale factors like 0.53125 )
					//
					pAltFont->m_fAltSBCSFontScaleFactor = RoundTenth((float)pFont->GetPointSize() / (float)pAltFont->GetPointSize());
					//
					// then override with the main properties of the original font...
					//
					pAltFont->mPointSize = pFont->GetPointSize();//(float) pAltFont->GetPointSize() * pAltFont->m_fAltSBCSFontScaleFactor;
					pAltFont->mHeight	 = pFont->GetHeight();//(float) pAltFont->GetHeight()	* pAltFont->m_fAltSBCSFontScaleFactor;
					pAltFont->mAscender	 = pFont->GetAscender();//(float) pAltFont->GetAscender()	* pAltFont->m_fAltSBCSFontScaleFactor;
					pAltFont->mDescender = pFont->GetDescender();//(float) pAltFont->GetDescender()	* pAltFont->m_fAltSBCSFontScaleFactor;

//					pAltFont->mPointSize = (float) pAltFont->GetPointSize() * pAltFont->m_fAltSBCSFontScaleFactor;
//					pAltFont->mHeight	 = (float) pAltFont->GetHeight()	* pAltFont->m_fAltSBCSFontScaleFactor;
//					pAltFont->mAscender	 = (float) pAltFont->GetAscender()	* pAltFont->m_fAltSBCSFontScaleFactor;
//					pAltFont->mDescender = (float) pAltFont->GetDescender()	* pAltFont->m_fAltSBCSFontScaleFactor;

					pAltFont->mbRoundCalcs = true;
					pAltFont->m_iOriginalFontWhenSBCSOverriden = pFont->m_iThisFont;
				}
				pFont->m_iAltSBCSFont = iAltFontIndex;
			}

			if ( pFont->m_iAltSBCSFont > 0)
			{
				return GetFont_Actual( pFont->m_iAltSBCSFont );
			}
		}
	}

	return NULL;
}



CFontInfo *GetFont(int index)
{
	CFontInfo *pFont = GetFont_Actual( index );

	if (pFont)
	{
		// any SBCS overrides? (this has to be pretty quick, and is (sort of))...
		//
		for (int i=0; g_SBCSOverrideLanguages[i].m_psName; i++)
		{
			CFontInfo *pAltFont = GetFont_SBCSOverride( pFont, g_SBCSOverrideLanguages[i].m_eLanguage, g_SBCSOverrideLanguages[i].m_psName );
			if (pAltFont)
			{
				return pAltFont;
			}
		}
	}

	return pFont;
}

float RE_Font_StrLenPixelsNew( const char *psText, const int iFontHandle, const float fScale ) {
	CFontInfo *curfont = GetFont(iFontHandle);
	if ( !curfont ) {
		return 0.0f;
	}

	float fScaleAsian = fScale;
	if (Language_IsAsian() && fScale > 0.7f )
	{
		fScaleAsian = fScale * 0.75f;
	}

	float maxLineWidth = 0.0f;
	float thisLineWidth = 0.0f;
	while ( *psText ) {
		int iAdvanceCount;
		unsigned int uiLetter = AnyLanguage_ReadCharFromString( psText, &iAdvanceCount, NULL );
		psText += iAdvanceCount;

		if ( uiLetter == '^' ) {
			if ( *psText >= '0' && *psText <= '9' ) {
				uiLetter = AnyLanguage_ReadCharFromString( psText, &iAdvanceCount, NULL );
				psText += iAdvanceCount;
				continue;
			}
		}

		if ( uiLetter == '\n' ) {
			thisLineWidth = 0.0f;
		}
		else {
			float iPixelAdvance = (float)curfont->GetLetterHorizAdvance( uiLetter );

			float fValue = iPixelAdvance * ((uiLetter > (unsigned)g_iNonScaledCharRange) ? fScaleAsian : fScale);

			if ( r_aspectCorrectFonts->integer == 1 ) {
				fValue *= ((float)(SCREEN_WIDTH * glConfig.vidHeight) / (float)(SCREEN_HEIGHT * glConfig.vidWidth));
			}
			else if ( r_aspectCorrectFonts->integer == 2 ) {
				fValue = ceilf(
					fValue * ((float)(SCREEN_WIDTH * glConfig.vidHeight) / (float)(SCREEN_HEIGHT * glConfig.vidWidth))
				);
			}
			thisLineWidth += curfont->mbRoundCalcs
				? roundf( fValue )
				: (r_aspectCorrectFonts->integer == 2)
					? ceilf( fValue )
					: fValue;
			if ( thisLineWidth > maxLineWidth ) {
				maxLineWidth = thisLineWidth;
			}
		}
	}
	return maxLineWidth;
}

int RE_Font_StrLenPixels( const char *psText, const int iFontHandle, const float fScale ) {
	return (int)ceilf( RE_Font_StrLenPixelsNew( psText, iFontHandle, fScale ) );
}

// not really a font function, but keeps naming consistant...
//
int RE_Font_StrLenChars(const char *psText)
{
	// logic for this function's letter counting must be kept same in this function and RE_Font_DrawString()
	//
	int iCharCount = 0;

	while ( *psText )
	{
		// in other words, colour codes and CR/LF don't count as chars, all else does...
		//
		int iAdvanceCount;
		unsigned int uiLetter = AnyLanguage_ReadCharFromString( psText, &iAdvanceCount, NULL );
		psText += iAdvanceCount;

		switch (uiLetter)
		{
			case '^':
				if (*psText >= '0' &&
					*psText <= '9')
				{
					psText++;
				}
				else
				{
					iCharCount++;
				}
				break;	// colour code (note next-char skip)
			case 10:								break;	// linefeed
			case 13:								break;	// return
			case '_':	iCharCount += (GetLanguageEnum() == eThai && (((unsigned char *)psText)[0] >= TIS_GLYPHS_START))?0:1; break;	// special word-break hack
			default:	iCharCount++;				break;
		}
	}

	return iCharCount;
}

int RE_Font_HeightPixels(const int iFontHandle, const float fScale)
{
	CFontInfo	*curfont;

	curfont = GetFont(iFontHandle);
	if(curfont)
	{
		float fValue = curfont->GetPointSize() * fScale;
		return curfont->mbRoundCalcs ? Round(fValue) : fValue;
	}
	return(0);
}

// iMaxPixelWidth is -1 for "all of string", else pixel display count...
//
void RE_Font_DrawString(int ox, int oy, const char *psText, const float *rgba, const int iFontHandle, int iMaxPixelWidth, const float fScale)
{
	static qboolean gbInShadow = qfalse;	// MUST default to this
	float				fox, foy, fx, fy;
	int					colour, offset;
	const glyphInfo_t	*pLetter;
	qhandle_t			hShader;

	assert (psText);

	if(iFontHandle & STYLE_BLINK)
	{
		if((ri.Milliseconds() >> 7) & 1)
		{
			return;
		}
	}

//	// test code only
//	if (GetLanguageEnum() == eTaiwanese)
//	{
//		psText = "Wp:¶}·F§a ¿p·G´µ¡A§Æ±æ§A¹³¥L­Ì»¡ªº¤@¼Ë¦æ¡C";
//	}
//	else
//	if (GetLanguageEnum() == eChinese)
//	{
//		//psText = "Ó¶±øÕ½³¡II  Ô¼º²?ÄªÁÖË¹  ÈÎÎñÊ§°Ü  ÄãÒªÌ×ÓÃ»­ÃæÉè¶¨µÄ±ä¸üÂð£¿  Ô¤Éè,S3 Ñ¹Ëõ,DXT1 Ñ¹Ëõ,DXT5 Ñ¹Ëõ,16 Bit,32 Bit";
//		psText = "Ó¶±øÕ½³¡II";
//	}
//	else
//	if (GetLanguageEnum() == eThai)
//	{
//		//psText = "ÁÒµÃ°Ò¹¼ÅÔµÀÑ³±ìÍØµÊÒË¡ÃÃÁÃËÑÊÊÓËÃÑºÍÑ¡¢ÃÐä·Â·Õèãªé¡Ñº¤ÍÁ¾ÔÇàµÍÃì";
//		psText = "ÁÒµÃ°Ò¹¼ÅÔµ";
//		psText = "ÃËÑÊÊÓËÃÑº";
//		psText = "ÃËÑÊÊÓËÃÑº   ÍÒ_¡Ô¹_¤ÍÃì·_1415";
//	}
//	else
//	if (GetLanguageEnum() == eKorean)
//	{
//		psText = "Wp:¼îÅ¸ÀÓÀÌ´Ù ¸Ö¸°. ±×µéÀÌ ¸»ÇÑ´ë·Î ³×°¡ ÀßÇÒÁö ±â´ëÇÏ°Ú´Ù.";
//	}
//	else
//	if (GetLanguageEnum() == eJapanese)
//	{
//		static char sBlah[200];
//		sprintf(sBlah,va("%c%c%c%c%c%c%c%c",0x82,0xA9,0x82,0xC8,0x8A,0xBF,0x8E,0x9A));
//		psText = &sBlah[0];
//	}
//	else
//	if (GetLanguageEnum() == eRussian)
//	{
////		//psText = "Íà âåðøèíå õîëìà ñòîèò ñòàðûé äîì ñ ïðèâèäåíèÿìè è áàøíÿ ñ âîëøåáíûìè ÷àñàìè."
//		psText = "Íà âåðøèíå õîëìà ñòîèò";
//	}
//	else
//	if (GetLanguageEnum() == ePolish)
//	{
//		psText = "za³o¿ony w 1364 roku, jest najstarsz¹ polsk¹ uczelni¹ i nale¿y...";
//		psText = "za³o¿ony nale¿y";
//	}


	CFontInfo *curfont = GetFont(iFontHandle);
	if(!curfont || !psText)
	{
		return;
	}

	float fScaleAsian = fScale;
	float fAsianYAdjust = 0.0f;
	if (Language_IsAsian() && fScale > 0.7f)
	{
		fScaleAsian = fScale * 0.75f;
		fAsianYAdjust = ((curfont->GetPointSize() * fScale) - (curfont->GetPointSize() * fScaleAsian)) / 2.0f;
	}

	// Draw a dropshadow if required
	if(iFontHandle & STYLE_DROPSHADOW)
	{
		offset = Round(curfont->GetPointSize() * fScale * 0.075f);

		const vec4_t v4DKGREY2 = {0.15f, 0.15f, 0.15f, rgba?rgba[3]:1.0f};

		gbInShadow = qtrue;
		RE_Font_DrawString(ox + offset, oy + offset, psText, v4DKGREY2, iFontHandle & SET_MASK, iMaxPixelWidth, fScale);
		gbInShadow = qfalse;
	}

	RE_SetColor( rgba );

	// Now we take off the training wheels and become a big font renderer
	// It's all floats from here on out
	fox = ox;
	foy = oy;

	fx = fox;
	foy += curfont->mbRoundCalcs ? Round((curfont->GetHeight() - (curfont->GetDescender() >> 1)) * fScale) : (curfont->GetHeight() - (curfont->GetDescender() >> 1)) * fScale;

	qboolean bNextTextWouldOverflow = qfalse;
	while (*psText && !bNextTextWouldOverflow)
	{
		int iAdvanceCount;
		unsigned int uiLetter = AnyLanguage_ReadCharFromString( psText, &iAdvanceCount, NULL );
		psText += iAdvanceCount;

		switch( uiLetter )
		{
		case 10:						//linefeed
			fx = fox;
			foy += curfont->mbRoundCalcs ? Round(curfont->GetPointSize() * fScale) : curfont->GetPointSize() * fScale;
			if (Language_IsAsian())
			{
				foy += 4.0f;	// this only comes into effect when playing in asian for "A long time ago in a galaxy" etc, all other text is line-broken in feeder functions
			}
			break;
		case 13:						// Return
			break;
		case 32:						// Space
			pLetter = curfont->GetLetter(' ');
			fx += curfont->mbRoundCalcs ? Round(pLetter->horizAdvance * fScale) : pLetter->horizAdvance * fScale;
			bNextTextWouldOverflow = ( iMaxPixelWidth != -1 && ((fx-fox) > (float)iMaxPixelWidth) ) ? qtrue : qfalse; // yeuch
			break;
		case '_':	// has a special word-break usage if in Thai (and followed by a thai char), and should not be displayed, else treat as normal
			if (GetLanguageEnum()== eThai && ((unsigned char *)psText)[0] >= TIS_GLYPHS_START)
			{
				break;
			}
			// else drop through and display as normal...
		case '^':
			if (uiLetter != '_')	// necessary because of fallthrough above
			{
				if (*psText >= '0' &&
					*psText <= '9')
				{
					colour = ColorIndex(*psText++);
					if (!gbInShadow)
					{
						vec4_t color;
						Com_Memcpy( color, g_color_table[colour], sizeof( color ) );
						color[3] = rgba ? rgba[3] : 1.0f;
						RE_SetColor( color );
					}
					break;
				}
			}
			//purposely falls thrugh
		default:
			pLetter = curfont->GetLetter( uiLetter, &hShader );			// Description of pLetter
			if(!pLetter->width)
			{
				pLetter = curfont->GetLetter('.');
			}

			float fThisScale = uiLetter > (unsigned)g_iNonScaledCharRange ? fScaleAsian : fScale;

			// sigh, super-language-specific hack...
			//
			if (uiLetter == TIS_SARA_AM && GetLanguageEnum() == eThai)
			{
				fx -= curfont->mbRoundCalcs ? Round(7.0f * fThisScale) : 7.0f * fThisScale;
			}

			float fAdvancePixels = curfont->mbRoundCalcs ? Round(pLetter->horizAdvance * fThisScale) : pLetter->horizAdvance * fThisScale;
			bNextTextWouldOverflow = ( iMaxPixelWidth != -1 && (((fx+fAdvancePixels)-fox) > (float)iMaxPixelWidth) ) ? qtrue : qfalse; // yeuch
			if (!bNextTextWouldOverflow)
			{
				// this 'mbRoundCalcs' stuff is crap, but the only way to make the font code work. Sigh...
				//
				fy = foy - (curfont->mbRoundCalcs ? Round(pLetter->baseline * fThisScale) : pLetter->baseline * fThisScale);
				if (curfont->m_fAltSBCSFontScaleFactor != -1)
				{
					fy += 3.0f; // I'm sick and tired of going round in circles trying to do this legally, so bollocks to it
				}

				RE_StretchPic(curfont->mbRoundCalcs ? fx + Round(pLetter->horizOffset * fThisScale) : fx + pLetter->horizOffset * fThisScale, // float x
								(uiLetter > (unsigned)g_iNonScaledCharRange) ? fy - fAsianYAdjust : fy,	// float y
								curfont->mbRoundCalcs ? Round(pLetter->width * fThisScale) : pLetter->width * fThisScale,	// float w
								curfont->mbRoundCalcs ? Round(pLetter->height * fThisScale) : pLetter->height * fThisScale, // float h
								pLetter->s,						// float s1
								pLetter->t,						// float t1
								pLetter->s2,					// float s2
								pLetter->t2,					// float t2
								//lastcolour.c,
								hShader							// qhandle_t hShader
								);
				if ( r_aspectCorrectFonts->integer == 1 ) {
					fx += fAdvancePixels
						* ((float)(SCREEN_WIDTH * glConfig.vidHeight) / (float)(SCREEN_HEIGHT * glConfig.vidWidth));
				}
				else if ( r_aspectCorrectFonts->integer == 2 ) {
					fx += ceilf( fAdvancePixels
						* ((float)(SCREEN_WIDTH * glConfig.vidHeight) / (float)(SCREEN_HEIGHT * glConfig.vidWidth)) );
				}
				else {
					fx += fAdvancePixels;
				}
			}
			break;
		}
	}
	//let it remember the old color //RE_SetColor(NULL);
}

int RE_RegisterFont(const char *psName)
{
	FontIndexMap_t::iterator it = g_mapFontIndexes.find(psName);
	if (it != g_mapFontIndexes.end() )
	{
		int iFontIndex = (*it).second;
		return iFontIndex;
	}

	// not registered, so...
	//
	{
		CFontInfo *pFont = new CFontInfo(psName);
		if (pFont->GetPointSize() > 0)
		{
			int iFontIndex = g_iCurrentFontIndex - 1;
			g_mapFontIndexes[psName] = iFontIndex;
			pFont->m_iThisFont = iFontIndex;
			return iFontIndex;
		}
		else
		{
			g_mapFontIndexes[psName] = 0;	// missing/invalid
		}
	}

	return 0;
}

void R_InitFonts(void)
{
	g_iCurrentFontIndex = 1;			// entry 0 is reserved for "missing/invalid"
	g_iNonScaledCharRange = INT_MAX;	// default all chars to have no special scaling (other than user supplied)
}

/*
===============
R_FontList_f
===============
*/
void R_FontList_f( void ) {
	Com_Printf ("------------------------------------\n");

	FontIndexMap_t::iterator it;
	for (it = g_mapFontIndexes.begin(); it != g_mapFontIndexes.end(); ++it)
	{
		CFontInfo *font = GetFont((*it).second);
		if( font )
		{
			Com_Printf("%3i:%s  ps:%hi h:%hi a:%hi d:%hi\n", (*it).second, font->m_sFontName,
				font->mPointSize, font->mHeight, font->mAscender, font->mDescender);
		}
	}
	Com_Printf ("------------------------------------\n");
}

void R_ShutdownFonts(void)
{
	for(int i = 1; i < g_iCurrentFontIndex; i++)	// entry 0 is reserved for "missing/invalid"
	{
		delete g_vFontArray[i];
	}
	g_mapFontIndexes.clear();
	g_vFontArray.clear();
	g_iCurrentFontIndex = 1;	// entry 0 is reserved for "missing/invalid"

	g_ThaiCodes.Clear();
}

// this is only really for debugging while tinkering with fonts, but harmless to leave in...
//
void R_ReloadFonts_f(void)
{
	// first, grab all the currently-registered fonts IN THE ORDER THEY WERE REGISTERED...
	//
	std::vector <sstring_t> vstrFonts;

	int iFontToFind;
	for (iFontToFind = 1; iFontToFind < g_iCurrentFontIndex; iFontToFind++)
	{
		FontIndexMap_t::iterator it;
		for (it = g_mapFontIndexes.begin(); it != g_mapFontIndexes.end(); ++it)
		{
			if (iFontToFind == (*it).second)
			{
				vstrFonts.push_back( (*it).first );
				break;
			}
		}
		if (it == g_mapFontIndexes.end() )
		{
			break;	// couldn't find this font
		}
	}
	if ( iFontToFind == g_iCurrentFontIndex ) // found all of them?
	{
		// now restart the font system...
		//
		R_ShutdownFonts();
		R_InitFonts();
		//
		// and re-register our fonts in the same order as before (note that some menu items etc cache the string lengths so really a vid_restart is better, but this is just for my testing)
		//
		for (size_t iFont = 0; iFont < vstrFonts.size(); iFont++)
		{
#ifdef _DEBUG
			int iNewFontHandle = RE_RegisterFont( vstrFonts[iFont].c_str() );
			assert( iNewFontHandle == (int)(iFont+1) );
#else
			RE_RegisterFont( vstrFonts[iFont].c_str() );
#endif
		}
		Com_Printf( "Done.\n" );
	}
	else
	{
		Com_Printf( "Problem encountered finding current fonts, ignoring.\n" );	// poo. Oh well, forget it.
	}
}


// end
