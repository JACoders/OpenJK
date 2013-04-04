// Filename:-	stringed_ingame.h
//

#ifndef STRINGED_INGAME_H
#define	STRINGED_INGAME_H


// alter these to suit your own game...
//
#define SE_BOOL					qboolean
#define SE_TRUE					qtrue
#define SE_FALSE				qfalse
#define	iSE_MAX_FILENAME_LENGTH	MAX_QPATH
#define sSE_STRINGS_DIR			"strings"

extern cvar_t	*se_language;

// some needed text-equates, do not alter these under any circumstances !!!! (unless you're me. Which you're not)
//
#define iSE_VERSION					1
#define sSE_KEYWORD_VERSION			"VERSION"
#define sSE_KEYWORD_CONFIG			"CONFIG"
#define sSE_KEYWORD_FILENOTES		"FILENOTES"
#define sSE_KEYWORD_REFERENCE		"REFERENCE"
#define sSE_KEYWORD_NOTES			"NOTES"
#define sSE_KEYWORD_LANG			"LANG_"
#define sSE_KEYWORD_ENDMARKER		"ENDMARKER"
#define sSE_FILE_EXTENSION			".st"	// editor-only NEVER used ingame, but I wanted all extensions together
#define sSE_EXPORT_FILE_EXTENSION	".ste"
#define sSE_INGAME_FILE_EXTENSION	".str"
#define sSE_EXPORT_SAME				"#same"



// available API calls...
//
typedef const char *LPCSTR;

void	SE_Init				( void );
void	SE_CheckForLanguageUpdates(void);
LPCSTR	SE_LoadLanguage		( LPCSTR psLanguage );
void	SE_NewLanguage		( void );
//
// for convenience, two ways of getting at the same data...
//
LPCSTR	SE_GetString		( LPCSTR psPackageReference, LPCSTR psStringReference);
LPCSTR	SE_GetString		( LPCSTR psPackageAndStringReference);


#endif	// #ifndef STRINGED_INGAME_H

/////////////////// eof ////////////////

