#ifndef __GAMEINFO_H__
#define __GAMEINFO_H__


#include "../game/q_shared.h"
#include <stdio.h>


typedef struct {
	int			(*FS_FOpenFile)( const char *qpath, fileHandle_t *file, fsMode_t mode );
	int 		(*FS_Read)( void *buffer, int len, fileHandle_t f );
	void		(*FS_FCloseFile)( fileHandle_t f );
	void		(*Cvar_Set)( const char *name, const char *value );
	void		(*Cvar_VariableStringBuffer)( const char *var_name, char *buffer, int bufsize );
	void		(*Cvar_Create)( const char *var_name, const char *var_value, int flags );
	int			(*FS_ReadFile)( const char *name, void **buf );
	void		(*FS_FreeFile)( void *buf );
	void		(*Printf)( const char *fmt, ... );
} gameinfo_import_t;


void GI_Init( gameinfo_import_t *import );

#endif
