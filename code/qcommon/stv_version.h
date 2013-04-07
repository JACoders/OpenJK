// Current version of the single player game
#include "../win32/autoversion.h"

#ifdef _DEBUG
	#define	Q3_VERSION		"(debug)OpenJK: v"VERSION_STRING_DOTTED
#elif defined FINAL_BUILD
	#define	Q3_VERSION		"OpenJK: v"VERSION_STRING_DOTTED
#else
	#define	Q3_VERSION		"(internal)OpenJK: v"VERSION_STRING_DOTTED
#endif
// end


