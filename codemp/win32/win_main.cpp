// win_main.c

#include "client/client.h"
#include "win_local.h"
#include "resource.h"
#include <errno.h>
#include <float.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <direct.h>
#include <io.h>
#include <conio.h>
#include "../sys/sys_loadlib.h"
#include "../sys/sys_local.h"
#include "qcommon/stringed_ingame.h"

//WinVars_t	g_wv;





//================================================================


// do a quick mem test to check for any potential future mem problems...
//
void QuickMemTest(void)
{
//	if (!Sys_LowPhysicalMemory())
	{
		const int iMemTestMegs = 128;	// useful search label
		// special test,
		void *pvData = malloc(iMemTestMegs * 1024 * 1024);
		if (pvData)
		{
			free(pvData);
		}
		else
		{
			// err...
			//
			LPCSTR psContinue = re->Language_IsAsian() ?
								"Your machine failed to allocate %dMB in a memory test, which may mean you'll have problems running this game all the way through.\n\nContinue anyway?"
								:
								SE_GetString("CON_TEXT_FAILED_MEMTEST");
								// ( since it's too much hassle doing MBCS code pages and decodings etc for MessageBox command )

			#define GetYesNo(psQuery)	(!!(MessageBox(NULL,psQuery,"Query",MB_YESNO|MB_ICONWARNING|MB_TASKMODAL)==IDYES))
			if (!GetYesNo(va(psContinue,iMemTestMegs)))
			{
				LPCSTR psNoMem = re->Language_IsAsian() ?
								"Insufficient memory to run this game!\n"
								:
								SE_GetString("CON_TEXT_INSUFFICIENT_MEMORY");
								// ( since it's too much hassle doing MBCS code pages and decodings etc for MessageBox command )

				Com_Error( ERR_FATAL, psNoMem );
			}
		}
	}
}

/* Begin Sam Lantinga Public Domain 4/13/98 */

static void UnEscapeQuotes(char *arg)
{
	char *last = NULL;

	while (*arg) {
		if (*arg == '"' && (last != NULL && *last == '\\')) {
			char *c_curr = arg;
			char *c_last = last;

			while (*c_curr) {
				*c_last = *c_curr;
				c_last = c_curr;
				c_curr++;
			}
			*c_last = '\0';
		}
		last = arg;
		arg++;
	}
}

/* Parse a command line buffer into arguments */
static int ParseCommandLine(char *cmdline, char **argv)
{
	char *bufp;
	char *lastp = NULL;
	int argc, last_argc;

	argc = last_argc = 0;
	for (bufp = cmdline; *bufp;) {
		/* Skip leading whitespace */
		while (isspace(*bufp)) {
			++bufp;
		}
		/* Skip over argument */
		if (*bufp == '"') {
			++bufp;
			if (*bufp) {
				if (argv) {
					argv[argc] = bufp;
				}
				++argc;
			}
			/* Skip over word */
			lastp = bufp;
			while (*bufp && (*bufp != '"' || *lastp == '\\')) {
				lastp = bufp;
				++bufp;
			}
		} else {
			if (*bufp) {
				if (argv) {
					argv[argc] = bufp;
				}
				++argc;
			}
			/* Skip over word */
			while (*bufp && !isspace(*bufp)) {
				++bufp;
			}
		}
		if (*bufp) {
			if (argv) {
				*bufp = '\0';
			}
			++bufp;
		}

		/* Strip out \ from \" sequences */
		if (argv && last_argc != argc) {
			UnEscapeQuotes(argv[last_argc]);
		}
		last_argc = argc;
	}
	if (argv) {
		argv[argc] = NULL;
	}
	return (argc);
}

/* End Sam Lantinga Public Domain 4/13/98 */

//=======================================================================
//int	totalMsec, countMsec;

#ifndef DEFAULT_BASEDIR
#	define DEFAULT_BASEDIR Sys_BinaryPath()
#endif

#if 0
int main( int argc, char **argv )
{
	int		i;
	char	commandLine[ MAX_STRING_CHARS ] = { 0 };

	Sys_CreateConsole();

	// no abort/retry/fail errors
	SetErrorMode( SEM_FAILCRITICALERRORS );

	// Set the initial time base
	Sys_Milliseconds();

	Sys_SetBinaryPath( Sys_Dirname( argv[ 0 ] ) );
	Sys_SetDefaultInstallPath( DEFAULT_BASEDIR );

	// Concatenate the command line for passing to Com_Init
	for( i = 1; i < argc; i++ )
	{
		const bool containsSpaces = (strchr(argv[i], ' ') != NULL);
		if (containsSpaces)
			Q_strcat( commandLine, sizeof( commandLine ), "\"" );

		Q_strcat( commandLine, sizeof( commandLine ), argv[ i ] );

		if (containsSpaces)
			Q_strcat( commandLine, sizeof( commandLine ), "\"" );

		Q_strcat( commandLine, sizeof( commandLine ), " " );
	}

	Com_Init( commandLine );

#if !defined(DEDICATED)
	QuickMemTest();
#endif

	NET_Init();

	// hide the early console since we've reached the point where we
	// have a working graphics subsystems
	if ( !com_dedicated->integer && !com_viewlog->integer ) {
		Sys_ShowConsole( 0, qfalse );
	}

    // main game loop
	while( 1 ) {
		// if not running as a game client, sleep a bit
		if ( g_wv.isMinimized || ( com_dedicated && com_dedicated->integer ) ) {
			Sleep( 5 );
		}

#ifdef _DEBUG
		if (!g_wv.activeApp)
		{
			Sleep(50);
		}
#endif // _DEBUG

		// make sure mouse and joystick are only called once a frame
		IN_Frame();

		// run the game
		Com_Frame();
	}
}
#endif

/*
==================
WinMain

==================
*/

#if 0
int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // should never get a previous instance in Win32
    if ( hPrevInstance ) {
        return 0;
	}

	/* Begin Sam Lantinga Public Domain 4/13/98 */

	TCHAR *text = GetCommandLine();
	char *cmdline = _strdup(text);
	if ( cmdline == NULL ) {
		MessageBox(NULL, "Out of memory - aborting", "Fatal Error", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}

	int    argc = ParseCommandLine(cmdline, NULL);
	char **argv = (char **)alloca(sizeof(char *) * argc + 1);
	if ( argv == NULL ) {
		MessageBox(NULL, "Out of memory - aborting", "Fatal Error", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}
	ParseCommandLine(cmdline, argv);

	/* End Sam Lantinga Public Domain 4/13/98 */

	g_wv.hInstance = hInstance;

	/* Begin Sam Lantinga Public Domain 4/13/98 */

	main(argc, argv);

	free(cmdline);

	/* End Sam Lantinga Public Domain 4/13/98 */

	// never gets here
	return 0;
}
#endif