/*
This file is part of Jedi Academy.

    Jedi Academy is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    Jedi Academy is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Jedi Academy.  If not, see <http://www.gnu.org/licenses/>.
*/
// Copyright 2001-2013 Raven Software

#ifndef __GAMEINFO_H__
#define __GAMEINFO_H__


#include "../qcommon/q_shared.h"
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
