// sys_null.h -- null system driver to aid porting efforts

#include <errno.h>
#include <stdio.h>
#include "../qcommon/qcommon.h"

int			sys_curtime;


//===================================================================

void Sys_BeginStreamedFile( FILE *f, int readAhead ) {
}

void Sys_EndStreamedFile( FILE *f ) {
}

int Sys_StreamedRead( void *buffer, int size, int count, FILE *f ) {
	return fread( buffer, size, count, f );
}

void Sys_StreamSeek( FILE *f, int offset, int origin ) {
	fseek( f, offset, origin );
}


//===================================================================


void Sys_mkdir ( const char *path ) {
}

void Sys_Error (char *error, ...) {
	va_list		argptr;

	printf ("Sys_Error: ");	
	va_start (argptr,error);
	vprintf (error,argptr);
	va_end (argptr);
	printf ("\n");

	exit (1);
}

void Sys_Quit (void) {
	exit (0);
}

void	Sys_UnloadGame (void) {
}

void	*Sys_GetGameAPI (void *parms) {
	return NULL;
}

char *Sys_GetClipboardData( void ) {
	return NULL;
}

int		Sys_Milliseconds (void) {
	return 0;
}

void	Sys_Mkdir (char *path) {
}

char	*Sys_FindFirst (char *path, unsigned musthave, unsigned canthave) {
	return NULL;
}

char	*Sys_FindNext (unsigned musthave, unsigned canthave) {
	return NULL;
}

void	Sys_FindClose (void) {
}

void	Sys_Init (void) {
}


void	Sys_EarlyOutput( char *string ) {
	printf( "%s", string );
}


int main (int argc, char **argv) {
	char *cmdline;
	int i,len;
//	int			startTime, endTime;

    // should never get a previous instance in Win32
//    if ( hPrevInstance ) {
//        return 0;
//	}

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

	Com_Init( cmdline );
	while (1) {
		Com_Frame( );
	}
	return (0);
}


