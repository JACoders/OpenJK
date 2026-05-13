/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
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

// tr_image.c

#include "tr_local.h"
#include "../rd-common/tr_common.h"

#include <map>

/*
** R_GammaCorrect
*/
void R_GammaCorrect( byte *buffer, int bufSize ) {
	int i;

	if ( vk.capture.image != VK_NULL_HANDLE )
		return;

	for ( i = 0; i < bufSize; i++ ) {
		buffer[i] = s_gammatable[buffer[i]];
	}
}

// makeup a nice clean, consistant name to query for and file under, for map<> usage...
//
char *GenerateImageMappingName( const char *name )
{
	static char sName[MAX_QPATH];
	int		i=0;
	char	letter;

	while (name[i] != '\0' && i<MAX_QPATH-1)
	{
		letter = tolower((unsigned char)name[i]);
		if (letter =='.') break;				// don't include extension
		if (letter =='\\') letter = '/';		// damn path names
		sName[i++] = letter;
	}
	sName[i]=0;

	return &sName[0];
}

static float R_BytesPerTex (int format)
{
	switch ( format ) {
	case 1:
		//"I    "
		return 1;
		break;
	case 2:
		//"IA   "
		return 2;
		break;
	case 3:
		//"RGB  "
		return glConfig.colorBits/8.0f;
		break;
	case 4:
		//"RGBA "
		return glConfig.colorBits/8.0f;
		break;

	case GL_RGBA4:
		//"RGBA4"
		return 2;
		break;
	case GL_RGB5:
		//"RGB5 "
		return 2;
		break;

	case GL_RGBA8:
		//"RGBA8"
		return 4;
		break;
	case GL_RGB8:
		//"RGB8"
		return 4;
		break;

	case GL_RGB4_S3TC:
		//"S3TC "
		return 0.33333f;
		break;
	case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
		//"DXT1 "
		return 0.33333f;
		break;
	case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
		//"DXT5 "
		return 1;
		break;
	default:
		//"???? "
		return 4;
	}
}

/*
===============
R_SumOfUsedImages
===============
*/
float R_SumOfUsedImages( qboolean bUseFormat )
{
	int	total = 0;
	image_t *pImage;

					  R_Images_StartIteration();
	while ( (pImage = R_Images_GetNextIteration()) != NULL)
	{
		if ( pImage->frameUsed == tr.frameCount- 1 ) {//it has already been advanced for the next frame, so...
			if (bUseFormat)
			{
				float  bytePerTex = R_BytesPerTex (pImage->internalFormat);
				total += bytePerTex * (pImage->width * pImage->height);
			}
			else
			{
				total += pImage->width * pImage->height;
			}
		}
	}

	return total;
}

/*
===============
R_ImageList_f
===============
*/
void R_ImageList_f( void ) {
	const image_t *image;
	int i, estTotalSize = 0;

	ri.Printf( PRINT_ALL, "\n -n- --w-- --h-- type  -size- mipmap --name-------\n" );

	for ( i = 0; i < tr.numImages; i++ )
	{
		const char *yesno[] = {"no ", "yes"};
		const char *format = "???? ";
		const char *sizeSuffix;
		int estSize;
		int displaySize;

		image = tr.images[ i ];
		estSize = image->uploadHeight * image->uploadWidth;

		switch ( image->internalFormat )
		{
			case VK_FORMAT_BC3_UNORM_BLOCK:
				format = "RGBA ";
				break;
			case VK_FORMAT_B8G8R8A8_UNORM:
				format = "BGRA ";
				estSize *= 4;
				break;
			case VK_FORMAT_R8G8B8A8_UNORM:
				format = "RGBA ";
				estSize *= 4;
				break;
			case VK_FORMAT_R8G8B8_UNORM:
				format = "RGB  ";
				estSize *= 3;
				break;
			case VK_FORMAT_B4G4R4A4_UNORM_PACK16:
				format = "RGBA ";
				estSize *= 2;
				break;
			case VK_FORMAT_A1R5G5B5_UNORM_PACK16:
				format = "RGB  ";
				estSize *= 2;
				break;
		}

		// mipmap adds about 50%
		if (image->flags & IMGFLAG_MIPMAP)
			estSize += estSize / 2;

		sizeSuffix = "b ";
		displaySize = estSize;

		if ( displaySize >= 2048 )
		{
			displaySize = ( displaySize + 1023 ) / 1024;
			sizeSuffix = "kb";
		}

		if ( displaySize >= 2048 )
		{
			displaySize = ( displaySize + 1023 ) / 1024;
			sizeSuffix = "Mb";
		}

		if ( displaySize >= 2048 )
		{
			displaySize = ( displaySize + 1023 ) / 1024;
			sizeSuffix = "Gb";
		}

		ri.Printf( PRINT_ALL, " %3i %5i %5i %s %4i%s %s %s\n", i, image->uploadWidth, image->uploadHeight, format, displaySize, sizeSuffix, yesno[(int)image->mipmap], image->imgName );
		estTotalSize += estSize;
	}

	ri.Printf( PRINT_ALL, " -----------------------\n" );
	ri.Printf( PRINT_ALL, " approx %i kbytes\n", (estTotalSize + 1023) / 1024 );
	ri.Printf( PRINT_ALL, " %i total images\n\n", tr.numImages );
}

//=======================================================================

class CStringComparator
{
public:
	bool operator()(const char *s1, const char *s2) const { return(strcmp(s1, s2) < 0); }
};

typedef std::map <const char *, image_t *, CStringComparator> AllocatedImages_t;
AllocatedImages_t AllocatedImages;
AllocatedImages_t::iterator itAllocatedImages;
int giTextureBindNum = 1024;	// will be set to this anyway at runtime, but wtf?

void R_Add_AllocatedImage( image_t *image) {
	AllocatedImages[image->imgName] = image;
}
// return = number of images in the list, for those interested
//
int R_Images_StartIteration( void )
{
	itAllocatedImages = AllocatedImages.begin();
	return AllocatedImages.size();
}

image_t *R_Images_GetNextIteration( void )
{
	if (itAllocatedImages == AllocatedImages.end())
		return NULL;

	image_t *pImage = (*itAllocatedImages).second;
	++itAllocatedImages;
	return pImage;
}

// clean up anything to do with an image_t struct, but caller will have to clear the internal to an image_t struct ready for either struct free() or overwrite...
//
// (avoid using ri->xxxx stuff here in case running on dedicated)
//
static void R_Images_DeleteImageContents( image_t *pImage )
{
	assert(pImage);	// should never be called with NULL
	if (pImage)
	{
		Z_Free(pImage);
	}
}

// special function used in conjunction with "devmapbsp"...
//
// (avoid using ri->xxxx stuff here in case running on dedicated)
//
void R_Images_DeleteLightMaps( void )
{
	for (AllocatedImages_t::iterator itImage = AllocatedImages.begin(); itImage != AllocatedImages.end(); /* empty */)
	{
		image_t *pImage = (*itImage).second;

		if (pImage->imgName[0] == '*' && strstr(pImage->imgName,"lightmap"))	// loose check, but should be ok
		{
			R_Images_DeleteImageContents(pImage);

			AllocatedImages.erase(itImage++);
		}
		else
		{
			++itImage;
		}
	}
}

// special function currently only called by Dissolve code...
//
void R_Images_DeleteImage( image_t *pImage )
{
	// Even though we supply the image handle, we need to get the corresponding iterator entry...
	//
	AllocatedImages_t::iterator itImage = AllocatedImages.find(pImage->imgName);
	if (itImage != AllocatedImages.end())
	{
		R_Images_DeleteImageContents(pImage);
		AllocatedImages.erase(itImage);
	}
	else
	{
		assert(0);
	}
}

// called only at app startup, vid_restart, app-exit
//
void R_Images_Clear( void )
{
	image_t *pImage;
	//	int iNumImages =
					  R_Images_StartIteration();
	while ( (pImage = R_Images_GetNextIteration()) != NULL)
	{
		R_Images_DeleteImageContents(pImage);
	}

	AllocatedImages.clear();

	giTextureBindNum = 1024;
}

void RE_RegisterImages_Info_f( void )
{
	image_t *pImage	= NULL;
	int iImage		= 0;
	int iTexels		= 0;

	int iNumImages	= R_Images_StartIteration();
	while ( (pImage	= R_Images_GetNextIteration()) != NULL)
	{
		ri.Printf (PRINT_ALL, "%d: (%4dx%4dy) \"%s\"",iImage, pImage->width, pImage->height, pImage->imgName);
		ri.Printf (PRINT_ALL, ", levused %d",pImage->iLastLevelUsedOn);
		ri.Printf (PRINT_ALL, "\n");

		iTexels += pImage->width * pImage->height;
		iImage++;
	}
	ri.Printf (PRINT_ALL, "%d Images. %d (%.2fMB) texels total, (not including mipmaps)\n",iNumImages, iTexels, (float)iTexels / 1024.0f / 1024.0f);
	ri.Printf (PRINT_ALL, "RE_RegisterMedia_GetLevel(): %d",RE_RegisterMedia_GetLevel());
}

// currently, this just goes through all the images and dumps any not referenced on this level...
//
qboolean RE_RegisterImages_LevelLoadEnd( void )
{
	vk_debug("RE_RegisterImages_LevelLoadEnd():\n");

//	int iNumImages = AllocatedImages.size();	// more for curiosity, really.

	qboolean imageDeleted = qfalse;
	for (AllocatedImages_t::iterator itImage = AllocatedImages.begin(); itImage != AllocatedImages.end(); /* blank */)
	{
		qboolean bEraseOccured = qfalse;

		image_t *pImage = (*itImage).second;

		// don't un-register system shaders (*fog, *dlight, *white, *default), but DO de-register lightmaps ("*<mapname>/lightmap%d")
		if (pImage->imgName[0] != '*' || strchr(pImage->imgName,'/'))
		{
			// image used on this level?
			//
			if ( pImage->iLastLevelUsedOn != RE_RegisterMedia_GetLevel() )
			{
				// nope, so dump it...
				//
				vk_debug("Dumping image \"%s\"\n",pImage->imgName);

				R_Images_DeleteImageContents(pImage);

				AllocatedImages.erase(itImage++);
				bEraseOccured = qtrue;
				imageDeleted = qtrue;
			}
		}

		if ( !bEraseOccured )
		{
			++itImage;
		}
	}

	vk_debug("RE_RegisterImages_LevelLoadEnd(): Ok\n");

	return imageDeleted;
}

image_t *noLoadImage( const char *name, imgFlags_t flags ) {
	if (!name) {
		return NULL;
	}

	char* pName = GenerateImageMappingName(name);

	//
	// see if the image is already loaded
	//
	AllocatedImages_t::iterator itAllocatedImage = AllocatedImages.find(pName);
	if (itAllocatedImage != AllocatedImages.end())
	{
		image_t* pImage = (*itAllocatedImage).second;

		// the white image can be used with any set of parms, but other mismatches are errors...
		//
		if (strcmp(name, "*white")) {
			if (pImage->flags != flags) {
				ri.Printf(PRINT_DEVELOPER, "WARNING: reused image %s with mixed flags (%i vs %i)\n", name, pImage->flags, flags);
			}
		}

		pImage->iLastLevelUsedOn = RE_RegisterMedia_GetLevel();

		return pImage;
	}

	return NULL;
}

#if 0
// returns image_t struct if we already have this, else NULL. No disk-open performed
//	(important for creating default images).
//
// This is called by both R_FindImageFile and anything that creates default images...
//
static image_t *R_FindImageFile_NoLoad( const char *name, qboolean mipmap, qboolean allowPicmip, qboolean allowTC, int glWrapClampMode )
{
	if (!name) {
		return NULL;
	}

	char *pName = GenerateImageMappingName(name);

	//
	// see if the image is already loaded
	//
	AllocatedImages_t::iterator itAllocatedImage = AllocatedImages.find(pName);
	if (itAllocatedImage != AllocatedImages.end())
	{
		image_t *pImage = (*itAllocatedImage).second;

		// the white image can be used with any set of parms, but other mismatches are errors...
		//
		if ( strcmp( pName, "*white" ) ) {
			if ( pImage->mipmap != !!mipmap ) {
				vk_debug("WARNING: reused image %s with mixed mipmap parm\n", pName );
			}
			if ( pImage->allowPicmip != !!allowPicmip ) {
				vk_debug("WARNING: reused image %s with mixed allowPicmip parm\n", pName );
			}
			if ( pImage->wrapClampMode != glWrapClampMode ) {
				vk_debug("WARNING: reused image %s with mixed glWrapClampMode parm\n", pName );
			}
		}

		pImage->iLastLevelUsedOn = RE_RegisterMedia_GetLevel();

		return pImage;
	}

	return NULL;
}
#endif

/*
=================
R_InitFogTable
=================
*/
void R_InitFogTable( void ) {
	int		i;
	float	d;
	float	exp;

	exp = 0.5;

	for ( i = 0 ; i < FOG_TABLE_SIZE ; i++ ) {
		d = pow ( (float)i/(FOG_TABLE_SIZE-1), exp );

		tr.fogTable[i] = d;
	}
}

/*
================
R_FogFactor

Returns a 0.0 to 1.0 fog density value
This is called for each texel of the fog texture on startup
and for each vertex of transparent shaders in fog dynamically
================
*/
float	R_FogFactor( float s, float t ) {
	float	d;

	s -= 1.0/512;
	if ( s < 0 ) {
		return 0;
	}
	if ( t < 1.0/32 ) {
		return 0;
	}
	if ( t < 31.0/32 ) {
		s *= (t - 1.0f/32.0f) / (30.0f/32.0f);
	}

	// we need to leave a lot of clamp range
	s *= 8;

	if ( s > 1.0 ) {
		s = 1.0;
	}

	d = tr.fogTable[(uint32_t)(s * (FOG_TABLE_SIZE - 1))];

	return d;
}