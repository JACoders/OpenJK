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

// My TGA loader...
//
//---------------------------------------------------
#pragma pack(push,1)
typedef struct TGAHeader_s {
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

	pHeader->wColourMapLength = LittleShort(pHeader->wColourMapLength);
	pHeader->wImageWidth = LittleShort(pHeader->wImageWidth);
	pHeader->wImageHeight = LittleShort(pHeader->wImageHeight);

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

