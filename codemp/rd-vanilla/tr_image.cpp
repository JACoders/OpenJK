//Anything above this #include will be ignored by the compiler
#include "qcommon/exe_headers.h"

// tr_image.c
#include "tr_local.h"
#ifdef _WIN32
#include "glext.h"
#endif

#ifdef _MSC_VER
#pragma warning (push, 3)	//go back down to 3 for the stl include
#endif
#include <map>
#ifdef _MSC_VER
#pragma warning (pop)
#endif
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

static void LoadTGA( const char *name, byte **pic, int *width, int *height );
static void LoadJPG( const char *name, byte **pic, int *width, int *height );


static byte			 s_intensitytable[256];
static unsigned char s_gammatable[256];

int		gl_filter_min = GL_LINEAR_MIPMAP_NEAREST;
int		gl_filter_max = GL_LINEAR;

//#define FILE_HASH_SIZE		1024	// actually the shader code still needs this (from another module, great),
//static	image_t*		hashTable[FILE_HASH_SIZE];

/*
** R_GammaCorrect
*/
void R_GammaCorrect( byte *buffer, int bufSize ) {
	int i;

	for ( i = 0; i < bufSize; i++ ) {
		buffer[i] = s_gammatable[buffer[i]];
	}
}

typedef struct {
	const char *name;
	int	minimize, maximize;
} textureMode_t;

textureMode_t modes[] = {
	{"GL_NEAREST", GL_NEAREST, GL_NEAREST},
	{"GL_LINEAR", GL_LINEAR, GL_LINEAR},
	{"GL_NEAREST_MIPMAP_NEAREST", GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST},
	{"GL_LINEAR_MIPMAP_NEAREST", GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR},
	{"GL_NEAREST_MIPMAP_LINEAR", GL_NEAREST_MIPMAP_LINEAR, GL_NEAREST},
	{"GL_LINEAR_MIPMAP_LINEAR", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR}
};


// makeup a nice clean, consistant name to query for and file under, for map<> usage...
//
static char *GenerateImageMappingName( const char *name )
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





/*
===============
GL_TextureMode
===============
*/
void GL_TextureMode( const char *string ) {
	int		i;
	image_t	*glt;

	for ( i=0 ; i< 6 ; i++ ) {
		if ( !Q_stricmp( modes[i].name, string ) ) {
			break;
		}
	}

	if ( i == 6 ) {
		Com_Printf ( "bad filter name\n");
		for ( i=0 ; i< 6 ; i++ ) {
			Com_Printf ("%s\n",modes[i].name);
			}
		return;
	}

	gl_filter_min = modes[i].minimize;
	gl_filter_max = modes[i].maximize;

	// If the level they requested is less than possible, set the max possible...
	if ( r_ext_texture_filter_anisotropic->value > glConfig.maxTextureFilterAnisotropy )
	{
		ri.Cvar_Set( "r_ext_texture_filter_anisotropic", va("%f",glConfig.maxTextureFilterAnisotropy) );
	}
	// change all the existing mipmap texture objects
					 R_Images_StartIteration();
	while ( (glt   = R_Images_GetNextIteration()) != NULL)
	{
		if ( glt->mipmap ) {
			GL_Bind (glt);
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);

			if(glConfig.maxTextureFilterAnisotropy>0) {
				if(r_ext_texture_filter_anisotropic->integer>1) {
					qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, r_ext_texture_filter_anisotropic->value);
				} else {
					qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.0f);
				}
			}
		}
	}
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
	int		i=0;
	image_t	*image;
	int		texels=0;
	float	texBytes = 0.0f;
	const char *yesno[] = {"no ", "yes"};

	Com_Printf ( "\n      -w-- -h-- -mm- -if-- wrap --name-------\n");

	int iNumImages = R_Images_StartIteration();
	while ( (image = R_Images_GetNextIteration()) != NULL)
	{
		texels   += image->width*image->height;
		texBytes += image->width*image->height * R_BytesPerTex (image->internalFormat);
		Com_Printf (  "%4i: %4i %4i  %s ",
			i, image->width, image->height, yesno[image->mipmap] );
		switch ( image->internalFormat ) {
		case 1:
			Com_Printf ("I    " );
			break;
		case 2:
			Com_Printf ("IA   " );
			break;
		case 3:
			Com_Printf ("RGB  " );
			break;
		case 4:
			Com_Printf ("RGBA " );
			break;
		case GL_RGBA8:
			Com_Printf ("RGBA8" );
			break;
		case GL_RGB8:
			Com_Printf ("RGB8" );
			break;
		case GL_RGB4_S3TC:
			Com_Printf ("S3TC " );
			break;
		case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
			Com_Printf ("DXT1 " );
			break;
		case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
			Com_Printf ("DXT5 " );
			break;
		case GL_RGBA4:
			Com_Printf ("RGBA4" );
			break;
		case GL_RGB5:
			Com_Printf ("RGB5 " );
			break;
		default:
			Com_Printf ("???? " );
		}

		switch ( image->wrapClampMode ) {
		case GL_REPEAT:
			Com_Printf ("rept " );
			break;
		case GL_CLAMP:
			Com_Printf ("clmp " );
			break;
		case GL_CLAMP_TO_EDGE:
			Com_Printf ("clpE " );
			break;
		default:
			Com_Printf ("%4i ", image->wrapClampMode );
			break;
		}
		
		Com_Printf ("%s\n", image->imgName );
		i++;
	}
	Com_Printf ( " ---------\n");
	Com_Printf ( "      -w-- -h-- -mm- -if- wrap --name-------\n");
	Com_Printf ( " %i total texels (not including mipmaps)\n", texels );
	Com_Printf ( " %.2fMB total texture mem (not including mipmaps)\n", texBytes/1048576.0f );
	Com_Printf ( " %i total images\n\n", iNumImages );
}

//=======================================================================


/*
================
R_LightScaleTexture

Scale up the pixel values in a texture to increase the
lighting range
================
*/
void R_LightScaleTexture (unsigned *in, int inwidth, int inheight, qboolean only_gamma )
{
	if ( only_gamma )
	{
		if ( !glConfig.deviceSupportsGamma )
		{
			int		i, c;
			byte	*p;

			p = (byte *)in;

			c = inwidth*inheight;
			for (i=0 ; i<c ; i++, p+=4)
			{
				p[0] = s_gammatable[p[0]];
				p[1] = s_gammatable[p[1]];
				p[2] = s_gammatable[p[2]];
			}
		}
	}
	else
	{
		int		i, c;
		byte	*p;

		p = (byte *)in;

		c = inwidth*inheight;

		if ( glConfig.deviceSupportsGamma )
		{
			for (i=0 ; i<c ; i++, p+=4)
			{
				p[0] = s_intensitytable[p[0]];
				p[1] = s_intensitytable[p[1]];
				p[2] = s_intensitytable[p[2]];
			}
		}
		else
		{
			for (i=0 ; i<c ; i++, p+=4)
			{
				p[0] = s_gammatable[s_intensitytable[p[0]]];
				p[1] = s_gammatable[s_intensitytable[p[1]]];
				p[2] = s_gammatable[s_intensitytable[p[2]]];
			}
		}
	}
}


/*
================
R_MipMap2

Operates in place, quartering the size of the texture
Proper linear filter
================
*/
static void R_MipMap2( unsigned *in, int inWidth, int inHeight ) {
	int			i, j, k;
	byte		*outpix;
	int			inWidthMask, inHeightMask;
	int			total;
	int			outWidth, outHeight;
	unsigned	*temp;

	outWidth = inWidth >> 1;
	outHeight = inHeight >> 1;
	temp = (unsigned int *)Hunk_AllocateTempMemory( outWidth * outHeight * 4 );

	inWidthMask = inWidth - 1;
	inHeightMask = inHeight - 1;

	for ( i = 0 ; i < outHeight ; i++ ) {
		for ( j = 0 ; j < outWidth ; j++ ) {
			outpix = (byte *) ( temp + i * outWidth + j );
			for ( k = 0 ; k < 4 ; k++ ) {
				total = 
					1 * ((byte *)&in[ ((i*2-1)&inHeightMask)*inWidth + ((j*2-1)&inWidthMask) ])[k] +
					2 * ((byte *)&in[ ((i*2-1)&inHeightMask)*inWidth + ((j*2)&inWidthMask) ])[k] +
					2 * ((byte *)&in[ ((i*2-1)&inHeightMask)*inWidth + ((j*2+1)&inWidthMask) ])[k] +
					1 * ((byte *)&in[ ((i*2-1)&inHeightMask)*inWidth + ((j*2+2)&inWidthMask) ])[k] +

					2 * ((byte *)&in[ ((i*2)&inHeightMask)*inWidth + ((j*2-1)&inWidthMask) ])[k] +
					4 * ((byte *)&in[ ((i*2)&inHeightMask)*inWidth + ((j*2)&inWidthMask) ])[k] +
					4 * ((byte *)&in[ ((i*2)&inHeightMask)*inWidth + ((j*2+1)&inWidthMask) ])[k] +
					2 * ((byte *)&in[ ((i*2)&inHeightMask)*inWidth + ((j*2+2)&inWidthMask) ])[k] +

					2 * ((byte *)&in[ ((i*2+1)&inHeightMask)*inWidth + ((j*2-1)&inWidthMask) ])[k] +
					4 * ((byte *)&in[ ((i*2+1)&inHeightMask)*inWidth + ((j*2)&inWidthMask) ])[k] +
					4 * ((byte *)&in[ ((i*2+1)&inHeightMask)*inWidth + ((j*2+1)&inWidthMask) ])[k] +
					2 * ((byte *)&in[ ((i*2+1)&inHeightMask)*inWidth + ((j*2+2)&inWidthMask) ])[k] +

					1 * ((byte *)&in[ ((i*2+2)&inHeightMask)*inWidth + ((j*2-1)&inWidthMask) ])[k] +
					2 * ((byte *)&in[ ((i*2+2)&inHeightMask)*inWidth + ((j*2)&inWidthMask) ])[k] +
					2 * ((byte *)&in[ ((i*2+2)&inHeightMask)*inWidth + ((j*2+1)&inWidthMask) ])[k] +
					1 * ((byte *)&in[ ((i*2+2)&inHeightMask)*inWidth + ((j*2+2)&inWidthMask) ])[k];
				outpix[k] = total / 36;
			}
		}
	}

	memcpy( in, temp, outWidth * outHeight * 4 );
	Hunk_FreeTempMemory( temp );
}

/*
================
R_MipMap

Operates in place, quartering the size of the texture
================
*/
static void R_MipMap (byte *in, int width, int height) {
	int		i, j;
	byte	*out;
	int		row;

	if ( !r_simpleMipMaps->integer ) {
		R_MipMap2( (unsigned *)in, width, height );
		return;
	}

	if ( width == 1 && height == 1 ) {
		return;
	}

	row = width * 4;
	out = in;
	width >>= 1;
	height >>= 1;

	if ( width == 0 || height == 0 ) {
		width += height;	// get largest
		for (i=0 ; i<width ; i++, out+=4, in+=8 ) {
			out[0] = ( in[0] + in[4] )>>1;
			out[1] = ( in[1] + in[5] )>>1;
			out[2] = ( in[2] + in[6] )>>1;
			out[3] = ( in[3] + in[7] )>>1;
		}
		return;
	}

	for (i=0 ; i<height ; i++, in+=row) {
		for (j=0 ; j<width ; j++, out+=4, in+=8) {
			out[0] = (in[0] + in[4] + in[row+0] + in[row+4])>>2;
			out[1] = (in[1] + in[5] + in[row+1] + in[row+5])>>2;
			out[2] = (in[2] + in[6] + in[row+2] + in[row+6])>>2;
			out[3] = (in[3] + in[7] + in[row+3] + in[row+7])>>2;
		}
	}
}


/*
==================
R_BlendOverTexture

Apply a color blend over a set of pixels
==================
*/
static void R_BlendOverTexture( byte *data, int pixelCount, byte blend[4] ) {
	int		i;
	int		inverseAlpha;
	int		premult[3];

	inverseAlpha = 255 - blend[3];
	premult[0] = blend[0] * blend[3];
	premult[1] = blend[1] * blend[3];
	premult[2] = blend[2] * blend[3];

	for ( i = 0 ; i < pixelCount ; i++, data+=4 ) {
		data[0] = ( data[0] * inverseAlpha + premult[0] ) >> 9;
		data[1] = ( data[1] * inverseAlpha + premult[1] ) >> 9;
		data[2] = ( data[2] * inverseAlpha + premult[2] ) >> 9;
	}
}

byte	mipBlendColors[16][4] = {
	{0,0,0,0},
	{255,0,0,128},
	{0,255,0,128},
	{0,0,255,128},
	{255,0,0,128},
	{0,255,0,128},
	{0,0,255,128},
	{255,0,0,128},
	{0,255,0,128},
	{0,0,255,128},
	{255,0,0,128},
	{0,255,0,128},
	{0,0,255,128},
	{255,0,0,128},
	{0,255,0,128},
	{0,0,255,128},
};





class CStringComparator
{
public:
	bool operator()(const char *s1, const char *s2) const { return(strcmp(s1, s2) < 0); } 
};

typedef map <LPCSTR, image_t *, CStringComparator>	AllocatedImages_t;
													AllocatedImages_t AllocatedImages;
													AllocatedImages_t::iterator itAllocatedImages;
int giTextureBindNum = 1024;	// will be set to this anyway at runtime, but wtf?


// return = number of images in the list, for those interested
//
int R_Images_StartIteration(void)
{
	itAllocatedImages = AllocatedImages.begin();
	return AllocatedImages.size();
}

image_t *R_Images_GetNextIteration(void)
{
	if (itAllocatedImages == AllocatedImages.end())
		return NULL;

	image_t *pImage = (*itAllocatedImages).second;
	++itAllocatedImages;
	return pImage;
}

// clean up anything to do with an image_t struct, but caller will have to clear the internal to an image_t struct ready for either struct free() or overwrite...
//
// (avoid using ri.xxxx stuff here in case running on dedicated)
//
static void R_Images_DeleteImageContents( image_t *pImage )
{
	assert(pImage);	// should never be called with NULL
	if (pImage)
	{
		if (qglDeleteTextures != NULL) {	//won't have one if we switched to dedicated.
			qglDeleteTextures( 1, &pImage->texnum );
		}
		Z_Free(pImage);
	}
}





/*
===============
Upload32

===============
*/
extern qboolean charSet;
static void Upload32( unsigned *data, 
						 GLenum format,
						 qboolean mipmap, 
						 qboolean picmip, 
						 qboolean isLightmap,
						 qboolean allowTC,
						 int *pformat, 
						 USHORT *pUploadWidth, USHORT *pUploadHeight, bool bRectangle = false )
{
	GLuint uiTarget = GL_TEXTURE_2D;
	if ( bRectangle )
	{
		uiTarget = GL_TEXTURE_RECTANGLE_EXT;
	}

	if (format == GL_RGBA)
	{
		int			samples;
		int			i, c;
		byte		*scan;
		float		rMax = 0, gMax = 0, bMax = 0;
		int			width = *pUploadWidth;
		int			height = *pUploadHeight; 

		//
		// perform optional picmip operation
		//
		if ( picmip ) {
			for(i = 0; i < r_picmip->integer; i++) {
				R_MipMap( (byte *)data, width, height );
				width >>= 1;
				height >>= 1;
				if (width < 1) {
					width = 1;
				}
				if (height < 1) {
					height = 1;
				}
			}
		}

		//
		// clamp to the current upper OpenGL limit
		// scale both axis down equally so we don't have to
		// deal with a half mip resampling
		//
		while ( width > glConfig.maxTextureSize	|| height > glConfig.maxTextureSize ) {
			R_MipMap( (byte *)data, width, height );
			width >>= 1;
			height >>= 1;
		}

		//
		// scan the texture for each channel's max values
		// and verify if the alpha channel is being used or not
		//
		c = width*height;
		scan = ((byte *)data);
		samples = 3;
		for ( i = 0; i < c; i++ )
		{
			if ( scan[i*4+0] > rMax )
			{
				rMax = scan[i*4+0];
			}
			if ( scan[i*4+1] > gMax )
			{
				gMax = scan[i*4+1];
			}
			if ( scan[i*4+2] > bMax )
			{
				bMax = scan[i*4+2];
			}
			if ( scan[i*4 + 3] != 255 ) 
			{
				samples = 4;
				break;
			}
		}

		// select proper internal format
		if ( samples == 3 )
		{
			if ( glConfig.textureCompression == TC_S3TC && allowTC )
			{
				*pformat = GL_RGB4_S3TC;
			}
			else if ( glConfig.textureCompression == TC_S3TC_DXT && allowTC )
			{	// Compress purely color - no alpha
				if ( r_texturebits->integer == 16 ) {
					*pformat = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;	//this format cuts to 16 bit
				}
				else {//if we aren't using 16 bit then, use 32 bit compression
					*pformat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
				}
			}
			else if ( isLightmap && r_texturebitslm->integer > 0 )
			{
				int lmBits = r_texturebitslm->integer & 0x30; // 16 or 32
				// Allow different bit depth when we are a lightmap
				if ( lmBits == 16 )
					*pformat = GL_RGB5;
				else
					*pformat = GL_RGB8;
			}
			else if ( r_texturebits->integer == 16 )
			{
				*pformat = GL_RGB5;
			}
			else if ( r_texturebits->integer == 32 )
			{
				*pformat = GL_RGB8;
			}
			else
			{
				*pformat = 3;
			}
		}
		else if ( samples == 4 )
		{
			if ( glConfig.textureCompression == TC_S3TC_DXT && allowTC)
			{	// Compress both alpha and color
				*pformat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
			}
			else if ( r_texturebits->integer == 16 )
			{
				*pformat = GL_RGBA4;
			}
			else if ( r_texturebits->integer == 32 )
			{
				*pformat = GL_RGBA8;
			}
			else
			{
				*pformat = 4;
			}
		}

		*pUploadWidth = width;
		*pUploadHeight = height;

		// copy or resample data as appropriate for first MIP level
		if (!mipmap)
		{
			qglTexImage2D( uiTarget, 0, *pformat, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data );
			goto done;
		}

		R_LightScaleTexture (data, width, height, (qboolean)!mipmap );

		qglTexImage2D( uiTarget, 0, *pformat, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data );

		if (mipmap)
		{
			int		miplevel;

			miplevel = 0;
			while (width > 1 || height > 1)
			{
				R_MipMap( (byte *)data, width, height );
				width >>= 1;
				height >>= 1;
				if (width < 1)
					width = 1;
				if (height < 1)
					height = 1;
				miplevel++;

				if ( r_colorMipLevels->integer ) 
				{
					R_BlendOverTexture( (byte *)data, width * height, mipBlendColors[miplevel] );
				}

				qglTexImage2D( uiTarget, miplevel, *pformat, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data );
			}
		}
	}
	else
	{
	}

done:

	if (mipmap)
	{
		qglTexParameterf(uiTarget, GL_TEXTURE_MIN_FILTER, gl_filter_min);
		qglTexParameterf(uiTarget, GL_TEXTURE_MAG_FILTER, gl_filter_max);
		if(r_ext_texture_filter_anisotropic->integer>1 && glConfig.maxTextureFilterAnisotropy>0)
		{			
			qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, r_ext_texture_filter_anisotropic->value );
		}
	}
	else
	{
		qglTexParameterf(uiTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		qglTexParameterf(uiTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	}

	GL_CheckErrors();
}

#if 0
//3d tex version -rww
static void Upload32_3D( unsigned *data, 
						 int img_depth,
						 qboolean mipmap, 
						 qboolean picmip, 
						 qboolean isLightmap,
						 qboolean allowTC,
						 int *pformat, 
						 USHORT *pUploadWidth, USHORT *pUploadHeight )
{
	int			samples;
	int			i, c;
	byte		*scan;
	float		rMax = 0, gMax = 0, bMax = 0;
	int			width = *pUploadWidth;
	int			height = *pUploadHeight; 
	int			depth = img_depth;

	//
	// perform optional picmip operation
	//
	if ( picmip ) {
		for(i = 0; i < r_picmip->integer; i++) {
			R_MipMap( (byte *)data, width, height );
			width >>= 1;
			height >>= 1;
			if (width < 1) {
				width = 1;
			}
			if (height < 1) {
				height = 1;
			}
		}
	}

	//
	// clamp to the current upper OpenGL limit
	// scale both axis down equally so we don't have to
	// deal with a half mip resampling
	//
	while ( width > glConfig.maxTextureSize	|| height > glConfig.maxTextureSize ) {
		R_MipMap( (byte *)data, width, height );
		width >>= 1;
		height >>= 1;
	}

	//
	// scan the texture for each channel's max values
	// and verify if the alpha channel is being used or not
	//
	c = width*height;
	scan = ((byte *)data);
	samples = 3;
	for ( i = 0; i < c; i++ )
	{
		if ( scan[i*4+0] > rMax )
		{
			rMax = scan[i*4+0];
		}
		if ( scan[i*4+1] > gMax )
		{
			gMax = scan[i*4+1];
		}
		if ( scan[i*4+2] > bMax )
		{
			bMax = scan[i*4+2];
		}
		if ( scan[i*4 + 3] != 255 ) 
		{
			samples = 4;
			break;
		}
	}

	// select proper internal format
	if ( samples == 3 )
	{
		if ( glConfig.textureCompression == TC_S3TC && allowTC )
		{
			*pformat = GL_RGB4_S3TC;
		}
		else if ( glConfig.textureCompression == TC_S3TC_DXT && allowTC )
		{	// Compress purely color - no alpha
			if ( r_texturebits->integer == 16 ) {
				*pformat = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;	//this format cuts to 16 bit
			}
			else {//if we aren't using 16 bit then, use 32 bit compression
				*pformat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
			}
		}
		else if ( isLightmap && r_texturebitslm->integer > 0 )
		{
			// Allow different bit depth when we are a lightmap
			if ( r_texturebitslm->integer == 16 )
			{
				*pformat = GL_RGB5;
			}
			else if ( r_texturebitslm->integer == 32 )
			{
				*pformat = GL_RGB8;
			}
		}
		else if ( r_texturebits->integer == 16 )
		{
			*pformat = GL_RGB5;
		}
		else if ( r_texturebits->integer == 32 )
		{
			*pformat = GL_RGB8;
		}
		else
		{
			*pformat = 3;
		}
	}
	else if ( samples == 4 )
	{
		if ( glConfig.textureCompression == TC_S3TC_DXT && allowTC)
		{	// Compress both alpha and color
			*pformat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
		}
		else if ( r_texturebits->integer == 16 )
		{
			*pformat = GL_RGBA4;
		}
		else if ( r_texturebits->integer == 32 )
		{
			*pformat = GL_RGBA8;
		}
		else
		{
			*pformat = 4;
		}
	}

	*pUploadWidth = width;
	*pUploadHeight = height;

	// copy or resample data as appropriate for first MIP level
	if (!mipmap)
	{
		qglTexImage3DEXT (GL_TEXTURE_3D, 0, *pformat, width, height, depth, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		goto done;
	}

	R_LightScaleTexture (data, width, height, (qboolean)!mipmap );

	qglTexImage3DEXT (GL_TEXTURE_3D, 0, *pformat, width, height, depth, 0, GL_RGBA, GL_UNSIGNED_BYTE, data );

	if (mipmap)
	{
		int		miplevel;

		miplevel = 0;
		while (width > 1 || height > 1)
		{
			R_MipMap( (byte *)data, width, height );
			width >>= 1;
			height >>= 1;
			if (width < 1)
				width = 1;
			if (height < 1)
				height = 1;
			miplevel++;

			if ( r_colorMipLevels->integer ) 
			{
				R_BlendOverTexture( (byte *)data, width * height, mipBlendColors[miplevel] );
			}

			qglTexImage2D (GL_TEXTURE_2D, miplevel, *pformat, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data );
		}
	}
done:

	if (mipmap)
	{
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
		if(r_ext_texture_filter_anisotropic->integer>1 && glConfig.maxTextureFilterAnisotropy>0) {
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 2.0f);
		}
	}
	else
	{
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	}

	GL_CheckErrors();
}
#endif

static void GL_ResetBinds(void)
{
	memset( glState.currenttextures, 0, sizeof( glState.currenttextures ) );
	if ( qglBindTexture != NULL ) 
	{
		if ( qglActiveTextureARB != NULL ) 
		{
			GL_SelectTexture( 1 );
			qglBindTexture( GL_TEXTURE_2D, 0 );
			GL_SelectTexture( 0 );
			qglBindTexture( GL_TEXTURE_2D, 0 );
		} 
		else 
		{
			qglBindTexture( GL_TEXTURE_2D, 0 );
		}
	}
}


// special function used in conjunction with "devmapbsp"...
//
// (avoid using ri.xxxx stuff here in case running on dedicated)
//
void R_Images_DeleteLightMaps(void)
{
	qboolean bEraseOccured = qfalse;
	for (AllocatedImages_t::iterator itImage = AllocatedImages.begin(); itImage != AllocatedImages.end(); bEraseOccured?itImage:++itImage)
	{			
		bEraseOccured = qfalse;

		image_t *pImage = (*itImage).second;
		
		if (pImage->imgName[0] == '*' && strstr(pImage->imgName,"lightmap"))	// loose check, but should be ok
		{
			R_Images_DeleteImageContents(pImage);
#ifdef _WIN32
			itImage = AllocatedImages.erase(itImage);
			bEraseOccured = qtrue;
#else
			// MS & Dinkimware got the map::erase return wrong (it's null)
			AllocatedImages_t::iterator itTemp = itImage;
			itImage++;
			AllocatedImages.erase(itTemp);
#endif
		}
	}

	GL_ResetBinds();
}

// special function currently only called by Dissolve code...
//
void R_Images_DeleteImage(image_t *pImage)
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
void R_Images_Clear(void)
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
		Com_Printf ("%d: (%4dx%4dy) \"%s\"",iImage, pImage->width, pImage->height, pImage->imgName);
		ri.Printf( PRINT_DEVELOPER, S_COLOR_RED ", levused %d",pImage->iLastLevelUsedOn);
		Com_Printf ("\n");

		iTexels += pImage->width * pImage->height;
		iImage++;
	}
	Com_Printf ("%d Images. %d (%.2fMB) texels total, (not including mipmaps)\n",iNumImages, iTexels, (float)iTexels / 1024.0f / 1024.0f);
	ri.Printf( PRINT_DEVELOPER, S_COLOR_RED "RE_RegisterMedia_GetLevel(): %d",RE_RegisterMedia_GetLevel());
}


// implement this if you need to, do a find for the caller. I don't need it though, so far.
//
//void		RE_RegisterImages_LevelLoadBegin(const char *psMapName);


// currently, this just goes through all the images and dumps any not referenced on this level...
//
qboolean RE_RegisterImages_LevelLoadEnd(void)
{
	ri.Printf( PRINT_DEVELOPER, S_COLOR_RED "RE_RegisterImages_LevelLoadEnd():\n");

//	int iNumImages = AllocatedImages.size();	// more for curiosity, really.

	qboolean bEraseOccured = qfalse;
	for (AllocatedImages_t::iterator itImage = AllocatedImages.begin(); itImage != AllocatedImages.end(); bEraseOccured?itImage:++itImage)
	{			
		bEraseOccured = qfalse;

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
				ri.Printf( PRINT_DEVELOPER, S_COLOR_RED "Dumping image \"%s\"\n",pImage->imgName);

				R_Images_DeleteImageContents(pImage);
#ifdef _WIN32
				itImage = AllocatedImages.erase(itImage);
				bEraseOccured = qtrue;
#else
				AllocatedImages_t::iterator itTemp = itImage;
				itImage++;
				AllocatedImages.erase(itTemp);	
#endif
			}
		}
	}


	// this check can be deleted AFAIC, it seems to be just a quake thing...
	//
//	iNumImages = R_Images_StartIteration();
//	if (iNumImages > MAX_DRAWIMAGES)
//	{
//		Com_Printf (S_COLOR_YELLOW  "Level uses %d images, old limit was MAX_DRAWIMAGES (%d)\n", iNumImages, MAX_DRAWIMAGES);
//	}

	ri.Printf( PRINT_DEVELOPER, S_COLOR_RED "RE_RegisterImages_LevelLoadEnd(): Ok\n");	

	GL_ResetBinds();

	return bEraseOccured;
}



// returns image_t struct if we already have this, else NULL. No disk-open performed 
//	(important for creating default images).
//
// This is called by both R_FindImageFile and anything that creates default images...
//
static image_t *R_FindImageFile_NoLoad(const char *name, qboolean mipmap, qboolean allowPicmip, qboolean allowTC, int glWrapClampMode )
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
				Com_Printf (S_COLOR_YELLOW  "WARNING: reused image %s with mixed mipmap parm\n", pName );
			}
			if ( pImage->allowPicmip != !!allowPicmip ) {
				Com_Printf (S_COLOR_YELLOW  "WARNING: reused image %s with mixed allowPicmip parm\n", pName );
			}
			if ( pImage->wrapClampMode != glWrapClampMode ) {
				Com_Printf (S_COLOR_YELLOW  "WARNING: reused image %s with mixed glWrapClampMode parm\n", pName );
			}
		}
			  
		pImage->iLastLevelUsedOn = RE_RegisterMedia_GetLevel();

		return pImage;
	}

	return NULL;
}



/*
================
R_CreateImage

This is the only way any image_t are created
================
*/
image_t *R_CreateImage( const char *name, const byte *pic, int width, int height, 
					   GLenum format, qboolean mipmap, qboolean allowPicmip, qboolean allowTC, int glWrapClampMode, bool bRectangle )
{
	image_t		*image;
	qboolean	isLightmap = qfalse;

	if (strlen(name) >= MAX_QPATH ) {
		Com_Error (ERR_DROP, "R_CreateImage: \"%s\" is too long\n", name);
	}

	if(glConfig.clampToEdgeAvailable && glWrapClampMode == GL_CLAMP) {
		glWrapClampMode = GL_CLAMP_TO_EDGE;
	}

	if (name[0] == '*')
	{
		const char *psLightMapNameSearchPos = strrchr(name,'/');
		if (  psLightMapNameSearchPos && !strncmp( psLightMapNameSearchPos+1, "lightmap", 8 ) ) {
			isLightmap = qtrue;
		}
	}

	if ( (width&(width-1)) || (height&(height-1)) )
	{
		Com_Error( ERR_FATAL, "R_CreateImage: %s dimensions (%i x %i) not power of 2!\n",name,width,height);
	}

	image = R_FindImageFile_NoLoad(name, mipmap, allowPicmip, allowTC, glWrapClampMode );
	if (image) {
		return image;
	}

	image = (image_t*) Z_Malloc( sizeof( image_t ), TAG_IMAGE_T, qtrue );
//	memset(image,0,sizeof(*image));	// qtrue above does this 
	
	image->texnum = 1024 + giTextureBindNum++;	// ++ is of course staggeringly important...

	// record which map it was used on...
	//
	image->iLastLevelUsedOn = RE_RegisterMedia_GetLevel();

	image->mipmap = !!mipmap;
	image->allowPicmip = !!allowPicmip;

	Q_strncpyz(image->imgName, name, sizeof(image->imgName));

	image->width = width;
	image->height = height;
	image->wrapClampMode = glWrapClampMode;

	if ( qglActiveTextureARB ) {
		GL_SelectTexture( 0 );
	}

	GLuint uiTarget = GL_TEXTURE_2D;
	if ( bRectangle ) 
	{
		qglDisable( uiTarget ); 
		uiTarget = GL_TEXTURE_RECTANGLE_EXT;
		qglEnable( uiTarget );
		glWrapClampMode = GL_CLAMP_TO_EDGE;	// default mode supported by rectangle.
		qglBindTexture( uiTarget, image->texnum );
	}
	else
	{
		GL_Bind(image);
	}

	Upload32( (unsigned *)pic,	format,
								(qboolean)image->mipmap,
								allowPicmip,
								isLightmap,
								allowTC,
								&image->internalFormat,
								&image->width,
								&image->height, bRectangle );

	qglTexParameterf( uiTarget, GL_TEXTURE_WRAP_S, glWrapClampMode );
	qglTexParameterf( uiTarget, GL_TEXTURE_WRAP_T, glWrapClampMode );

	qglBindTexture( uiTarget, 0 );	//jfm: i don't know why this is here, but it breaks lightmaps when there's only 1
	glState.currenttextures[glState.currenttmu] = 0;	//mark it not bound

	LPCSTR psNewName = GenerateImageMappingName(name);
	Q_strncpyz(image->imgName, psNewName, sizeof(image->imgName));
	AllocatedImages[ image->imgName ] = image;

	if ( bRectangle )
	{
		qglDisable( uiTarget );
		qglEnable( GL_TEXTURE_2D );
	}

	return image;
}

//rwwRMG - added
void R_CreateAutomapImage( const char *name, const byte *pic, int width, int height, qboolean mipmap, qboolean allowPicmip, qboolean allowTC, int glWrapClampMode ) 
{
	R_CreateImage(name, pic, width, height, GL_RGBA, mipmap, allowPicmip, allowTC, glWrapClampMode);
}

/*
=========================================================

TARGA LOADING

=========================================================
*/
/*
Ghoul2 Insert Start
*/

bool LoadTGAPalletteImage ( const char *name, byte **pic, int *width, int *height)
{
	int		columns, rows, numPixels;
	byte	*buf_p;
	byte	*buffer;
	TargaHeader	targa_header;
	byte	*dataStart;

	*pic = NULL;

	//
	// load the file
	//
	ri.FS_ReadFile ( ( char * ) name, (void **)&buffer);
	if (!buffer) {
		return false;
	}

	buf_p = buffer;

	targa_header.id_length = *buf_p++;
	targa_header.colormap_type = *buf_p++;
	targa_header.image_type = *buf_p++;
	
	targa_header.colormap_index = LittleShort ( *(short *)buf_p );
	buf_p += 2;
	targa_header.colormap_length = LittleShort ( *(short *)buf_p );
	buf_p += 2;
	targa_header.colormap_size = *buf_p++;
	targa_header.x_origin = LittleShort ( *(short *)buf_p );
	buf_p += 2;
	targa_header.y_origin = LittleShort ( *(short *)buf_p );
	buf_p += 2;
	targa_header.width = LittleShort ( *(short *)buf_p );
	buf_p += 2;
	targa_header.height = LittleShort ( *(short *)buf_p );
	buf_p += 2;
	targa_header.pixel_size = *buf_p++;
	targa_header.attributes = *buf_p++;

	if (targa_header.image_type!=1 )
	{
		Com_Error (ERR_DROP, "LoadTGAPalletteImage: Only type 1 (uncompressed pallettised) TGA images supported\n");
	}

	if ( targa_header.colormap_type == 0 )
	{
		Com_Error( ERR_DROP, "LoadTGAPalletteImage: colormaps ONLY supported\n" );
	}

	columns = targa_header.width;
	rows = targa_header.height;
	numPixels = columns * rows;

	if (width)
		*width = columns;
	if (height)
		*height = rows;

	*pic = (unsigned char *) Z_Malloc (numPixels, TAG_TEMP_WORKSPACE, qfalse );
	if (targa_header.id_length != 0)
	{
		buf_p += targa_header.id_length;  // skip TARGA image comment
	}
	dataStart = buf_p + (targa_header.colormap_length * (targa_header.colormap_size / 4));
	memcpy(*pic, dataStart, numPixels);
	ri.FS_FreeFile (buffer);

	return true;
}

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

static void LoadTGA ( const char *name, byte **pic, int *width, int *height)
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

static void R_JPGErrorExit(j_common_ptr cinfo)
{
	char buffer[JMSG_LENGTH_MAX];

	(*cinfo->err->format_message) (cinfo, buffer);

	/* Let the memory manager delete any temp files before we die */
	jpeg_destroy(cinfo);

	Com_Printf("%s", buffer);
}

static void R_JPGOutputMessage(j_common_ptr cinfo)
{
	char buffer[JMSG_LENGTH_MAX];

	/* Create the message */
	(*cinfo->err->format_message) (cinfo, buffer);

	/* Send it to stderr, adding a newline */
	Com_Printf("%s\n", buffer);
}

static void LoadJPG( const char *filename, unsigned char **pic, int *width, int *height ) {
	/* This struct contains the JPEG decompression parameters and pointers to
	* working space (which is allocated as needed by the JPEG library).
	*/
	struct jpeg_decompress_struct cinfo = { NULL };
	/* We use our private extension JPEG error handler.
	* Note that this struct must live as long as the main JPEG parameter
	* struct, to avoid dangling-pointer problems.
	*/
	/* This struct represents a JPEG error handler.  It is declared separately
	* because applications often want to supply a specialized error handler
	* (see the second half of this file for an example).  But here we just
	* take the easy way out and use the standard error handler, which will
	* print a message on stderr and call exit() if compression fails.
	* Note that this struct must live as long as the main JPEG parameter
	* struct, to avoid dangling-pointer problems.
	*/
	struct jpeg_error_mgr jerr;
	/* More stuff */
	JSAMPARRAY buffer;		/* Output row buffer */
	unsigned int row_stride;  /* physical row width in output buffer */
	unsigned int pixelcount, memcount;
	unsigned int sindex, dindex;
	byte *out;
	union {
		byte *b;
		void *v;
	} fbuffer;
	byte  *buf;
	/* In this example we want to open the input file before doing anything else,
	* so that the setjmp() error recovery below can assume the file is open.
	* VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
	* requires it in order to read binary files.
	*/

	int len = ri.FS_ReadFile ( ( char * ) filename, &fbuffer.v);
	if (!fbuffer.b || len < 0) {
		return;
	}

	/* Step 1: allocate and initialize JPEG decompression object */

	/* We have to set up the error handler first, in case the initialization
	* step fails.  (Unlikely, but it could happen if you are out of memory.)
	* This routine fills in the contents of struct jerr, and returns jerr's
	* address which we place into the link field in cinfo.
	*/
	cinfo.err = jpeg_std_error(&jerr);
	cinfo.err->error_exit = R_JPGErrorExit;
	cinfo.err->output_message = R_JPGOutputMessage;

	/* Now we can initialize the JPEG decompression object. */
	jpeg_create_decompress(&cinfo);

	/* Step 2: specify data source (eg, a file) */

	jpeg_mem_src(&cinfo, fbuffer.b, len);

	/* Step 3: read file parameters with jpeg_read_header() */

	(void) jpeg_read_header(&cinfo, TRUE);
	/* We can ignore the return value from jpeg_read_header since
	*   (a) suspension is not possible with the stdio data source, and
	*   (b) we passed TRUE to reject a tables-only JPEG file as an error.
	* See libjpeg.doc for more info.
	*/

	/* Step 4: set parameters for decompression */

 
	/* Make sure it always converts images to RGB color space. This will 
	* automatically convert 8-bit greyscale images to RGB as well.	*/
	cinfo.out_color_space = JCS_RGB;

	/* Step 5: Start decompressor */

	(void) jpeg_start_decompress(&cinfo);
	/* We can ignore the return value since suspension is not possible
	* with the stdio data source.
	*/

	/* We may need to do some setup of our own at this point before reading
	* the data.  After jpeg_start_decompress() we have the correct scaled
	* output image dimensions available, as well as the output colormap
	* if we asked for color quantization.
	* In this example, we need to make an output work buffer of the right size.
	*/ 
	/* JSAMPLEs per row in output buffer */
	pixelcount = cinfo.output_width * cinfo.output_height;

	if(!cinfo.output_width || !cinfo.output_height 
		|| ((pixelcount * 4) / cinfo.output_width) / 4 != cinfo.output_height 
		|| pixelcount > 0x1FFFFFFF || cinfo.output_components != 3 
		) 
	{ 
		// Free the memory to make sure we don't leak memory 
		ri.FS_FreeFile (fbuffer.v); 
		jpeg_destroy_decompress(&cinfo); 

		Com_Printf("LoadJPG: %s has an invalid image format: %dx%d*4=%d, components: %d", filename, 
			cinfo.output_width, cinfo.output_height, pixelcount * 4, cinfo.output_components);
		return;
	}

	memcount = pixelcount * 4;
	row_stride = cinfo.output_width * cinfo.output_components;

	out = (byte *)Z_Malloc(memcount, TAG_TEMP_WORKSPACE, qfalse);

	*width = cinfo.output_width;
	*height = cinfo.output_height;

	/* Step 6: while (scan lines remain to be read) */
	/*           jpeg_read_scanlines(...); */

	/* Here we use the library's state variable cinfo.output_scanline as the
	* loop counter, so that we don't have to keep track ourselves.
	*/
	while (cinfo.output_scanline < cinfo.output_height) {
		/* jpeg_read_scanlines expects an array of pointers to scanlines.
		* Here the array is only one element long, but you could ask for
		* more than one scanline at a time if that's more convenient.
		*/
		buf = ((out+(row_stride*cinfo.output_scanline)));
		buffer = &buf;
		(void) jpeg_read_scanlines(&cinfo, buffer, 1);
	}

	buf = out;
	// Expand from RGB to RGBA
	sindex = pixelcount * cinfo.output_components;
	dindex = memcount;
 
	do {
		buf[--dindex] = 255; 
		buf[--dindex] = buf[--sindex];
		buf[--dindex] = buf[--sindex];
		buf[--dindex] = buf[--sindex];
	} while(sindex);

	*pic = out;

	/* Step 7: Finish decompression */

	(void) jpeg_finish_decompress(&cinfo);
	/* We can ignore the return value since suspension is not possible
	* with the stdio data source.
	*/

	/* Step 8: Release JPEG decompression object */

	/* This is an important step since it will release a good deal of memory. */
	jpeg_destroy_decompress(&cinfo);

	/* After finish_decompress, we can close the input file.
	* Here we postpone it until after no more JPEG errors are possible,
	* so as to simplify the setjmp error logic above.  (Actually, I don't
	* think that jpeg_destroy can do an error exit, but why assume anything...)
	*/
	ri.FS_FreeFile (fbuffer.v);
	/* At this point you may want to check to see whether any corrupt-data
	* warnings occurred (test whether jerr.pub.num_warnings is nonzero).
	*/

	/* And we're done! */
}


/* Expanded data destination object for stdio output */

typedef struct {
	struct jpeg_destination_mgr pub; /* public fields */

	byte* outfile;		/* target stream */
	int	size;
} my_destination_mgr;

typedef my_destination_mgr * my_dest_ptr;


/*
* Initialize destination --- called by jpeg_start_compress
* before any data is actually written.
*/

static void init_destination (j_compress_ptr cinfo)
{
	my_dest_ptr dest = (my_dest_ptr) cinfo->dest;

	dest->pub.next_output_byte = dest->outfile;
	dest->pub.free_in_buffer = dest->size;
}


/*
* Empty the output buffer --- called whenever buffer fills up.
*
* In typical applications, this should write the entire output buffer
* (ignoring the current state of next_output_byte & free_in_buffer),
* reset the pointer & count to the start of the buffer, and return TRUE
* indicating that the buffer has been dumped.
*
* In applications that need to be able to suspend compression due to output
* overrun, a FALSE return indicates that the buffer cannot be emptied now.
* In this situation, the compressor will return to its caller (possibly with
* an indication that it has not accepted all the supplied scanlines).  The
* application should resume compression after it has made more room in the
* output buffer.  Note that there are substantial restrictions on the use of
* suspension --- see the documentation.
*
* When suspending, the compressor will back up to a convenient restart point
* (typically the start of the current MCU). next_output_byte & free_in_buffer
* indicate where the restart point will be if the current call returns FALSE.
* Data beyond this point will be regenerated after resumption, so do not
* write it out when emptying the buffer externally.
*/

static boolean empty_output_buffer (j_compress_ptr cinfo)
{
	my_dest_ptr dest = (my_dest_ptr) cinfo->dest;

	jpeg_destroy_compress(cinfo);

	// Make crash fatal or we would probably leak memory.
	Com_Error(ERR_FATAL, "Output buffer for encoded JPEG image has insufficient size of %d bytes", dest->size);

	return FALSE;
}

/*
* Terminate destination --- called by jpeg_finish_compress
* after all data has been written.  Usually needs to flush buffer.
*
* NB: *not* called by jpeg_abort or jpeg_destroy; surrounding
* application must deal with any cleanup that should happen even
* for error exit.
*/

static void term_destination(j_compress_ptr cinfo)
{
}


/*
* Prepare for output to a stdio stream.
* The caller must have already opened the stream, and is responsible
* for closing it after finishing compression.
*/

static void jpegDest (j_compress_ptr cinfo, byte* outfile, int size)
{
	my_dest_ptr dest;

	/* The destination object is made permanent so that multiple JPEG images
	* can be written to the same file without re-executing jpeg_stdio_dest.
	* This makes it dangerous to use this manager and a different destination
	* manager serially with the same JPEG object, because their private object
	* sizes may be different.  Caveat programmer.
	*/
	if (cinfo->dest == NULL) {	/* first time for this JPEG object? */
		cinfo->dest = (struct jpeg_destination_mgr *)
			(*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
			sizeof(my_destination_mgr));
	}

	dest = (my_dest_ptr) cinfo->dest;
	dest->pub.init_destination = init_destination;
	dest->pub.empty_output_buffer = empty_output_buffer;
	dest->pub.term_destination = term_destination;
	dest->outfile = outfile;
	dest->size = size;
}

/*
=================
SaveJPGToBuffer

Encodes JPEG from image in image_buffer and writes to buffer.
Expects RGB input data
=================
*/
size_t RE_SaveJPGToBuffer(byte *buffer, size_t bufSize, int quality,
	int image_width, int image_height, byte *image_buffer, int padding)
{
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	JSAMPROW row_pointer[1];	/* pointer to JSAMPLE row[s] */
	my_dest_ptr dest;
	int row_stride;		/* physical row width in image buffer */
	size_t outcount;

	/* Step 1: allocate and initialize JPEG compression object */

	cinfo.err = jpeg_std_error(&jerr);
	cinfo.err->error_exit = R_JPGErrorExit;
	cinfo.err->output_message = R_JPGOutputMessage;
	
	/* Now we can initialize the JPEG compression object. */
	jpeg_create_compress(&cinfo);

	/* Step 2: specify data destination (eg, a file) */
	/* Note: steps 2 and 3 can be done in either order. */

	jpegDest(&cinfo, buffer, bufSize);

	/* Step 3: set parameters for compression */
	cinfo.image_width = image_width; 	/* image width and height, in pixels */
	cinfo.image_height = image_height;
	cinfo.input_components = 3;		/* # of color components per pixel */
	cinfo.in_color_space = JCS_RGB; 	/* colorspace of input image */
	jpeg_set_defaults(&cinfo);
	jpeg_set_quality(&cinfo, quality, TRUE /* limit to baseline-JPEG values */);

	/* If quality is set high, disable chroma subsampling */
	if (quality >= 85) {
		cinfo.comp_info[0].h_samp_factor = 1;
		cinfo.comp_info[0].v_samp_factor = 1;
	}

	/* Step 4: Start compressor */

	jpeg_start_compress(&cinfo, TRUE);

	/* Step 5: while (scan lines remain to be written) */
	/*           jpeg_write_scanlines(...); */

	row_stride = image_width * cinfo.input_components + padding; /* JSAMPLEs per row in image_buffer */

	while (cinfo.next_scanline < cinfo.image_height) {
		/* jpeg_write_scanlines expects an array of pointers to scanlines.
		* Here the array is only one element long, but you could pass
		* more than one scanline at a time if that's more convenient.
		*/
		row_pointer[0] = &image_buffer[((cinfo.image_height-1)*row_stride)-cinfo.next_scanline * row_stride];
		(void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
	}

	/* Step 6: Finish compression */
	jpeg_finish_compress(&cinfo);

	dest = (my_dest_ptr) cinfo.dest;
	outcount = dest->size - dest->pub.free_in_buffer;

	/* Step 7: release JPEG compression object */
	jpeg_destroy_compress(&cinfo);

	/* And we're done! */
	return outcount;
}


void RE_SaveJPG(char * filename, int quality, int image_width, int image_height, byte *image_buffer, int padding)
{
	byte *out;
	size_t bufSize;

	bufSize = image_width * image_height * 3;
	out = (byte *)Hunk_AllocateTempMemory(bufSize);

	bufSize = RE_SaveJPGToBuffer(out, bufSize, quality, image_width, image_height, image_buffer, padding);
	ri.FS_WriteFile(filename, out, bufSize);

	Hunk_FreeTempMemory(out);
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
	unsigned int x, y;
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

void user_read_data( png_structp png_ptr, png_bytep data, png_size_t length );
void png_print_error ( png_structp png_ptr, png_const_charp err )
{
	ri.Printf (PRINT_ERROR, "%s\n", err);
}

void png_print_warning ( png_structp png_ptr, png_const_charp warning )
{
	ri.Printf (PRINT_WARNING, "%s\n", warning);
}

bool IsPowerOfTwo ( int i ) { return (i & (i - 1)) == 0; }

struct PNGFileReader
{
	PNGFileReader ( char *buf ) : buf(buf), offset(0), png_ptr(NULL), info_ptr(NULL) {}
	~PNGFileReader()
	{
		ri.FS_FreeFile (buf);

		if ( info_ptr != NULL )
		{
			// Destroys both structs
			png_destroy_info_struct (png_ptr, &info_ptr);
		}
		else if ( png_ptr != NULL )
		{
			png_destroy_read_struct (&png_ptr, NULL, NULL);
		}
	}

	int Read ( byte **data, unsigned int *width, unsigned int *height )
	{
		// Setup the pointers
		*data = NULL;
		*width = 0;
		*height = 0;

		// Make sure we're actually reading PNG data.
		const int SIGNATURE_LEN = 8;

		byte ident[SIGNATURE_LEN];
		memcpy (ident, buf, SIGNATURE_LEN);

		if ( !png_check_sig (ident, SIGNATURE_LEN) )
		{
			ri.Printf (PRINT_ERROR, "PNG signature not found in given image.");
			return 0;
		}

		png_ptr = png_create_read_struct (PNG_LIBPNG_VER_STRING, NULL, png_print_error, png_print_warning);
		if ( png_ptr == NULL )
		{
			ri.Printf (PRINT_ERROR, "Could not allocate enough memory to load the image.");
			return 0;
		}

		info_ptr = png_create_info_struct (png_ptr);
		if ( setjmp (png_jmpbuf (png_ptr)) )
		{
			return 0;
		}

		// We've read the signature
		offset += SIGNATURE_LEN;

		// Setup reading information, and read header
		png_set_read_fn (png_ptr, (png_voidp)this, &user_read_data);
		png_set_sig_bytes (png_ptr, SIGNATURE_LEN);
		png_read_info (png_ptr, info_ptr);

		unsigned int width_;
		unsigned int height_;
		int depth;
		int colortype;

		png_get_IHDR (png_ptr, info_ptr, &width_, &height_, &depth, &colortype, NULL, NULL, NULL);

		// While modern OpenGL can handle non-PoT textures, it's faster to handle only PoT
		// so that the graphics driver doesn't have to fiddle about with the texture when uploading.
		if ( !IsPowerOfTwo (width_) || !IsPowerOfTwo (height_) )
		{
			ri.Printf (PRINT_ERROR, "Width or height is not a power-of-two.\n");
			return 0;
		}

		// This function is equivalent to using what used to be LoadPNG32. LoadPNG8 also existed,
		// but this only seemed to be used by the RMG system which does not work in JKA. If this
		// does need to be re-implemented, then colortype should be PNG_COLOR_TYPE_PALETTE or
		// PNG_COLOR_TYPE_GRAY.
		if ( colortype != PNG_COLOR_TYPE_RGB && colortype != PNG_COLOR_TYPE_RGBA )
		{
			ri.Printf (PRINT_ERROR, "Image is not 24-bit or 32-bit.");
			return 0;
		}

		// Read the png data
		if ( colortype == PNG_COLOR_TYPE_RGB )
		{
			// Expand RGB -> RGBA
			png_set_add_alpha (png_ptr, 0xff, PNG_FILLER_AFTER);
		}

		png_read_update_info (png_ptr, info_ptr);

		// We always assume there are 4 channels. RGB channels are expanded to RGBA when read.
		byte *tempData = (byte *)ri.Z_Malloc (width_ * height_ * 4, TAG_TEMP_PNG, qfalse, 4);
		if ( !tempData )
		{
			ri.Printf (PRINT_ERROR, "Could not allocate enough memory to load the image.");
			return 0;
		}

		// Dynamic array of row pointers, with 'height' elements, initialized to NULL.
		byte **row_pointers = (byte **)ri.Hunk_AllocateTempMemory (sizeof (byte *) * height_);
		if ( !row_pointers )
		{
			ri.Printf (PRINT_ERROR, "Could not allocate enough memory to load the image.");

			ri.Z_Free (tempData);
			
			return 0;
		}

		// Re-set the jmp so that these new memory allocations can be reclaimed
		if ( setjmp (png_jmpbuf (png_ptr)) )
		{
			ri.Hunk_FreeTempMemory (row_pointers);
			ri.Z_Free (tempData);
			return 0;
		}

		for ( unsigned int i = 0, j = 0; i < height_; i++, j += 4 )
		{
			row_pointers[i] = tempData + j * width_;
		}

		png_read_image (png_ptr, row_pointers);

		// Finish reading
		png_read_end (png_ptr, NULL);

		ri.Hunk_FreeTempMemory (row_pointers);

		// Finally assign all the parameters
		*data = tempData;
		*width = width_;
		*height = height_;

		return 1;
	}

	void ReadBytes ( void *dest, size_t len )
	{
		memcpy (dest, buf + offset, len);
		offset += len;
	}

private:
	char *buf;
	size_t offset;
	png_structp png_ptr;
	png_infop info_ptr;
};

void user_read_data( png_structp png_ptr, png_bytep data, png_size_t length ) {
	png_voidp r = png_get_io_ptr (png_ptr);
	PNGFileReader *reader = (PNGFileReader *)r;
	reader->ReadBytes (data, length);
}

// Loads a PNG image from file.
static int LoadPNG ( const char *filename, byte **data, unsigned int *width, unsigned int *height )
{
	char *buf = NULL;
	int len = ri.FS_ReadFile (filename, (void **)&buf);
	if ( len < 0 || buf == NULL )
	{
		return 0;
	}

	PNGFileReader reader (buf);
	return reader.Read (data, width, height);
}

//===================================================================

/*
=================
R_LoadImage

Loads any of the supported image types into a cannonical
32 bit format.
=================
*/
void R_LoadImage( const char *shortname, byte **pic, int *width, int *height, GLenum *format ) {
	char	name[MAX_QPATH];

	*pic = NULL;
	*width = 0;
	*height = 0;
	*format = GL_RGBA;
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


void R_LoadDataImage( const char *name, byte **pic, int *width, int *height)
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
//	MD_PushTag(TAG_DATA_IMAGE_LOAD);

	strcpy(work, name);

	COM_DefaultExtension( work, sizeof( work ), ".jpg" );
	LoadJPG( work, pic, width, height );

	if (!pic || !*pic)
	{ //both png and jpeg failed, try targa
		strcpy(work, name);
		COM_DefaultExtension( work, sizeof( work ), ".tga" );
		LoadTGA( work, pic, width, height );
	}

	if(*pic)
	{
//		MD_PopTag();
		return;
	}
	// Dataimage loading failed
	Com_Printf("Couldn't read %s -- dataimage load failed\n", name);
//	MD_PopTag();
}

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

/*
===============
R_FindImageFile

Finds or loads the given image.
Returns NULL if it fails, not a default image.
==============
*/
image_t	*R_FindImageFile( const char *name, qboolean mipmap, qboolean allowPicmip, qboolean allowTC, int glWrapClampMode ) {
	image_t	*image;
	int		width, height;
	byte	*pic;
	GLenum	format;

	if (!name || ri.Cvar_VariableIntegerValue( "dedicated" ) )	// stop ghoul2 horribleness as regards image loading from server
	{
		return NULL;
	}

	// need to do this here as well as in R_CreateImage, or R_FindImageFile_NoLoad() may complain about
	//	different clamp parms used...
	//
	if(glConfig.clampToEdgeAvailable && glWrapClampMode == GL_CLAMP) {
		glWrapClampMode = GL_CLAMP_TO_EDGE;
	}

	image = R_FindImageFile_NoLoad(name, mipmap, allowPicmip, allowTC, glWrapClampMode );
	if (image) {
		return image;
	}

	//
	// load the pic from disk
	//
	R_LoadImage( name, &pic, &width, &height, &format );
	if ( pic == NULL ) {                                    // if we dont get a successful load
		return NULL;                                        // bail
	}


	// refuse to find any files not power of 2 dims...
	//
	if ( (width&(width-1)) || (height&(height-1)) )
	{
		Com_Printf ("Refusing to load non-power-2-dims(%d,%d) pic \"%s\"...\n", width,height,name );
		return NULL;
	}

	image = R_CreateImage( ( char * ) name, pic, width, height, format, mipmap, allowPicmip, allowTC, glWrapClampMode );
	Z_Free( pic );
	return image;
}


/*
================
R_CreateDlightImage
================
*/
#define	DLIGHT_SIZE	16
static void R_CreateDlightImage( void ) 
{
	int		width, height;
	byte	*pic;
	GLenum	format;

	R_LoadImage("gfx/2d/dlight", &pic, &width, &height, &format);
	if (pic)
	{                                    
		tr.dlightImage = R_CreateImage("*dlight", pic, width, height, GL_RGBA, qfalse, qfalse, qfalse, GL_CLAMP );
		Z_Free(pic);
	}
	else
	{	// if we dont get a successful load
		int		x,y;
		byte	data[DLIGHT_SIZE][DLIGHT_SIZE][4];
		int		b;

		// make a centered inverse-square falloff blob for dynamic lighting
		for (x=0 ; x<DLIGHT_SIZE ; x++) {
			for (y=0 ; y<DLIGHT_SIZE ; y++) {
				float	d;

				d = ( DLIGHT_SIZE/2 - 0.5f - x ) * ( DLIGHT_SIZE/2 - 0.5f - x ) +
					( DLIGHT_SIZE/2 - 0.5f - y ) * ( DLIGHT_SIZE/2 - 0.5f - y );
				b = 4000 / d;
				if (b > 255) {
					b = 255;
				} else if ( b < 75 ) {
					b = 0;
				}
				data[y][x][0] = 
					data[y][x][1] = 
					data[y][x][2] = b;
				data[y][x][3] = 255;			
			}
		}
		tr.dlightImage = R_CreateImage("*dlight", (byte *)data, DLIGHT_SIZE, DLIGHT_SIZE, GL_RGBA, qfalse, qfalse, qfalse, GL_CLAMP );
	}
}


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

	d = tr.fogTable[ (int)(s * (FOG_TABLE_SIZE-1)) ];

	return d;
}

/*
================
R_CreateFogImage
================
*/
#define	FOG_S	256
#define	FOG_T	32
static void R_CreateFogImage( void ) {
	int		x,y;
	byte	*data;
	float	g;
	float	d;
	float	borderColor[4];

	data = (unsigned char *)Hunk_AllocateTempMemory( FOG_S * FOG_T * 4 );

	g = 2.0;

	// S is distance, T is depth
	for (x=0 ; x<FOG_S ; x++) {
		for (y=0 ; y<FOG_T ; y++) {
			d = R_FogFactor( ( x + 0.5f ) / FOG_S, ( y + 0.5f ) / FOG_T );

			data[(y*FOG_S+x)*4+0] = 
			data[(y*FOG_S+x)*4+1] = 
			data[(y*FOG_S+x)*4+2] = 255;
			data[(y*FOG_S+x)*4+3] = 255*d;
		}
	}
	// standard openGL clamping doesn't really do what we want -- it includes
	// the border color at the edges.  OpenGL 1.2 has clamp-to-edge, which does
	// what we want.
	tr.fogImage = R_CreateImage("*fog", (byte *)data, FOG_S, FOG_T, GL_RGBA, qfalse, qfalse, qfalse, GL_CLAMP );
	Hunk_FreeTempMemory( data );

	borderColor[0] = 1.0;
	borderColor[1] = 1.0;
	borderColor[2] = 1.0;
	borderColor[3] = 1;

	qglTexParameterfv( GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor );
}

/*
==================
R_CreateDefaultImage
==================
*/
#define	DEFAULT_SIZE	16
static void R_CreateDefaultImage( void ) {
	int		x;
	byte	data[DEFAULT_SIZE][DEFAULT_SIZE][4];

	// the default image will be a box, to allow you to see the mapping coordinates
	memset( data, 32, sizeof( data ) );
	for ( x = 0 ; x < DEFAULT_SIZE ; x++ ) {
		data[0][x][0] =
		data[0][x][1] =
		data[0][x][2] =
		data[0][x][3] = 255;

		data[x][0][0] =
		data[x][0][1] =
		data[x][0][2] =
		data[x][0][3] = 255;

		data[DEFAULT_SIZE-1][x][0] =
		data[DEFAULT_SIZE-1][x][1] =
		data[DEFAULT_SIZE-1][x][2] =
		data[DEFAULT_SIZE-1][x][3] = 255;

		data[x][DEFAULT_SIZE-1][0] =
		data[x][DEFAULT_SIZE-1][1] =
		data[x][DEFAULT_SIZE-1][2] =
		data[x][DEFAULT_SIZE-1][3] = 255;
	}
	tr.defaultImage = R_CreateImage("*default", (byte *)data, DEFAULT_SIZE, DEFAULT_SIZE, GL_RGBA, qtrue, qfalse, qfalse, GL_REPEAT );
}

/*
==================
R_CreateBuiltinImages
==================
*/
void R_CreateBuiltinImages( void ) {
	int		x,y;
	byte	data[DEFAULT_SIZE][DEFAULT_SIZE][4];

	R_CreateDefaultImage();

	// we use a solid white image instead of disabling texturing
	memset( data, 255, sizeof( data ) );
	tr.whiteImage = R_CreateImage("*white", (byte *)data, 8, 8, GL_RGBA, qfalse, qfalse, qfalse, GL_REPEAT);

	tr.screenImage = R_CreateImage("*screen", (byte *)data, 8, 8, GL_RGBA, qfalse, qfalse, qfalse, GL_REPEAT );

	// Create the scene glow image. - AReis
	tr.screenGlow = 1024 + giTextureBindNum++;
	qglDisable( GL_TEXTURE_2D );
	qglEnable( GL_TEXTURE_RECTANGLE_EXT );
	qglBindTexture( GL_TEXTURE_RECTANGLE_EXT, tr.screenGlow );
	qglTexImage2D( GL_TEXTURE_RECTANGLE_EXT, 0, GL_RGBA16, glConfig.vidWidth, glConfig.vidHeight, 0, GL_RGB, GL_FLOAT, 0 );
	qglTexParameteri( GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	qglTexParameteri( GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	qglTexParameteri( GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_WRAP_S, GL_CLAMP );
	qglTexParameteri( GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_WRAP_T, GL_CLAMP );

	// Create the scene image. - AReis
	tr.sceneImage = 1024 + giTextureBindNum++;
	qglBindTexture( GL_TEXTURE_RECTANGLE_EXT, tr.sceneImage );
	qglTexImage2D( GL_TEXTURE_RECTANGLE_EXT, 0, GL_RGBA16, glConfig.vidWidth, glConfig.vidHeight, 0, GL_RGB, GL_FLOAT, 0 );
	qglTexParameteri( GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	qglTexParameteri( GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	qglTexParameteri( GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_WRAP_S, GL_CLAMP );
	qglTexParameteri( GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_WRAP_T, GL_CLAMP );

	// Create the minimized scene blur image.
	if ( r_DynamicGlowWidth->integer > glConfig.vidWidth  )
	{
		r_DynamicGlowWidth->integer = glConfig.vidWidth;
	}
	if ( r_DynamicGlowHeight->integer > glConfig.vidHeight  )
	{
		r_DynamicGlowHeight->integer = glConfig.vidHeight;
	}
	tr.blurImage = 1024 + giTextureBindNum++;
	qglBindTexture( GL_TEXTURE_RECTANGLE_EXT, tr.blurImage );
	qglTexImage2D( GL_TEXTURE_RECTANGLE_EXT, 0, GL_RGBA16, r_DynamicGlowWidth->integer, r_DynamicGlowHeight->integer, 0, GL_RGB, GL_FLOAT, 0 );
	qglTexParameteri( GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	qglTexParameteri( GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	qglTexParameteri( GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_WRAP_S, GL_CLAMP );
	qglTexParameteri( GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_WRAP_T, GL_CLAMP );
	qglDisable( GL_TEXTURE_RECTANGLE_EXT );
	qglEnable( GL_TEXTURE_2D );


	// with overbright bits active, we need an image which is some fraction of full color,
	// for default lightmaps, etc
	for (x=0 ; x<DEFAULT_SIZE ; x++) {
		for (y=0 ; y<DEFAULT_SIZE ; y++) {
			data[y][x][0] = 
			data[y][x][1] = 
			data[y][x][2] = tr.identityLightByte;
			data[y][x][3] = 255;			
		}
	}

	tr.identityLightImage = R_CreateImage("*identityLight", (byte *)data, 8, 8, GL_RGBA, qfalse, qfalse, qfalse, GL_REPEAT);

	for(x=0;x<NUM_SCRATCH_IMAGES;x++) {
		// scratchimage is usually used for cinematic drawing
		tr.scratchImage[x] = R_CreateImage(va("*scratch%d",x), (byte *)data, DEFAULT_SIZE, DEFAULT_SIZE, GL_RGBA, qfalse, qtrue, qfalse, GL_CLAMP);
	}

	R_CreateDlightImage();

	R_CreateFogImage();
}


/*
===============
R_SetColorMappings
===============
*/
void R_SetColorMappings( void ) {
	int		i, j;
	float	g;
	int		inf;
	int		shift;

	// setup the overbright lighting
	tr.overbrightBits = r_overBrightBits->integer;
	if ( !glConfig.deviceSupportsGamma ) {
		tr.overbrightBits = 0;		// need hardware gamma for overbright
	}

	// never overbright in windowed mode
	if ( !glConfig.isFullscreen ) 
	{
		tr.overbrightBits = 0;
	}

	if ( tr.overbrightBits > 1 ) {
		tr.overbrightBits = 1;
	}

	if ( tr.overbrightBits < 0 ) {
		tr.overbrightBits = 0;
	}

	tr.identityLight = 1.0f / ( 1 << tr.overbrightBits );
	tr.identityLightByte = 255 * tr.identityLight;


	if ( r_intensity->value < 1.0f ) {
		ri.Cvar_Set( "r_intensity", "1" );
	}

	if ( r_gamma->value < 0.5f ) {
		ri.Cvar_Set( "r_gamma", "0.5" );
	} else if ( r_gamma->value > 3.0f ) {
		ri.Cvar_Set( "r_gamma", "3.0" );
	}

	g = r_gamma->value;

	shift = tr.overbrightBits;

	for ( i = 0; i < 256; i++ ) {
		if ( g == 1 ) {
			inf = i;
		} else {
			inf = 255 * pow ( i/255.0f, 1.0f / g ) + 0.5f;
		}
		inf <<= shift;
		if (inf < 0) {
			inf = 0;
		}
		if (inf > 255) {
			inf = 255;
		}
		s_gammatable[i] = inf;
	}

	for (i=0 ; i<256 ; i++) {
		j = i * r_intensity->value;
		if (j > 255) {
			j = 255;
		}
		s_intensitytable[i] = j;
	}

	if ( glConfig.deviceSupportsGamma )
	{
		GLimp_SetGamma( s_gammatable, s_gammatable, s_gammatable );
	}
}

/*
===============
R_InitImages
===============
*/
void	R_InitImages( void ) {
	//memset(hashTable, 0, sizeof(hashTable));	// DO NOT DO THIS NOW (because of image cacheing)	-ste.
	// build brightness translation tables
	R_SetColorMappings();

	// create default texture and white texture
	R_CreateBuiltinImages();
}

/*
===============
R_DeleteTextures
===============
*/
// (only gets called during vid_restart now (and app exit), not during map load)
//
void R_DeleteTextures( void ) {

	R_Images_Clear();	
	GL_ResetBinds();
}

/*
============================================================================

SKINS

============================================================================
*/

static char *CommaParse( char **data_p );
//can't be dec'd here since we need it for non-dedicated builds now as well.

/*
===============
RE_RegisterSkin

===============
*/

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
		if ( (unsigned)skin->numSurfaces >= ARRAY_LEN( skin->surfaces ) )
		{
			assert( ARRAY_LEN( skin->surfaces ) > (unsigned)skin->numSurfaces );
			Com_Printf( "WARNING: RE_RegisterSkin( '%s' ) more than %d surfaces!\n", name, ARRAY_LEN( skin->surfaces ) );
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

/*
===============
R_SkinList_f
===============
*/
void	R_SkinList_f( void ) {
	int			i, j;
	skin_t		*skin;

	Com_Printf ( "------------------\n");

	for ( i = 0 ; i < tr.numSkins ; i++ ) {
		skin = tr.skins[i];

		Com_Printf ("%3i:%s\n", i, skin->name );
		for ( j = 0 ; j < skin->numSurfaces ; j++ ) {
			Com_Printf ("       %s = %s\n", 
				skin->surfaces[j]->name, ((shader_t* )skin->surfaces[j]->shader)->name );
		}
	}
	Com_Printf ( "------------------\n");
}
