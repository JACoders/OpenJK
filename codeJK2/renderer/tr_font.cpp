// leave this as first line for PCH reasons...
//
#include "../server/exe_headers.h"

#include "../qcommon/sstring.h"	// stl string class won't compile in here (MS shite), so use Gil's.
#include "tr_local.h"
#include "tr_font.h"

inline int Round(float value)
{
	return((int)floorf(value + 0.5f));
}

int							fontIndex;	// entry 0 is reserved index for missing/invalid, else ++ with each new font registered
vector<CFontInfo *>			fontArray;
typedef map<sstring_t, int>	fontIndexMap_t;
							fontIndexMap_t fontIndexMap;
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

static int Korean_InitFields(int &iGlyphTPs, LPCSTR &psLang)
{
	psLang		= "kor";
	iGlyphTPs	= GLYPH_MAX_KOREAN_SHADERS;
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
			uiCode <  ((BIG5_HIBYTE_START0<<8)|BIG5_LOBYTE_LOBOUND0+20)
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

static int Taiwanese_InitFields(int &iGlyphTPs, LPCSTR &psLang)
{
	psLang		= "tai";
	iGlyphTPs	= GLYPH_MAX_TAIWANESE_SHADERS;
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
			uiCode <  ((SHIFTJIS_HIBYTE_START0<<8)|SHIFTJIS_LOBYTE_START0+18)
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


static int Japanese_InitFields(int &iGlyphTPs, LPCSTR &psLang)
{
	psLang		= "jap";
	iGlyphTPs	= GLYPH_MAX_JAPANESE_SHADERS;
	return 64;	// m_iAsianGlyphsAcross
}

// ============================================================================





// takes char *, returns integer char at that point, and advances char * on by enough bytes to move
//	past the letter (either western 1 byte or Asian multi-byte)...
//
// looks messy, but the actual execution route is quite short, so it's fast...
//
unsigned int AnyLanguage_ReadCharFromString( const char **ppsText, qboolean *pbIsTrailingPunctuation /* = NULL */)
{	
	const byte *psString = (const byte *) *ppsText;	// avoid sign-promote bug
	unsigned int uiLetter;

	if ( Language_IsKorean() )
	{
		if ( Korean_ValidKSC5601Hangul( psString[0], psString[1] ))
		{
			uiLetter = (psString[0] * 256) + psString[1];
			*ppsText += 2;

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
	else
	if ( Language_IsTaiwanese() )
	{
		if ( Taiwanese_ValidBig5Code( (psString[0] * 256) + psString[1] ))
		{
			uiLetter = (psString[0] * 256) + psString[1];
			*ppsText += 2;

			// need to ask if this is a trailing (ie like a comma or full-stop) punctuation?...
			//
			if ( pbIsTrailingPunctuation)
			{
				*pbIsTrailingPunctuation = Taiwanese_IsTrailingPunctuation( uiLetter );
			}

			return uiLetter;
		}
	}
	else
	if ( Language_IsJapanese() )
	{
		if ( Japanese_ValidShiftJISCode( psString[0], psString[1] ))
		{
			uiLetter = (psString[0] * 256) + psString[1];
			*ppsText += 2;

			// need to ask if this is a trailing (ie like a comma or full-stop) punctuation?...
			//
			if ( pbIsTrailingPunctuation)
			{
				*pbIsTrailingPunctuation = Japanese_IsTrailingPunctuation( uiLetter );
			}

			return uiLetter;
		}
	}

	// ... must not have been an MBCS code...
	//
	uiLetter = psString[0];
	*ppsText += 1;	// NOT ++

	if (pbIsTrailingPunctuation)
	{
		*pbIsTrailingPunctuation = (uiLetter == '!' || 
									uiLetter == '?' || 
									uiLetter == ',' || 
									uiLetter == '.' || 
									uiLetter == ';' || 
									uiLetter == ':'
									);
	}

	return uiLetter;
}

// needed for subtitle printing since original code no longer worked once camera bar height was changed to 480/10
//	rather than refdef height / 10. I now need to bodge the coords to come out right.
//
qboolean Language_IsAsian(void)
{
	return (Language_IsKorean() || Language_IsTaiwanese() || Language_IsJapanese());
}

qboolean Language_UsesSpaces(void)
{
	return !(Language_IsTaiwanese() || Language_IsJapanese());
}


// ======================================================================

CFontInfo::CFontInfo(const char *fontName)
{
	int			len, i;
	void		*buff;
	dfontdat_t	*fontdat;

	len = ri.FS_ReadFile(fontName, NULL);
	if (len == sizeof(dfontdat_t))
	{
		ri.FS_ReadFile(fontName, &buff);
		fontdat = (dfontdat_t *)buff;

		for(i = 0; i < GLYPH_COUNT; i++)
		{
			mGlyphs[i] = fontdat->mGlyphs[i];
		}
		mPointSize = fontdat->mPointSize;
		mHeight = fontdat->mHeight;
		mAscender = fontdat->mAscender;
		mDescender = fontdat->mDescender;
		mAsianHack = fontdat->mKoreanHack;
		mbRoundCalcs = !!strstr(fontName,"ergo");

		ri.FS_FreeFile(buff);
	}
	else
	{
		mHeight = 0;
		mShader = 0;
	}

	Q_strncpyz(m_sFontName, fontName, sizeof(m_sFontName));
	COM_StripExtension( m_sFontName, m_sFontName );	// so we get better error printing if failed to load shader (ie lose ".fontdat")
	mShader = RE_RegisterShaderNoMip(m_sFontName);

	FlagNoAsianGlyphs();
	UpdateAsianIfNeeded(true);

	// finished...
	fontArray.resize(fontIndex + 1);
	fontArray[fontIndex++] = this;


	extern cvar_t *com_buildScript;
	if (com_buildScript->integer == 2)
	{
		static qboolean bDone = qfalse;	// Do this once only (for speed)...
		if (!bDone)
		{
			bDone = qtrue;

			char sTemp[MAX_QPATH];
			int iGlyphTPs = 0;
			const char *psLang = NULL;

			for (int iLang=0; iLang<3; iLang++)
			{
				switch (iLang)
				{
					case 0:	m_iAsianGlyphsAcross = Korean_InitFields	(iGlyphTPs, psLang);	break;
					case 1: m_iAsianGlyphsAcross = Taiwanese_InitFields	(iGlyphTPs, psLang);	break;
					case 2: m_iAsianGlyphsAcross = Japanese_InitFields	(iGlyphTPs, psLang);	break;
				}

				for (int i=0; i<iGlyphTPs; i++)
				{
					Com_sprintf(sTemp,sizeof(sTemp), "fonts/%s_%d_1024_%d.tga", psLang, 1024/m_iAsianGlyphsAcross, i);

					// RE_RegisterShaderNoMip( sTemp );	// don't actually need to load it, so...
					fileHandle_t f;
					FS_FOpenFileRead( sTemp, &f, qfalse );
					if (f) {
						FS_FCloseFile( f );
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
	if (mHeight)	// western charset exists in first place?
	{
		qboolean bKorean	= Language_IsKorean();
		qboolean bTaiwanese	= Language_IsTaiwanese();
		qboolean bJapanese	= Language_IsJapanese();

		if (bKorean || bTaiwanese || bJapanese)
		{
			const int iThisLanguage = Language_GetIntegerValue();

			int iCappedHeight = mHeight < 16 ? 16: mHeight;	// arbitrary limit on small char sizes because Asian chars don't squash well

			if (m_iAsianLanguageLoaded != iThisLanguage || !AsianGlyphsAvailable() || bForceReEval)
			{
				m_iAsianLanguageLoaded  = iThisLanguage;

				int iGlyphTPs = 0;
				const char *psLang = NULL;

				if (bKorean)
				{
					m_iAsianGlyphsAcross = Korean_InitFields(iGlyphTPs, psLang);
				}
				else 
				if (bTaiwanese)
				{
					m_iAsianGlyphsAcross = Taiwanese_InitFields(iGlyphTPs, psLang);
				}
				else 
				if (bJapanese)
				{
					m_iAsianGlyphsAcross = Japanese_InitFields(iGlyphTPs, psLang);
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
				m_AsianGlyph.horizAdvance	= iCappedHeight + ((bTaiwanese||bJapanese)?3:-1);	// Asian chars contain a small amount of space in the glyph
				m_AsianGlyph.horizOffset	= 0;				// ""
				// .. or you can use the mKoreanHack value (which is the same number as the calc below)
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

// needed to add *piShader param because of multiple TPs, 
//	if not passed in, then I also skip S,T calculations for re-usable static asian glyphinfo struct...
//
const glyphInfo_t *CFontInfo::GetLetter(const unsigned int uiLetter, int *piShader /* = NULL */)
{ 	
	if ( AsianGlyphsAvailable() )
	{
		int iCollapsedAsianCode = 0;

		// small definition private to this module, I don't normally use language numbers in this module but switch-case is more useful.
		// This is for a small set of hacks to do with onconsistant windows glyph placement...(sigh)
		//
		typedef enum
		{
			eDefault,
			//eKorean,
			eTaiwanese,	// 15x15 glyphs tucked against BR of 16x16 space
			eJapanese	// 15x15 glyphs tucked against TL of 16x16 space
		} Language_e;

		Language_e eLanguage = eDefault;

		if ( Language_IsKorean() )
		{
			iCollapsedAsianCode = Korean_CollapseKSC5601HangulCode( uiLetter );
		}
		else
		if ( Language_IsTaiwanese() )
		{
			iCollapsedAsianCode = Taiwanese_CollapseBig5Code( uiLetter );
			eLanguage = eTaiwanese;
		}
		else
		if ( Language_IsJapanese() )
		{
			iCollapsedAsianCode = Japanese_CollapseShiftJISCode( uiLetter );
			eLanguage = eJapanese;
		}

		if (iCollapsedAsianCode)
		{
			if (piShader)
			{
				// (Note!!  assumption for S,T calculations: all asian glyph textures pages are square except for last one
				//			which may or may not be half height)
				//				
				int iTexturePageIndex = iCollapsedAsianCode / (m_iAsianGlyphsAcross * m_iAsianGlyphsAcross);

				if (iTexturePageIndex > m_iAsianPagesLoaded)
				{
					assert(0);				// should never happen
					iTexturePageIndex = 0;
				}

				iCollapsedAsianCode -= iTexturePageIndex *  (m_iAsianGlyphsAcross * m_iAsianGlyphsAcross);

				const int iColumn	= iCollapsedAsianCode % m_iAsianGlyphsAcross;
				const int iRow		= iCollapsedAsianCode / m_iAsianGlyphsAcross;				
				const bool bHalfT	= (iTexturePageIndex == (m_iAsianPagesLoaded - 1) && m_bAsianLastPageHalfHeight);
				const int iAsianGlyphsDown = (bHalfT) ? m_iAsianGlyphsAcross / 2 : m_iAsianGlyphsAcross;

				switch (eLanguage)
				{
					default:					
					{
						// standard (also Korean)...
						//
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
					{
						m_AsianGlyph.s  = (float)(((1024 / m_iAsianGlyphsAcross) * ( iColumn    ))  ) / 1024.0f;
						m_AsianGlyph.t  = (float)(((1024 / iAsianGlyphsDown    ) * ( iRow       ))  ) / 1024.0f;
						m_AsianGlyph.s2 = (float)(((1024 / m_iAsianGlyphsAcross) * ( iColumn+1  ))-1) / 1024.0f;
						m_AsianGlyph.t2 = (float)(((1024 / iAsianGlyphsDown    ) * ( iRow+1     ))-1) / 1024.0f;
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

	return(mGlyphs + (uiLetter & 0xff)); 
}

const int CFontInfo::GetAsianCode(ulong uiLetter) const
{
	int iCollapsedAsianCode = 0;

	if (AsianGlyphsAvailable())
	{
		if ( Language_IsKorean() )
		{
			iCollapsedAsianCode = Korean_CollapseKSC5601HangulCode( uiLetter );
		}
		else
		if ( Language_IsTaiwanese() )
		{
			iCollapsedAsianCode = Taiwanese_CollapseBig5Code( uiLetter );
		}
		else
		if ( Language_IsJapanese() )
		{
			iCollapsedAsianCode = Japanese_CollapseShiftJISCode( uiLetter );
		}
	}

	return iCollapsedAsianCode;
}

const int CFontInfo::GetLetterWidth(unsigned int uiLetter) const
{
	if ( GetAsianCode(uiLetter) )
	{
		return m_AsianGlyph.width;
	}

	uiLetter &= 0xff;
	if(mGlyphs[uiLetter].width)
	{
		return(mGlyphs[uiLetter].width);
	}
	return(mGlyphs['.'].width);
}

const int CFontInfo::GetLetterHorizAdvance(unsigned int uiLetter) const
{
	if ( GetAsianCode(uiLetter) )
	{
		return m_AsianGlyph.horizAdvance;
	}

	uiLetter &= 0xff;
	if(mGlyphs[uiLetter].horizAdvance)
	{
		return(mGlyphs[uiLetter].horizAdvance);
	}
	return(mGlyphs['.'].horizAdvance);
}

CFontInfo *GetFont(int index)
{
	index &= SET_MASK;
	if((index >= 1) && (index < fontIndex))
	{
		CFontInfo *pFont = fontArray[index];

		if (pFont)
		{
			pFont->UpdateAsianIfNeeded();
		}

		return pFont;
	}
	return(NULL);
}


int RE_Font_StrLenPixels(const char *psText, const int iFontHandle, const float fScale)
{			
	int			iMaxWidth = 0;
	int			iThisWidth= 0;
	CFontInfo	*curfont;

	curfont = GetFont(iFontHandle);
	if(!curfont)
	{
		return(0);
	}

	float fScaleA = fScale;
	if (Language_IsAsian() && fScale > 0.7f )
	{
		fScaleA = fScale * 0.75f;
	}

	while(*psText)
	{
		unsigned int uiLetter = AnyLanguage_ReadCharFromString( &psText );
		if (uiLetter == 0x0A)
		{
			iThisWidth = 0;
		}
		else
		{
			int iPixelAdvance = curfont->GetLetterHorizAdvance( uiLetter );
	
			iThisWidth += Round(iPixelAdvance * ((uiLetter > 255) ? fScaleA : fScale));
			if (iThisWidth > iMaxWidth)
			{
				iMaxWidth = iThisWidth;
			}
		}
	}

	return iMaxWidth;
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
		switch (AnyLanguage_ReadCharFromString( &psText ))
		{
			case '^':					psText++;	break;	// colour code (note next-char skip)
			case 10:								break;	// linefeed
			case 13:								break;	// return 
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
		return(Round(curfont->GetPointSize() * fScale));
	}
	return(0);
}

// iMaxPixelWidth is -1 for "all of string", else pixel display count...
//
void RE_Font_DrawString(int ox, int oy, const char *psText, const float *rgba, const int iFontHandle, int iMaxPixelWidth, const float fScale)
{
	static qboolean gbInShadow = qfalse;	// MUST default to this
	int					x, y, colour, offset;
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

/*	if (Language_IsTaiwanese())
	{
		psText = "Wp:¶}·F§a ¿p·G´µ¡A§Æ±æ§A¹³¥L­Ì»¡ªº¤@¼Ë¦æ¡C";
	}
	else
	if (Language_IsKorean())
	{
		psText = "Wp:¼îÅ¸ÀÓÀÌ´Ù ¸Ö¸°. ±×µéÀÌ ¸»ÇÑ´ë·Î ³×°¡ ÀßÇÒÁö ±â´ëÇÏ°Ú´Ù.";
	}
	else
	if (Language_IsJapanese())
	{
		char sBlah[200];
		sprintf(sBlah,va("%c%c %c%c %c%c %c%c",0x82,0xA9,0x82,0xC8,0x8A,0xBF,0x8E,0x9A));
		psText = &sBlah[0];
		//psText = ¡@¡A¡B¡C¡D¡E¡F¡G¡H¡I¡J¡K¡L¡M¡N¡O¡P¡Q¡R¡S¡T¡U¡V¡W¡X¡Y¡Z¡[¡\¡]¡^¡_¡`¡a¡b¡c¡d¡e¡f¡g¡h¡i¡j¡k¡l¡m¡n¡o¡p¡q¡r¡s¡t¡u¡v¡w¡x¡y¡z¡{¡|¡}¡~    ¡¡¡¢¡£¡¤¡¥¡¦¡§¡¨¡©¡ª¡«¡¬¡­¡®¡¯¡°¡±¡²¡³¡´¡µ¡¶¡·¡¸¡¹¡º¡»¡¼¡½¡¾¡¿¡À¡Á¡Â¡Ã¡Ä¡Å¡Æ¡Ç¡È¡É¡Ê¡Ë¡Ì¡Í¡Î¡Ï¡Ð¡Ñ¡Ò¡Ó¡Ô¡Õ¡Ö¡×¡Ø¡Ù¡Ú¡Û¡Ü¡Ý¡Þ¡ß¡à¡á¡â¡ã¡ä¡å¡æ¡ç¡è¡é¡ê¡ë¡ì¡í¡î¡ï¡ð¡ñ¡ò¡ó¡ô¡õ¡ö¡÷¡ø¡ù¡ú¡û¡ü¡ý¡þ 1¢@¢A¢B¢C¢D¢E¢F¢G¢H¢I¢J¢K¢L¢M¢N¢O¢P¢Q¢R¢S¢T¢U¢V¢W¢X¢Y¢Z¢[¢\¢]¢^¢_¢`¢a¢b¢c¢d¢e¢f¢g¢h¢i¢j¢k¢l¢m¢n¢o¢p¢q¢r¢s¢t¢u¢v¢w¢x¢y¢z¢{¢|¢}¢~    ¢¡¢¢¢£¢¤¢¥¢¦¢§¢¨¢©¢ª¢«¢¬¢­¢®¢¯¢°¢±¢²¢³¢´¢µ¢¶¢·¢¸¢¹¢º¢»¢¼¢½¢¾¢¿¢À¢Á¢Â¢Ã¢Ä¢Å¢Æ¢Ç¢È¢É¢Ê¢Ë¢Ì¢Í¢Î¢Ï¢Ð¢Ñ¢Ò¢Ó¢Ô¢Õ¢Ö¢×¢Ø¢Ù¢Ú¢Û¢Ü¢Ý¢Þ¢ß¢à¢á¢â¢ã¢ä¢å¢æ¢ç¢è¢é¢ê¢ë¢ì¢í¢î¢ï¢ð¢ñ¢ò¢ó¢ô¢õ¢ö¢÷¢ø¢ù¢ú¢û¢ü¢ý¢þ 2£@£A£B£C£D£E£F£G£H£I£J£K£L£M£N£O£P£Q£R£S£T£U£V£W£X£Y£Z£[£\£]£^£_£`£a£b£c£d£e£f£g£h£i£j£k£l£m£n£o£p£q£r£s£t£u£v£w£x£y£z£{£|£}£~    £¡£¢£££¤£¥£¦£§£¨£©£ª£«£¬£­£®£¯£°£±£²£³£´£µ£¶£·£¸£¹£º£»£¼£½£¾£¿£À£Á£Â£Ã£Ä£Å£Æ£Ç£È£É£Ê£Ë£Ì£Í£Î£Ï£Ð£Ñ£Ò£Ó£Ô£Õ£Ö£×£Ø£Ù£Ú£Û£Ü£Ý£Þ£ß£à£á£â£ã£ä£å£æ£ç£è£é£ê£ë£ì£í£î£ï£ð£ñ£ò£ó£ô£õ£ö£÷£ø£ù£ú£û£ü£ý£þ 3¤@¤A¤B¤C¤D¤E¤F¤G¤H¤I¤J¤K¤L¤M¤N¤O¤P¤Q¤R¤S¤T¤U¤V¤W¤X¤Y¤Z¤[¤\¤]¤^¤
	}
*/

	CFontInfo *curfont = GetFont(iFontHandle);
	if(!curfont)
	{
		return;
	}

	float fScaleA = fScale;
	int iAsianYAdjust = 0;
	if (Language_IsAsian() && fScale > 0.7f)
	{
		fScaleA = fScale * 0.75f;
		iAsianYAdjust = /*Round*/((((float)curfont->GetPointSize() * fScale) - ((float)curfont->GetPointSize() * fScaleA))/2);
	}

	// Draw a dropshadow if required
	if(iFontHandle & STYLE_DROPSHADOW)
	{
		offset = Round(curfont->GetPointSize() * fScale * 0.075f);

		gbInShadow = qtrue;
		RE_Font_DrawString(ox + offset, oy + offset, psText, colorTable[CT_DKGREY2], iFontHandle & SET_MASK, iMaxPixelWidth, fScale);
		gbInShadow = qfalse;
	}
		
	RE_SetColor( rgba );

	x = ox;
	oy += Round((curfont->GetHeight() - (curfont->GetDescender() >> 1)) * fScale);

	qboolean bNextTextWouldOverflow = qfalse;
	while (*psText && !bNextTextWouldOverflow)
	{
		unsigned int uiLetter = AnyLanguage_ReadCharFromString(&psText);	// 'psText' ptr has been advanced now
		switch( uiLetter )
		{
		case '^':
			colour = ColorIndex(*psText++);
			if (!gbInShadow)
			{
				RE_SetColor( g_color_table[colour] );
			}
			break;
		case 10:						//linefeed
			x = ox;
			oy += Round(curfont->GetPointSize() * fScale);
			if (Language_IsAsian())
			{
				oy += 4;	// this only comes into effect when playing in asian for "A long time ago in a galaxy" etc, all other text is line-broken in feeder functions
			}
			break;
		case 13:						// Return
			break;
		case 32:						// Space
			pLetter = curfont->GetLetter(' ');			
			x += Round(pLetter->horizAdvance * fScale);
			bNextTextWouldOverflow = ( iMaxPixelWidth != -1 && ((x-ox)>iMaxPixelWidth) );
			break;

		default:
			pLetter = curfont->GetLetter( uiLetter, &hShader );			// Description of pLetter
			if(!pLetter->width)
			{
				pLetter = curfont->GetLetter('.');
			}

			float fThisScale = uiLetter > 255 ? fScaleA : fScale;
			int iAdvancePixels = Round(pLetter->horizAdvance * fThisScale);
			bNextTextWouldOverflow = ( iMaxPixelWidth != -1 && (((x+iAdvancePixels)-ox)>iMaxPixelWidth) );
			if (!bNextTextWouldOverflow)
			{
				// this 'mbRoundCalcs' stuff is crap, but the only way to make the font code work. Sigh...
				//				
				y = oy - (curfont->mbRoundCalcs ? Round(pLetter->baseline * fThisScale) : pLetter->baseline * fThisScale);

				RE_StretchPic ( x + Round(pLetter->horizOffset * fScale), // float x
								(uiLetter > 255) ? y - iAsianYAdjust : y,	// float y
								curfont->mbRoundCalcs ? Round(pLetter->width * fThisScale) : pLetter->width * fThisScale,	// float w
								curfont->mbRoundCalcs ? Round(pLetter->height * fThisScale) : pLetter->height * fThisScale, // float h
								pLetter->s,						// float s1
								pLetter->t,						// float t1
								pLetter->s2,					// float s2
								pLetter->t2,					// float t2
								//lastcolour.c, 
								hShader							// qhandle_t hShader
								);

				x += iAdvancePixels;
			}
			break;
		}		
	}
	//let it remember the old color //RE_SetColor(NULL);;
}

int RE_RegisterFont(const char *psName) 
{
	fontIndexMap_t::iterator it = fontIndexMap.find(psName);
	if (it != fontIndexMap.end() )
	{
		int iIndex = (*it).second;

		CFontInfo *pFont = GetFont(iIndex);
		if (pFont)
		{
			pFont->UpdateAsianIfNeeded();
		}
		else
		{
			return 0;	// iIndex will be 0 anyway, but this makes it clear
		}

		return iIndex;
	}

	// not registered, so...
	//
	{
		char		temp[MAX_QPATH];
		Com_sprintf(temp, MAX_QPATH, "fonts/%s.fontdat", psName);
		CFontInfo *pFont = new CFontInfo(temp);
		if (pFont->GetPointSize() > 0)
		{
			fontIndexMap[psName] = fontIndex - 1;
			return fontIndex - 1;
		}
		else
		{
			fontIndexMap[psName] = 0;	// missing/invalid
		}
	}

	return(0);
}


void R_InitFonts(void)
{
	fontIndex = 1;	// entry 0 is reserved for "missing/invalid"
}

void R_ShutdownFonts(void)
{
	for(int i = 1; i < fontIndex; i++)	// entry 0 is reserved for "missing/invalid"
	{
		delete fontArray[i];
	}
	fontIndexMap.clear();
	fontArray.clear();
	fontIndex = 1;	// entry 0 is reserved for "missing/invalid"
}


// end

