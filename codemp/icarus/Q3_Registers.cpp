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

#include "game/g_public.h"
#include "Q3_Registers.h"

extern	void	Q3_DebugPrint( int level, const char *format, ... );

varString_m		varStrings;
varFloat_m		varFloats;
varString_m		varVectors;	//Work around for vector types

int				numVariables = 0;

/*
-------------------------
Q3_VariableDeclared
-------------------------
*/

int Q3_VariableDeclared( const char *name )
{
	//Check the strings
	varString_m::iterator	vsi = varStrings.find( name );

	if ( vsi != varStrings.end() )
		return VTYPE_STRING;

	//Check the floats
	varFloat_m::iterator	vfi = varFloats.find( name );

	if ( vfi != varFloats.end() )
		return VTYPE_FLOAT;

	//Check the vectors
	varString_m::iterator	vvi = varVectors.find( name );

	if ( vvi != varVectors.end() )
		return VTYPE_VECTOR;

	return VTYPE_NONE;
}

/*
-------------------------
Q3_DeclareVariable
-------------------------
*/

void Q3_DeclareVariable( int type, const char *name )
{
	//Cannot declare the same variable twice
	if ( Q3_VariableDeclared( name ) != VTYPE_NONE )
		return;

	if ( numVariables > MAX_VARIABLES )
	{
		Q3_DebugPrint( WL_ERROR, "too many variables already declared, maximum is %d\n", MAX_VARIABLES );
		return;
	}

	switch( type )
	{
	case TK_FLOAT:
		varFloats[ name ] = 0.0f;
		break;

	case TK_STRING:
		varStrings[ name ] = "NULL";
		break;

	case TK_VECTOR:
		varVectors[ name ] = "0.0 0.0 0.0";
		break;

	default:
		Q3_DebugPrint( WL_ERROR, "unknown 'type' for declare() function!\n" );
		return;
		break;
	}

	numVariables++;
}

/*
-------------------------
Q3_FreeVariable
-------------------------
*/

void Q3_FreeVariable( const char *name )
{
	//Check the strings
	varString_m::iterator	vsi = varStrings.find( name );

	if ( vsi != varStrings.end() )
	{
		varStrings.erase( vsi );
		numVariables--;
		return;
	}

	//Check the floats
	varFloat_m::iterator	vfi = varFloats.find( name );

	if ( vfi != varFloats.end() )
	{
		varFloats.erase( vfi );
		numVariables--;
		return;
	}

	//Check the strings
	varString_m::iterator	vvi = varVectors.find( name );

	if ( vvi != varVectors.end() )
	{
		varVectors.erase( vvi );
		numVariables--;
		return;
	}
}

/*
-------------------------
Q3_GetFloatVariable
-------------------------
*/

int Q3_GetFloatVariable( const char *name, float *value )
{
	//Check the floats
	varFloat_m::iterator	vfi = varFloats.find( name );

	if ( vfi != varFloats.end() )
	{
		*value = (*vfi).second;
		return true;
	}

	return false;
}

/*
-------------------------
Q3_GetStringVariable
-------------------------
*/

int Q3_GetStringVariable( const char *name, const char **value )
{
	//Check the strings
	varString_m::iterator	vsi = varStrings.find( name );

	if ( vsi != varStrings.end() )
	{
		*value = (const char *) ((*vsi).second).c_str();
		return true;
	}

	return false;
}

/*
-------------------------
Q3_GetVectorVariable
-------------------------
*/

int Q3_GetVectorVariable( const char *name, vec3_t value )
{
	//Check the strings
	varString_m::iterator	vvi = varVectors.find( name );

	if ( vvi != varVectors.end() )
	{
		const char *str = ((*vvi).second).c_str();

		sscanf( str, "%f %f %f", &value[0], &value[1], &value[2] );
		return true;
	}

	return false;
}

/*
-------------------------
Q3_InitVariables
-------------------------
*/

void Q3_InitVariables( void )
{
	varStrings.clear();
	varFloats.clear();
	varVectors.clear();

	if ( numVariables > 0 )
		Q3_DebugPrint( WL_WARNING, "%d residual variables found!\n", numVariables );

	numVariables = 0;
}

/*
-------------------------
Q3_SetVariable_Float
-------------------------
*/

int Q3_SetFloatVariable( const char *name, float value )
{
	//Check the floats
	varFloat_m::iterator	vfi = varFloats.find( name );

	if ( vfi == varFloats.end() )
		return VTYPE_FLOAT;

	(*vfi).second = value;

	return true;
}

/*
-------------------------
Q3_SetVariable_String
-------------------------
*/

int Q3_SetStringVariable( const char *name, const char *value )
{
	//Check the strings
	varString_m::iterator	vsi = varStrings.find( name );

	if ( vsi == varStrings.end() )
		return false;

	(*vsi).second = value;

	return true;
}

/*
-------------------------
Q3_SetVariable_Vector
-------------------------
*/

int Q3_SetVectorVariable( const char *name, const char *value )
{
	//Check the strings
	varString_m::iterator	vvi = varVectors.find( name );

	if ( vvi == varVectors.end() )
		return false;

	(*vvi).second = value;

	return true;
}

/*
-------------------------
Q3_VariableSaveFloats
-------------------------
*/

void Q3_VariableSaveFloats( varFloat_m &fmap )
{
	return;
	/*
	int numFloats = fmap.size();
	gi.AppendToSaveGame( 'FVAR', &numFloats, sizeof( numFloats ) );

	varFloat_m::iterator	vfi;
	STL_ITERATE( vfi, fmap )
	{
		//Save out the map id
		int	idSize = strlen( ((*vfi).first).c_str() );

		//Save out the real data
		gi.AppendToSaveGame( 'FIDL', &idSize, sizeof( idSize ) );
		gi.AppendToSaveGame( 'FIDS', (void *) ((*vfi).first).c_str(), idSize );

		//Save out the float value
		gi.AppendToSaveGame( 'FVAL', &((*vfi).second), sizeof( float ) );
	}
	*/
}

/*
-------------------------
Q3_VariableSaveStrings
-------------------------
*/

void Q3_VariableSaveStrings( varString_m &smap )
{
	return;
	/*
	int numStrings = smap.size();
	gi.AppendToSaveGame( 'SVAR', &numStrings, sizeof( numStrings ) );

	varString_m::iterator	vsi;
	STL_ITERATE( vsi, smap )
	{
		//Save out the map id
		int	idSize = strlen( ((*vsi).first).c_str() );

		//Save out the real data
		gi.AppendToSaveGame( 'SIDL', &idSize, sizeof( idSize ) );
		gi.AppendToSaveGame( 'SIDS', (void *) ((*vsi).first).c_str(), idSize );

		//Save out the string value
		idSize = strlen( ((*vsi).second).c_str() );

		gi.AppendToSaveGame( 'SVSZ', &idSize, sizeof( idSize ) );
		gi.AppendToSaveGame( 'SVAL', (void *) ((*vsi).second).c_str(), idSize );
	}
	*/
}

/*
-------------------------
Q3_VariableSave
-------------------------
*/

int Q3_VariableSave( void )
{
	Q3_VariableSaveFloats( varFloats );
	Q3_VariableSaveStrings( varStrings );
	Q3_VariableSaveStrings( varVectors);

	return qtrue;
}

/*
-------------------------
Q3_VariableLoadFloats
-------------------------
*/

void Q3_VariableLoadFloats( varFloat_m &fmap )
{
	return;
	/*
	int		numFloats;
	char	tempBuffer[1024];

	gi.ReadFromSaveGame( 'FVAR', &numFloats, sizeof( numFloats ) );

	for ( int i = 0; i < numFloats; i++ )
	{
		int idSize;

		gi.ReadFromSaveGame( 'FIDL', &idSize, sizeof( idSize ) );
		gi.ReadFromSaveGame( 'FIDS', &tempBuffer, idSize );
		tempBuffer[ idSize ] = 0;

		float	val;

		gi.ReadFromSaveGame( 'FVAL', &val, sizeof( float ) );

		Q3_DeclareVariable( TK_FLOAT, (const char *) &tempBuffer );
		Q3_SetFloatVariable( (const char *) &tempBuffer, val );
	}
	*/
}

/*
-------------------------
Q3_VariableLoadStrings
-------------------------
*/

void Q3_VariableLoadStrings( int type, varString_m &fmap )
{
	return;
	/*
	int		numFloats;
	char	tempBuffer[1024];
	char	tempBuffer2[1024];

	gi.ReadFromSaveGame( 'SVAR', &numFloats, sizeof( numFloats ) );

	for ( int i = 0; i < numFloats; i++ )
	{
		int idSize;

		gi.ReadFromSaveGame( 'SIDL', &idSize, sizeof( idSize ) );
		gi.ReadFromSaveGame( 'SIDS', &tempBuffer, idSize );
		tempBuffer[ idSize ] = 0;

		gi.ReadFromSaveGame( 'SVSZ', &idSize, sizeof( idSize ) );
		gi.ReadFromSaveGame( 'SVAL', &tempBuffer2, idSize );
		tempBuffer2[ idSize ] = 0;

		switch ( type )
		{
		case TK_STRING:
			Q3_DeclareVariable( TK_STRING, (const char *) &tempBuffer );
			Q3_SetStringVariable( (const char *) &tempBuffer, (const char *) &tempBuffer2 );
			break;

		case TK_VECTOR:
			Q3_DeclareVariable( TK_VECTOR, (const char *) &tempBuffer );
			Q3_SetVectorVariable( (const char *) &tempBuffer, (const char *) &tempBuffer2 );
			break;
		}
	}
	*/
}

/*
-------------------------
Q3_VariableLoad
-------------------------
*/

int Q3_VariableLoad( void )
{
	Q3_InitVariables();

	Q3_VariableLoadFloats( varFloats );
	Q3_VariableLoadStrings( TK_STRING, varStrings );
	Q3_VariableLoadStrings( TK_VECTOR, varVectors);

	return qfalse;
}
