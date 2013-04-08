/*
This file is part of Jedi Knight 2.

    Jedi Knight 2 is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    Jedi Knight 2 is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Jedi Knight 2.  If not, see <http://www.gnu.org/licenses/>.
*/
// Copyright 2001-2013 Raven Software

#ifndef __Q3_REGISTERS__
#define __Q3_REGISTERS__

#define	MAX_VARIABLES	32

typedef map < string, string >		varString_m;
typedef map < string, float >		varFloat_m;

extern	varString_m	varStrings;
extern	varFloat_m	varFloats;
extern	varString_m	varVectors;

enum
{
	VTYPE_NONE = 0,
	VTYPE_FLOAT,
	VTYPE_STRING,
	VTYPE_VECTOR,
};

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

#endif	//__Q3_REGISTERS__