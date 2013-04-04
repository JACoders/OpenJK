// Copyright (C) 2000-2002 Raven Software, Inc.
//
#include "../win32/AutoVersion.h"

// Current version of the multi player game
#ifdef _DEBUG
	#define	Q3_VERSION		"(debug)JAmp: v"VERSION_STRING_DOTTED
#elif defined FINAL_BUILD
	#define	Q3_VERSION		"JAmp: v"VERSION_STRING_DOTTED
#else
	#define	Q3_VERSION		"(internal)JAmp: v"VERSION_STRING_DOTTED
#endif

//end
