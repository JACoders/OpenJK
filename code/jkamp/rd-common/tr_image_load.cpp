/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2005 - 2015, ioquake3 contributors
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

#include "tr_common.h"

const int MAX_IMAGE_LOADERS = 10;
struct ImageLoaderMap
{
	const char *extension;
	ImageLoaderFn loader;
} imageLoaders[MAX_IMAGE_LOADERS];
int numImageLoaders;

/*
=================
Finds the image loader associated with the given extension.
=================
*/
const ImageLoaderMap *FindImageLoader ( const char *extension )
{
	for ( int i = 0; i < numImageLoaders; i++ )
	{
		if ( Q_stricmp (extension, imageLoaders[i].extension) == 0 )
		{
			return &imageLoaders[i];
		}
	}

	return NULL;
}

/*
=================
Adds a new image loader to load the specified image file extension.
The 'extension' string should not begin with a period (full stop).
=================
*/
qboolean R_ImageLoader_Add ( const char *extension, ImageLoaderFn imageLoader )
{
	if ( numImageLoaders >= MAX_IMAGE_LOADERS )
	{
		ri.Printf (PRINT_DEVELOPER, "R_AddImageLoader: Cannot add any more image loaders (maximum %d).\n", MAX_IMAGE_LOADERS);
		return qfalse;
	}

	if ( FindImageLoader (extension) != NULL )
	{
		ri.Printf (PRINT_DEVELOPER, "R_AddImageLoader: Image loader already exists for extension \"%s\".\n", extension);
		return qfalse;
	}

	ImageLoaderMap *newImageLoader = &imageLoaders[numImageLoaders];
	newImageLoader->extension = extension;
	newImageLoader->loader = imageLoader;

	numImageLoaders++;

	return qtrue;
}

/*
=================
Initializes the image loader, and adds the built-in
image loaders
=================
*/
void R_ImageLoader_Init()
{
	Com_Memset (imageLoaders, 0, sizeof (imageLoaders));
	numImageLoaders = 0;

	R_ImageLoader_Add ("jpg", LoadJPG);
	R_ImageLoader_Add ("png", LoadPNG);
	R_ImageLoader_Add ("tga", LoadTGA);
}

/*
=================
Loads any of the supported image types into a cannonical
32 bit format.
=================
*/
void R_LoadImage( const char *shortname, byte **pic, int *width, int *height ) {
	*pic = NULL;
	*width = 0;
	*height = 0;

	// Try loading the image with the original extension (if possible).
	const char *extension = COM_GetExtension (shortname);
	const ImageLoaderMap *imageLoader = FindImageLoader (extension);
	if ( imageLoader != NULL )
	{
		imageLoader->loader (shortname, pic, width, height);
		if ( *pic )
		{
			return;
		}
	}

	// Loop through all the image loaders trying to load this image.
	char extensionlessName[MAX_QPATH];
	COM_StripExtension(shortname, extensionlessName, sizeof( extensionlessName ));
	for ( int i = 0; i < numImageLoaders; i++ )
	{
		const ImageLoaderMap *tryLoader = &imageLoaders[i];
		if ( tryLoader == imageLoader )
		{
			// Already tried this one.
			continue;
		}

		const char *name = va ("%s.%s", extensionlessName, tryLoader->extension);
		tryLoader->loader (name, pic, width, height);
		if ( *pic )
		{
			return;
		}
	}
}
