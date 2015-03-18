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

#pragma once

#include "qcommon/q_shared.h"

// Filename:-	stringed_ingame.h
//

// alter these to suit your own game...
//
#define SE_BOOL					qboolean
#define SE_TRUE					qtrue
#define SE_FALSE				qfalse
#define	iSE_MAX_FILENAME_LENGTH	MAX_QPATH
#define sSE_STRINGS_DIR			"strings"
#define sSE_DEBUGSTR_PREFIX		"["		// any string you want prefixing onto the debug versions of strings (to spot hardwired english etc)
#define sSE_DEBUGSTR_SUFFIX		"]"		// ""

extern cvar_t	*se_language;

// some needed text-equates, do not alter these under any circumstances !!!! (unless you're me. Which you're not)
//
#define iSE_VERSION					1
#define sSE_KEYWORD_VERSION			"VERSION"
#define sSE_KEYWORD_CONFIG			"CONFIG"
#define sSE_KEYWORD_FILENOTES		"FILENOTES"
#define sSE_KEYWORD_REFERENCE		"REFERENCE"
#define sSE_KEYWORD_FLAGS 			"FLAGS"
#define sSE_KEYWORD_NOTES			"NOTES"
#define sSE_KEYWORD_LANG			"LANG_"
#define sSE_KEYWORD_ENDMARKER		"ENDMARKER"
#define sSE_FILE_EXTENSION			".st"	// editor-only NEVER used ingame, but I wanted all extensions together
#define sSE_EXPORT_FILE_EXTENSION	".ste"
#define sSE_INGAME_FILE_EXTENSION	".str"
#define sSE_EXPORT_SAME				"#same"



// available API calls...
//
void	SE_Init				( void );
void	SE_ShutDown			( void );
void	SE_CheckForLanguageUpdates(void);
int		SE_GetNumLanguages	( void );
const char *SE_GetLanguageName	( int iLangIndex );	// eg "german"
const char *SE_GetLanguageDir	( int iLangIndex );	// eg "strings/german"
const char *SE_LoadLanguage		( const char *psLanguage, SE_BOOL bLoadDebug = SE_TRUE );
void	SE_NewLanguage		( void );
//
// for convenience, two ways of getting at the same data...
//
const char *SE_GetString		( const char *psPackageReference, const char *psStringReference);
const char *SE_GetString		( const char *psPackageAndStringReference);
//
// ditto...
//
int		SE_GetFlags			( const char *psPackageReference, const char *psStringReference );
int		SE_GetFlags			( const char *psPackageAndStringReference );
//
// general flag functions... (SEP_GetFlagMask() return should be used with SEP_GetFlags() return)
//
int		SE_GetNumFlags		( void );
const char *SE_GetFlagName		( int iFlagIndex );
int		SE_GetFlagMask		( const char *psFlagName );


// note that so far the only place in the game that needs to know these is the font system so it can know how to
//	interpret char codes, for this reason I'm only exposing these simple bool queries...
//
inline SE_BOOL Language_IsRussian(void)
{
	return (se_language && !Q_stricmp(se_language->string, "russian")) ? SE_TRUE : SE_FALSE;
}

inline SE_BOOL Language_IsPolish(void)
{
	return (se_language && !Q_stricmp(se_language->string, "polish")) ? SE_TRUE : SE_FALSE;
}

inline SE_BOOL Language_IsKorean(void)
{
	return (se_language && !Q_stricmp(se_language->string, "korean")) ? SE_TRUE : SE_FALSE;
}

inline SE_BOOL Language_IsTaiwanese(void)
{
	return (se_language && !Q_stricmp(se_language->string, "taiwanese")) ? SE_TRUE : SE_FALSE;
}

inline SE_BOOL Language_IsJapanese(void)
{
	return (se_language && !Q_stricmp(se_language->string, "japanese")) ? SE_TRUE : SE_FALSE;
}

inline SE_BOOL Language_IsChinese(void)
{
	return (se_language && !Q_stricmp(se_language->string, "chinese")) ? SE_TRUE : SE_FALSE;
}

inline SE_BOOL Language_IsThai(void)
{
	return (se_language && !Q_stricmp(se_language->string, "thai")) ? SE_TRUE : SE_FALSE;
}

/////////////////// eof ////////////////
