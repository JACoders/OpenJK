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

// Filename:-	tr_jpeg_interface.h
//
#pragma warning (disable: 4100)	//unreferenced formal parameter
#pragma warning (disable: 4127)	//conditional expression is constant
#pragma warning (disable: 4244)	//int to unsigned short

#ifndef TR_JPEG_INTERFACE_H
#define TR_JPEG_INTERFACE_H


#ifdef __cplusplus
extern "C"
{
#endif


#ifndef LPCSTR 
typedef const char * LPCSTR;
#endif

int LoadJPG( const char *filename, unsigned char **pic, int *width, int *height );
void SaveJPG( const char *filename, int quality, int image_width, int image_height, unsigned char *image_buffer);

void JPG_ErrorThrow(LPCSTR message);
void JPG_MessageOut(LPCSTR message);
#define ERROR_STRING_NO_RETURN(message) JPG_ErrorThrow(message)
#define MESSAGE_STRING(message)			JPG_MessageOut(message)


#ifdef __cplusplus
};
#endif



#endif	// #ifndef TR_JPEG_INTERFACE_H


////////////////// eof //////////////////

