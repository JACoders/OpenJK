/*
===========================================================================
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2013 - 2015, OpenJK contributors

This file is part of the OpenJK source code.

OpenJK is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
===========================================================================
*/

#pragma once

enum
{
	VTYPE_NONE = 0,
	VTYPE_FLOAT,
	VTYPE_STRING,
	VTYPE_VECTOR,
};

#ifdef __cplusplus

#define	MAX_VARIABLES	32

typedef std::map < std::string, std::string >		varString_m;
typedef std::map < std::string, float >		varFloat_m;

extern	varString_m	varStrings;
extern	varFloat_m	varFloats;
extern	varString_m	varVectors;

extern void Q3_InitVariables( void );
extern void Q3_DeclareVariable( int type, const char *name );
extern void Q3_FreeVariable( const char *name );
extern int  Q3_GetStringVariable( const char *name, const char **value );
extern int  Q3_GetFloatVariable( const char *name, float *value );
extern int  Q3_GetVectorVariable( const char *name, vec3_t value );
extern int  Q3_VariableDeclared( const char *name );
extern int  Q3_SetFloatVariable( const char *name, float value );
extern int  Q3_SetStringVariable( const char *name, const char *value );
extern int  Q3_SetVectorVariable( const char *name, const char *value );

#endif //__cplusplus
