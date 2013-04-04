
/*
 * UNPUBLISHED -- Rights  reserved  under  the  copyright  laws  of the 
 * United States.  Use  of a copyright notice is precautionary only and 
 * does not imply publication or disclosure.                            
 *                                                                      
 * THIS DOCUMENTATION CONTAINS CONFIDENTIAL AND PROPRIETARY INFORMATION 
 * OF    VICARIOUS   VISIONS,  INC.    ANY  DUPLICATION,  MODIFICATION, 
 * DISTRIBUTION, OR DISCLOSURE IS STRICTLY PROHIBITED WITHOUT THE PRIOR 
 * EXPRESS WRITTEN PERMISSION OF VICARIOUS VISIONS, INC.
 */

#include <xtl.h>

HANDLE s_BCThread = INVALID_HANDLE_VALUE;

static DWORD WINAPI _BinkCopier(LPVOID)
{
#ifndef FINAL_BUILD
	OutputDebugString( "_BinkCopier starting.\n" );
#endif

#ifdef XBOX_DEMO
	// Demo only has two planets, and needs to re-map paths.
	// But we're in a thread, and va isn't thread-safe. Fuck.
	char planetPath[64];
	extern char demoBasePath[64];

	strcpy( planetPath, demoBasePath );
	strcat( planetPath, "\\base\\video\\tatooine.bik" );
	CopyFile( planetPath, "Z:\\tatooine.bik", FALSE );

	strcpy( planetPath, demoBasePath );
	strcat( planetPath, "\\base\\video\\chandrila.bik" );
	CopyFile( planetPath, "Z:\\chandrila.bik", FALSE );
#else
	CopyFile( "D:\\base\\video\\cos.bik", "Z:\\cos.bik", FALSE );
	CopyFile( "D:\\base\\video\\bakura.bik", "Z:\\bakura.bik", FALSE );
	CopyFile( "D:\\base\\video\\blenjeel.bik", "Z:\\blenjeel.bik", FALSE );
	CopyFile( "D:\\base\\video\\chandrila.bik", "Z:\\chandrila.bik", FALSE );
	CopyFile( "D:\\base\\video\\core.bik", "Z:\\core.bik", FALSE );
	CopyFile( "D:\\base\\video\\ast.bik", "Z:\\ast.bik", FALSE );
	CopyFile( "D:\\base\\video\\dosunn.bik", "Z:\\dosunn.bik", FALSE );
	CopyFile( "D:\\base\\video\\krildor.bik", "Z:\\krildor.bik", FALSE );
	CopyFile( "D:\\base\\video\\narkreeta.bik", "Z:\\narkreeta.bik", FALSE );
	CopyFile( "D:\\base\\video\\ordman.bik", "Z:\\ordman.bik", FALSE );
	CopyFile( "D:\\base\\video\\tanaab.bik", "Z:\\tanaab.bik", FALSE );
	CopyFile( "D:\\base\\video\\tatooine.bik", "Z:\\tatooine.bik", FALSE );
	CopyFile( "D:\\base\\video\\yalara.bik", "Z:\\yalara.bik", FALSE );
	CopyFile( "D:\\base\\video\\zonju.bik", "Z:\\zonju.bik", FALSE );
#endif

#ifndef FINAL_BUILD
	OutputDebugString( "_BinkCopier exiting.\n" );
#endif

	ExitThread(0);
	return TRUE;
}

// Spawn our short-lived worker thread that copies all the planet movies to the Z: drive
void Sys_BinkCopyInit(void)
{
	// Create a thread to service IO
	s_BCThread = CreateThread(NULL, 64*1024, _BinkCopier, 0, 0, NULL);
}
