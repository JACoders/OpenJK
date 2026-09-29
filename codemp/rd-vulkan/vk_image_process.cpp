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

#include "tr_local.h"

void R_SetColorMappings( void)
{
    int		i, j;
    int		inf;
    int		shift = 0;
    float   g;
    qboolean applyGamma;

    if ( !tr.inited ) {
        // it may be called from window handling functions where gamma flags is now yet known/set
        return;
    }

    // setup the overbright lighting
    // negative value will force gamma in windowed mode
    tr.overbrightBits = abs(r_overBrightBits->integer);


	// never overbright in windowed mode
	if ( !glConfig.isFullscreen && r_overBrightBits->integer >= 0 && !vk.fboActive ) {
		tr.overbrightBits = 0;
		applyGamma = qfalse;
	} else {
		if ( !glConfig.deviceSupportsGamma && !vk.fboActive ) {
			tr.overbrightBits = 0; // need hardware gamma for overbright
			applyGamma = qfalse;
		} else {
			applyGamma = qtrue;
		}
	}

    // clear
    for (i = 0; i < 255; i++)
    {
        s_intensitytable[i] = s_gammatable[i] = i;
    }

    // allow 2 overbright bits in 24 bit, but only 1 in 16 bit
    if (glConfig.colorBits > 16) {
        if (tr.overbrightBits > 2)
            tr.overbrightBits = 2;
    }
    else {
        if (tr.overbrightBits > 1)
            tr.overbrightBits = 1;
    }
    if (tr.overbrightBits < 0)
        tr.overbrightBits = 0;

    tr.identityLight = 1.0f / (1 << tr.overbrightBits);
    tr.identityLightByte = 255 * tr.identityLight;

    if (r_intensity->value < 1.0f) {
        ri.Cvar_Set("r_intensity", "1");
    }

    if (r_gamma->value < 0.5f) {
        ri.Cvar_Set("r_gamma", "0.5");
    }
    else if (r_gamma->value > 3.0f) {
        ri.Cvar_Set("r_gamma", "3.0");
    }

    g = r_gamma->value;

    shift = tr.overbrightBits;

    for (i = 0; i < ARRAY_LEN(s_gammatable); i++) {
        if (g == 1.0f) {
            inf = i;
        }
        else {
            inf = 255 * pow(i / 255.0f, 1.0f / g) + 0.5f;
        }
        inf <<= shift;
        if (inf < 0) {
            inf = 0;
        }
        else if (inf > 255) {
            inf = 255;
        }
        s_gammatable[i] = inf;
    }

    for (i = 0; i < ARRAY_LEN(s_intensitytable); i++)
    {
        j = i * r_intensity->value;
        if (j > 255) {
            j = 255;
        }
        s_intensitytable[i] = j;
    }

    if (glConfig.deviceSupportsGamma) {
        if (vk.fboActive)
            ri.WIN_SetGamma(&glConfig, s_gammatable_linear, s_gammatable_linear, s_gammatable_linear);
        else {
            if ( applyGamma ) {
                ri.WIN_SetGamma(&glConfig, s_gammatable, s_gammatable, s_gammatable);
            }
        }
    }
}

/*
================
R_LightScaleTexture

Scale up the pixel values in a texture to increase the
lighting range
================
*/
void R_LightScaleTexture( byte *in, int inwidth, int inheight, qboolean only_gamma )
{
    if (in == NULL)
        return;

    if (only_gamma)
    {
        if (!glConfig.deviceSupportsGamma && !vk.fboActive)
        {
            int		i, c;
            byte    *p;

            p = (byte*)in;

            c = inwidth * inheight;
            for (i = 0; i < c; i++, p += 4)
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
        byte    *p;

        p = (byte*)in;

        c = inwidth * inheight;

        if (glConfig.deviceSupportsGamma || vk.fboActive)
        {
            for (i = 0; i < c; i++, p += 4)
            {
                p[0] = s_intensitytable[p[0]];
                p[1] = s_intensitytable[p[1]];
                p[2] = s_intensitytable[p[2]];
            }
        }
        else
        {
            for (i = 0; i < c; i++, p += 4)
            {
                p[0] = s_gammatable[s_intensitytable[p[0]]];
                p[1] = s_gammatable[s_intensitytable[p[1]]];
                p[2] = s_gammatable[s_intensitytable[p[2]]];
            }
        }
    }
}

/*
==================
R_BlendOverTexture

Apply a color blend over a set of pixels
==================
*/
void R_BlendOverTexture( unsigned char *data, const uint32_t pixelCount, uint32_t l )
{
    uint32_t i;

    static const unsigned char mipBlendColors[16][4] =
    {
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

    const unsigned int alpha = mipBlendColors[l][3];
    const unsigned int inverseAlpha = 255 - alpha;

    const unsigned int bR = mipBlendColors[l][0] * alpha;
    const unsigned int bG = mipBlendColors[l][1] * alpha;
    const unsigned int bB = mipBlendColors[l][2] * alpha;

    for (i = 0; i < pixelCount; i++, data += 4)
    {
        data[0] = (data[0] * inverseAlpha + bR) >> 9;
        data[1] = (data[1] * inverseAlpha + bG) >> 9;
        data[2] = (data[2] * inverseAlpha + bB) >> 9;
    }
}

/*
================
MIP maps

In computer graphics, mipmaps (also MIP maps) or pyramids are pre-calculated,
optimized sequences of images, each of which is a progressively lower resolution
representation of the same image. The height and width of each image, or level, 
in the mipmap is a power of two smaller than the previous level. 
Mipmaps do not have to be square. They are intended to increase rendering speed
and reduce aliasing artifacts.
A high-resolution mipmap image is used for high-density samples, such as for 
objects close to the camera. Lower-resolution images are used as the object
appears farther away.
This is a more efficient way of downfiltering (minifying) a texture than
sampling all texels in the original texture that would contribute to a 
screen pixel; it is faster to take a constant number of samples from the
appropriately downfiltered textures. Mipmaps are widely used in 3D computer games. 

The letters "MIP" in the name are an acronym of the Latin phrase multum in parvo, 
meaning "much in little".Since mipmaps, by definition, are pre-allocated, 
additional storage space is required to take advantage of them. 
Mipmap textures are used in 3D scenes to decrease the time required to 
render a scene. They also improve the scene's realism.
================
*/
void R_MipMap( byte *out, byte *in, int width, int height ) {
    int		i, j;
    int		row;

    if (in == NULL)
        return;

    if (!r_simpleMipMaps->integer) {
        R_MipMap2((unsigned*)out, (unsigned*)in, width, height);
        return;
    }

    if (width == 1 && height == 1) {
        return;
    }

    row = width * 4;
    width >>= 1;
    height >>= 1;

    if (width == 0 || height == 0) {
        width += height;	// get largest
        for (i = 0; i < width; i++, out += 4, in += 8) {
            out[0] = (in[0] + in[4]) >> 1;
            out[1] = (in[1] + in[5]) >> 1;
            out[2] = (in[2] + in[6]) >> 1;
            out[3] = (in[3] + in[7]) >> 1;
        }
        return;
    }

    for (i = 0; i < height; i++, in += row) {
        for (j = 0; j < width; j++, out += 4, in += 8) {
            out[0] = (in[0] + in[4] + in[row + 0] + in[row + 4]) >> 2;
            out[1] = (in[1] + in[5] + in[row + 1] + in[row + 5]) >> 2;
            out[2] = (in[2] + in[6] + in[row + 2] + in[row + 6]) >> 2;
            out[3] = (in[3] + in[7] + in[row + 3] + in[row + 7]) >> 2;
        }
    }
}

void R_MipMap2( unsigned * const out, unsigned * const in, int inWidth, int inHeight ) {
    int			i, j, k;
    byte        *outpix;
    int			inWidthMask, inHeightMask;
    int			total;
    int			outWidth, outHeight;
    unsigned    *temp;

    outWidth = inWidth >> 1;
    outHeight = inHeight >> 1;

    if (out == in)
        temp = (unsigned int*)ri.Hunk_AllocateTempMemory(outWidth * outHeight * 4);
    else
        temp = out;

    inWidthMask = inWidth - 1;
    inHeightMask = inHeight - 1;

    for (i = 0; i < outHeight; i++) {
        for (j = 0; j < outWidth; j++) {
            outpix = (byte*)(temp + i * outWidth + j);
            for (k = 0; k < 4; k++) {
                total =
                    1 * ((byte*)&in[((i * 2 - 1) & inHeightMask) * inWidth + ((j * 2 - 1) & inWidthMask)])[k] +
                    2 * ((byte*)&in[((i * 2 - 1) & inHeightMask) * inWidth + ((j * 2) & inWidthMask)])[k] +
                    2 * ((byte*)&in[((i * 2 - 1) & inHeightMask) * inWidth + ((j * 2 + 1) & inWidthMask)])[k] +
                    1 * ((byte*)&in[((i * 2 - 1) & inHeightMask) * inWidth + ((j * 2 + 2) & inWidthMask)])[k] +

                    2 * ((byte*)&in[((i * 2) & inHeightMask) * inWidth + ((j * 2 - 1) & inWidthMask)])[k] +
                    4 * ((byte*)&in[((i * 2) & inHeightMask) * inWidth + ((j * 2) & inWidthMask)])[k] +
                    4 * ((byte*)&in[((i * 2) & inHeightMask) * inWidth + ((j * 2 + 1) & inWidthMask)])[k] +
                    2 * ((byte*)&in[((i * 2) & inHeightMask) * inWidth + ((j * 2 + 2) & inWidthMask)])[k] +

                    2 * ((byte*)&in[((i * 2 + 1) & inHeightMask) * inWidth + ((j * 2 - 1) & inWidthMask)])[k] +
                    4 * ((byte*)&in[((i * 2 + 1) & inHeightMask) * inWidth + ((j * 2) & inWidthMask)])[k] +
                    4 * ((byte*)&in[((i * 2 + 1) & inHeightMask) * inWidth + ((j * 2 + 1) & inWidthMask)])[k] +
                    2 * ((byte*)&in[((i * 2 + 1) & inHeightMask) * inWidth + ((j * 2 + 2) & inWidthMask)])[k] +

                    1 * ((byte*)&in[((i * 2 + 2) & inHeightMask) * inWidth + ((j * 2 - 1) & inWidthMask)])[k] +
                    2 * ((byte*)&in[((i * 2 + 2) & inHeightMask) * inWidth + ((j * 2) & inWidthMask)])[k] +
                    2 * ((byte*)&in[((i * 2 + 2) & inHeightMask) * inWidth + ((j * 2 + 1) & inWidthMask)])[k] +
                    1 * ((byte*)&in[((i * 2 + 2) & inHeightMask) * inWidth + ((j * 2 + 2) & inWidthMask)])[k];
                outpix[k] = total / 36;
            }
        }
    }

    if (out == in) {
        Com_Memcpy(out, temp, outWidth * outHeight * 4);
        ri.Hunk_FreeTempMemory(temp);
    }
}

/*
================

Used to resample images in a more general than quartering fashion.

This will only be filtered properly if the resampled size
is greater than half the original size.

If a larger shrinking is needed, use the mipmap function before or after.
================
*/
void ResampleTexture( unsigned *in, int inwidth, int inheight, unsigned *out,
    int outwidth, int outheight ) {
    int		i, j;
    unsigned* inrow, * inrow2;
    unsigned	frac, fracstep;
    unsigned	p1[2048], p2[2048];
    byte* pix1, * pix2, * pix3, * pix4;

    if (outwidth > 2048)
        ri.Error(ERR_DROP, "ResampleTexture: max width");

    fracstep = inwidth * 0x10000 / outwidth;

    frac = fracstep >> 2;
    for (i = 0; i < outwidth; i++) {
        p1[i] = 4 * (frac >> 16);
        frac += fracstep;
    }
    frac = 3 * (fracstep >> 2);
    for (i = 0; i < outwidth; i++) {
        p2[i] = 4 * (frac >> 16);
        frac += fracstep;
    }

    for (i = 0; i < outheight; i++, out += outwidth) {
        inrow = in + inwidth * (int)((i + 0.25) * inheight / outheight);
        inrow2 = in + inwidth * (int)((i + 0.75) * inheight / outheight);
        for (j = 0; j < outwidth; j++) {
            pix1 = (byte*)inrow + p1[j];
            pix2 = (byte*)inrow + p2[j];
            pix3 = (byte*)inrow2 + p1[j];
            pix4 = (byte*)inrow2 + p2[j];
            ((byte*)(out + j))[0] = (pix1[0] + pix2[0] + pix3[0] + pix4[0]) >> 2;
            ((byte*)(out + j))[1] = (pix1[1] + pix2[1] + pix3[1] + pix4[1]) >> 2;
            ((byte*)(out + j))[2] = (pix1[2] + pix2[2] + pix3[2] + pix4[2]) >> 2;
            ((byte*)(out + j))[3] = (pix1[3] + pix2[3] + pix3[3] + pix4[3]) >> 2;
        }
    }
}