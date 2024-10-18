/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// tr_image.c
#include "tr_local.h"
#include "glext.h"

static byte			 s_intensitytable[256];
static unsigned char s_gammatable[256];

int		gl_filter_min = GL_LINEAR_MIPMAP_NEAREST;
int		gl_filter_max = GL_LINEAR;

#define FILE_HASH_SIZE 1553 // Prime numbers are a good size for hash tables :)
#define NUM_IMAGES_PER_POOL_ALLOC 512

static struct ImagesPool
{
	image_t *pPool;
	ImagesPool *pNext;
} *imagesPool;

static image_t *hashTable[FILE_HASH_SIZE];

/*
Extends the size of the images pool allocator
*/
static void R_ExtendImagesPool()
{
	ImagesPool *pool = (ImagesPool *)Z_Malloc(sizeof(*pool), TAG_GENERAL);
	image_t *freeImages = (image_t *)Z_Malloc(sizeof(*freeImages) * NUM_IMAGES_PER_POOL_ALLOC, TAG_IMAGE_T);

	for ( int i = 0; i < (NUM_IMAGES_PER_POOL_ALLOC - 1); i++ )
	{
		freeImages[i].poolNext = &freeImages[i + 1];
	}
	freeImages[NUM_IMAGES_PER_POOL_ALLOC - 1].poolNext = tr.imagesFreeList;

	pool->pPool = freeImages;
	pool->pNext = imagesPool;
	imagesPool = pool;

	tr.imagesFreeList = freeImages;
}

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

/*
================
return a hash value for the filename
================
*/
static long generateHashValue( const char *fname ) {
	int		i;
	long	hash;
	char	letter;

	hash = 0;
	i = 0;
	while (fname[i] != '\0') {
		letter = tolower(fname[i]);
		if (letter =='.') break;				// don't include extension
		if (letter =='\\') letter = '/';		// damn path names
		hash+=(long)(letter)*(i+119);
		i++;
	}

	return hash % FILE_HASH_SIZE;
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
		ri.Printf (PRINT_ALL, "bad filter name\n");
		return;
	}

	gl_filter_min = modes[i].minimize;
	gl_filter_max = modes[i].maximize;

	if ( r_ext_texture_filter_anisotropic->value > glConfig.maxTextureFilterAnisotropy )
	{
		ri.Cvar_SetValue ("r_ext_texture_filter_anisotropic", glConfig.maxTextureFilterAnisotropy);
	}

	// change all the existing mipmap texture objects
	glt = tr.images;
	for ( i = 0 ; i < tr.numImages ; i++, glt = glt->poolNext ) {
		if ( glt->flags & IMGFLAG_MIPMAP ) {
			GL_Bind (glt);
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);

			if ( r_ext_texture_filter_anisotropic->value > 0.0f )
			{
				if ( glConfig.maxTextureFilterAnisotropy > 1.0f )
				{
					qglTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, r_ext_texture_filter_anisotropic->value);
				}
				else
				{
					qglTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.0f);
				}
			}
		}
	}
}

/*
===============
R_SumOfUsedImages
===============
*/
int R_SumOfUsedImages( void ) {
	int	total;
	int i;
	image_t *image = tr.images;

	total = 0;
	for ( i = 0; i < tr.numImages; i++, image = image->poolNext ) {
		if ( image->frameUsed == tr.frameCount ) {
			total += image->uploadWidth * image->uploadHeight;
		}
	}

	return total;
}

static float GetReadableSize( int bytes, const char **units )
{
	float result = bytes;
	*units = "b ";

	if (result >= 1024.0f)
	{
		result /= 1024.0f;
		*units = "kb";
	}

	if (result >= 1024.0f)
	{
		result /= 1024.0f;
		*units = "Mb";
	}

	if (result >= 1024.0f)
	{
		result /= 1024.0f;
		*units = "Gb";
	}

	return result;
}

/*
===============
R_ImageList_f
===============
*/
void R_ImageList_f( void ) {
	int i;
	int estTotalSize = 0;
	const char *sizeSuffix;
	image_t *image = tr.images;

	ri.Printf(PRINT_ALL, "\n      -w-- -h-- type  -size- --name-------\n");

	for ( i = 0 ; i < tr.numImages ; i++, image = image->poolNext )
	{
		const char *format = "???? ";
		int estSize;

		estSize = image->uploadHeight * image->uploadWidth;

		switch(image->internalFormat)
		{
			case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT:
				format = "sDXT1";
				// 64 bits per 16 pixels, so 4 bits per pixel
				estSize /= 2;
				break;
			case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:
				format = "sDXT5";
				// 128 bits per 16 pixels, so 1 byte per pixel
				break;
			case GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB:
				format = "sBPTC";
				// 128 bits per 16 pixels, so 1 byte per pixel
				break;
			case GL_COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT:
				format = "LATC ";
				// 128 bits per 16 pixels, so 1 byte per pixel
				break;
			case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
				format = "DXT1 ";
				// 64 bits per 16 pixels, so 4 bits per pixel
				estSize /= 2;
				break;
			case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
				format = "DXT5 ";
				// 128 bits per 16 pixels, so 1 byte per pixel
				break;
			case GL_COMPRESSED_RGBA_BPTC_UNORM_ARB:
				format = "BPTC ";
				// 128 bits per 16 pixels, so 1 byte per pixel
				break;
			case GL_RGB4_S3TC:
				format = "S3TC ";
				// same as DXT1?
				estSize /= 2;
				break;
			case GL_RGBA4:
			case GL_RGBA8:
			case GL_RGBA:
				format = "RGBA ";
				// 4 bytes per pixel
				estSize *= 4;
				break;
			case GL_RGB5:
			case GL_RGB8:
			case GL_RGB:
				format = "RGB  ";
				// 3 bytes per pixel?
				estSize *= 3;
				break;
			case GL_SRGB:
			case GL_SRGB8:
				format = "sRGB ";
				// 3 bytes per pixel?
				estSize *= 3;
				break;
			case GL_SRGB_ALPHA:
			case GL_SRGB8_ALPHA8:
				format = "sRGBA";
				// 4 bytes per pixel?
				estSize *= 4;
				break;
			case GL_DEPTH_COMPONENT24:
				format = "D24  ";
				break;
		}

		// mipmap adds about 50%
		if (image->flags & IMGFLAG_MIPMAP)
			estSize += estSize / 2;

		float printSize = GetReadableSize(estSize, &sizeSuffix);

		ri.Printf(PRINT_ALL, "%4i: %4ix%4i %s %7.2f%s %s\n", i, image->uploadWidth, image->uploadHeight, format, printSize, sizeSuffix, image->imgName);
		estTotalSize += estSize;
	}

	float printSize = GetReadableSize(estTotalSize, &sizeSuffix);

	ri.Printf (PRINT_ALL, " ---------\n");
	ri.Printf (PRINT_ALL, " approx %i bytes (%.2f%s)\n", estTotalSize, printSize, sizeSuffix);
	ri.Printf (PRINT_ALL, " %i total images\n\n", tr.numImages );
}

//=======================================================================

/*
================
ResampleTexture

Used to resample images in a more general than quartering fashion.

This will only be filtered properly if the resampled size
is greater than half the original size.

If a larger shrinking is needed, use the mipmap function
before or after.
================
*/
static void ResampleTexture( byte *in, int inwidth, int inheight, byte *out,
							int outwidth, int outheight ) {
	int		i, j;
	byte	*inrow, *inrow2;
	int		frac, fracstep;
	int		p1[2048], p2[2048];
	byte	*pix1, *pix2, *pix3, *pix4;

	if (outwidth>2048)
		ri.Error(ERR_DROP, "ResampleTexture: max width");

	fracstep = inwidth*0x10000/outwidth;

	frac = fracstep>>2;
	for ( i=0 ; i<outwidth ; i++ ) {
		p1[i] = 4*(frac>>16);
		frac += fracstep;
	}
	frac = 3*(fracstep>>2);
	for ( i=0 ; i<outwidth ; i++ ) {
		p2[i] = 4*(frac>>16);
		frac += fracstep;
	}

	for (i=0 ; i<outheight ; i++) {
		inrow = in + 4*inwidth*(int)((i+0.25)*inheight/outheight);
		inrow2 = in + 4*inwidth*(int)((i+0.75)*inheight/outheight);
		for (j=0 ; j<outwidth ; j++) {
			pix1 = inrow + p1[j];
			pix2 = inrow + p2[j];
			pix3 = inrow2 + p1[j];
			pix4 = inrow2 + p2[j];
			*out++ = (pix1[0] + pix2[0] + pix3[0] + pix4[0])>>2;
			*out++ = (pix1[1] + pix2[1] + pix3[1] + pix4[1])>>2;
			*out++ = (pix1[2] + pix2[2] + pix3[2] + pix4[2])>>2;
			*out++ = (pix1[3] + pix2[3] + pix3[3] + pix4[3])>>2;
		}
	}
}

static void RGBAtoYCoCgA(const byte *in, byte *out, int width, int height)
{
	int x, y;

	for (y = 0; y < height; y++)
	{
		const byte *inbyte  = in  + y * width * 4;
		byte       *outbyte = out + y * width * 4;

		for (x = 0; x < width; x++)
		{
			byte r, g, b, a, rb2;

			r = *inbyte++;
			g = *inbyte++;
			b = *inbyte++;
			a = *inbyte++;
			rb2 = (r + b) >> 1;

			*outbyte++ = (g + rb2) >> 1;       // Y  =  R/4 + G/2 + B/4
			*outbyte++ = (r - b + 256) >> 1;   // Co =  R/2       - B/2
			*outbyte++ = (g - rb2 + 256) >> 1; // Cg = -R/4 + G/2 - B/4
			*outbyte++ = a;
		}
	}
}

static void YCoCgAtoRGBA(const byte *in, byte *out, int width, int height)
{
	int x, y;

	for (y = 0; y < height; y++)
	{
		const byte *inbyte  = in  + y * width * 4;
		byte       *outbyte = out + y * width * 4;

		for (x = 0; x < width; x++)
		{
			byte _Y, Co, Cg, a;

			_Y = *inbyte++;
			Co = *inbyte++;
			Cg = *inbyte++;
			a  = *inbyte++;

			*outbyte++ = CLAMP(_Y + Co - Cg,       0, 255); // R = Y + Co - Cg
			*outbyte++ = CLAMP(_Y      + Cg - 128, 0, 255); // G = Y + Cg
			*outbyte++ = CLAMP(_Y - Co - Cg + 256, 0, 255); // B = Y - Co - Cg
			*outbyte++ = a;
		}
	}
}


// uses a sobel filter to change a texture to a normal map
static void RGBAtoNormal(const byte *in, byte *out, int width, int height, qboolean clampToEdge)
{
	int x, y, max;

	// convert to heightmap, storing in alpha
	// same as converting to Y in YCoCg
	max = 1;
	for (y = 0; y < height; y++)
	{
		const byte *inbyte  = in  + y * width * 4;
		byte       *outbyte = out + y * width * 4 + 3;

		for (x = 0; x < width; x++)
		{
			byte result = (inbyte[0] >> 2) + (inbyte[1] >> 1) + (inbyte[2] >> 2);
			result = result * result / 255; // Make linear
			*outbyte = result;
			max = MAX(max, *outbyte);
			outbyte += 4;
			inbyte  += 4;
		}
	}

	// level out heights
	if (max < 255)
	{
		for (y = 0; y < height; y++)
		{
			byte *outbyte = out + y * width * 4 + 3;

			for (x = 0; x < width; x++)
			{
				*outbyte = *outbyte + (255 - max);
				outbyte += 4;
			}
		}
	}


	// now run sobel filter over height values to generate X and Y
	// then normalize
	for (y = 0; y < height; y++)
	{
		byte *outbyte = out + y * width * 4;

		for (x = 0; x < width; x++)
		{
			// 0 1 2
			// 3 4 5
			// 6 7 8

			byte s[9];
			int x2, y2, i;
			vec3_t normal;

			i = 0;
			for (y2 = -1; y2 <= 1; y2++)
			{
				int src_y = y + y2;

				if (clampToEdge)
				{
					src_y = CLAMP(src_y, 0, height - 1);
				}
				else
				{
					src_y = (src_y + height) % height;
				}


				for (x2 = -1; x2 <= 1; x2++)
				{
					int src_x = x + x2;

					if (clampToEdge)
					{
						src_x = CLAMP(src_x, 0, width - 1);
					}
					else
					{
						src_x = (src_x + width) % width;
					}

					s[i++] = *(out + (src_y * width + src_x) * 4 + 3);
				}
			}

			normal[0] =        s[0]            -     s[2]
						 + 2 * s[3]            - 2 * s[5]
						 +     s[6]            -     s[8];

			normal[1] =        s[0] + 2 * s[1] +     s[2]

						 -     s[6] - 2 * s[7] -     s[8];

			normal[2] = s[4] * 4;

			if (!VectorNormalize2(normal, normal))
			{
				VectorSet(normal, 0, 0, 1);
			}

			*outbyte++ = FloatToOffsetByte(normal[0]);
			*outbyte++ = FloatToOffsetByte(normal[1]);
			*outbyte++ = FloatToOffsetByte(normal[2]);
			outbyte++;
		}
	}
}

#define COPYSAMPLE(a,b) *(unsigned int *)(a) = *(unsigned int *)(b)

// based on Fast Curve Based Interpolation
// from Fast Artifacts-Free Image Interpolation (http://www.andreagiachetti.it/icbi/)
// assumes data has a 2 pixel thick border of clamped or wrapped data
// expects data to be a grid with even (0, 0), (2, 0), (0, 2), (2, 2) etc pixels filled
// only performs FCBI on specified component
static void DoFCBI(byte *in, byte *out, int width, int height, int component)
{
	int x, y;
	byte *outbyte, *inbyte;

	// copy in to out
	for (y = 2; y < height - 2; y += 2)
	{
		inbyte  = in  + (y * width + 2) * 4 + component;
		outbyte = out + (y * width + 2) * 4 + component;

		for (x = 2; x < width - 2; x += 2)
		{
			*outbyte = *inbyte;
			outbyte += 8;
			inbyte += 8;
		}
	}

	for (y = 3; y < height - 3; y += 2)
	{
		// diagonals
		//
		// NWp  - northwest interpolated pixel
		// NEp  - northeast interpolated pixel
		// NWd  - northwest first derivative
		// NEd  - northeast first derivative
		// NWdd - northwest second derivative
		// NEdd - northeast second derivative
		//
		// Uses these samples:
		//
		//         0
		//   - - a - b - -
		//   - - - - - - -
		//   c - d - e - f
		// 0 - - - - - - -
		//   g - h - i - j
		//   - - - - - - -
		//   - - k - l - -
		//
		// x+2 uses these samples:
		//
		//         0
		//   - - - - a - b - -
		//   - - - - - - - - -
		//   - - c - d - e - f
		// 0 - - - - - - - - -
		//   - - g - h - i - j
		//   - - - - - - - - -
		//   - - - - k - l - -
		//
		// so we can reuse 8 of them on next iteration
		//
		// a=b, c=d, d=e, e=f, g=h, h=i, i=j, k=l
		//
		// only b, f, j, and l need to be sampled on next iteration

		byte sa, sb, sc, sd, se, sf, sg, sh, si, sj, sk, sl;
		byte *line1, *line2, *line3, *line4;

		x = 3;

		// optimization one
		//                       SAMPLE2(sa, x-1, y-3);
		//SAMPLE2(sc, x-3, y-1); SAMPLE2(sd, x-1, y-1); SAMPLE2(se, x+1, y-1);
		//SAMPLE2(sg, x-3, y+1); SAMPLE2(sh, x-1, y+1); SAMPLE2(si, x+1, y+1);
		//                       SAMPLE2(sk, x-1, y+3);

		// optimization two
		line1 = in + ((y - 3) * width + (x - 1)) * 4 + component;
		line2 = in + ((y - 1) * width + (x - 3)) * 4 + component;
		line3 = in + ((y + 1) * width + (x - 3)) * 4 + component;
		line4 = in + ((y + 3) * width + (x - 1)) * 4 + component;

		//                                   COPYSAMPLE(sa, line1); line1 += 8;
		//COPYSAMPLE(sc, line2); line2 += 8; COPYSAMPLE(sd, line2); line2 += 8; COPYSAMPLE(se, line2); line2 += 8;
		//COPYSAMPLE(sg, line3); line3 += 8; COPYSAMPLE(sh, line3); line3 += 8; COPYSAMPLE(si, line3); line3 += 8;
		//                                   COPYSAMPLE(sk, line4); line4 += 8;

		                         sa = *line1; line1 += 8;
		sc = *line2; line2 += 8; sd = *line2; line2 += 8; se = *line2; line2 += 8;
		sg = *line3; line3 += 8; sh = *line3; line3 += 8; si = *line3; line3 += 8;
		                         sk = *line4; line4 += 8;

		outbyte = out + (y * width + x) * 4 + component;

		for ( ; x < width - 3; x += 2)
		{
			int NWd, NEd, NWp, NEp;

			// original
			//                       SAMPLE2(sa, x-1, y-3); SAMPLE2(sb, x+1, y-3);
			//SAMPLE2(sc, x-3, y-1); SAMPLE2(sd, x-1, y-1); SAMPLE2(se, x+1, y-1); SAMPLE2(sf, x+3, y-1);
			//SAMPLE2(sg, x-3, y+1); SAMPLE2(sh, x-1, y+1); SAMPLE2(si, x+1, y+1); SAMPLE2(sj, x+3, y+1);
			//                       SAMPLE2(sk, x-1, y+3); SAMPLE2(sl, x+1, y+3);

			// optimization one
			//SAMPLE2(sb, x+1, y-3);
			//SAMPLE2(sf, x+3, y-1);
			//SAMPLE2(sj, x+3, y+1);
			//SAMPLE2(sl, x+1, y+3);

			// optimization two
			//COPYSAMPLE(sb, line1); line1 += 8;
			//COPYSAMPLE(sf, line2); line2 += 8;
			//COPYSAMPLE(sj, line3); line3 += 8;
			//COPYSAMPLE(sl, line4); line4 += 8;

			sb = *line1; line1 += 8;
			sf = *line2; line2 += 8;
			sj = *line3; line3 += 8;
			sl = *line4; line4 += 8;

			NWp = sd + si;
			NEp = se + sh;
			NWd = abs(sd - si);
			NEd = abs(se - sh);

			if (NWd > 100 || NEd > 100 || abs(NWp-NEp) > 200)
			{
				if (NWd < NEd)
					*outbyte = NWp >> 1;
				else
					*outbyte = NEp >> 1;
			}
			else
			{
				int NWdd, NEdd;

				//NEdd = abs(sg + sd + sb - 3 * (se + sh) + sk + si + sf);
				//NWdd = abs(sa + se + sj - 3 * (sd + si) + sc + sh + sl);
				NEdd = abs(sg + sb - 3 * NEp + sk + sf + NWp);
				NWdd = abs(sa + sj - 3 * NWp + sc + sl + NEp);

				if (NWdd > NEdd)
					*outbyte = NWp >> 1;
				else
					*outbyte = NEp >> 1;
			}

			outbyte += 8;

			//                    COPYSAMPLE(sa, sb);
			//COPYSAMPLE(sc, sd); COPYSAMPLE(sd, se); COPYSAMPLE(se, sf);
			//COPYSAMPLE(sg, sh); COPYSAMPLE(sh, si); COPYSAMPLE(si, sj);
			//                    COPYSAMPLE(sk, sl);

			         sa = sb;
			sc = sd; sd = se; se = sf;
			sg = sh; sh = si; si = sj;
			         sk = sl;
		}
	}

	// hack: copy out to in again
	for (y = 3; y < height - 3; y += 2)
	{
		inbyte = out + (y * width + 3) * 4 + component;
		outbyte = in + (y * width + 3) * 4 + component;

		for (x = 3; x < width - 3; x += 2)
		{
			*outbyte = *inbyte;
			outbyte += 8;
			inbyte += 8;
		}
	}

	for (y = 2; y < height - 3; y++)
	{
		// horizontal & vertical
		//
		// hp  - horizontally interpolated pixel
		// vp  - vertically interpolated pixel
		// hd  - horizontal first derivative
		// vd  - vertical first derivative
		// hdd - horizontal second derivative
		// vdd - vertical second derivative
		// Uses these samples:
		//
		//       0
		//   - a - b -
		//   c - d - e
		// 0 - f - g -
		//   h - i - j
		//   - k - l -
		//
		// x+2 uses these samples:
		//
		//       0
		//   - - - a - b -
		//   - - c - d - e
		// 0 - - - f - g -
		//   - - h - i - j
		//   - - - k - l -
		//
		// so we can reuse 7 of them on next iteration
		//
		// a=b, c=d, d=e, f=g, h=i, i=j, k=l
		//
		// only b, e, g, j, and l need to be sampled on next iteration

		byte sa, sb, sc, sd, se, sf, sg, sh, si, sj, sk, sl;
		byte *line1, *line2, *line3, *line4, *line5;

		//x = (y + 1) % 2;
		x = (y + 1) % 2 + 2;

		// optimization one
		//            SAMPLE2(sa, x-1, y-2);
		//SAMPLE2(sc, x-2, y-1); SAMPLE2(sd, x,   y-1);
		//            SAMPLE2(sf, x-1, y  );
		//SAMPLE2(sh, x-2, y+1); SAMPLE2(si, x,   y+1);
		//            SAMPLE2(sk, x-1, y+2);

		line1 = in + ((y - 2) * width + (x - 1)) * 4 + component;
		line2 = in + ((y - 1) * width + (x - 2)) * 4 + component;
		line3 = in + ((y    ) * width + (x - 1)) * 4 + component;
		line4 = in + ((y + 1) * width + (x - 2)) * 4 + component;
		line5 = in + ((y + 2) * width + (x - 1)) * 4 + component;

		//                 COPYSAMPLE(sa, line1); line1 += 8;
		//COPYSAMPLE(sc, line2); line2 += 8; COPYSAMPLE(sd, line2); line2 += 8;
		//                 COPYSAMPLE(sf, line3); line3 += 8;
		//COPYSAMPLE(sh, line4); line4 += 8; COPYSAMPLE(si, line4); line4 += 8;
        //                 COPYSAMPLE(sk, line5); line5 += 8;

		             sa = *line1; line1 += 8;
		sc = *line2; line2 += 8; sd = *line2; line2 += 8;
		             sf = *line3; line3 += 8;
		sh = *line4; line4 += 8; si = *line4; line4 += 8;
		             sk = *line5; line5 += 8;

		outbyte = out + (y * width + x) * 4 + component;

		for ( ; x < width - 3; x+=2)
		{
			int hd, vd, hp, vp;

			//            SAMPLE2(sa, x-1, y-2); SAMPLE2(sb, x+1, y-2);
			//SAMPLE2(sc, x-2, y-1); SAMPLE2(sd, x,   y-1); SAMPLE2(se, x+2, y-1);
			//            SAMPLE2(sf, x-1, y  ); SAMPLE2(sg, x+1, y  );
			//SAMPLE2(sh, x-2, y+1); SAMPLE2(si, x,   y+1); SAMPLE2(sj, x+2, y+1);
			//            SAMPLE2(sk, x-1, y+2); SAMPLE2(sl, x+1, y+2);

			// optimization one
			//SAMPLE2(sb, x+1, y-2);
			//SAMPLE2(se, x+2, y-1);
			//SAMPLE2(sg, x+1, y  );
			//SAMPLE2(sj, x+2, y+1);
			//SAMPLE2(sl, x+1, y+2);

			//COPYSAMPLE(sb, line1); line1 += 8;
			//COPYSAMPLE(se, line2); line2 += 8;
			//COPYSAMPLE(sg, line3); line3 += 8;
			//COPYSAMPLE(sj, line4); line4 += 8;
			//COPYSAMPLE(sl, line5); line5 += 8;

			sb = *line1; line1 += 8;
			se = *line2; line2 += 8;
			sg = *line3; line3 += 8;
			sj = *line4; line4 += 8;
			sl = *line5; line5 += 8;

			hp = sf + sg;
			vp = sd + si;
			hd = abs(sf - sg);
			vd = abs(sd - si);

			if (hd > 100 || vd > 100 || abs(hp-vp) > 200)
			{
				if (hd < vd)
					*outbyte = hp >> 1;
				else
					*outbyte = vp >> 1;
			}
			else
			{
				int hdd, vdd;

				//hdd = abs(sc[i] + sd[i] + se[i] - 3 * (sf[i] + sg[i]) + sh[i] + si[i] + sj[i]);
				//vdd = abs(sa[i] + sf[i] + sk[i] - 3 * (sd[i] + si[i]) + sb[i] + sg[i] + sl[i]);

				hdd = abs(sc + se - 3 * hp + sh + sj + vp);
				vdd = abs(sa + sk - 3 * vp + sb + sl + hp);

				if (hdd > vdd)
					*outbyte = hp >> 1;
				else
					*outbyte = vp >> 1;
			}

			outbyte += 8;

			//          COPYSAMPLE(sa, sb);
			//COPYSAMPLE(sc, sd); COPYSAMPLE(sd, se);
			//          COPYSAMPLE(sf, sg);
			//COPYSAMPLE(sh, si); COPYSAMPLE(si, sj);
			//          COPYSAMPLE(sk, sl);
			    sa = sb;
			sc = sd; sd = se;
			    sf = sg;
			sh = si; si = sj;
			    sk = sl;
		}
	}
}

// Similar to FCBI, but throws out the second order derivatives for speed
static void DoFCBIQuick(byte *in, byte *out, int width, int height, int component)
{
	int x, y;
	byte *outbyte, *inbyte;

	// copy in to out
	for (y = 2; y < height - 2; y += 2)
	{
		inbyte  = in  + (y * width + 2) * 4 + component;
		outbyte = out + (y * width + 2) * 4 + component;

		for (x = 2; x < width - 2; x += 2)
		{
			*outbyte = *inbyte;
			outbyte += 8;
			inbyte += 8;
		}
	}

	for (y = 3; y < height - 4; y += 2)
	{
		byte sd, se, sh, si;
		byte *line2, *line3;

		x = 3;

		line2 = in + ((y - 1) * width + (x - 1)) * 4 + component;
		line3 = in + ((y + 1) * width + (x - 1)) * 4 + component;

		sd = *line2; line2 += 8;
		sh = *line3; line3 += 8;

		outbyte = out + (y * width + x) * 4 + component;

		for ( ; x < width - 4; x += 2)
		{
			int NWd, NEd, NWp, NEp;

			se = *line2; line2 += 8;
			si = *line3; line3 += 8;

			NWp = sd + si;
			NEp = se + sh;
			NWd = abs(sd - si);
			NEd = abs(se - sh);

			if (NWd < NEd)
				*outbyte = NWp >> 1;
			else
				*outbyte = NEp >> 1;

			outbyte += 8;

			sd = se;
			sh = si;
		}
	}

	// hack: copy out to in again
	for (y = 3; y < height - 3; y += 2)
	{
		inbyte  = out + (y * width + 3) * 4 + component;
		outbyte = in  + (y * width + 3) * 4 + component;

		for (x = 3; x < width - 3; x += 2)
		{
			*outbyte = *inbyte;
			outbyte += 8;
			inbyte += 8;
		}
	}

	for (y = 2; y < height - 3; y++)
	{
		byte sd, sf, sg, si;
		byte *line2, *line3, *line4;

		x = (y + 1) % 2 + 2;

		line2 = in + ((y - 1) * width + (x    )) * 4 + component;
		line3 = in + ((y    ) * width + (x - 1)) * 4 + component;
		line4 = in + ((y + 1) * width + (x    )) * 4 + component;

		outbyte = out + (y * width + x) * 4 + component;

		sf = *line3; line3 += 8;

		for ( ; x < width - 3; x+=2)
		{
			int hd, vd, hp, vp;

			sd = *line2; line2 += 8;
			sg = *line3; line3 += 8;
			si = *line4; line4 += 8;

			hp = sf + sg;
			vp = sd + si;
			hd = abs(sf - sg);
			vd = abs(sd - si);

			if (hd < vd)
				*outbyte = hp >> 1;
			else
				*outbyte = vp >> 1;

			outbyte += 8;

			sf = sg;
		}
	}
}

// Similar to DoFCBIQuick, but just takes the average instead of checking derivatives
// as well, this operates on all four components
static void DoLinear(byte *in, byte *out, int width, int height)
{
	int x, y, i;
	byte *outbyte, *inbyte;

	// copy in to out
	for (y = 2; y < height - 2; y += 2)
	{
		x = 2;

		inbyte  = in  + (y * width + x) * 4;
		outbyte = out + (y * width + x) * 4;

		for ( ; x < width - 2; x += 2)
		{
			COPYSAMPLE(outbyte, inbyte);
			outbyte += 8;
			inbyte += 8;
		}
	}

	for (y = 1; y < height - 1; y += 2)
	{
		byte sd[4] = {0}, se[4] = {0}, sh[4] = {0}, si[4] = {0};
		byte *line2, *line3;

		x = 1;

		line2 = in + ((y - 1) * width + (x - 1)) * 4;
		line3 = in + ((y + 1) * width + (x - 1)) * 4;

		COPYSAMPLE(sd, line2); line2 += 8;
		COPYSAMPLE(sh, line3); line3 += 8;

		outbyte = out + (y * width + x) * 4;

		for ( ; x < width - 1; x += 2)
		{
			COPYSAMPLE(se, line2); line2 += 8;
			COPYSAMPLE(si, line3); line3 += 8;

			for (i = 0; i < 4; i++)
			{
				*outbyte++ = (sd[i] + si[i] + se[i] + sh[i]) >> 2;
			}

			outbyte += 4;

			COPYSAMPLE(sd, se);
			COPYSAMPLE(sh, si);
		}
	}

	// hack: copy out to in again
	for (y = 1; y < height - 1; y += 2)
	{
		x = 1;

		inbyte  = out + (y * width + x) * 4;
		outbyte = in  + (y * width + x) * 4;

		for ( ; x < width - 1; x += 2)
		{
			COPYSAMPLE(outbyte, inbyte);
			outbyte += 8;
			inbyte += 8;
		}
	}

	for (y = 1; y < height - 1; y++)
	{
		byte sd[4], sf[4], sg[4], si[4];
		byte *line2, *line3, *line4;

		x = y % 2 + 1;

		line2 = in + ((y - 1) * width + (x    )) * 4;
		line3 = in + ((y    ) * width + (x - 1)) * 4;
		line4 = in + ((y + 1) * width + (x    )) * 4;

		COPYSAMPLE(sf, line3); line3 += 8;

		outbyte = out + (y * width + x) * 4;

		for ( ; x < width - 1; x += 2)
		{
			COPYSAMPLE(sd, line2); line2 += 8;
			COPYSAMPLE(sg, line3); line3 += 8;
			COPYSAMPLE(si, line4); line4 += 8;

			for (i = 0; i < 4; i++)
			{
				*outbyte++ = (sf[i] + sg[i] + sd[i] + si[i]) >> 2;
			}

			outbyte += 4;

			COPYSAMPLE(sf, sg);
		}
	}
}


static void ExpandHalfTextureToGrid( byte *data, int width, int height)
{
	int x, y;

	for (y = height / 2; y > 0; y--)
	{
		byte *outbyte = data + ((y * 2 - 1) * (width)     - 2) * 4;
		byte *inbyte  = data + (y           * (width / 2) - 1) * 4;

		for (x = width / 2; x > 0; x--)
		{
			COPYSAMPLE(outbyte, inbyte);

			outbyte -= 8;
			inbyte -= 4;
		}
	}
}

static void FillInNormalizedZ(const byte *in, byte *out, int width, int height)
{
	int x, y;

	for (y = 0; y < height; y++)
	{
		const byte *inbyte  = in  + y * width * 4;
		byte       *outbyte = out + y * width * 4;

		for (x = 0; x < width; x++)
		{
			byte nx, ny, nz, h;
			float fnx, fny, fll, fnz;

			nx = *inbyte++;
			ny = *inbyte++;
			inbyte++;
			h  = *inbyte++;

			fnx = OffsetByteToFloat(nx);
			fny = OffsetByteToFloat(ny);
			fll = 1.0f - fnx * fnx - fny * fny;
			if (fll >= 0.0f)
				fnz = (float)sqrt(fll);
			else
				fnz = 0.0f;

			nz = FloatToOffsetByte(fnz);

			*outbyte++ = nx;
			*outbyte++ = ny;
			*outbyte++ = nz;
			*outbyte++ = h;
		}
	}
}


// size must be even
#define WORKBLOCK_SIZE     128
#define WORKBLOCK_BORDER   4
#define WORKBLOCK_REALSIZE (WORKBLOCK_SIZE + WORKBLOCK_BORDER * 2)

// assumes that data has already been expanded into a 2x2 grid
static void FCBIByBlock(byte *data, int width, int height, qboolean clampToEdge, qboolean normalized)
{
	byte workdata[WORKBLOCK_REALSIZE * WORKBLOCK_REALSIZE * 4];
	byte outdata[WORKBLOCK_REALSIZE * WORKBLOCK_REALSIZE * 4];
	byte *inbyte, *outbyte;
	int x, y;
	int srcx, srcy;

	ExpandHalfTextureToGrid(data, width, height);

	for (y = 0; y < height; y += WORKBLOCK_SIZE)
	{
		for (x = 0; x < width; x += WORKBLOCK_SIZE)
		{
			int x2, y2;
			int workwidth, workheight, fullworkwidth, fullworkheight;

			workwidth =  MIN(WORKBLOCK_SIZE, width  - x);
			workheight = MIN(WORKBLOCK_SIZE, height - y);

			fullworkwidth =  workwidth  + WORKBLOCK_BORDER * 2;
			fullworkheight = workheight + WORKBLOCK_BORDER * 2;

			//memset(workdata, 0, WORKBLOCK_REALSIZE * WORKBLOCK_REALSIZE * 4);

			// fill in work block
			for (y2 = 0; y2 < fullworkheight; y2 += 2)
			{
				srcy = y + y2 - WORKBLOCK_BORDER;

				if (clampToEdge)
				{
					srcy = CLAMP(srcy, 0, height - 2);
				}
				else
				{
					srcy = (srcy + height) % height;
				}

				outbyte = workdata + y2   * fullworkwidth * 4;
				inbyte  = data     + srcy * width         * 4;

				for (x2 = 0; x2 < fullworkwidth; x2 += 2)
				{
					srcx = x + x2 - WORKBLOCK_BORDER;

					if (clampToEdge)
					{
						srcx = CLAMP(srcx, 0, width - 2);
					}
					else
					{
						srcx = (srcx + width) % width;
					}

					COPYSAMPLE(outbyte, inbyte + srcx * 4);
					outbyte += 8;
				}
			}

			// submit work block
			DoLinear(workdata, outdata, fullworkwidth, fullworkheight);

			if (!normalized)
			{
				switch (r_imageUpsampleType->integer)
				{
					case 0:
						break;
					case 1:
						DoFCBIQuick(workdata, outdata, fullworkwidth, fullworkheight, 0);
						break;
					case 2:
					default:
						DoFCBI(workdata, outdata, fullworkwidth, fullworkheight, 0);
						break;
				}
			}
			else
			{
				switch (r_imageUpsampleType->integer)
				{
					case 0:
						break;
					case 1:
						DoFCBIQuick(workdata, outdata, fullworkwidth, fullworkheight, 0);
						DoFCBIQuick(workdata, outdata, fullworkwidth, fullworkheight, 1);
						break;
					case 2:
					default:
						DoFCBI(workdata, outdata, fullworkwidth, fullworkheight, 0);
						DoFCBI(workdata, outdata, fullworkwidth, fullworkheight, 1);
						break;
				}
			}

			// copy back work block
			for (y2 = 0; y2 < workheight; y2++)
			{
				inbyte = outdata + ((y2 + WORKBLOCK_BORDER) * fullworkwidth + WORKBLOCK_BORDER) * 4;
				outbyte = data +   ((y + y2)                * width         + x)                * 4;
				for (x2 = 0; x2 < workwidth; x2++)
				{
					COPYSAMPLE(outbyte, inbyte);
					outbyte += 4;
					inbyte += 4;
				}
			}
		}
	}
}
#undef COPYSAMPLE

/*
================
R_LightScaleTexture

Scale up the pixel values in a texture to increase the
lighting range
================
*/
void R_LightScaleTexture (byte *in, int inwidth, int inheight, qboolean only_gamma )
{
	if ( only_gamma )
	{
		if ( !glConfig.deviceSupportsGamma )
		{
			int		i, c;
			byte	*p;

			p = in;

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

		p = in;

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
static void R_MipMap2( byte *in, int inWidth, int inHeight ) {
	int			i, j, k;
	byte		*outpix;
	int			inWidthMask, inHeightMask;
	int			total;
	int			outWidth, outHeight;
	unsigned	*temp;

	outWidth = inWidth >> 1;
	outHeight = inHeight >> 1;
	temp = (unsigned int *)ri.Hunk_AllocateTempMemory( outWidth * outHeight * 4 );

	inWidthMask = inWidth - 1;
	inHeightMask = inHeight - 1;

	for ( i = 0 ; i < outHeight ; i++ ) {
		for ( j = 0 ; j < outWidth ; j++ ) {
			outpix = (byte *) ( temp + i * outWidth + j );
			for ( k = 0 ; k < 4 ; k++ ) {
				total =
					1 * (&in[ 4*(((i*2-1)&inHeightMask)*inWidth + ((j*2-1)&inWidthMask)) ])[k] +
					2 * (&in[ 4*(((i*2-1)&inHeightMask)*inWidth + ((j*2  )&inWidthMask)) ])[k] +
					2 * (&in[ 4*(((i*2-1)&inHeightMask)*inWidth + ((j*2+1)&inWidthMask)) ])[k] +
					1 * (&in[ 4*(((i*2-1)&inHeightMask)*inWidth + ((j*2+2)&inWidthMask)) ])[k] +

					2 * (&in[ 4*(((i*2  )&inHeightMask)*inWidth + ((j*2-1)&inWidthMask)) ])[k] +
					4 * (&in[ 4*(((i*2  )&inHeightMask)*inWidth + ((j*2  )&inWidthMask)) ])[k] +
					4 * (&in[ 4*(((i*2  )&inHeightMask)*inWidth + ((j*2+1)&inWidthMask)) ])[k] +
					2 * (&in[ 4*(((i*2  )&inHeightMask)*inWidth + ((j*2+2)&inWidthMask)) ])[k] +

					2 * (&in[ 4*(((i*2+1)&inHeightMask)*inWidth + ((j*2-1)&inWidthMask)) ])[k] +
					4 * (&in[ 4*(((i*2+1)&inHeightMask)*inWidth + ((j*2  )&inWidthMask)) ])[k] +
					4 * (&in[ 4*(((i*2+1)&inHeightMask)*inWidth + ((j*2+1)&inWidthMask)) ])[k] +
					2 * (&in[ 4*(((i*2+1)&inHeightMask)*inWidth + ((j*2+2)&inWidthMask)) ])[k] +

					1 * (&in[ 4*(((i*2+2)&inHeightMask)*inWidth + ((j*2-1)&inWidthMask)) ])[k] +
					2 * (&in[ 4*(((i*2+2)&inHeightMask)*inWidth + ((j*2  )&inWidthMask)) ])[k] +
					2 * (&in[ 4*(((i*2+2)&inHeightMask)*inWidth + ((j*2+1)&inWidthMask)) ])[k] +
					1 * (&in[ 4*(((i*2+2)&inHeightMask)*inWidth + ((j*2+2)&inWidthMask)) ])[k];
				outpix[k] = total / 36;
			}
		}
	}

	Com_Memcpy( in, temp, outWidth * outHeight * 4 );
	ri.Hunk_FreeTempMemory( temp );
}


static void R_MipMapsRGB( byte *in, int inWidth, int inHeight)
{
	int			i, j, k;
	int			outWidth, outHeight;
	byte		*temp;

	if ( r_simpleMipMaps->integer )
		return;

	outWidth = inWidth >> 1;
	outHeight = inHeight >> 1;
	temp = (byte *)ri.Hunk_AllocateTempMemory( outWidth * outHeight * 4 );

	for ( i = 0 ; i < outHeight ; i++ ) {
		byte *outbyte = temp + (  i          * outWidth ) * 4;
		byte *inbyte1 = in   + (  i * 2      * inWidth  ) * 4;
		byte *inbyte2 = in   + ( (i * 2 + 1) * inWidth  ) * 4;
		for ( j = 0 ; j < outWidth ; j++ ) {
			for ( k = 0 ; k < 3 ; k++ ) {
				float total, current;

				current = ByteToFloat(inbyte1[0]); total  = sRGBtoRGB((double)current);
				current = ByteToFloat(inbyte1[4]); total += sRGBtoRGB((double)current);
				current = ByteToFloat(inbyte2[0]); total += sRGBtoRGB((double)current);
				current = ByteToFloat(inbyte2[4]); total += sRGBtoRGB((double)current);

				total *= 0.25f;

				inbyte1++;
				inbyte2++;

				current = RGBtosRGB(total);
				*outbyte++ = FloatToByte(current);
			}
			*outbyte++ = (inbyte1[0] + inbyte1[4] + inbyte2[0] + inbyte2[4]) >> 2;
			inbyte1 += 5;
			inbyte2 += 5;
		}
	}

	Com_Memcpy( in, temp, outWidth * outHeight * 4 );
	ri.Hunk_FreeTempMemory( temp );
}

/*
================
R_MipMap

Operates in place, quartering the size of the texture
================
*/
static void R_MipMap (byte *in, int width, int height) {

	if ( !r_simpleMipMaps->integer )
		R_MipMap2( in, width, height );

}


static void R_MipMapLuminanceAlpha (const byte *in, byte *out, int width, int height)
{
	int  i, j, row;

	if ( r_simpleMipMaps->integer )
		return;

	if ( width == 1 && height == 1 ) {
		return;
	}

	row = width * 4;
	width >>= 1;
	height >>= 1;

	if ( width == 0 || height == 0 ) {
		width += height;	// get largest
		for (i=0 ; i<width ; i++, out+=4, in+=8 ) {
			out[0] =
			out[1] =
			out[2] = (in[0] + in[4]) >> 1;
			out[3] = (in[3] + in[7]) >> 1;
		}
		return;
	}

	for (i=0 ; i<height ; i++, in+=row) {
		for (j=0 ; j<width ; j++, out+=4, in+=8) {
			out[0] =
			out[1] =
			out[2] = (in[0] + in[4] + in[row  ] + in[row+4]) >> 2;
			out[3] = (in[3] + in[7] + in[row+3] + in[row+7]) >> 2;
		}
	}

}


static void R_MipMapNormalHeight (const byte *in, byte *out, int width, int height, qboolean swizzle)
{
	int		i, j;
	int		row;
	int sx = swizzle ? 3 : 0;
	int sa = swizzle ? 0 : 3;

	if ( r_simpleMipMaps->integer )
		return;

	if ( width == 1 && height == 1 ) {
		return;
	}

	row = width * 4;
	width >>= 1;
	height >>= 1;

	for (i=0 ; i<height ; i++, in+=row) {
		for (j=0 ; j<width ; j++, out+=4, in+=8) {
			vec3_t v;

			v[0] =  OffsetByteToFloat(in[sx      ]);
			v[1] =  OffsetByteToFloat(in[       1]);
			v[2] =  OffsetByteToFloat(in[       2]);

			v[0] += OffsetByteToFloat(in[sx    +4]);
			v[1] += OffsetByteToFloat(in[       5]);
			v[2] += OffsetByteToFloat(in[       6]);

			v[0] += OffsetByteToFloat(in[sx+row  ]);
			v[1] += OffsetByteToFloat(in[   row+1]);
			v[2] += OffsetByteToFloat(in[   row+2]);

			v[0] += OffsetByteToFloat(in[sx+row+4]);
			v[1] += OffsetByteToFloat(in[   row+5]);
			v[2] += OffsetByteToFloat(in[   row+6]);

			VectorNormalizeFast(v);

			//v[0] *= 0.25f;
			//v[1] *= 0.25f;
			//v[2] = 1.0f - v[0] * v[0] - v[1] * v[1];
			//v[2] = sqrt(MAX(v[2], 0.0f));

			out[sx] = FloatToOffsetByte(v[0]);
			out[1 ] = FloatToOffsetByte(v[1]);
			out[2 ] = FloatToOffsetByte(v[2]);
			out[sa] = MAX(MAX(in[sa], in[sa+4]), MAX(in[sa+row], in[sa+row+4]));
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

static void RawImage_SwizzleRA( byte *data, int width, int height )
{
	int i;
	byte *ptr = data, swap;

	for (i=0; i<width*height; i++, ptr+=4)
	{
		// swap red and alpha
		swap = ptr[0];
		ptr[0] = ptr[3];
		ptr[3] = swap;
	}
}


/*
===============
RawImage_ScaleToPower2

===============
*/
static void RawImage_ScaleToPower2( byte **data, int *inout_width, int *inout_height, int *inout_scaled_width, int *inout_scaled_height, imgType_t type, int flags, byte **resampledBuffer)
{
	int width =         *inout_width;
	int height =        *inout_height;
	int scaled_width;
	int scaled_height;
	qboolean picmip = (qboolean)(flags & IMGFLAG_PICMIP);
	qboolean mipmap = (qboolean)(flags & IMGFLAG_MIPMAP);
	qboolean clampToEdge = (qboolean)(flags & IMGFLAG_CLAMPTOEDGE);

	//
	// convert to exact power of 2 sizes
	//
	if (!mipmap)
	{
		scaled_width = width;
		scaled_height = height;
	}
	else
	{
		scaled_width = NextPowerOfTwo(width);
		scaled_height = NextPowerOfTwo(height);
	}

	if ( r_roundImagesDown->integer && scaled_width > width )
		scaled_width >>= 1;
	if ( r_roundImagesDown->integer && scaled_height > height )
		scaled_height >>= 1;

	if ( picmip && data && resampledBuffer && r_imageUpsample->integer &&
	     scaled_width < r_imageUpsampleMaxSize->integer && scaled_height < r_imageUpsampleMaxSize->integer)
	{
		int finalwidth, finalheight;
		//int startTime, endTime;

		//startTime = ri.Milliseconds();

		finalwidth = scaled_width << r_imageUpsample->integer;
		finalheight = scaled_height << r_imageUpsample->integer;

		while ( finalwidth > r_imageUpsampleMaxSize->integer
			|| finalheight > r_imageUpsampleMaxSize->integer ) {
			finalwidth >>= 1;
			finalheight >>= 1;
		}

		while ( finalwidth > glConfig.maxTextureSize
			|| finalheight > glConfig.maxTextureSize ) {
			finalwidth >>= 1;
			finalheight >>= 1;
		}

		*resampledBuffer = (byte *)ri.Hunk_AllocateTempMemory( finalwidth * finalheight * 4 );

		if (scaled_width != width || scaled_height != height)
		{
			ResampleTexture (*data, width, height, *resampledBuffer, scaled_width, scaled_height);
		}
		else
		{
			byte *inbyte, *outbyte;
			int i;

			inbyte = *data;
			outbyte = *resampledBuffer;

			for (i = width * height * 4; i > 0; i--)
			{
				*outbyte++ = *inbyte++;
			}
		}

		if (type == IMGTYPE_COLORALPHA)
			RGBAtoYCoCgA(*resampledBuffer, *resampledBuffer, scaled_width, scaled_height);

		while (scaled_width < finalwidth || scaled_height < finalheight)
		{
			scaled_width <<= 1;
			scaled_height <<= 1;

			FCBIByBlock(*resampledBuffer, scaled_width, scaled_height, clampToEdge, (qboolean)(type == IMGTYPE_NORMAL || type == IMGTYPE_NORMALHEIGHT));
		}

		if (type == IMGTYPE_COLORALPHA)
		{
			YCoCgAtoRGBA(*resampledBuffer, *resampledBuffer, scaled_width, scaled_height);
		}
		else if (type == IMGTYPE_NORMAL || type == IMGTYPE_NORMALHEIGHT)
		{
			FillInNormalizedZ(*resampledBuffer, *resampledBuffer, scaled_width, scaled_height);
		}


		//endTime = ri.Milliseconds();

		//ri.Printf(PRINT_ALL, "upsampled %dx%d to %dx%d in %dms\n", width, height, scaled_width, scaled_height, endTime - startTime);

		*data = *resampledBuffer;
		width = scaled_width;
		height = scaled_height;
	}
	else if ( scaled_width != width || scaled_height != height ) {
		if (data && resampledBuffer)
		{
			*resampledBuffer = (byte *)ri.Hunk_AllocateTempMemory( scaled_width * scaled_height * 4 );
			ResampleTexture (*data, width, height, *resampledBuffer, scaled_width, scaled_height);
			*data = *resampledBuffer;
		}
		width = scaled_width;
		height = scaled_height;
	}

	//
	// perform optional picmip operation
	//
	if ( picmip ) {
		scaled_width >>= r_picmip->integer;
		scaled_height >>= r_picmip->integer;
	}

	//
	// clamp to minimum size
	//
	if (scaled_width < 1) {
		scaled_width = 1;
	}
	if (scaled_height < 1) {
		scaled_height = 1;
	}

	//
	// clamp to the current upper OpenGL limit
	// scale both axis down equally so we don't have to
	// deal with a half mip resampling
	//
	while ( scaled_width > glConfig.maxTextureSize
		|| scaled_height > glConfig.maxTextureSize ) {
		scaled_width >>= 1;
		scaled_height >>= 1;
	}

	*inout_width         = width;
	*inout_height        = height;
	*inout_scaled_width  = scaled_width;
	*inout_scaled_height = scaled_height;
}


static qboolean RawImage_HasAlpha(const byte *scan, int numPixels)
{
	int i;

	if (!scan)
		return qtrue;

	for ( i = 0; i < numPixels; i++ )
	{
		if ( scan[i*4 + 3] != 255 )
		{
			return qtrue;
		}
	}

	return qfalse;
}

static GLenum RawImage_GetFormat(const byte *data, int numPixels, qboolean lightMap, imgType_t type, int flags)
{
	int samples = 3;
	GLenum internalFormat = GL_RGB8;
	qboolean forceNoCompression = (qboolean)(flags & IMGFLAG_NO_COMPRESSION);
	qboolean normalmap = (qboolean)(type == IMGTYPE_NORMAL || type == IMGTYPE_NORMALHEIGHT);

	if(normalmap)
	{
		if ((!RawImage_HasAlpha(data, numPixels) || (type == IMGTYPE_NORMAL)) && !forceNoCompression && (glRefConfig.textureCompression & TCR_LATC))
		{
			internalFormat = GL_COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT;
		}
		else
		{
			if ( !forceNoCompression && glConfig.textureCompression == TC_S3TC_ARB )
			{
				internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
			}
			else if ( r_texturebits->integer == 16 )
			{
				internalFormat = GL_RGBA4;
			}
			else if ( r_texturebits->integer == 32 )
			{
				internalFormat = GL_RGBA8;
			}
			else
			{
				internalFormat = GL_RGBA8;
			}
		}
	}
	else if(lightMap)
	{
#if 0
		if(r_greyscale->integer)
			internalFormat = GL_LUMINANCE;
		else
#endif
			internalFormat = GL_RGBA;
	}
	else
	{
		if (RawImage_HasAlpha(data, numPixels))
		{
			samples = 4;
		}

		// select proper internal format
		if ( samples == 3 )
		{
#if 0
			if(r_greyscale->integer)
			{
				if(r_texturebits->integer == 16)
					internalFormat = GL_LUMINANCE8;
				else if(r_texturebits->integer == 32)
					internalFormat = GL_LUMINANCE16;
				else
					internalFormat = GL_LUMINANCE;
			}
			else
#endif
			{
				if ( !forceNoCompression && (glRefConfig.textureCompression & TCR_BPTC) )
				{
					internalFormat = GL_COMPRESSED_RGBA_BPTC_UNORM_ARB;
				}
				else if ( !forceNoCompression && glConfig.textureCompression == TC_S3TC_ARB )
				{
					internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
				}
				else if ( !forceNoCompression && glConfig.textureCompression == TC_S3TC )
				{
					internalFormat = GL_RGB4_S3TC;
				}
				else if ( r_texturebits->integer == 16 )
				{
					internalFormat = GL_RGB5;
				}
				else if ( r_texturebits->integer == 32 )
				{
					internalFormat = GL_RGB8;
				}
				else
				{
					internalFormat = GL_RGB8;
				}
			}
		}
		else if ( samples == 4 )
		{
#if 0
			if(r_greyscale->integer)
			{
				if(r_texturebits->integer == 16)
					internalFormat = GL_LUMINANCE8_ALPHA8;
				else if(r_texturebits->integer == 32)
					internalFormat = GL_LUMINANCE16_ALPHA16;
				else
					internalFormat = GL_LUMINANCE_ALPHA;
			}
			else
#endif
			{
				if ( !forceNoCompression && (glRefConfig.textureCompression & TCR_BPTC) )
				{
					internalFormat = GL_COMPRESSED_RGBA_BPTC_UNORM_ARB;
				}
				else if ( !forceNoCompression && glConfig.textureCompression == TC_S3TC_ARB )
				{
					internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
				}
				else if ( r_texturebits->integer == 16 )
				{
					internalFormat = GL_RGBA4;
				}
				else if ( r_texturebits->integer == 32 )
				{
					internalFormat = GL_RGBA8;
				}
				else
				{
					internalFormat = GL_RGBA8;
				}
			}
		}

		if (flags & IMGFLAG_SRGB)
		{
			switch(internalFormat)
			{
				case GL_RGB:
					internalFormat = GL_SRGB8;
					break;

				case GL_RGB4:
				case GL_RGB5:
				case GL_RGB8:
					internalFormat = GL_SRGB8;
					break;

				case GL_RGBA:
					internalFormat = GL_SRGB_ALPHA;
					break;

				case GL_RGBA4:
				case GL_RGBA8:
					internalFormat = GL_SRGB8_ALPHA8;
					break;

				case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
					internalFormat = GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT;
					break;

				case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
					internalFormat = GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT;
					break;

				case GL_COMPRESSED_RGBA_BPTC_UNORM_ARB:
					internalFormat = GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB;
					break;
			}
		}
	}

	return internalFormat;
}

static int CalcNumMipmapLevels ( int width, int height )
{
	return static_cast<int>(ceil (log2 (Q_max (width, height))) + 1);
}

static qboolean IsBPTCTextureFormat( GLenum internalformat )
{
	switch ( internalformat )
	{
		case GL_COMPRESSED_RGBA_BPTC_UNORM_ARB:
		case GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB:
		case GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT_ARB:
		case GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT_ARB:
			return qtrue;

		default:
			return qfalse;
	}
}

static qboolean ShouldUseImmutableTextures(int imageFlags, GLenum internalformat)
{
	if ( glRefConfig.hardwareVendor == IHV_AMD )
	{
		// Corrupted texture data is seen when using BPTC + immutable textures
		if ( IsBPTCTextureFormat( internalformat ) )
		{
			return qfalse;
		}
	}

	if ( imageFlags & IMGFLAG_MUTABLE )
	{
		return qfalse;
	}

	return glRefConfig.immutableTextures;
}

static void RawImage_UploadTexture( byte *data, int x, int y, int width, int height, GLenum internalFormat, imgType_t type, int flags, qboolean subtexture )
{
	int dataFormat, dataType;

	switch (internalFormat)
	{
	case GL_DEPTH_COMPONENT:
	case GL_DEPTH_COMPONENT16:
	case GL_DEPTH_COMPONENT24:
	case GL_DEPTH_COMPONENT32:
		dataFormat = GL_DEPTH_COMPONENT;
		dataType = GL_UNSIGNED_BYTE;
		break;
	case GL_RG16F:
		dataFormat = GL_RG;
		dataType = GL_HALF_FLOAT;
		break;
	case GL_RGB16F:
		dataFormat = GL_RGB;
		dataType = GL_HALF_FLOAT;
		break;
	case GL_RGBA16F:
		dataFormat = GL_RGBA;
		dataType = GL_HALF_FLOAT;
		break;
	case GL_RG32F:
		dataFormat = GL_RG;
		dataType = GL_FLOAT;
		break;
	case GL_RGBA32F:
		dataFormat = GL_RGBA;
		dataType = GL_FLOAT;
		break;
	default:
		dataFormat = GL_RGBA;
		dataType = GL_UNSIGNED_BYTE;
		break;
	}

	if ( subtexture )
	{
		qglTexSubImage2D (GL_TEXTURE_2D, 0, x, y, width, height, dataFormat, dataType, data);
	}
	else
	{
		if ( ShouldUseImmutableTextures( flags, internalFormat ) )
		{
			int numLevels = (flags & IMGFLAG_MIPMAP) ? CalcNumMipmapLevels (width, height) : 1;

			qglTexStorage2D (GL_TEXTURE_2D, numLevels, internalFormat, width, height);

			if ( data != NULL )
			{
				qglTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, width, height, dataFormat, dataType, data);
			}
		}
		else
		{
			qglTexImage2D (GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, dataType, data );
		}
	}

	if ((flags & IMGFLAG_MIPMAP) && (!r_simpleMipMaps->integer) &&
		(data != NULL || !ShouldUseImmutableTextures(flags, internalFormat) ))
	{
		// Don't need to generate mipmaps if we are generating an immutable texture and
		// the data is NULL. All levels have already been allocated by glTexStorage2D.

		int miplevel = 0;

		while (width > 1 || height > 1)
		{
			if (data)
			{
				if (type == IMGTYPE_NORMAL || type == IMGTYPE_NORMALHEIGHT)
				{
					if (internalFormat == GL_COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT)
					{
						R_MipMapLuminanceAlpha( data, data, width, height );
					}
					else
					{
						R_MipMapNormalHeight( data, data, width, height, qtrue);
					}
				}
				else if (flags & IMGFLAG_SRGB)
				{
					R_MipMapsRGB( data, width, height );
				}
				else
				{
					R_MipMap( data, width, height );
				}
			}

			width >>= 1;
			height >>= 1;
			if (width < 1)
				width = 1;
			if (height < 1)
				height = 1;
			miplevel++;

			if ( data && r_colorMipLevels->integer )
				R_BlendOverTexture( (byte *)data, width * height, mipBlendColors[miplevel] );

			if ( subtexture )
			{
				x >>= 1;
				y >>= 1;
				qglTexSubImage2D( GL_TEXTURE_2D, miplevel, x, y, width, height, dataFormat, dataType, data );
			}
			else
			{
				if ( ShouldUseImmutableTextures(flags, internalFormat) )
				{
					qglTexSubImage2D (GL_TEXTURE_2D, miplevel, 0, 0, width, height, dataFormat, dataType, data );
				}
				else
				{
					qglTexImage2D (GL_TEXTURE_2D, miplevel, internalFormat, width, height, 0, dataFormat, dataType, data );
				}
			}
		}
	}
}

static bool IsPowerOfTwo ( int i )
{
	return (i & (i - 1)) == 0;
}

/*
===============
Upload32

===============
*/
extern qboolean charSet;
static void Upload32( byte *data, int width, int height, imgType_t type, int flags,
	qboolean lightMap, GLenum internalFormat, int *pUploadWidth, int *pUploadHeight)
{
	byte		*scaledBuffer = NULL;
	byte		*resampledBuffer = NULL;
	int			scaled_width = width;
	int			scaled_height = height;
	int			i, c;
	byte		*scan;

	if ( !IsPowerOfTwo (width) || !IsPowerOfTwo (height) )
	{
		RawImage_ScaleToPower2(&data, &width, &height, &scaled_width, &scaled_height, type, flags, &resampledBuffer);
	}

	scaledBuffer = (byte *)ri.Hunk_AllocateTempMemory( sizeof( unsigned ) * scaled_width * scaled_height );

	//
	// scan the texture for each channel's max values
	// and verify if the alpha channel is being used or not
	//
	c = width*height;
	scan = data;

	if( r_greyscale->integer )
	{
		for ( i = 0; i < c; i++ )
		{
			byte luma = LUMA(scan[i*4], scan[i*4 + 1], scan[i*4 + 2]);
			scan[i*4] = luma;
			scan[i*4 + 1] = luma;
			scan[i*4 + 2] = luma;
		}
	}
	else if( r_greyscale->value )
	{
		for ( i = 0; i < c; i++ )
		{
			float luma = LUMA(scan[i*4], scan[i*4 + 1], scan[i*4 + 2]);
			scan[i*4] = LERP(scan[i*4], luma, r_greyscale->value);
			scan[i*4 + 1] = LERP(scan[i*4 + 1], luma, r_greyscale->value);
			scan[i*4 + 2] = LERP(scan[i*4 + 2], luma, r_greyscale->value);
		}
	}

	// normals are always swizzled
	if (type == IMGTYPE_NORMAL || type == IMGTYPE_NORMALHEIGHT)
	{
		RawImage_SwizzleRA(data, width, height);
	}

	// LATC2 is only used for normals
	if (internalFormat == GL_COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT)
	{
		byte *in = data;
		int c = width * height;
		while (c--)
		{
			in[0] = in[1];
			in[2] = in[1];
			in += 4;
		}
	}

	// copy or resample data as appropriate for first MIP level
	if ( ( scaled_width == width ) &&
		( scaled_height == height ) ) {
		if (!(flags & IMGFLAG_MIPMAP))
		{
			RawImage_UploadTexture( data, 0, 0, scaled_width, scaled_height, internalFormat, type, flags, qfalse );
			//qglTexImage2D (GL_TEXTURE_2D, 0, internalFormat, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
			*pUploadWidth = scaled_width;
			*pUploadHeight = scaled_height;

			goto done;
		}
		Com_Memcpy (scaledBuffer, data, width*height*4);
	}
	else if ( !r_simpleMipMaps->integer )
	{
		// use the normal mip-mapping function to go down from here
		while ( width > scaled_width || height > scaled_height ) {

			if (flags & IMGFLAG_SRGB)
			{
				R_MipMapsRGB( (byte *)data, width, height );
			}
			else
			{
				R_MipMap( (byte *)data, width, height );
			}

			width >>= 1;
			height >>= 1;
			if ( width < 1 ) {
				width = 1;
			}
			if ( height < 1 ) {
				height = 1;
			}
		}
		Com_Memcpy( scaledBuffer, data, width * height * 4 );
	}

	if (!(flags & IMGFLAG_NOLIGHTSCALE))
		R_LightScaleTexture (scaledBuffer, scaled_width, scaled_height, (qboolean)(!(flags & IMGFLAG_MIPMAP)) );

	*pUploadWidth = scaled_width;
	*pUploadHeight = scaled_height;

	RawImage_UploadTexture(scaledBuffer, 0, 0, scaled_width, scaled_height, internalFormat, type, flags, qfalse);

done:

	if (flags & IMGFLAG_MIPMAP)
	{
		if (r_ext_texture_filter_anisotropic->value > 1.0f && glConfig.maxTextureFilterAnisotropy > 0.0f)
		{
			qglTexParameterf ( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT,
							  Com_Clamp( 1.0f, glConfig.maxTextureFilterAnisotropy, r_ext_texture_filter_anisotropic->value ) );
		}

		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
	}
	else
	{
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	}

	GL_CheckErrors();

	if ( scaledBuffer != 0 )
		ri.Hunk_FreeTempMemory( scaledBuffer );
	if ( resampledBuffer != 0 )
		ri.Hunk_FreeTempMemory( resampledBuffer );
}


static void EmptyTexture( int width, int height, imgType_t type, int flags,
	qboolean lightMap, GLenum internalFormat, int *pUploadWidth, int *pUploadHeight )
{
	int			scaled_width, scaled_height;

	RawImage_ScaleToPower2(NULL, &width, &height, &scaled_width, &scaled_height, type, flags, NULL);

	*pUploadWidth = scaled_width;
	*pUploadHeight = scaled_height;

	RawImage_UploadTexture(NULL, 0, 0, scaled_width, scaled_height, internalFormat, type, flags, qfalse);

	if (flags & IMGFLAG_MIPMAP)
	{
		if (r_ext_texture_filter_anisotropic->integer > 1 && glConfig.maxTextureFilterAnisotropy > 0.0f)
		{
			qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT,
					(GLint)Com_Clamp( 1, glConfig.maxTextureFilterAnisotropy, r_ext_texture_filter_anisotropic->integer ) );
		}

		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
	}
	else
	{
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	}

	// Fix for sampling depth buffer on old nVidia cards
	// from http://www.idevgames.com/forums/thread-4141-post-34844.html#pid34844
	switch(internalFormat)
	{
		case GL_DEPTH_COMPONENT:
		case GL_DEPTH_COMPONENT16:
		case GL_DEPTH_COMPONENT24:
		case GL_DEPTH_COMPONENT32:
			//qglTexParameterf(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_LUMINANCE );
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
			break;
		default:
			break;
	}

	GL_CheckErrors();
}

static image_t *R_AllocImage()
{
	image_t *result;

	if ( !tr.imagesFreeList )
	{
		R_ExtendImagesPool();
	}

	// Remove from free list
	result = tr.imagesFreeList;
	tr.imagesFreeList = tr.imagesFreeList->poolNext;

	// Add to list of used images
	result->poolNext = tr.images;
	tr.images = result;

	tr.numImages++;

	return result;
}

#if 0
static void R_FreeImage( image_t *imageToFree )
{
	if ( imageToFree )
	{
		// Images aren't deleted individually very often. Not a
		// problem to do this really...
		if ( imageToFree == tr.images )
		{
			tr.images = imageToFree->poolNext;
			imageToFree->poolNext = tr.imagesFreeList;
			tr.imagesFreeList = imageToFree;

			tr.numImages--;
		}
		else
		{
			image_t *image = tr.images;
			while ( image )
			{
				if ( image->poolNext == imageToFree )
				{
					image->poolNext = imageToFree->poolNext;
					imageToFree->poolNext = tr.imagesFreeList;
					tr.imagesFreeList = imageToFree;

					tr.numImages--;
					break;
				}

				image = image->poolNext;
			}
		}
	}
}
#endif

/*
================
R_CreateImage

This is the only way any 2d image_t are created
================
*/
image_t *R_CreateImage( const char *name, byte *pic, int width, int height, imgType_t type, int flags, int internalFormat ) {
	image_t		*image;
	qboolean	isLightmap = qfalse;
	long		hash;
	int         glWrapClampMode;

	if (strlen(name) >= MAX_QPATH ) {
		ri.Error (ERR_DROP, "R_CreateImage: \"%s\" is too long", name);
	}
	if ( !strncmp( name, "*lightmap", 9 ) ) {
		isLightmap = qtrue;
	}

	image = R_AllocImage();
	qglGenTextures(1, &image->texnum);

	image->type = type;
	image->flags = flags;

	Q_strncpyz (image->imgName, name, sizeof (image->imgName));

	image->width = width;
	image->height = height;
	if (flags & IMGFLAG_CLAMPTOEDGE)
		glWrapClampMode = GL_CLAMP_TO_EDGE;
	else
		glWrapClampMode = GL_REPEAT;

	if (!internalFormat)
	{
		if (image->flags & IMGFLAG_CUBEMAP)
			internalFormat = r_hdr->integer ? GL_RGBA16F : GL_RGBA8;
		else
			internalFormat = RawImage_GetFormat(pic, width * height, isLightmap, image->type, image->flags);
	}

	image->internalFormat = internalFormat;

	// lightmaps are always allocated on TMU 1
	if ( isLightmap ) {
		image->TMU = 1;
	} else {
		image->TMU = 0;
	}

	GL_SelectTexture( image->TMU );

	if (image->flags & IMGFLAG_CUBEMAP)
	{
		GL_Bind(image);
		qglTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		qglTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		qglTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

		int format = GL_BGRA;
		switch (internalFormat)
		{
		case GL_DEPTH_COMPONENT:
		case GL_DEPTH_COMPONENT16:
		case GL_DEPTH_COMPONENT24:
		case GL_DEPTH_COMPONENT32:
			format = GL_DEPTH_COMPONENT;
			break;
		default:
			break;
		}

		if (image->flags & IMGFLAG_MIPMAP)
		{
			qglTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		}
		else
		{
			qglTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		}
		qglTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		if ( ShouldUseImmutableTextures( image->flags, internalFormat ) )
		{
			int numLevels = (image->flags & IMGFLAG_MIPMAP) ? CalcNumMipmapLevels (width, height) : 1;

			qglTexStorage2D (GL_TEXTURE_CUBE_MAP, numLevels, internalFormat, width, height);

			if ( pic != NULL )
			{
				for ( int i = 0; i < 6; i++ )
				{
					qglTexSubImage2D (GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, 0, 0, width, height, format, GL_UNSIGNED_BYTE, pic);
				}
			}
		}
		else
		{
			qglTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, pic);
			qglTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, pic);
			qglTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, pic);
			qglTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, pic);
			qglTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, pic);
			qglTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, pic);
		}

		if (image->flags & IMGFLAG_MIPMAP)
			qglGenerateMipmap(GL_TEXTURE_CUBE_MAP);

		image->uploadWidth = width;
		image->uploadHeight = height;
	}
	else
	{
		GL_Bind(image);

		if (pic)
		{
			Upload32( pic, image->width, image->height, image->type, image->flags,
				isLightmap, image->internalFormat, &image->uploadWidth,
				&image->uploadHeight );
		}
		else
		{
			EmptyTexture(image->width, image->height, image->type, image->flags,
				isLightmap, image->internalFormat, &image->uploadWidth,
				&image->uploadHeight );
		}

		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, glWrapClampMode );
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, glWrapClampMode );

		if (image->flags & IMGFLAG_MIPMAP && r_simpleMipMaps->integer)
			qglGenerateMipmap(GL_TEXTURE_2D);
	}

	GL_SelectTexture( 0 );

	hash = generateHashValue(name);
	image->next = hashTable[hash];
	hashTable[hash] = image;

	return image;
}

/*
================
R_Create2DImageArray

This is the only way any 2d array sampler image_t are created
================
*/
image_t *R_Create2DImageArray(const char *name, byte *pic, int width, int height, int layers, imgType_t type, int flags, int internalFormat)
{
	image_t		*image;
	long		hash;
	int         glWrapClampMode;
	int			format;

	if (strlen(name) >= MAX_QPATH) {
		ri.Error(ERR_DROP, "R_Create2DImageArray: \"%s\" is too long", name);
	}

	image = R_AllocImage();
	qglGenTextures(1, &image->texnum);

	image->type = type;
	image->flags = flags | IMGFLAG_2D_ARRAY;

	Q_strncpyz(image->imgName, name, sizeof(image->imgName));

	image->width = width;
	image->height = height;
	image->layers = layers;
	image->internalFormat = internalFormat;

	if (flags & IMGFLAG_CLAMPTOEDGE)
		glWrapClampMode = GL_CLAMP_TO_EDGE;
	else
		glWrapClampMode = GL_REPEAT;

	switch (internalFormat)
	{
	case GL_DEPTH_COMPONENT:
	case GL_DEPTH_COMPONENT16:
	case GL_DEPTH_COMPONENT24:
	case GL_DEPTH_COMPONENT32:
		format = GL_DEPTH_COMPONENT;
		break;
	default:
		format = GL_BGRA;
		break;
	}

	GL_SelectTexture(0);
	GL_Bind(image);
	if (ShouldUseImmutableTextures(image->flags, internalFormat))
		qglTexStorage3D(GL_TEXTURE_2D_ARRAY, 0, internalFormat, width, height, layers);
	else
		qglTexImage3D(GL_TEXTURE_2D_ARRAY, 0, internalFormat, width, height, layers, 0, format, GL_UNSIGNED_BYTE, NULL);

	switch (internalFormat)
	{
	case GL_DEPTH_COMPONENT:
	case GL_DEPTH_COMPONENT16:
	case GL_DEPTH_COMPONENT24:
	case GL_DEPTH_COMPONENT32:

		qglTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		qglTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		if (flags & IMGLFAG_SHADOWCOMP)
		{
			qglTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			qglTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			qglTexParameterf(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
			qglTexParameterf(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
		}
		else
		{
			qglTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			qglTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		}

		break;
	default:
		qglTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		qglTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		qglTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, glWrapClampMode);
		qglTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, glWrapClampMode);
		break;
	}
	qglBindTexture(GL_TEXTURE_2D_ARRAY, 0);
	GL_SelectTexture(0);

	hash = generateHashValue(name);
	image->next = hashTable[hash];
	hashTable[hash] = image;

	return image;
}

void R_UpdateSubImage( image_t *image, byte *pic, int x, int y, int width, int height )
{
	byte *scaledBuffer = NULL;
	byte *resampledBuffer = NULL;
	int	 scaled_width, scaled_height, scaled_x, scaled_y;
	byte *data = pic;

	// normals are always swizzled
	if (image->type == IMGTYPE_NORMAL || image->type == IMGTYPE_NORMALHEIGHT)
	{
		RawImage_SwizzleRA(pic, width, height);
	}

	// LATC2 is only used for normals
	if (image->internalFormat == GL_COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT)
	{
		byte *in = data;
		int c = width * height;
		while (c--)
		{
			in[0] = in[1];
			in[2] = in[1];
			in += 4;
		}
	}


	RawImage_ScaleToPower2(&pic, &width, &height, &scaled_width, &scaled_height, image->type, image->flags, &resampledBuffer);

	scaledBuffer = (byte *)ri.Hunk_AllocateTempMemory( sizeof( unsigned ) * scaled_width * scaled_height );

	GL_SelectTexture( image->TMU );
	GL_Bind(image);

	// copy or resample data as appropriate for first MIP level
	if ( ( scaled_width == width ) &&
		( scaled_height == height ) ) {
		if (!(image->flags & IMGFLAG_MIPMAP))
		{
			scaled_x = x * scaled_width / width;
			scaled_y = y * scaled_height / height;
			RawImage_UploadTexture( data, scaled_x, scaled_y, scaled_width, scaled_height, image->internalFormat, image->type, image->flags, qtrue );
			//qglTexSubImage2D( GL_TEXTURE_2D, 0, scaled_x, scaled_y, scaled_width, scaled_height, GL_RGBA, GL_UNSIGNED_BYTE, data );

			GL_CheckErrors();
			goto done;
		}
		Com_Memcpy (scaledBuffer, data, width*height*4);
	}
	else if ( !r_simpleMipMaps->integer )
	{
		// use the normal mip-mapping function to go down from here
		while ( width > scaled_width || height > scaled_height ) {

			if (image->flags & IMGFLAG_SRGB)
			{
				R_MipMapsRGB( (byte *)data, width, height );
			}
			else
			{
				R_MipMap( (byte *)data, width, height );
			}

			width >>= 1;
			height >>= 1;
			x >>= 1;
			y >>= 1;
			if ( width < 1 ) {
				width = 1;
			}
			if ( height < 1 ) {
				height = 1;
			}
		}
		Com_Memcpy( scaledBuffer, data, width * height * 4 );
	}
	else if ( !r_simpleMipMaps->integer )
	{
		qglGenerateMipmap(GL_TEXTURE_2D);
	}

	if (!(image->flags & IMGFLAG_NOLIGHTSCALE))
		R_LightScaleTexture (scaledBuffer, scaled_width, scaled_height, (qboolean)(!(image->flags & IMGFLAG_MIPMAP)) );

	scaled_x = x * scaled_width / width;
	scaled_y = y * scaled_height / height;
	RawImage_UploadTexture( (byte *)data, scaled_x, scaled_y, scaled_width, scaled_height, image->internalFormat, image->type, image->flags, qtrue );

done:

	GL_SelectTexture( 0 );

	GL_CheckErrors();

	if ( scaledBuffer != 0 )
		ri.Hunk_FreeTempMemory( scaledBuffer );
	if ( resampledBuffer != 0 )
		ri.Hunk_FreeTempMemory( resampledBuffer );
}

image_t* R_GetLoadedImage(const char *name, int flags) {
	long	hash;
	image_t	*image;

	hash = generateHashValue(name);
	for (image = hashTable[hash]; image; image = image->next) {
		if (!strcmp(name, image->imgName)) {
			// the white image can be used with any set of parms, but other mismatches are errors
			if (strcmp(name, "*white")) {
				if (image->flags != flags) {
					ri.Printf(PRINT_DEVELOPER, "WARNING: reused image %s with mixed flags (%i vs %i)\n", name, image->flags, flags);
				}
			}
			return image;
		}
	}
	return NULL;
}

void R_LoadPackedMaterialImage(shaderStage_t *stage, const char *packedImageName, int flags)
{
	char	packedName[MAX_QPATH];
	int		packedWidth, packedHeight;
	byte	*packedPic;
	image_t *image;

	if (!packedImageName) {
		return;
	}

	float baseSpecularScale = 1.0f;
	switch (stage->specularType)
	{
	case SPEC_RMOS:
	case SPEC_MOSR:
	case SPEC_ORMS:
		// Don't scale base specular
		break;
	default:
		baseSpecularScale = 0.5f; // Basespecular is assumed to be 0.04 and shader assumes 0.08
		break;
	}

	COM_StripExtension(packedImageName, packedName, sizeof(packedName));
	Q_strcat(packedName, sizeof(packedName), "_ORMS");

	//
	// see if the image is already loaded
	//
	image = R_GetLoadedImage(packedName, flags);
	if (image != NULL)
	{
		// Don't scale occlusion, roughness and metalness
		stage->specularScale[0] =
		stage->specularScale[2] =
		stage->specularScale[3] = 1.0f;
		stage->specularScale[1] = baseSpecularScale;

		stage->bundle[TB_ORMSMAP].image[0] = image;
		return;
	}

	R_LoadImage(packedImageName, &packedPic, &packedWidth, &packedHeight);
	if (packedPic == NULL) {
		return;
	}

	// Don't scale occlusion, roughness and metalness
	stage->specularScale[0] =
	stage->specularScale[2] =
	stage->specularScale[3] = 1.0f;
	stage->specularScale[1] = baseSpecularScale;

	GLint swizzle[4] = { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA };

	switch (stage->specularType)
	{
	case SPEC_RMO:
		swizzle[0] = GL_BLUE;
		swizzle[1] = GL_RED;
		swizzle[2] = GL_GREEN;
		swizzle[3] = GL_ONE;
		break;
	case SPEC_RMOS:
		swizzle[0] = GL_BLUE;
		swizzle[1] = GL_RED;
		swizzle[2] = GL_GREEN;
		swizzle[3] = GL_ALPHA;
		break;
	case SPEC_MOXR:
		swizzle[0] = GL_GREEN;
		swizzle[1] = GL_ALPHA;
		swizzle[2] = GL_RED;
		swizzle[3] = GL_ONE;
		break;
	case SPEC_MOSR:
		swizzle[0] = GL_GREEN;
		swizzle[1] = GL_ALPHA;
		swizzle[2] = GL_RED;
		swizzle[3] = GL_BLUE;
		break;
	case SPEC_ORM:
	case SPEC_ORMS:
	default:
		break;
	}

	stage->bundle[TB_ORMSMAP].image[0] = R_CreateImage(packedName, packedPic, packedWidth, packedHeight, IMGTYPE_COLORALPHA, flags & ~IMGFLAG_SRGB, 0);
	glBindTexture(GL_TEXTURE_2D, stage->bundle[TB_ORMSMAP].image[0]->texnum);
	glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzle);
	Z_Free(packedPic);
}

image_t *R_BuildSDRSpecGlossImage(shaderStage_t *stage, const char *specImageName, int flags)
{
	char	sdrName[MAX_QPATH];
	int		specWidth, specHeight;
	byte	*specPic;
	image_t *image;

	if (!specImageName)
		return NULL;

	COM_StripExtension(specImageName, sdrName, sizeof(sdrName));
	Q_strcat(sdrName, sizeof(sdrName), "_SDR");

	//
	// see if the image is already loaded
	//
	image = R_GetLoadedImage(sdrName, flags);
	if (image != NULL)
		return image;

	R_LoadImage(specImageName, &specPic, &specWidth, &specHeight);
	if (specPic == NULL)
		return NULL;

	byte *sdrSpecPic = (byte *)ri.Hunk_AllocateTempMemory(sizeof(unsigned) * specWidth * specHeight);
	vec3_t currentColor;
	for (int i = 0; i < specWidth * specHeight * 4; i += 4)
	{
		currentColor[0] = ByteToFloat(specPic[i + 0]);
		currentColor[1] = ByteToFloat(specPic[i + 1]);
		currentColor[2] = ByteToFloat(specPic[i + 2]);

		float ratio =
			(sRGBtoRGB(currentColor[0]) + sRGBtoRGB(currentColor[1]) + sRGBtoRGB(currentColor[1])) /
			(currentColor[0] + currentColor[1] + currentColor[2]);

		sdrSpecPic[i + 0] = FloatToByte(currentColor[0] * ratio);
		sdrSpecPic[i + 1] = FloatToByte(currentColor[1] * ratio);
		sdrSpecPic[i + 2] = FloatToByte(currentColor[2] * ratio);
		sdrSpecPic[i + 3] = specPic[i + 3];
	}
	ri.Hunk_FreeTempMemory(specPic);

	return R_CreateImage(sdrName, sdrSpecPic, specWidth, specHeight, IMGTYPE_COLORALPHA, flags & ~IMGFLAG_SRGB, 0);
}

static void R_CreateNormalMap ( const char *name, byte *pic, int width, int height, int flags )
{
	char normalName[MAX_QPATH];
	image_t *normalImage;
	int normalWidth, normalHeight;
	int normalFlags;

	normalFlags = (flags & ~(IMGFLAG_GENNORMALMAP | IMGFLAG_SRGB)) | IMGFLAG_NOLIGHTSCALE;

	COM_StripExtension(name, normalName, sizeof(normalName));
	Q_strcat(normalName, sizeof(normalName), "_n");

	// find normalmap in case it's there
	normalImage = R_FindImageFile(normalName, IMGTYPE_NORMAL, normalFlags);

	// if not, generate it
	if (normalImage == NULL)
	{
		byte *normalPic;
		int x, y;

		normalWidth = width;
		normalHeight = height;
		normalPic = (byte *)Z_Malloc(width * height * 4, TAG_GENERAL);
		RGBAtoNormal(pic, normalPic, width, height, (qboolean)(flags & IMGFLAG_CLAMPTOEDGE));

#if 1
		// Brighten up the original image to work with the normal map
		RGBAtoYCoCgA(pic, pic, width, height);
		for (y = 0; y < height; y++)
		{
			byte *picbyte  = pic       + y * width * 4;
			byte *normbyte = normalPic + y * width * 4;
			for (x = 0; x < width; x++)
			{
				int div = MAX(normbyte[2] - 127, 16);
				picbyte[0] = CLAMP(picbyte[0] * 128 / div, 0, 255);
				picbyte  += 4;
				normbyte += 4;
			}
		}
		YCoCgAtoRGBA(pic, pic, width, height);
#else
		// Blur original image's luma to work with the normal map
		{
			byte *blurPic;

			RGBAtoYCoCgA(pic, pic, width, height);
			blurPic = ri.Malloc(width * height);

			for (y = 1; y < height - 1; y++)
			{
				byte *picbyte  = pic     + y * width * 4;
				byte *blurbyte = blurPic + y * width;

				picbyte += 4;
				blurbyte += 1;

				for (x = 1; x < width - 1; x++)
				{
					int result;

					result = *(picbyte - (width + 1) * 4) + *(picbyte - width * 4) + *(picbyte - (width - 1) * 4) +
					*(picbyte -          1  * 4) + *(picbyte            ) + *(picbyte +          1  * 4) +
					*(picbyte + (width - 1) * 4) + *(picbyte + width * 4) + *(picbyte + (width + 1) * 4);

					result /= 9;

					*blurbyte = result;
					picbyte += 4;
					blurbyte += 1;
				}
			}

			// FIXME: do borders

			for (y = 1; y < height - 1; y++)
			{
				byte *picbyte  = pic     + y * width * 4;
				byte *blurbyte = blurPic + y * width;

				picbyte += 4;
				blurbyte += 1;

				for (x = 1; x < width - 1; x++)
				{
					picbyte[0] = *blurbyte;
					picbyte += 4;
					blurbyte += 1;
				}
			}

			ri.Free(blurPic);

			YCoCgAtoRGBA(pic, pic, width, height);
		}
#endif

		R_CreateImage( normalName, normalPic, normalWidth, normalHeight, IMGTYPE_NORMAL, normalFlags, 0 );
		Z_Free( normalPic );
	}
}

/*
===============
R_FindImageFile

Finds or loads the given image.
Returns NULL if it fails, not a default image.
==============
*/
image_t	*R_FindImageFile( const char *name, imgType_t type, int flags )
{
	image_t	*image;
	int		width, height;
	byte	*pic;
	int internalFormat = 0;
	int loadFlags = flags;

	if (!name) {
		return NULL;
	}

	if ((image = R_GetLoadedImage(name, flags)) != NULL)
		return image;

	//
	// load the pic from disk
	//
	if (r_hdr->integer && (flags & IMGFLAG_HDR || flags & IMGFLAG_HDR_LIGHTMAP))
	{
		char filename[MAX_QPATH];
		Com_sprintf(filename, sizeof(filename), "%s.hdr", name);
		float	*floatBuffer;
		R_LoadHDRImage(filename, &pic, &width, &height);
		if (pic == NULL)
		{
			R_LoadImage(name, &pic, &width, &height);
		}
		else
		{
			for (int i = 0; i < width*height; i++)
			{
				vec4_t color;
				floatBuffer = (float*)pic;
				memcpy(color, &floatBuffer[i*3], 12);
				if (flags & IMGFLAG_HDR_LIGHTMAP)
				{
					color[0] = color[0] / M_PI;
					color[1] = color[1] / M_PI;
					color[2] = color[2] / M_PI;
				}
				color[3] = 1.0f;

				uint16_t *hdr_color = (uint16_t *)(&pic[i * 8]);
				hdr_color[0] = FloatToHalf(color[0]);
				hdr_color[1] = FloatToHalf(color[1]);
				hdr_color[2] = FloatToHalf(color[2]);
				hdr_color[3] = FloatToHalf(1.0f);
			}
			internalFormat = GL_RGBA16F;
			loadFlags = flags & ~(IMGFLAG_GENNORMALMAP | IMGFLAG_MIPMAP);
		}
	}
	else
	{
		R_LoadImage(name, &pic, &width, &height);
	}

	if ( pic == NULL ) {
		return NULL;
	}

	if (r_normalMapping->integer && !(type == IMGTYPE_NORMAL) &&
		(flags & IMGFLAG_PICMIP) && (flags & IMGFLAG_MIPMAP) && (flags & IMGFLAG_GENNORMALMAP))
	{
		R_CreateNormalMap( name, pic, width, height, flags );
	}

	// flip height info, so we don't need to do this in the shader later
	if (type == IMGTYPE_NORMALHEIGHT)
	{
		for (int i = 0; i < width*height; i++)
		{
			pic[4 * i + 3] = 255 - pic[4 * i + 3];
		}
	}

	image = R_CreateImage( name, pic, width, height, type, loadFlags, internalFormat);
	Z_Free( pic );

	return image;
}


/*
================
R_CreateDlightImage
================
*/
#define	DLIGHT_SIZE	16
static void R_CreateDlightImage( void ) {
	int		width, height;
	byte	*pic;

	R_LoadImage("gfx/2d/dlight", &pic, &width, &height);
	if (pic)
	{
		tr.dlightImage = R_CreateImage("*dlight", pic, width, height, IMGTYPE_COLORALPHA, IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE, 0 );
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
		tr.dlightImage = R_CreateImage("*dlight", (byte *)data, DLIGHT_SIZE, DLIGHT_SIZE, IMGTYPE_COLORALPHA, IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE, 0 );
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
	float	d;
	float	borderColor[4];

	data = (byte *)ri.Hunk_AllocateTempMemory( FOG_S * FOG_T * 4 );

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
	tr.fogImage = R_CreateImage("*fog", (byte *)data, FOG_S, FOG_T, IMGTYPE_COLORALPHA, IMGFLAG_CLAMPTOEDGE, 0 );
	ri.Hunk_FreeTempMemory( data );

	borderColor[0] = 1.0;
	borderColor[1] = 1.0;
	borderColor[2] = 1.0;
	borderColor[3] = 1;

	qglTexParameterfv( GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor );
}

/*
================
R_CreateEnvBrdfLUT
based on https://github.com/knarkowicz/IntegrateDFG
================
*/
static void R_CreateEnvBrdfLUT(void) {

	static const int LUT_WIDTH = 128;
	static const int LUT_HEIGHT = 128;

	if (!r_cubeMapping->integer)
		return;

	uint16_t data[LUT_WIDTH][LUT_HEIGHT][3];

	unsigned const numSamples = 1024;

	for (unsigned y = 0; y < LUT_HEIGHT; ++y)
	{
		float const NdotV = (y + 0.5f) / LUT_HEIGHT;
		float const vx = sqrtf(1.0f - NdotV * NdotV);
		float const vy = 0.0f;
		float const vz = NdotV;

		for (unsigned x = 0; x < LUT_WIDTH; ++x)
		{
			float const roughness = (x + 0.5f) / LUT_WIDTH;
			float const m = roughness * roughness;
			float const m2 = m * m;

			float scale = 0.0f;
			float bias = 0.0f;
			float velvet = 0.0f;

			for (unsigned i = 0; i < numSamples; ++i)
			{
				float const e1 = (float)i / numSamples;
				float const e2 = (float)((double)ReverseBits(i) / (double)0x100000000LL);
				float const phi = 2.0f * M_PI * e1;

				// GGX Distribution
				{
					float const cosTheta = sqrtf((1.0f - e2) / (1.0f + (m2 - 1.0f) * e2));
					float const sinTheta = sqrtf(1.0f - cosTheta * cosTheta);

					float const hx = sinTheta * cosf(phi);
					float const hy = sinTheta * sinf(phi);
					float const hz = cosTheta;

					float const vdh = vx * hx + vy * hy + vz * hz;
					float const lz = 2.0f * vdh * hz - vz;

					float const NdotL = MAX(lz, 0.0f);
					float const NdotH = MAX(hz, 0.0f);
					float const VdotH = MAX(vdh, 0.0f);

					if (NdotL > 0.0f)
					{
						float const visibility = GSmithCorrelated(roughness, NdotV, NdotL);
						float const NdotLVisPDF = NdotL * visibility * (4.0f * VdotH / NdotH);
						float const fresnel = powf(1.0f - VdotH, 5.0f);

						scale += NdotLVisPDF * (1.0f - fresnel);
						bias += NdotLVisPDF * fresnel;
					}
				}

				// Charlie Distribution
				{
					float const sinTheta = sqrtf(powf(e2, (2.0f * roughness) / ((2.0f * roughness) + 1.0f)));
					float const cosTheta = sqrtf(1.0f - sinTheta * sinTheta);

					float const hx = sinTheta * cosf(phi);
					float const hy = sinTheta * sinf(phi);
					float const hz = cosTheta;

					float const vdh = vx * hx + vy * hy + vz * hz;
					float const lz = 2.0f * vdh * hz - vz;

					float const NdotL = MAX(lz, 0.0f);
					float const NdotH = MAX(hz, 0.0f);
					float const VdotH = MAX(vdh, 0.0f);

					if (NdotL > 0.0f)
					{
						float const velvetV = V_Neubelt(NdotV, NdotL);
						float const rcp_pdf = 4.0f * VdotH / NdotH;
						velvet += NdotL * velvetV * rcp_pdf;
					}
				}
			}

			scale /= numSamples;
			bias /= numSamples;
			velvet /= numSamples;

			data[y][x][0] = FloatToHalf(scale);
			data[y][x][1] = FloatToHalf(bias);
			data[y][x][2] = FloatToHalf(velvet);
		}
	}

	tr.envBrdfImage = R_CreateImage(
		"*envBrdfLUT",
		(byte*)data,
		LUT_WIDTH,
		LUT_HEIGHT,
		IMGTYPE_COLORALPHA,
		IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE,
		GL_RGB16F);
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
	Com_Memset( data, 32, sizeof( data ) );
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
	tr.defaultImage = R_CreateImage(
		"*default", (byte *)data, DEFAULT_SIZE, DEFAULT_SIZE,
		IMGTYPE_COLORALPHA, IMGFLAG_MIPMAP, GL_RGBA8);
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
	Com_Memset( data, 255, sizeof( data ) );
	tr.whiteImage = R_CreateImage(
		"*white", (byte *)data, 8, 8, IMGTYPE_COLORALPHA, IMGFLAG_NONE,
		GL_RGBA8);

	if (r_dlightMode->integer >= 2)
	{
		tr.pointShadowArrayImage = R_Create2DImageArray(
			va("*pointshadowmapImage"),
			NULL,
			DSHADOW_MAP_SIZE,
			DSHADOW_MAP_SIZE,
			MAX_DLIGHTS*6,
			IMGTYPE_COLORALPHA,
			IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE | IMGLFAG_SHADOWCOMP | IMGFLAG_MUTABLE,
			GL_DEPTH_COMPONENT16);
	}

	// with overbright bits active, we need an image which is some fraction of
	// full color, for default lightmaps, etc
	for (x = 0; x < DEFAULT_SIZE; x++) {
		for (y  =0; y < DEFAULT_SIZE; y++) {
			data[y][x][0] =
			data[y][x][1] =
			data[y][x][2] = tr.identityLightByte;
			data[y][x][3] = 255;
		}
	}

	tr.identityLightImage = R_CreateImage(
		"*identityLight", (byte *)data, 8, 8, IMGTYPE_COLORALPHA,
		IMGFLAG_NONE, 0);

	// scratchimage is usually used for cinematic drawing
	for (x = 0; x < 32; x++) {
		tr.scratchImage[x] = R_CreateImage(
			"*scratch", (byte *)data, DEFAULT_SIZE, DEFAULT_SIZE,
			IMGTYPE_COLORALPHA,
			IMGFLAG_PICMIP | IMGFLAG_CLAMPTOEDGE | IMGFLAG_MUTABLE, 0);
	}

	R_CreateDlightImage();
	R_CreateFogImage();
	R_CreateEnvBrdfLUT();

	int width = glConfig.vidWidth;
	int height = glConfig.vidHeight;

	int hdrFormat = GL_RGBA8;
	if (r_hdr->integer)
		hdrFormat = GL_RGBA16F;

	int rgbFormat = GL_RGBA8;

	tr.renderImage = R_CreateImage(
		"_render", NULL, width, height, IMGTYPE_COLORALPHA,
		IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE, hdrFormat);

	tr.glowImage = R_CreateImage(
		"*glow", NULL, width, height, IMGTYPE_COLORALPHA,
		IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE, hdrFormat);

	int glowImageWidth = width;
	int glowImageHeight = height;
	for (int i = 0; i < ARRAY_LEN(tr.glowImageScaled); i++)
	{
		tr.glowImageScaled[i] = R_CreateImage(
			va("*glowScaled%d", i), NULL, glowImageWidth, glowImageHeight,
			IMGTYPE_COLORALPHA, IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE,
			hdrFormat);

		glowImageWidth = Q_max(1, glowImageWidth >> 1);
		glowImageHeight = Q_max(1, glowImageHeight >> 1);
	}

	if (r_drawSunRays->integer)
		tr.sunRaysImage = R_CreateImage(
			"*sunRays", NULL, width, height, IMGTYPE_COLORALPHA,
			IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE, rgbFormat);

	tr.renderDepthImage  = R_CreateImage(
		"*renderdepth",  NULL, width, height, IMGTYPE_COLORALPHA,
		IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE, GL_DEPTH24_STENCIL8);
	tr.textureDepthImage = R_CreateImage(
		"*texturedepth", NULL, PSHADOW_MAP_SIZE, PSHADOW_MAP_SIZE,
		IMGTYPE_COLORALPHA, IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE,
		GL_DEPTH_COMPONENT24);

	{
		unsigned short sdata[4];
		void *p;

		if (hdrFormat == GL_RGBA16F)
		{
			sdata[0] = FloatToHalf(0.0f);
			sdata[1] = FloatToHalf(0.45f);
			sdata[2] = FloatToHalf(1.0f);
			sdata[3] = FloatToHalf(1.0f);
			p = &sdata[0];
		}
		else
		{
			data[0][0][0] = 0;
			data[0][0][1] = 0.45f * 255;
			data[0][0][2] = 255;
			data[0][0][3] = 255;
			p = data;
		}

		tr.calcLevelsImage = R_CreateImage(
			"*calcLevels", (byte *)p, 1, 1, IMGTYPE_COLORALPHA,
			IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE, hdrFormat);
		tr.targetLevelsImage = R_CreateImage(
			"*targetLevels", (byte *)p, 1, 1, IMGTYPE_COLORALPHA,
			IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE, hdrFormat);
		tr.fixedLevelsImage = R_CreateImage(
			"*fixedLevels", (byte *)p, 1, 1, IMGTYPE_COLORALPHA,
			IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE, hdrFormat);
	}

	for (x = 0; x < 2; x++)
	{
		tr.textureScratchImage[x] = R_CreateImage(
			va("*textureScratch%d", x), NULL, 256, 256, IMGTYPE_COLORALPHA,
			IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE, GL_RGBA8);
	}

	for (x = 0; x < 2; x++)
	{
		tr.quarterImage[x] = R_CreateImage(
			va("*quarter%d", x), NULL, width / 2, height / 2,
			IMGTYPE_COLORALPHA, IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE,
			GL_RGBA8);
	}

	if (r_ssao->integer)
	{
		tr.screenSsaoImage = R_CreateImage(
			"*screenSsao", NULL, width / 2, height / 2, IMGTYPE_COLORALPHA,
			IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE, GL_RGBA8);
		tr.hdrDepthImage = R_CreateImage(
			"*hdrDepth", NULL, width, height, IMGTYPE_COLORALPHA,
			IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE, GL_R32F);
	}

	if (r_shadows->integer == 4)
	{
		tr.pshadowArrayImage = R_Create2DImageArray(
			va("*pshadowmapArray"),
			NULL,
			PSHADOW_MAP_SIZE,
			PSHADOW_MAP_SIZE,
			MAX_DRAWN_PSHADOWS,
			IMGTYPE_COLORALPHA,
			IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE | IMGLFAG_SHADOWCOMP | IMGFLAG_MUTABLE,
			GL_DEPTH_COMPONENT16);
	}

	if (r_sunlightMode->integer)
	{
		tr.sunShadowArrayImage = R_Create2DImageArray(
			va("*sunShadowmapArray"),
			NULL,
			r_shadowMapSize->integer,
			r_shadowMapSize->integer,
			3, // number of cascades
			IMGTYPE_COLORALPHA,
			IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE | IMGLFAG_SHADOWCOMP | IMGFLAG_MUTABLE,
			GL_DEPTH_COMPONENT16);

		tr.screenShadowImage = R_CreateImage(
			"*screenShadow", NULL, width, height, IMGTYPE_COLORALPHA,
			IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE, GL_R8);
	}

	if (r_cubeMapping->integer)
	{
		tr.renderCubeImage = R_CreateImage(
			"*renderCube", NULL, CUBE_MAP_SIZE, CUBE_MAP_SIZE,
			IMGTYPE_COLORALPHA,
			IMGFLAG_NO_COMPRESSION |
			IMGFLAG_CLAMPTOEDGE |
			IMGFLAG_MIPMAP |
			IMGFLAG_CUBEMAP,
			hdrFormat);

		tr.renderCubeDepthImage = R_CreateImage(
			"*renderdepth", NULL, CUBE_MAP_SIZE, CUBE_MAP_SIZE,
			IMGTYPE_COLORALPHA,
			IMGFLAG_NO_COMPRESSION |
			IMGFLAG_CLAMPTOEDGE,
			GL_DEPTH24_STENCIL8);
	}

	tr.weatherDepthImage = R_CreateImage(
		"*weatherDepth",
		nullptr,
		1024,
		1024,
		IMGTYPE_COLORALPHA,
		IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE,
		GL_DEPTH_COMPONENT16);
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

	// setup the overbright lighting
	tr.overbrightBits = r_overBrightBits->integer;

	// allow 2 overbright bits
	if ( tr.overbrightBits > 2 ) {
		tr.overbrightBits = 2;
	} else if ( tr.overbrightBits < 0 ) {
		tr.overbrightBits = 0;
	}

	tr.identityLight = 1.0f / ( 1 << tr.overbrightBits );
	tr.identityLightByte = 255 * tr.identityLight;

	if ( r_intensity->value <= 1 ) {
		ri.Cvar_Set( "r_intensity", "1" );
	}

	if ( r_gamma->value < 0.5f ) {
		ri.Cvar_Set( "r_gamma", "0.5" );
	} else if ( r_gamma->value > 3.0f ) {
		ri.Cvar_Set( "r_gamma", "3.0" );
	}

	g = r_gamma->value;

	for ( i = 0; i < 256; i++ ) {
		if ( g == 1 ) {
			inf = i;
		} else {
			inf = 255 * pow ( i/255.0f, 1.0f / g ) + 0.5f;
		}
		inf <<= tr.overbrightBits;
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
		ri.WIN_SetGamma( &glConfig, s_gammatable, s_gammatable, s_gammatable );
	}
}

/*
Initialise the images pool allocator
*/
void R_InitImagesPool()
{
	Com_Memset(hashTable, 0, sizeof(hashTable));

	imagesPool = NULL;
	tr.imagesFreeList = NULL;
	R_ExtendImagesPool();
}

/*
===============
R_InitImages
===============
*/
void R_InitImages( void ) {
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
void R_DeleteTextures( void ) {
	image_t *image = tr.images;
	while ( image )
	{
		qglDeleteTextures(1, &image->texnum);
		image = image->poolNext;
	}

	tr.numImages = 0;

	// Free pool and allocated images
	while ( imagesPool )
	{
		ImagesPool *pNext = imagesPool->pNext;
		Z_Free(imagesPool->pPool);
		Z_Free(imagesPool);

		imagesPool = pNext;
	}

	Com_Memset( glState.currenttextures, 0, sizeof( glState.currenttextures ) );
	GL_SelectTexture( 1 );
	qglBindTexture( GL_TEXTURE_2D, 0 );
	GL_SelectTexture( 0 );
	qglBindTexture( GL_TEXTURE_2D, 0 );
}
