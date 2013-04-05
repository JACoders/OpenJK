// sys_null.h -- null system driver to aid porting efforts

#include "../qcommon/qcommon.h"
#include "errno.h"

int			sys_curtime;


//===================================================================

void Sys_BeginStreamedFile( fileHandle_t f, int readAhead ) {
}

void Sys_EndStreamedFile( fileHandle_t f ) {
}

int Sys_StreamedRead( void *buffer, int size, int count, fileHandle_t f ) {
	return FS_Read( buffer, size, count, f );
}

void Sys_StreamSeek( fileHandle_t f, int offset, int origin ) {
	FS_Seek( f, offset, origin );
}


//===================================================================


void Sys_mkdir (char *path) {
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


void main (int argc, char **argv) {
	Com_Init (argc, argv);

	while (1) {
		Com_Frame( );
	}
}


