//Anything above this #include will be ignored by the compiler
#include "qcommon/exe_headers.h"

// tr_image.c
#include "tr_local.h"

#pragma warning (push, 3)	//go back down to 3 for the stl include
#include <map>
#pragma warning (pop)
using namespace std;


/*
 * Include file for users of JPEG library.
 * You will need to have included system headers that define at least
 * the typedefs FILE and size_t before you can include jpeglib.h.
 * (stdio.h is sufficient on ANSI-conforming systems.)
 * You may also wish to include "jerror.h".
 */


#define JPEG_INTERNALS
#include "jpeg-8c/jpeglib.h"
#include "libpng/png.h"

// My TGA loader...
//
//---------------------------------------------------
#pragma pack(push,1)
typedef struct
{
	byte	byIDFieldLength;	// must be 0
	byte	byColourmapType;	// 0 = truecolour, 1 = paletted, else bad
	byte	byImageType;		// 1 = colour mapped (palette), uncompressed, 2 = truecolour, uncompressed, else bad
	word	w1stColourMapEntry;	// must be 0
	word	wColourMapLength;	// 256 for 8-bit palettes, else 0 for true-colour
	byte	byColourMapEntrySize; // 24 for 8-bit palettes, else 0 for true-colour
	word	wImageXOrigin;		// ignored
	word	wImageYOrigin;		// ignored
	word	wImageWidth;		// in pixels
	word	wImageHeight;		// in pixels
	byte	byImagePlanes;		// bits per pixel	(8 for paletted, else 24 for true-colour)
	byte	byScanLineOrder;	// Image descriptor bytes
								// bits 0-3 = # attr bits (alpha chan)
								// bits 4-5 = pixel order/dir
								// bits 6-7 scan line interleave (00b=none,01b=2way interleave,10b=4way)
} TGAHeader_t;
#pragma pack(pop)


// *pic == pic, else NULL for failed.
//
//  returns false if found but had a format error, else true for either OK or not-found (there's a reason for this)
//

void LoadTGA ( const char *name, byte **pic, int *width, int *height)
{
	char sErrorString[1024];
	bool bFormatErrors = false;

	// these don't need to be declared or initialised until later, but the compiler whines that 'goto' skips them.
	//
	byte *pRGBA = NULL;	
	byte *pOut	= NULL;
	byte *pIn	= NULL;


	*pic = NULL;

#define TGA_FORMAT_ERROR(blah) {sprintf(sErrorString,blah); bFormatErrors = true; goto TGADone;}
//#define TGA_FORMAT_ERROR(blah) Com_Error( ERR_DROP, blah );

	//
	// load the file
	//
	byte *pTempLoadedBuffer = 0;
	ri.FS_ReadFile ( ( char * ) name, (void **)&pTempLoadedBuffer);
	if (!pTempLoadedBuffer) {
		return;
	}

	TGAHeader_t *pHeader = (TGAHeader_t *) pTempLoadedBuffer;

	if (pHeader->byColourmapType!=0)
	{	
		TGA_FORMAT_ERROR("LoadTGA: colourmaps not supported\n" );		
	}

	if (pHeader->byImageType != 2 && pHeader->byImageType != 3 && pHeader->byImageType != 10)
	{
		TGA_FORMAT_ERROR("LoadTGA: Only type 2 (RGB), 3 (gray), and 10 (RLE-RGB) images supported\n");		
	}
		
	if (pHeader->w1stColourMapEntry != 0)
	{
		TGA_FORMAT_ERROR("LoadTGA: colourmaps not supported\n" );		
	}

	if (pHeader->wColourMapLength !=0 && pHeader->wColourMapLength != 256)
	{
		TGA_FORMAT_ERROR("LoadTGA: ColourMapLength must be either 0 or 256\n" );
	}

	if (pHeader->byColourMapEntrySize != 0 && pHeader->byColourMapEntrySize != 24)
	{
		TGA_FORMAT_ERROR("LoadTGA: ColourMapEntrySize must be either 0 or 24\n" );
	}

	if ( ( pHeader->byImagePlanes != 24 && pHeader->byImagePlanes != 32) && (pHeader->byImagePlanes != 8 && pHeader->byImageType != 3))
	{
		TGA_FORMAT_ERROR("LoadTGA: Only type 2 (RGB), 3 (gray), and 10 (RGB) TGA images supported\n");
	}

	if ((pHeader->byScanLineOrder&0x30)!=0x00 &&
		(pHeader->byScanLineOrder&0x30)!=0x10 &&
		(pHeader->byScanLineOrder&0x30)!=0x20 &&
		(pHeader->byScanLineOrder&0x30)!=0x30
		)
	{
		TGA_FORMAT_ERROR("LoadTGA: ScanLineOrder must be either 0x00,0x10,0x20, or 0x30\n");		
	}



	// these last checks are so i can use ID's RLE-code. I don't dare fiddle with it or it'll probably break...
	//
	if ( pHeader->byImageType == 10)
	{
		if ((pHeader->byScanLineOrder & 0x30) != 0x00)
		{
			TGA_FORMAT_ERROR("LoadTGA: RLE-RGB Images (type 10) must be in bottom-to-top format\n");
		}
		if (pHeader->byImagePlanes != 24 && pHeader->byImagePlanes != 32)	// probably won't happen, but avoids compressed greyscales?
		{
			TGA_FORMAT_ERROR("LoadTGA: RLE-RGB Images (type 10) must be 24 or 32 bit\n");
		}
	}

	// now read the actual bitmap in...
	//
	// Image descriptor bytes
	// bits 0-3 = # attr bits (alpha chan)
	// bits 4-5 = pixel order/dir
	// bits 6-7 scan line interleave (00b=none,01b=2way interleave,10b=4way)
	//
	int iYStart,iXStart,iYStep,iXStep;

	switch(pHeader->byScanLineOrder & 0x30)		
	{
		default:	// default case stops the compiler complaining about using uninitialised vars
		case 0x00:					//	left to right, bottom to top

			iXStart = 0;
			iXStep  = 1;

			iYStart = pHeader->wImageHeight-1;
			iYStep  = -1;

			break;

		case 0x10:					//  right to left, bottom to top

			iXStart = pHeader->wImageWidth-1;
			iXStep  = -1;

			iYStart = pHeader->wImageHeight-1;
			iYStep	= -1;

			break;

		case 0x20:					//  left to right, top to bottom

			iXStart = 0;
			iXStep  = 1;

			iYStart = 0;
			iYStep  = 1;

			break;

		case 0x30:					//  right to left, top to bottom

			iXStart = pHeader->wImageWidth-1;
			iXStep  = -1;

			iYStart = 0;
			iYStep  = 1;

			break;
	}

	// feed back the results...
	//
	if (width)
		*width = pHeader->wImageWidth;
	if (height)
		*height = pHeader->wImageHeight;

	pRGBA	= (byte *) Z_Malloc (pHeader->wImageWidth * pHeader->wImageHeight * 4, TAG_TEMP_WORKSPACE, qfalse);
	*pic	= pRGBA;
	pOut	= pRGBA;
	pIn		= pTempLoadedBuffer + sizeof(*pHeader);

	// I don't know if this ID-thing here is right, since comments that I've seen are at the end of the file, 
	//	with a zero in this field. However, may as well...
	//
	if (pHeader->byIDFieldLength != 0)
		pIn += pHeader->byIDFieldLength;	// skip TARGA image comment

	byte red,green,blue,alpha;

	if ( pHeader->byImageType == 2 || pHeader->byImageType == 3 )	// RGB or greyscale
	{
		for (int y=iYStart, iYCount=0; iYCount<pHeader->wImageHeight; y+=iYStep, iYCount++)
		{
			pOut = pRGBA + y * pHeader->wImageWidth *4;			
			for (int x=iXStart, iXCount=0; iXCount<pHeader->wImageWidth; x+=iXStep, iXCount++)
			{
				switch (pHeader->byImagePlanes)
				{
					case 8:
						blue	= *pIn++;
						green	= blue;
						red		= blue;
						*pOut++ = red;
						*pOut++ = green;
						*pOut++ = blue;
						*pOut++ = 255;
						break;

					case 24:
						blue	= *pIn++;
						green	= *pIn++;
						red		= *pIn++;
						*pOut++ = red;
						*pOut++ = green;
						*pOut++ = blue;
						*pOut++ = 255;
						break;

					case 32:
						blue	= *pIn++;
						green	= *pIn++;
						red		= *pIn++;
						alpha	= *pIn++;
						*pOut++ = red;
						*pOut++ = green;
						*pOut++ = blue;
						*pOut++ = alpha;
						break;
					
					default:
						assert(0);	// if we ever hit this, someone deleted a header check higher up
						TGA_FORMAT_ERROR("LoadTGA: Image can only have 8, 24 or 32 planes for RGB/greyscale\n");						
						break;
				}
			}		
		}
	}
	else 
	if (pHeader->byImageType == 10)   // RLE-RGB
	{
		// I've no idea if this stuff works, I normally reject RLE targas, but this is from ID's code
		//	so maybe I should try and support it...
		//
		byte packetHeader, packetSize, j;

		for (int y = pHeader->wImageHeight-1; y >= 0; y--)
		{
			pOut = pRGBA + y * pHeader->wImageWidth *4;
			for (int x=0; x<pHeader->wImageWidth;)
			{
				packetHeader = *pIn++;
				packetSize   = 1 + (packetHeader & 0x7f);
				if (packetHeader & 0x80)         // run-length packet
				{
					switch (pHeader->byImagePlanes) 
					{
						case 24:

							blue	= *pIn++;
							green	= *pIn++;
							red		= *pIn++;
							alpha	= 255;
							break;

						case 32:
							
							blue	= *pIn++;
							green	= *pIn++;
							red		= *pIn++;
							alpha	= *pIn++;
							break;

						default:
							assert(0);	// if we ever hit this, someone deleted a header check higher up
							TGA_FORMAT_ERROR("LoadTGA: RLE-RGB can only have 24 or 32 planes\n");
							break;
					}
	
					for (j=0; j<packetSize; j++) 
					{
						*pOut++	= red;
						*pOut++	= green;
						*pOut++	= blue;
						*pOut++	= alpha;
						x++;
						if (x == pHeader->wImageWidth)  // run spans across rows
						{
							x = 0;
							if (y > 0)
								y--;
							else
								goto breakOut;
							pOut = pRGBA + y * pHeader->wImageWidth * 4;
						}
					}
				}
				else 
				{	// non run-length packet

					for (j=0; j<packetSize; j++) 
					{
						switch (pHeader->byImagePlanes) 
						{
							case 24:

								blue	= *pIn++;
								green	= *pIn++;
								red		= *pIn++;
								*pOut++ = red;
								*pOut++ = green;
								*pOut++ = blue;
								*pOut++ = 255;
								break;

							case 32:
								blue	= *pIn++;
								green	= *pIn++;
								red		= *pIn++;
								alpha	= *pIn++;
								*pOut++ = red;
								*pOut++ = green;
								*pOut++ = blue;
								*pOut++ = alpha;
								break;

							default:
								assert(0);	// if we ever hit this, someone deleted a header check higher up
								TGA_FORMAT_ERROR("LoadTGA: RLE-RGB can only have 24 or 32 planes\n");
								break;
						}
						x++;
						if (x == pHeader->wImageWidth)  // pixel packet run spans across rows
						{
							x = 0;
							if (y > 0)
								y--;
							else
								goto breakOut;
							pOut = pRGBA + y * pHeader->wImageWidth * 4;
						}
					}
				}
			}
			breakOut:;
		}
	}

TGADone:

	ri.FS_FreeFile (pTempLoadedBuffer);

	if (bFormatErrors)
	{
		Com_Error( ERR_DROP, "%s( File: \"%s\" )\n",sErrorString,name);
	}
}

void user_read_data( png_structp png_ptr, png_bytep data, png_size_t length ) {
}
void user_write_data( png_structp png_ptr, png_bytep data, png_size_t length ) {
	fileHandle_t fp = (fileHandle_t)png_get_io_ptr( png_ptr );
	ri.FS_Write( data, length, fp );
}
void user_flush_data( png_structp png_ptr ) {
	//TODO: ri.FS_Flush?
}

#ifdef _MSC_VER
	#pragma warning( push )
	#pragma warning( disable: 4611 )
#endif

int RE_SavePNG( char *filename, byte *buf, size_t width, size_t height, int byteDepth ) {
	fileHandle_t fp;
	png_structp png_ptr = NULL;
	png_infop info_ptr = NULL;
	int x, y;
	png_byte ** row_pointers = NULL;
	/* "status" contains the return value of this function. At first
	it is set to a value which means 'failure'. When the routine
	has finished its work, it is set to a value which means
	'success'. */
	int status = -1;
	/* The following number is set by trial and error only. I cannot
	see where it it is documented in the libpng manual.
	*/
	int depth = 8;

	fp = ri.FS_FOpenFileWrite( filename );
	if ( !fp ) {
		goto fopen_failed;
	}

	png_ptr = png_create_write_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (png_ptr == NULL) {
		goto png_create_write_struct_failed;
	}

	info_ptr = png_create_info_struct (png_ptr);
	if (info_ptr == NULL) {
		goto png_create_info_struct_failed;
	}

	/* Set up error handling. */

	if (setjmp (png_jmpbuf (png_ptr))) {
		goto png_failure;
	}

	/* Set image attributes. */

	png_set_IHDR (png_ptr,
		info_ptr,
		width,
		height,
		depth,
		PNG_COLOR_TYPE_RGB,
		PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_DEFAULT,
		PNG_FILTER_TYPE_DEFAULT);

	/* Initialize rows of PNG. */

	row_pointers = (png_byte **)png_malloc (png_ptr, height * sizeof (png_byte *));
	for ( y=0; y<height; ++y ) {
		png_byte *row = (png_byte *)png_malloc (png_ptr, sizeof (uint8_t) * width * byteDepth);
		row_pointers[height-y-1] = row;
		for (x = 0; x < width; ++x) {
			byte *px = buf + (width * y + x)*3;
			*row++ = px[0];
			*row++ = px[1];
			*row++ = px[2];
		}
	}

	/* Write the image data to "fp". */

//	png_init_io (png_ptr, fp);
	png_set_write_fn( png_ptr, (png_voidp)fp, user_write_data, user_flush_data );
	png_set_rows (png_ptr, info_ptr, row_pointers);
	png_write_png (png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

	/* The routine has successfully written the file, so we set
	"status" to a value which indicates success. */

	status = 0;

	for (y = 0; y < height; y++) {
		png_free (png_ptr, row_pointers[y]);
	}
	png_free (png_ptr, row_pointers);

png_failure:
png_create_info_struct_failed:
	png_destroy_write_struct (&png_ptr, &info_ptr);
png_create_write_struct_failed:
	ri.FS_FCloseFile( fp );
fopen_failed:
	return status;
}

#ifdef _MSC_VER
	#pragma warning( pop )
#endif

void R_InvertImage(byte *data, int width, int height, int depth)
{
	byte	*newData;
	byte	*oldData;
	byte	*saveData;
	int		y, stride;

	stride = width * depth;

	oldData = data + ((height - 1) * stride);
	newData = (byte *)Z_Malloc(height * stride, TAG_TEMP_IMAGE, qfalse );
	saveData = newData;

	for(y = 0; y < height; y++)
	{
		memcpy(newData, oldData, stride);
		newData += stride;
		oldData -= stride;
	}
	memcpy(data, saveData, height * stride);
	Z_Free(saveData);
}

// Lanczos3 image resampling. Better than bicubic, based on sin(x)/x algorithm

#define	LANCZOS3	(3.0f)
#define M_PI_OVER_3	(M_PI / 3.0f)

typedef struct
{
	int		pixel;
	float	weight;
} contrib_t;

typedef struct
{
	int			n;		// number of contributors
	contrib_t	*p;		// pointer to list of contributions
} contrib_list_t;

// sin(x)/x * sin(x/3)/(x/3)

float Lanczos3(float t)
{
	if(!t)
	{
		return(1.0f);
	}
	t = (float)fabs(t);
	if(t < 3.0f)
	{
		return(sinf(t * M_PI) * sinf(t * M_PI_OVER_3) / (t * M_PI * t * M_PI_OVER_3));
	}
	return(0.0f);
}

void R_Resample(byte *source, int swidth, int sheight, byte *dest, int dwidth, int dheight, int components)
{
	int				i, j, k, l, count, left, right, num;
	int				pixel;
	byte			*raster;
	float			center, weight, scale, width, height;
	contrib_list_t	*contributors;

//	MD_PushTag(TAG_RESAMPLE);

	byte *work = (byte *)Z_Malloc(dwidth * sheight * components, TAG_RESAMPLE);

	// Pre calculate filter contributions for rows
	contributors = (contrib_list_t *)Z_Malloc(sizeof(contrib_list_t) * dwidth, TAG_RESAMPLE);

	float xscale = (float)dwidth / (float)swidth;

	if(xscale < 1.0f)
	{
		width = ceilf(LANCZOS3 / xscale);
		scale = xscale;
	}
	else
	{
		width = LANCZOS3;
		scale = 1.0f;
	}
	num = ((int)width * 2) + 1;

	for(i = 0; i < dwidth; i++)
	{
		contributors[i].n = 0;
		contributors[i].p = (contrib_t *)Z_Malloc(num * sizeof(contrib_t), TAG_RESAMPLE);

		center = (float)i / xscale;
		left = (int)ceilf(center - width);
		right = (int)floorf(center + width);

		for(j = left; j <= right; j++)
		{
			weight = Lanczos3((center - (float)j) * scale) * scale;
			if(j < 0)
			{
				pixel = -j;
			}
			else if(j >= swidth)
			{
				pixel = (swidth - j) + swidth - 1;
			}
			else
			{
				pixel = j;
			}
			count = contributors[i].n++;
			contributors[i].p[count].pixel = pixel;
			contributors[i].p[count].weight = weight;
		}
	}
	// Apply filters to zoom horizontally from source to work
	for(k = 0; k < sheight; k++)
	{
		raster = source + (k * swidth * components);
		for(i = 0; i < dwidth; i++)
		{
			for(l = 0; l < components; l++)
			{
				weight = 0.0f;
				for(j = 0; j < contributors[i].n; j++)
				{
					weight += raster[(contributors[i].p[j].pixel * components) + l] * contributors[i].p[j].weight;
				}
				pixel = (byte)Com_Clamp(0.0f, 255.0f, weight);
				work[(k * dwidth * components) + (i * components) + l] = pixel;
			}
		}
	}
	// Clean up
	for(i = 0; i < dwidth; i++)
	{
		Z_Free(contributors[i].p);
	}
	Z_Free(contributors);

	// Columns
	contributors = (contrib_list_t *)Z_Malloc(sizeof(contrib_list_t) * dheight, TAG_RESAMPLE);

	float yscale = (float)dheight / (float)sheight;
	if(yscale < 1.0f)
	{
		height = ceilf(LANCZOS3 / yscale);
		scale = yscale;
	}
	else
	{
		height = LANCZOS3;
		scale = 1.0f;
	}
	num = ((int)height * 2) + 1;

	for(i = 0; i < dheight; i++)
	{
		contributors[i].n = 0;
		contributors[i].p = (contrib_t *)Z_Malloc(num * sizeof(contrib_t), TAG_RESAMPLE);

		center = (float)i / yscale;
		left = (int)ceilf(center - height);
		right = (int)floorf(center + height);

		for(j = left; j <= right; j++)
		{
			weight = Lanczos3((center - (float)j) * scale) * scale;
			if(j < 0)
			{
				pixel = -j;
			}
			else if(j >= sheight)
			{
				pixel = (sheight - j) + sheight - 1;
			}
			else
			{
				pixel = j;
			}
			count = contributors[i].n++;
			contributors[i].p[count].pixel = pixel;
			contributors[i].p[count].weight = weight;
		}
	}
	// Apply filter to columns
	for(k = 0; k < dwidth; k++)
	{
		for(l = 0; l < components; l++)
		{
			for(i = 0; i < dheight; i++)
			{
				weight = 0.0f;
				for(j = 0; j < contributors[i].n; j++)
				{
					weight += work[(contributors[i].p[j].pixel * dwidth * components) + (k * components) + l] * contributors[i].p[j].weight;
				}
				pixel = (byte)Com_Clamp(0.0f, 255.0f, weight);
				dest[(i * dwidth * components) + (k * components) + l] = pixel;
			}
		}
	}
	// Clean up
	for(i = 0; i < dheight; i++)
	{
		Z_Free(contributors[i].p);
	}
	Z_Free(contributors);
	Z_Free(work);

//	MD_PopTag();
}

bool gServerSkinHack = false;


shader_t *R_FindServerShader( const char *name, const int *lightmapIndex, const byte *styles, qboolean mipRawImage );
static char *CommaParse( char **data_p );
/*
===============
RE_SplitSkins
input = skinname, possibly being a macro for three skins
return= true if three part skins found
output= qualified names to three skins if return is true, undefined if false
===============
*/
bool RE_SplitSkins(const char *INname, char *skinhead, char *skintorso, char *skinlower)
{	//INname= "models/players/jedi_tf/|head01_skin1|torso01|lower01";
	if (strchr(INname, '|'))
	{
		char name[MAX_QPATH];
		strcpy(name, INname);
		char *p = strchr(name, '|');
		*p=0;
		p++;
		//fill in the base path
		strcpy (skinhead, name);
		strcpy (skintorso, name);
		strcpy (skinlower, name);

		//now get the the individual files
		
		//advance to second
		char *p2 = strchr(p, '|'); 
		assert(p2);
		if (!p2)
		{
			return false;
		}
		*p2=0;
		p2++;
		strcat (skinhead, p);
		strcat (skinhead, ".skin");


		//advance to third
		p = strchr(p2, '|');
		assert(p);
		if (!p)
		{
			return false;
		}
		*p=0;
		p++;
		strcat (skintorso,p2);
		strcat (skintorso, ".skin");

		strcat (skinlower,p);
		strcat (skinlower, ".skin");
		
		return true;
	}
	return false;
}

// given a name, go get the skin we want and return
qhandle_t RE_RegisterIndividualSkin( const char *name , qhandle_t hSkin) 
{
	skin_t			*skin;
	skinSurface_t	*surf;
	char			*text, *text_p;
	char			*token;
	char			surfName[MAX_QPATH];

	// load and parse the skin file
    ri.FS_ReadFile( name, (void **)&text );
	if ( !text ) {
#ifndef FINAL_BUILD
		Com_Printf( "WARNING: RE_RegisterSkin( '%s' ) failed to load!\n", name );
#endif
		return 0;
	}

	assert (tr.skins[hSkin]);	//should already be setup, but might be an 3part append

	skin = tr.skins[hSkin];

	text_p = text;
	while ( text_p && *text_p ) {
		// get surface name
		token = CommaParse( &text_p );
		Q_strncpyz( surfName, token, sizeof( surfName ) );

		if ( !token[0] ) {
			break;
		}
		// lowercase the surface name so skin compares are faster
		Q_strlwr( surfName );

		if ( *text_p == ',' ) {
			text_p++;
		}

		if ( !strncmp( token, "tag_", 4 ) ) {	//these aren't in there, but just in case you load an id style one...
			continue;
		}
		
		// parse the shader name
		token = CommaParse( &text_p );

		if ( !strcmp( &surfName[strlen(surfName)-4], "_off") )
		{
			if ( !strcmp( token ,"*off" ) )
			{
				continue;	//don't need these double offs
			}
			surfName[strlen(surfName)-4] = 0;	//remove the "_off"
		}
		if (sizeof( skin->surfaces) / sizeof( skin->surfaces[0] ) <= skin->numSurfaces)
		{
			assert( sizeof( skin->surfaces) / sizeof( skin->surfaces[0] ) > skin->numSurfaces );
			Com_Printf( "WARNING: RE_RegisterSkin( '%s' ) more than %d surfaces!\n", name, sizeof( skin->surfaces) / sizeof( skin->surfaces[0] ) );
			break;
		}
		surf = (skinSurface_t *) Hunk_Alloc( sizeof( *skin->surfaces[0] ), h_low );
		skin->surfaces[skin->numSurfaces] = (_skinSurface_t *)surf;

		Q_strncpyz( surf->name, surfName, sizeof( surf->name ) );

		if (gServerSkinHack)	surf->shader = R_FindServerShader( token, lightmapsNone, stylesDefault, qtrue );
		else					surf->shader = R_FindShader( token, lightmapsNone, stylesDefault, qtrue );
		skin->numSurfaces++;
	}

	ri.FS_FreeFile( text );


	// never let a skin have 0 shaders
	if ( skin->numSurfaces == 0 ) {
		return 0;		// use default skin
	}

	return hSkin;
}

qhandle_t RE_RegisterSkin( const char *name ) {
	qhandle_t	hSkin;
	skin_t		*skin;

	if ( !name || !name[0] ) {
		Com_Printf( "Empty name passed to RE_RegisterSkin\n" );
		return 0;
	}

	if ( strlen( name ) >= MAX_QPATH ) {
		Com_Printf( "Skin name exceeds MAX_QPATH\n" );
		return 0;
	}

	// see if the skin is already loaded
	for ( hSkin = 1; hSkin < tr.numSkins ; hSkin++ ) {
		skin = tr.skins[hSkin];
		if ( !Q_stricmp( skin->name, name ) ) {
			if( skin->numSurfaces == 0 ) {
				return 0;		// default skin
			}
			return hSkin;
		}
	}

	// allocate a new skin
	if ( tr.numSkins == MAX_SKINS ) {
		Com_Printf( "WARNING: RE_RegisterSkin( '%s' ) MAX_SKINS hit\n", name );
		return 0;
	}
	tr.numSkins++;
	skin = (struct skin_s *)Hunk_Alloc( sizeof( skin_t ), h_low );
	tr.skins[hSkin] = skin;
	Q_strncpyz( skin->name, name, sizeof( skin->name ) );
	skin->numSurfaces = 0;

	// make sure the render thread is stopped
	R_SyncRenderThread();

	// If not a .skin file, load as a single shader
	if ( strcmp( name + strlen( name ) - 5, ".skin" ) ) {
/*		skin->numSurfaces = 1;
		skin->surfaces[0] = (skinSurface_t *)Hunk_Alloc( sizeof(skin->surfaces[0]), h_low );
		skin->surfaces[0]->shader = R_FindShader( name, lightmapsNone, stylesDefault, qtrue );
		return hSkin;
*/
	}

	char skinhead[MAX_QPATH]={0};
	char skintorso[MAX_QPATH]={0};
	char skinlower[MAX_QPATH]={0};
	if ( RE_SplitSkins(name, (char*)&skinhead, (char*)&skintorso, (char*)&skinlower ) )
	{//three part
		hSkin = RE_RegisterIndividualSkin(skinhead, hSkin);
		if (hSkin)
		{
			hSkin = RE_RegisterIndividualSkin(skintorso, hSkin);
			if (hSkin)
			{
				hSkin = RE_RegisterIndividualSkin(skinlower, hSkin);
			}
		}
	}
	else
	{//single skin
		hSkin = RE_RegisterIndividualSkin(name, hSkin);
	}
	return(hSkin);
}



/*
==================
CommaParse

This is unfortunate, but the skin files aren't
compatable with our normal parsing rules.
==================
*/
static char *CommaParse( char **data_p ) {
	int c = 0, len;
	char *data;
	static	char	com_token[MAX_TOKEN_CHARS];

	data = *data_p;
	len = 0;
	com_token[0] = 0;

	// make sure incoming data is valid
	if ( !data ) {
		*data_p = NULL;
		return com_token;
	}

	while ( 1 ) {
		// skip whitespace
		while( (c = *data) <= ' ') {
			if( !c ) {
				break;
			}
			data++;
		}


		c = *data;

		// skip double slash comments
		if ( c == '/' && data[1] == '/' )
		{
			while (*data && *data != '\n')
				data++;
		}
		// skip /* */ comments
		else if ( c=='/' && data[1] == '*' ) 
		{
			while ( *data && ( *data != '*' || data[1] != '/' ) ) 
			{
				data++;
			}
			if ( *data ) 
			{
				data += 2;
			}
		}
		else
		{
			break;
		}
	}

	if ( c == 0 ) {
		return "";
	}

	// handle quoted strings
	if (c == '\"')
	{
		data++;
		while (1)
		{
			c = *data++;
			if (c=='\"' || !c)
			{
				com_token[len] = 0;
				*data_p = ( char * ) data;
				return com_token;
			}
			if (len < MAX_TOKEN_CHARS)
			{
				com_token[len] = c;
				len++;
			}
		}
	}

	// parse a regular word
	do
	{
		if (len < MAX_TOKEN_CHARS)
		{
			com_token[len] = c;
			len++;
		}
		data++;
		c = *data;
	} while (c>32 && c != ',' );

	if (len == MAX_TOKEN_CHARS)
	{
//		Com_Printf ("Token exceeded %i chars, discarded.\n", MAX_TOKEN_CHARS);
		len = 0;
	}
	com_token[len] = 0;

	*data_p = ( char * ) data;
	return com_token;
}

/*
===============
RE_RegisterServerSkin

Mangled version of the above function to load .skin files on the server.
===============
*/
qhandle_t RE_RegisterServerSkin( const char *name ) {
	qhandle_t r;

	if (ri.Cvar_VariableIntegerValue( "cl_running" ) &&
		ri.Com_TheHunkMarkHasBeenMade() &&
		ShaderHashTableExists())
	{ //If the client is running then we can go straight into the normal registerskin func
		return RE_RegisterSkin(name);
	}

	gServerSkinHack = true;
	r = RE_RegisterSkin(name);
	gServerSkinHack = false;

	return r;
}

/*
===============
R_InitSkins
===============
*/
void	R_InitSkins( void ) {
	skin_t		*skin;

	tr.numSkins = 1;

	// make the default skin have all default shaders
	skin = tr.skins[0] = (struct skin_s *)ri.Hunk_Alloc( sizeof( skin_t ), h_low );
	Q_strncpyz( skin->name, "<default skin>", sizeof( skin->name )  );
	skin->numSurfaces = 1;
	skin->surfaces[0] = (_skinSurface_t *)ri.Hunk_Alloc( sizeof( skinSurface_t ), h_low );
	skin->surfaces[0]->shader = tr.defaultShader;
}

/*
===============
R_GetSkinByHandle
===============
*/
skin_t	*R_GetSkinByHandle( qhandle_t hSkin ) {
	if ( hSkin < 1 || hSkin >= tr.numSkins ) {
		return tr.skins[0];
	}
	return tr.skins[ hSkin ];
}
