// Filename:-	sourcesafe.h
//

// any questions: bug me -Ste.

#ifndef SOURCSAFE_H
#define SOURCSAFE_H


//#define sEF1_SS_INI		"\\\\RAVEND\\VSS\\SRCSAFE.INI"
//#define sEF1_SS_PROJECT	"$/StarTrek/BaseQ3/Maps"	// note no slash on the end!

//#define sCHC_SS_INI		"\\\\Ravend\\vss_projects\\StarWars\\SRCSAFE.INI"						
//#define sCHC_SS_PROJECT	"$/base/maps/work"	// note no slash on the end!

/*
extern CString	g_cstrSourceSafeINI;		// these default to sensible values for Trek, but can be changed
extern CString	g_cstrSourceSafeProject;	//
extern bool		g_bUseSourceSafe;			// only external for prefs setting, use "SS_FunctionsAvailable()" for query
*/
void SS_SetString_Ini(LPCSTR psArg);
void SS_SetString_Project(LPCSTR psArg);

// Usage notes:
//
// By choice, you should check "SS_FunctionsAvailable()" yourself before calling any of the functions below. 
//	They do check them internally (so no SS code is called if you've turned it off because of a bad connection etc)
//	but since these all return bools it can be misleading as to whether (eg) SS code is disabled/missing, or the answer 
//	from the call was false. Likewise you should check if the item is under sourcesafe control before calling the other 
//	operations, else you'll get an error box if you try and check out an item not in the SS database, rather than it 
//	quietly ignoring the request if you make it part of you fopen() handler. Hokay?
//
// Note that because I don't have any flag documentation I don't allow multiple checkouts (because I don't know how to...)
//	but you should do one of the function calls below to check that no-one else has the item checked out before attempting
//	to check it out yourself, otherwise you'll just get an error box saying that he checkout failed, which is far less
//	helpful.
//
// Likewise, if you do a check-in where nothing's changed, it won't show up in the history list because I don't know how
//	to set the flag to say always-show-new-checkin-even-no-differences.
//
bool SS_FunctionsAvailable	( void );
bool SS_SetupOk				( void );	// similar to above, but doesn't care if functions are user-disabled (note that all other functions DO care, so this is only useful in rare places)
bool SS_IsUnderSourceControl( LPCSTR psPathedFilename );	// call this before deciding whether or not to call any others
bool SS_Add					( LPCSTR psPathedFilename );
bool SS_CheckIn				( LPCSTR psPathedFilename );
bool SS_CheckOut			( LPCSTR psPathedFilename );
bool SS_UndoCheckOut		( LPCSTR psPathedFilename );
bool SS_IsCheckedOut		( LPCSTR psPathedFilename );
bool SS_IsCheckedOutByMe	( LPCSTR psPathedFilename );
bool SS_ListVersions		( LPCSTR psPathedFilename, CString &strOutput );	// do whatever you want with the string
bool SS_ListCheckOuts		( LPCSTR psPathedFilename, CString &strOutput, int &iCount );
//
void SS_Startup_OnceOnly	(void);
void SS_Shutdown_OnceOnly	(void);
//
void SS_LoadFromRegistry(void);
void SS_SaveToRegistry(void);

#endif	// #ifndef SOURCSAFE_H


//////////////// eof ////////////////

