// Bootleg TGA support, stolen from Q3

#include <windows.h>
#include <string.h>
#include <assert.h>
#include <cstdio>

typedef unsigned char byte;
typedef unsigned short word;

// Prints an error message, and aborts the program
void Error(const char *msg)
{
	fprintf(stderr, "ERROR: %s\n", msg);
	exit(0);
}

/*
================
R_MipMap2

Uses temp mem, but then copies back to input, quartering the size of the texture
Proper linear filter
================
*/
void R_MipMap2( unsigned *in, int inWidth, int inHeight )
{
	int			i, j, k;
	byte		*outpix;
	int			inWidthMask, inHeightMask;
	int			total;
	int			outWidth, outHeight;
	unsigned	*temp;

	outWidth = inWidth >> 1;
	outHeight = inHeight >> 1;
	temp = new unsigned[outWidth*outHeight*4];

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
	delete [] temp;
}

/*
================
R_MipMap

Operates in place, quartering the size of the texture
================
*/
void R_MipMap (byte *in, int width, int height, bool simple)
{
	int		i, j;
	byte	*out;
	int		row;

	if ( width == 1 && height == 1 ) {
		return;
	}

	if ( !simple ) {
		R_MipMap2( (unsigned *)in, width, height );
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
	// these don't need to be declared or initialised until later, but the compiler whines that 'goto' skips them.
	//
	byte *pRGBA = NULL;	
	byte *pOut	= NULL;
	byte *pIn	= NULL;

	*pic = NULL;

#define TGA_FORMAT_ERROR(blah) Error(blah)

	//
	// load the file
	//
	byte *pTempLoadedBuffer = 0;
	HANDLE hnd = CreateFile(name, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hnd == INVALID_HANDLE_VALUE)
		return;
	DWORD dwSize = GetFileSize(hnd, NULL);
	if (dwSize == INVALID_FILE_SIZE)
	{
		CloseHandle(hnd);
		return;
	}
	pTempLoadedBuffer = new byte[dwSize];
	DWORD dwRead;
	ReadFile(hnd, pTempLoadedBuffer, dwSize, &dwRead, NULL);
	CloseHandle(hnd);
	if (dwRead != dwSize)
	{
		delete [] pTempLoadedBuffer;
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

	pRGBA	= new byte[pHeader->wImageWidth * pHeader->wImageHeight * 4];
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

	delete [] pTempLoadedBuffer;
}

void SaveTGA( byte *pic, int width, int height, char *fileName )
{
	TGAHeader_t header;
	memset (&header, 0, sizeof(header));
	header.byImageType = 2;	// Uncompressed
	header.wImageHeight = height;
	header.wImageWidth = width;
	header.byImagePlanes = 32;	// BPP
	header.byScanLineOrder = 0x20;

	// swap rgb to bgr
	byte *buffer = pic;
	byte temp;
	for (int i = 0; i < width * height * 4; i += 4)
	{
		temp = buffer[i];
		buffer[i] = buffer[i+2];
		buffer[i+2] = temp;
	}

	HANDLE hnd = CreateFile(fileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	DWORD dwWritten;
	if (hnd == INVALID_HANDLE_VALUE)
		return;
	WriteFile(hnd, &header, sizeof(header), &dwWritten, NULL);
	if (dwWritten != sizeof(header))
	{
		printf("ERROR: Couldn't write to file %s\n", fileName);
		CloseHandle(hnd);
		return;
	}
	WriteFile(hnd, pic, width * height * 4, &dwWritten, NULL);
	CloseHandle(hnd);
}
