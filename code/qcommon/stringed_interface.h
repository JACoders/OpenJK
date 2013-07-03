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

// Filename:-	stringed_interface.h
//
// These are the functions that get replaced by game-specific ones (or StringEd code) so SGE can access files etc
//

#ifndef STRINGED_INTERFACE_H
#define STRINGED_INTERFACE_H

#ifdef _MSC_VER
#pragma warning ( disable : 4786 )			// disable the usual stupid and pointless STL warning
#endif
#include <string>
using namespace std;

unsigned char *	SE_LoadFileData			( const char *psFileName, int *piLoadedLength = 0);
void			SE_FreeFileDataAfterLoad( unsigned char *psLoadedFile );
int				SE_BuildFileList		( const char *psStartDir, string &strResults );

#endif	// #ifndef STRINGED_INTERFACE_H


////////////////// eof ///////////////////

