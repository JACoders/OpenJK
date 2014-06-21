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

// Filename: tr_stl.h
//
//  I had to make this new file, because if I put the STL "map" include inside tr_local.h then one of the other header
//	files got compile errors because of using "map" in the function protos as a GLEnum, this way seemed simpler...

#pragma once

// REM this out if you want to compile without using STL (but slower of course)
//
#define USE_STL_FOR_SHADER_LOOKUPS

#ifdef USE_STL_FOR_SHADER_LOOKUPS
	void ShaderEntryPtrs_Clear(void);
	int ShaderEntryPtrs_Size(void);
	const char  *ShaderEntryPtrs_Lookup(const char *psShaderName);
	void ShaderEntryPtrs_Insert(const char  *token, const char  *p);
#else

	#define ShaderEntryPtrs_Clear()	
#endif	// #ifdef USE_STL_FOR_SHADER_LOOKUPS
