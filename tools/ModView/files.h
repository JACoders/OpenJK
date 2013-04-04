// Filename:-	files.h
//

#ifndef FILES_H
#define FILES_H


char*	Com_StringContains(char *str1, char *str2, int casesensitive);
int		Com_Filter(char *filter, char *name, int casesensitive);
int		Com_FilterPath(char *filter, char *name, int casesensitive);

void*	Z_Malloc(int size);
void	Z_Free(void *mem);
void*	S_Malloc(int size);

char	*CopyString( const char *in );

void	Sys_ListFilteredFiles( const char *basedir, char *subdirs, char *filter, char **flist, int *numfiles );
char**	Sys_ListFiles( const char *directory, const char *extension, char *filter, int *numfiles, qboolean wantsubs );
void	Sys_FreeFileList( char **flist );

#endif	// #ifndef FILES_H


/////////////////////// eof ////////////////////////

