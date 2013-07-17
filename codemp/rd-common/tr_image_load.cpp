#include "tr_common.h"

const int MAX_IMAGE_LOADERS = 10;
struct ImageLoaderMap
{
	const char *extension;
	ImageLoaderFn loader;
} imageLoaders[MAX_IMAGE_LOADERS];
int numImageLoaders;

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

void R_ImageLoader_Init()
{
	R_ImageLoader_Add ("jpg", LoadJPG);
	R_ImageLoader_Add ("png", LoadPNG);
	R_ImageLoader_Add ("tga", LoadTGA);
}

/*
=================
R_LoadImage

Loads any of the supported image types into a cannonical
32 bit format.
=================
*/
void R_LoadImage( const char *shortname, byte **pic, int *width, int *height ) {
	*pic = NULL;
	*width = 0;
	*height = 0;

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

	ri.Printf (PRINT_WARNING, "R_LoadImage: Failed to load image \"%s\".\n", shortname);
}


void R_LoadDataImage( const char *name, byte **pic, int *width, int *height )
{
	int		len;
	char	work[MAX_QPATH];

	*pic = NULL;
	*width = 0;
	*height = 0;

	len = strlen(name);
	if(len >= MAX_QPATH)
	{
		return;
	}
	if (len < 5)
	{
		return;
	}

	strcpy(work, name);

	COM_DefaultExtension( work, sizeof( work ), ".jpg" );
	LoadJPG( work, pic, width, height );

	if (!pic || !*pic)
	{ //jpeg failed, try targa
		strcpy(work, name);
		COM_DefaultExtension( work, sizeof( work ), ".tga" );
		LoadTGA( work, pic, width, height );
	}

	if(*pic)
	{
		return;
	}

	// Dataimage loading failed
	ri.Printf(PRINT_WARNING, "Couldn't read %s -- dataimage load failed\n", name);
}


