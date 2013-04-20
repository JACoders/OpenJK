#include <assert.h>
#include <fcntl.h>
#include <stdlib.h>
#include "qcommon/qcommon.h"
#include "sys/sys_local.h"

// bk000306: upped this from 64
#define	MAX_QUED_EVENTS		256
#define	MASK_QUED_EVENTS	( MAX_QUED_EVENTS - 1 )


sysEvent_t	eventQue[MAX_QUED_EVENTS];
// bk000306: initialize
int		eventHead = 0;
int             eventTail = 0;
byte		sys_packetReceived[MAX_MSGLEN];

void Sys_SetDefaultCDPath(const char *path);

void Sys_PrintBinVersion( const char* name ) {
  char* date = __DATE__;
  char* time = __TIME__;
  char* sep = "==============================================================";
  fprintf( stdout, "\n\n%s\n", sep );
#ifdef DEDICATED
  fprintf( stdout, "Linux Quake3 Dedicated Server [%s %s]\n", date, time );
#else
  fprintf( stdout, "Linux Quake3 Full Executable  [%s %s]\n", date, time );
#endif
  fprintf( stdout, " local install: %s\n", name );
  fprintf( stdout, "%s\n\n", sep );
}

void Sys_ParseArgs( int argc, char* argv[] ) {

  if ( argc==2 ) {
    if ( (!strcmp( argv[1], "--version" ))
	 || ( !strcmp( argv[1], "-v" )) )
    {
      Sys_PrintBinVersion( argv[0] );
      Sys_Exit(0);
    }
  }
}

int main ( int argc, char* argv[] )
{
	int		len, i;
	char	*cmdline;
	void Sys_SetDefaultCDPath(const char *path);

	// merge the command line, this is kinda silly
	for (len = 1, i = 1; i < argc; i++)
		len += strlen(argv[i]) + 1;
	cmdline = (char *)malloc(len);
	*cmdline = 0;
	for (i = 1; i < argc; i++) {
		if (i > 1)
			strcat(cmdline, " ");
		strcat(cmdline, argv[i]);
	}

	// done before Com/Sys_Init since we need this for error output
	//Sys_CreateConsole();

	// no abort/retry/fail errors
	//SetErrorMode (SEM_FAILCRITICALERRORS);

	// get the initial time base
	Sys_Milliseconds();

#if 0
	// if we find the CD, add a +set cddir xxx command line
	Sys_ScanForCD();
#endif

	//Sys_InitStreamThread();

	Com_Init (cmdline);

#if !defined(DEDICATED)
	//QuickMemTest();
#endif

	NET_Init();

	// hide the early console since we've reached the point where we
	// have a working graphics subsystems
	if (!com_dedicated->integer && !com_viewlog->integer)
	{
		Sys_ShowConsole(0, qfalse);
	}

	// main game loop
	while (1)
	{
		// if not running as a game client, sleep a bit
		/*if (g_wv.isMinimized || (com_dedicated && com_dedicated->integer))
		{
			Sleep(5);
		}*/

#ifdef _DEBUG
//		if (!g_wv.activeApp)
//		{
//			Sleep(50);
//		}
#endif // _DEBUG
		// set low precision every frame, because some system calls
		// reset it arbitrarily
		//		_controlfp( _PC_24, _MCW_PC );

		//		startTime = Sys_Milliseconds();

		// make sure mouse and joystick are only called once a frame
		IN_Frame();

		// run the game
		Com_Frame();

		//		endTime = Sys_Milliseconds();
		//		totalMsec += endTime - startTime;
		//		countMsec++;
	}

	// never gets here
}
