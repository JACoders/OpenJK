#ifndef __STRIPPUB_H
#define __STRIPPUB_H


#define STRIP_VERSION	1

#define MAX_LANGUAGES	10
#define MAX_STRINGS		256
#define MAX_ID			255

enum
{
	SP_LANGUAGE_ENGLISH = 0,
	SP_LANGUAGE_FRENCH,
	SP_LANGUAGE_GERMAN,
	SP_LANGUAGE_BRITISH,
	SP_LANGUAGE_KOREAN,
	SP_LANGUAGE_TAIWANESE,
	SP_LANGUAGE_ITALIAN,
	SP_LANGUAGE_SPANISH,
	SP_LANGUAGE_JAPANESE,
	SP_LANGUAGE_10,
	SP_LANGUGAGE_MAX,
	SP_LANGUAGE_ALL = 255
};


#define SP_PACKAGE		0xff00
#define SP_STRING		0x00ff

#define SP_GET_PACKAGE(x) ( (x & SP_PACKAGE) >> 8 )

// Flags
#define SP_FLAG1				0x00000001	// CENTERED
#define SP_FLAG2				0x00000002
#define SP_FLAG3				0x00000004
#define SP_FLAG4				0x00000008
#define SP_FLAG5				0x00000010
#define	SP_FLAG6				0x00000020
#define	SP_FLAG7				0x00000040
#define	SP_FLAG8				0x00000080
#define	SP_FLAG9				0x00000100
#define SP_FLAG_ORIGINAL		0x00000200

// Registration
#define SP_REGISTER_CLIENT	 (0x01)
#define SP_REGISTER_SERVER	 (0x02)
#define SP_REGISTER_MENU	 (0x04)
#define SP_REGISTER_REQUIRED (0x08)


// Registration
qboolean				SP_Register(const char *Package, unsigned char Registration);
void					SP_Unload(unsigned char Registration);

// Direct string functions
int						SP_GetStringID(const char *Reference);
#ifdef _DEBUG
const char *SP_GetReferenceText(unsigned short ID, const char *&psPackageName, const char *&psPackageReference, const char *&psText);
#endif
const char				*SP_GetStringText(unsigned short ID);
const char				*SP_GetStringTextString(const char *Reference);

// Initialization
void					SP_Init(void);
void					SP_CheckForLanguageUpdates(void);
inline void				SP_Shutdown(void)
{
	SP_Unload(SP_REGISTER_CLIENT | SP_REGISTER_SERVER | SP_REGISTER_MENU | SP_REGISTER_REQUIRED );
}

extern cvar_t	*sp_language;
// query function from font code
// 
inline qboolean Language_IsKorean(void)
{
	return (sp_language && sp_language->integer == SP_LANGUAGE_KOREAN);
}

inline qboolean Language_IsTaiwanese(void)
{
	return (sp_language && sp_language->integer == SP_LANGUAGE_TAIWANESE);
}

inline qboolean Language_IsJapanese(void)
{
	return (sp_language && sp_language->integer == SP_LANGUAGE_JAPANESE);
}

inline int Language_GetIntegerValue(void)
{
	if (sp_language)
	{
		return sp_language->integer;
	}

	return 0;
}



#endif // __STRIPPUB_H
