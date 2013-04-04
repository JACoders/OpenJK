// Generic PNG file loading code

// leave this as first line for PCH reasons...
//
//#include "../game/q_shared.h"
//#include "../qcommon/qcommon.h"

#include "zip.h"
#include "png.h"
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <stdio.h>
//#include "../qcommon/memory.h"

static int BigLong(int l)
{
	byte    b1,b2,b3,b4;

	b1 = l&255;
	b2 = (l>>8)&255;
	b3 = (l>>16)&255;
	b4 = (l>>24)&255;

	return ((int)b1<<24) + ((int)b2<<16) + ((int)b3<<8) + b4;
}

// Error returns

#define PNG_ERROR_OK			0
#define PNG_ERROR_DECOMP		1
#define PNG_ERROR_COMP			2
#define PNG_ERROR_MEMORY		3
#define PNG_ERROR_NOSIG	   		4
#define PNG_ERROR_TOO_SMALL	   	5
#define PNG_ERROR_WNP2	   		6
#define PNG_ERROR_HNP2	   		7
#define PNG_ERROR_NOT_TC	   	8
#define PNG_ERROR_INV_FIL	   	9
#define PNG_ERROR_FAILED_CRC	10
#define PNG_ERROR_CREATE_FAIL	11
#define PNG_ERROR_WRITE			12
#define PNG_ERROR_NOT_PALETTE	13
#define PNG_ERROR_NOT8BIT		14
#define PNG_ERROR_TOO_LARGE		15

static int png_error = PNG_ERROR_OK;

static const byte png_signature[8] = { 0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a };
static const char png_copyright[] = "Copyright\0Raven Software Inc. 2001";
static const char *png_errors[] = 
{
	"OK.",
	"Error decompressing image data.",
	"Error compressing image data.",
	"Error allocating memory.",
	"PNG signature not found.",
	"Image is too small to load.",
	"Width is not a power of two.",
	"Height is not a power of two.",
	"Image is not 24 or 32 bit.",
	"Invalid filter or compression type.",
	"Failed CRC check.",
	"Could not create file.",
	"Error writing to file.",
	"Image is not indexed colour.",
	"Image does not have 8 bits per sample.",
	"Image is too large",
};

// Gets the error string for a failed PNG operation

const char *PNG_GetError(void)
{
	return(png_errors[png_error]);
}

// Create a header chunk

void PNG_CreateHeader(png_ihdr_t *header, int width, int height, int bytedepth)
{
	header->width = BigLong(width);
	header->height = BigLong(height);
	header->bitdepth = 8;

	if(bytedepth == 3)
	{
		header->colortype = 2;
	}
	if(bytedepth == 4)
	{
		header->colortype = 6;
	}
	header->compression = 0;
	header->filter = 0;
	header->interlace = 0;
}

// Processes the header chunk and checks to see if all the data is valid

bool PNG_HandleIHDR(const byte *data, png_image_t *image)
{
	png_ihdr_t *ihdr = (png_ihdr_t *)data;

	image->width = BigLong(ihdr->width);
	image->height = BigLong(ihdr->height);

	// Make sure image is a reasonable size
	if((image->width < 2) || (image->height < 2))
	{
		png_error = PNG_ERROR_TOO_SMALL;
		return(false);
	}
	if(image->width > MAX_PNG_WIDTH)
	{
		png_error = PNG_ERROR_TOO_LARGE;
		return(false);
	}
	if(ihdr->bitdepth != 8)
	{
		png_error = PNG_ERROR_NOT8BIT;
		return(false);
	}
	// Check for non power of two size (but not for data files)
	if(image->isimage)
	{
		if(image->width & (image->width - 1))
		{
			png_error = PNG_ERROR_WNP2;
			return(false);
		}
		if(image->height & (image->height - 1))
		{
			png_error = PNG_ERROR_HNP2;
			return(false);
		}
	}
	// Make sure we have a 24 or 32 bit image (for images)
	if(image->isimage)
	{
		if((ihdr->colortype != 2) && (ihdr->colortype != 6))
		{
			png_error = PNG_ERROR_NOT_TC;
			return(false);
		}
	}
	// Make sure we have an 8 bit grayscale image for data files
	if(!image->isimage)
	{
		if(ihdr->colortype && (ihdr->colortype != 3))
		{
			png_error = PNG_ERROR_NOT_PALETTE;
			return(false);
		}
	}
	// Make sure we aren't using any wacky compression or filter algos
	if(ihdr->compression || ihdr->filter)
	{
		png_error = PNG_ERROR_INV_FIL;
		return(false);
	}
	// Extract the data we need
	if(!ihdr->colortype || (ihdr->colortype == 3))
	{
		image->bytedepth = 1;
	}
	if(ihdr->colortype == 2)
	{
		image->bytedepth = 3;
	}
	if(ihdr->colortype == 6)
	{
		image->bytedepth = 4;
	}
	return(true);
}

// Filter a row of data

void PNG_Filter(byte *out, byte filter, const byte *in, const byte *lastline, ulong rowbytes, ulong bpp)
{
	ulong		i;

	switch(filter)
	{
	case PNG_FILTER_VALUE_NONE:
		memcpy(out, in, rowbytes);
		break;
	case PNG_FILTER_VALUE_SUB:
		for(i = 0; i < bpp; i++)
		{
			*out++ = *in++;
		}
		for(i = bpp; i < rowbytes; i++)
		{
			*out++ = *in - *(in - bpp);
			in++;
		}
		break;
	case PNG_FILTER_VALUE_UP:
		for(i = 0; i < rowbytes; i++)
		{
			if(lastline)
			{
				*out++ = *in++ - *lastline++;
			}
			else
			{
				*out++ = *in++;
			}
		}
		break;
	case PNG_FILTER_VALUE_AVG:
		for(i = 0; i < bpp; i++)
		{
			if(lastline)
			{
				*out++ = *in++ - (*lastline++ >> 1);
			}
			else
			{
				*out++ = *in++;
			}
		}
		for(i = bpp; i < rowbytes; i++)
		{
			if(lastline)
			{
				*out++ = *in - ((*lastline++ + *(in - bpp)) >> 1);
			}
			else
			{
				*out++ = *in - (*(in - bpp) >> 1);
			}
			in++;
		}
		break;
	case PNG_FILTER_VALUE_PAETH:
		int			a, b, c;
		int			pa, pb, pc, p;

		for(i = 0; i < bpp; i++)
		{
			if(lastline)
			{
				*out++ = *in++ - *lastline++;
			}
			else
			{
				*out++ = *in++;
			}
		}
		for(i = bpp; i < rowbytes; i++)
		{
			a = *(in - bpp);
			c = 0;
			b = 0;
			if(lastline)
			{
				c = *(lastline - bpp);
				b = *lastline++;
			}

			p = b - c;
			pc = a - c;

			pa = p < 0 ? -p : p;
			pb = pc < 0 ? -pc : pc;
			pc = (p + pc) < 0 ? -(p + pc) : p + pc;

			p = (pa <= pb && pa <= pc) ? a : (pb <= pc) ? b : c;

			*out++ = *in++ - p;
		}
		break;
	}
}

// Unfilters a row of data

void PNG_Unfilter(byte *out, byte filter, const byte *lastline, ulong rowbytes, ulong bpp)
{
	ulong	i;

	switch(filter)
	{
	case PNG_FILTER_VALUE_NONE:
		break;
	case PNG_FILTER_VALUE_SUB:
		out += bpp;
		for(i = bpp; i < rowbytes; i++)
		{
			*out += *(out - bpp);
			out++;
		}
		break;
	case PNG_FILTER_VALUE_UP:   
		for(i = 0; i < rowbytes; i++)
		{
			if(lastline)
			{
				*out += *lastline++;
			}
			out++;
		}
		break;
	case PNG_FILTER_VALUE_AVG:  
		for(i = 0; i < bpp; i++)
		{
			if(lastline)
			{
				*out += *lastline++ >> 1;
			}
			out++;
		}
		for(i = bpp; i < rowbytes; i++)
		{
			if(lastline)
			{
				*out += (*lastline++ + *(out - bpp)) >> 1;
			}
			else
			{
				*out += *(out - bpp) >> 1;
			}
			out++;
		}
		break;
	case PNG_FILTER_VALUE_PAETH:
		int			a, b, c;
		int			pa, pb, pc, p;

		for(i = 0; i < bpp; i++)
		{
			if(lastline)
			{
				*out += *lastline++;
			}
			out++;
		}
		for(i = bpp; i < rowbytes; i++)
		{
			a = *(out - bpp);
			c = 0;
			b = 0;
			if(lastline)
			{
				c = *(lastline - bpp);
				b = *lastline++;
			}
			p = b - c;
			pc = a - c;

			pa = p < 0 ? -p : p;
			pb = pc < 0 ? -pc : pc;
			pc = (p + pc) < 0 ? -(p + pc) : p + pc;

			p = (pa <= pb && pa <= pc) ? a : (pb <= pc) ? b : c;

			*out++ += p;
		}
		break;
	default:
		break;
	}
}

// Pack up the image data line by line

bool PNG_Pack(byte *out, ulong *size, ulong maxsize, byte *data, int width, int height, int bytedepth)
{
	z_stream		zdata;
	ulong			rowbytes;
	ulong			y;
	const byte		*lastline, *source;
	// Storage for filter type and filtered row
	byte			workline[(MAX_PNG_WIDTH * MAX_PNG_DEPTH) + 1];

	// Number of bytes per row
	rowbytes = width * bytedepth;

	memset(&zdata, 0, sizeof(z_stream));
	if(deflateInit(&zdata, Z_FAST_COMPRESSION_HIGH) != Z_OK)
	{
		png_error = PNG_ERROR_COMP;
		return(false);
	}

	zdata.next_out = out;
	zdata.avail_out = maxsize;

	lastline = NULL;
	source = data + ((height - 1) * rowbytes);
	for(y = 0; y < height; y++)
	{
		// Refilter using the most compressable filter algo
		// Assume paeth to speed things up
		workline[0] = (byte)PNG_FILTER_VALUE_PAETH;
		PNG_Filter(workline + 1, (byte)PNG_FILTER_VALUE_PAETH, source, lastline, rowbytes, bytedepth);

		zdata.next_in = workline;
		zdata.avail_in = rowbytes + 1;
		if(deflate(&zdata, Z_SYNC_FLUSH) != Z_OK)
		{
			deflateEnd(&zdata);
			png_error = PNG_ERROR_COMP;
			return(false);
		}
		lastline = source;
		source -= rowbytes;
	}
	if(deflate(&zdata, Z_FINISH) != Z_STREAM_END)
	{
		png_error = PNG_ERROR_COMP;
		return(false);
	}
	*size = zdata.total_out;
	deflateEnd(&zdata);
	return(true);
}

// Unpack the image data, line by line

bool PNG_Unpack(const byte *data, const ulong datasize, png_image_t *image)
{
	ulong		rowbytes, zerror, y;
	byte		filter;
	z_stream	zdata;
	byte		*lastline, *out;

//	MD_PushTag(TAG_ZIP_TEMP);

	memset(&zdata, 0, sizeof(z_stream));
	if(inflateInit(&zdata, Z_SYNC_FLUSH) != Z_OK)
	{
		png_error = PNG_ERROR_DECOMP;
//		MD_PopTag();
		return(false);
	}
	zdata.next_in = (byte *)data;
	zdata.avail_in = datasize;

	rowbytes = image->width * image->bytedepth;

	lastline = NULL;
	out = image->data;
	for(y = 0; y < image->height; y++)
	{
		// Inflate a row of data
		zdata.next_out = &filter;
		zdata.avail_out = 1;
		if(inflate(&zdata) != Z_OK)
		{
			inflateEnd(&zdata);
			png_error = PNG_ERROR_DECOMP;
//			MD_PopTag();
			return(false);
		}
		zdata.next_out = out;
		zdata.avail_out = rowbytes;
		zerror = inflate(&zdata);
		if((zerror != Z_OK) && (zerror != Z_STREAM_END))
		{
			inflateEnd(&zdata);
			png_error = PNG_ERROR_DECOMP;
//			MD_PopTag();
			return(false);
		}

		// Unfilter a row of data
		PNG_Unfilter(out, filter, lastline, rowbytes, image->bytedepth);

		lastline = out;
		out += rowbytes;
	}
	inflateEnd(&zdata);
//	MD_PopTag();
	return(true);
}

// Scan through all chunks and process each one

bool PNG_Load(const byte *data, ulong datasize, png_image_t *image)
{
	bool			moredata;
	const byte		*next;
	byte			*workspace, *work;
	ulong			length, type, crc, totallength;

	png_error = PNG_ERROR_OK;

	if(memcmp(data, png_signature, sizeof(png_signature)))
	{
		png_error = PNG_ERROR_NOSIG;
		return(false);
	}
	data += sizeof(png_signature);

//	workspace = (byte *)Z_Malloc(datasize, TAG_TEMP_PNG, qfalse);
	workspace = new byte[datasize];
	work = workspace;
	totallength = 0;

	moredata = true;
	while(moredata)
	{
		length = BigLong(*(ulong *)data);
		data += sizeof(ulong);

		type = BigLong(*(ulong *)data);
		const byte *crcbase = data;
		data += sizeof(ulong);

		// CRC checksum location
		next = data + length + sizeof(ulong);

		// CRC checksum includes header field
		crc = crc32(0, crcbase, length + sizeof(ulong));
		if(crc != (ulong)BigLong(*(ulong *)(next - 4)))
		{
			if(image->data)
			{
				delete [] image->data;
				//Z_Free(image->data);
				image->data = NULL;
			}
			delete [] workspace;
			//Z_Free(workspace);
			png_error = PNG_ERROR_FAILED_CRC;
			return(false);		
		}
		switch(type)
		{
		case PNG_IHDR:
			if(!PNG_HandleIHDR(data, image))
			{
				delete [] workspace;
				//Z_Free(workspace);
				return(false);
			}
			image->data = (byte *)Z_Malloc(image->width * image->height * image->bytedepth, TAG_TEMP_PNG, qfalse);
			break;
		case PNG_IDAT:
			// Need to copy all the various IDAT chunks into one big one
			// Everything but 3dsmax has one IDAT chunk
			memcpy(work, data, length);
			work += length;
			totallength += length;
			break;
		case PNG_IEND:
			if(!PNG_Unpack(workspace, totallength, image))
			{
				delete [] workspace;
				delete [] image->data;
				//Z_Free(workspace);
				//Z_Free(image->data);
				image->data = NULL;
				return(false);
			}
			moredata = false;
			break;
		default:
			break;
		}
		data = next;
	}
	delete [] workspace;
	//Z_Free(workspace);
	return(true);
}

// Outputs a crc'd chunk of PNG data
#if 0
bool PNG_OutputChunk(fileHandle_t fp, ulong type, byte *data, ulong size)
{
	ulong	crc, little, outcount;

	// Output a standard PNG chunk - length, type, data, crc
	little = BigLong(size);
	outcount = FS_Write(&little, sizeof(little), fp);

	little = BigLong(type);
	crc = crc32(0, (byte *)&little, sizeof(little));
	outcount += FS_Write(&little, sizeof(little), fp);

	if(size)
	{
		crc = crc32(crc, data, size);
		outcount += FS_Write(data, size, fp);
	}

	little = BigLong(crc);
	outcount += FS_Write(&little, sizeof(little), fp);

	if(outcount != (size + 12))
	{
		png_error = PNG_ERROR_WRITE;
		return(false);
	}
	return(true);
}

// Saves a PNG format compressed image
bool PNG_Save(const char *name, byte *data, int width, int height, int bytedepth)
{
	byte			*work;
	fileHandle_t	fp;
	int				maxsize;
	ulong			size, outcount;
	png_ihdr_t		png_header;

	png_error = PNG_ERROR_OK;

	// Create the file
	fp = FS_FOpenFileWrite(name);
	if(!fp)
	{
		png_error = PNG_ERROR_CREATE_FAIL;
		return(false);
	}
	// Write out the PNG signature
	outcount = FS_Write(png_signature, sizeof(png_signature), fp);
	if(outcount != sizeof(png_signature))
	{
		FS_FCloseFile(fp);
		png_error = PNG_ERROR_WRITE;
		return(false);
	}
	// Create and output a valid header
	PNG_CreateHeader(&png_header, width, height, bytedepth);
	if(!PNG_OutputChunk(fp, PNG_IHDR, (byte *)&png_header, sizeof(png_header)))
	{
		FS_FCloseFile(fp);
		return(false);
	}
	// Create and output the copyright info
 	if(!PNG_OutputChunk(fp, PNG_tEXt, (byte *)png_copyright, sizeof(png_copyright)))
	{
		FS_FCloseFile(fp);
		return(false);
	}
	// Max size of compressed image (source size + 0.1% + 12)
	maxsize = (width * height * bytedepth) + 4096;
	work = (byte *)Z_Malloc(maxsize, TAG_TEMP_PNG, qtrue);	// fixme: optimise to qfalse sometime - ok?
  
	// Pack up the image data
	if(!PNG_Pack(work, &size, maxsize, data, width, height, bytedepth))
	{
		Z_Free(work);
		FS_FCloseFile(fp);
		return(false);
	}
	// Write out the compressed image data
	if(!PNG_OutputChunk(fp, PNG_IDAT, (byte *)work, size))
	{
		Z_Free(work);
		FS_FCloseFile(fp);
		return(false);
	}
	Z_Free(work);
	// Output terminating chunk
	if(!PNG_OutputChunk(fp, PNG_IEND, NULL, 0))
	{
		FS_FCloseFile(fp);
		return(false);
	}
	FS_FCloseFile(fp);
	return(true);
}
#endif

/*
=============
PNG_ConvertTo32
=============
*/

void PNG_ConvertTo32(png_image_t *image)
{
	byte	*temp;
	byte	*old, *old2;
	ulong	i;

	//temp = (byte *)Z_Malloc(image->width * image->height * 4, TAG_TEMP_PNG, qtrue);
	temp = new byte[image->width * image->height * 4];
	old = image->data;
	old2 = old;
	image->data = temp;
	image->bytedepth = 4;

	for(i = 0; i < image->width * image->height; i++)
	{
		*temp++ = *old++;
		*temp++ = *old++;
		*temp++ = *old++;
		*temp++ = 0xff;
	}
	//Z_Free(old2);
	delete [] old2;
}

/*
=============
LoadPNG32
=============
*/
bool LoadPNG32 (char *name, byte **pixels, int *width, int *height, int *bytedepth)
{
	byte		*buffer;
	byte		**bufferptr = &buffer;
	int			nLen;
	png_image_t	png_image;

	if(!pixels)
	{
		bufferptr = NULL;
	}


	byte *pTempLoadedBuffer = 0;
	HANDLE hnd = CreateFile(name, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hnd == INVALID_HANDLE_VALUE)
		return false;
	DWORD dwSize = GetFileSize(hnd, NULL);
	if (dwSize == INVALID_FILE_SIZE)
	{
		CloseHandle(hnd);
		return false;
	}
	pTempLoadedBuffer = new byte[dwSize];
	DWORD dwRead;
	ReadFile(hnd, pTempLoadedBuffer, dwSize, &dwRead, NULL);
	CloseHandle(hnd);
	if (dwRead != dwSize)
	{
		delete [] pTempLoadedBuffer;
		return false;
	}
	*bufferptr = pTempLoadedBuffer;
	nLen = dwSize;


	*pixels = NULL;
	png_image.isimage = true;
	if(!PNG_Load(buffer, nLen, &png_image))
	{
		printf ("Error parsing %s: %s\n", name, PNG_GetError());
		return(false);
	}
	if(png_image.bytedepth != 4)
	{
		PNG_ConvertTo32(&png_image);
	}
	*pixels = png_image.data;
	if(width)
	{
		*width = png_image.width;
	}
	if(height)
	{
		*height = png_image.height;
	}
	if(bytedepth)
	{
		*bytedepth = png_image.bytedepth;
	}
	//FS_FreeFile(buffer);
	return(true);
}

/*
=============
LoadPNG8
=============
*/
#if 0
bool LoadPNG8 (char *name, byte **pixels, int *width, int *height)
{
	byte		*buffer;
	byte		**bufferptr = &buffer;
	int			nLen;
	png_image_t	png_image;

	if(!pixels)
	{
		bufferptr = NULL;
	}
	nLen = FS_ReadFile ( ( char * ) name, (void **)bufferptr);
	if (nLen == -1) 
	{
		if(pixels)
		{
			*pixels = NULL;
		}
		return(false);
	}
	if(!pixels)
	{
		return(true);
	}
	*pixels = NULL;
	png_image.isimage = false;
	if(!PNG_Load(buffer, nLen, &png_image))
	{
		Com_Printf ("Error parsing %s: %s\n", name, PNG_GetError());
		return(false);
	}
	*pixels = png_image.data;
	if(width)
	{
		*width = png_image.width;
	}
	if(height)
	{
		*height = png_image.height;
	}
	FS_FreeFile(buffer);
	return(true);
}
#endif

// end