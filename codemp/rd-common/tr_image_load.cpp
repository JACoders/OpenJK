#include "tr_common.h"

/*
=================
R_LoadImage

Loads any of the supported image types into a cannonical
32 bit format.
=================
*/
void R_LoadImage( const char *shortname, byte **pic, int *width, int *height ) {
	char	name[MAX_QPATH];

	*pic = NULL;
	*width = 0;
	*height = 0;
	COM_StripExtension(shortname,name, sizeof( name ));
	COM_DefaultExtension(name, sizeof(name), ".jpg");
	LoadJPG( name, pic, width, height );
	if (*pic) {
		return;
	}

	COM_StripExtension(shortname,name, sizeof( name ));
	COM_DefaultExtension(name, sizeof(name), ".png");	
	LoadPNG( name, pic, (unsigned int *)width, (unsigned int *)height ); 			// try png first
	if (*pic){
		return;
	}

	COM_StripExtension(shortname,name, sizeof( name ));
	COM_DefaultExtension(name, sizeof(name), ".tga");
	LoadTGA( name, pic, width, height );            // try tga first
	if (*pic){
		return;
	}
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


