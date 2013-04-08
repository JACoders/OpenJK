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

#pragma once
#if !defined(RM_INSTANCEFILE_H_INC)
#define RM_INSTANCEFILE_H_INC

#ifdef DEBUG_LINKING
	#pragma message("...including RM_InstanceFile.h")
#endif

class CRMInstance;

class CRMInstanceFile
{
public:

	CRMInstanceFile ( );
	~CRMInstanceFile ( );

	bool			Open			( const char* instance );
	void			Close			( void );
	CRMInstance*	CreateInstance	( const char* name );

protected:

	CGenericParser2		mParser;
	CGPGroup*			mInstances;
};

#endif