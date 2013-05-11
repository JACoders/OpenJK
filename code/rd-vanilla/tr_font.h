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

// Filename:-	tr_font.h
//
// font support 

// This file is shared in the single and multiplayer codebases, so be CAREFUL WHAT YOU ADD/CHANGE!!!!!

#ifndef TR_FONT_H
#define TR_FONT_H


void R_ShutdownFonts(void);
void R_InitFonts(void);
int RE_RegisterFont(const char *psName);
int RE_Font_StrLenPixels(const char *psText, const int iFontHandle, const float fScale = 1.0f);
int RE_Font_StrLenChars(const char *psText);
int RE_Font_HeightPixels(const int iFontHandle, const float fScale = 1.0f);
void RE_Font_DrawString(int ox, int oy, const char *psText, const float *rgba, const int iFontHandle, int iMaxPixelWidth, const float fScale = 1.0f);

// Dammit, I can't use this more elegant form because of !@#@!$%% VM code... (can't alter passed in ptrs, only contents of)
//
//unsigned int AnyLanguage_ReadCharFromString( const char **ppsText, qboolean *pbIsTrailingPunctuation = NULL);
//
// so instead we have to use this messier method...
//
unsigned int AnyLanguage_ReadCharFromString( char *psText, int *piAdvanceCount, qboolean *pbIsTrailingPunctuation = NULL);
unsigned int AnyLanguage_ReadCharFromString( char **psText, qboolean *pbIsTrailingPunctuation = NULL);

qboolean Language_IsAsian(void);
qboolean Language_UsesSpaces(void);


#endif	// #ifndef TR_FONT_H

// end

