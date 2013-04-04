// Generic PNG file loading code

#include "../stdafx.h"
#include "../includes.h"
#include "../r_common.h"
//#include "../renderer/tr_local.h"

#include "png.h"
#include "../zlib/zlib.h"

// Generic PNG file loading code

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

static int png_error = PNG_ERROR_OK;

static const byte png_signature[8] = { 0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a };
static const char png_copyright[] = "Copyright\0Raven Software Inc. 2000";
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
};

// Gets the error string for a failed PNG operation

const char *PNG_GetError(void)
{
	return(png_errors[png_error]);
}

// Create a header chunk

void PNG_CreateHeader(png_ihdr_t *header, png_image_t *image)
{
	header->width = BigLong(image->width);
	header->height = BigLong(image->height);
	header->bitdepth = 8;

	if(image->bytedepth == 3)
	{
		header->colortype = 2;
	}
	if(image->bytedepth == 4)
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
	if((image->width < 8) || (image->height < 8))
	{
		png_error = PNG_ERROR_TOO_SMALL;
		return(false);
	}
	// Check for non power of two size
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
	// Make sure we have a 24 or 32 bit image
	if((ihdr->colortype != 2) && (ihdr->colortype != 6))
	{
		png_error = PNG_ERROR_NOT_TC;
		return(false);
	}
	// Make sure we aren't using any wacky compression or filter algos
	if(ihdr->compression || ihdr->filter)
	{
		png_error = PNG_ERROR_INV_FIL;
		return(false);
	}
	// Extract the data we need
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

bool PNG_Pack(byte *out, ulong *size, ulong maxsize, png_image_t *image)
{
	z_stream		zdata;
	z_stream		zfilter;
	ulong			min_out, min_index, rowbytes;
	ulong			y, i;
	const byte		*lastline, *source;

	// Number of bytes per row
	rowbytes = image->width * image->bytedepth;

	memset(&zdata, 0, sizeof(z_stream));
	if(deflateInit(&zdata, Z_DEFAULT_COMPRESSION) != Z_OK)
	{
		png_error = PNG_ERROR_COMP;
		return(false);
	}

	zdata.next_out = out;
	zdata.avail_out = maxsize;

	// Storage for temp zipped data
	byte *templine = new byte [(image->width * image->bytedepth) + 128];
	// Storage for filter type and filtered row
	byte *workline = new byte [(image->width * image->bytedepth) + 1];

	lastline = NULL;
	source = image->data;
	for(y = 0; y < image->height; y++)
	{
		// Zip total_out field is cumulative
		min_out = zdata.total_out + rowbytes + 128;
		min_index = 0;

		for(i = 0; i < PNG_FILTER_NUM; i++)
		{
			if(deflateCopy(&zfilter, &zdata) != Z_OK)
			{
				deflateEnd(&zdata);
				delete [] templine;
				delete [] workline;
				png_error = PNG_ERROR_COMP;
				return(false);
			}

			zfilter.next_out = templine;
			zfilter.avail_out = rowbytes + 128;

			// Create filtered row to compress
			workline[0] = (byte)i;
			PNG_Filter(workline + 1, (byte)i, source, lastline, rowbytes, image->bytedepth);

			// Compress it all in one chunk
			zfilter.next_in = workline;
			zfilter.avail_in = rowbytes + 1;
			if(deflate(&zfilter, Z_SYNC_FLUSH) != Z_OK)
			{
				deflateEnd(&zdata);
				delete [] templine;
				delete [] workline;
				png_error = PNG_ERROR_COMP;
				return(false);
			}

			if(zfilter.total_out < min_out)
			{
				min_out = zfilter.total_out;
				min_index = i;
			}
			deflateEnd(&zfilter);
		}
		// Refilter using the most compressable filter algo
		workline[0] = (byte)min_index;
		PNG_Filter(workline + 1, (byte)min_index, source, lastline, rowbytes, image->bytedepth);

		zdata.next_in = workline;
		zdata.avail_in = rowbytes + 1;
		if(deflate(&zdata, Z_SYNC_FLUSH) != Z_OK)
		{
			deflateEnd(&zdata);
			delete [] templine;
			delete [] workline;
			png_error = PNG_ERROR_COMP;
			return(false);
		}
		lastline = source;
		source += rowbytes;
	}
	if(deflate(&zdata, Z_FINISH) != Z_STREAM_END)
	{
		delete [] templine;
		delete [] workline;
		png_error = PNG_ERROR_COMP;
		return(false);
	}
	*size = zdata.total_out;
	deflateEnd(&zdata);
	delete [] templine;
	delete [] workline;
	return(true);
}

// Unpack the image data, line by line

bool PNG_Unpack(const byte *data, const ulong datasize, png_image_t *image)
{
	ulong		rowbytes, zerror, y;
	byte		filter;
	z_stream	zdata;
	byte		*lastline, *out;

	memset(&zdata, 0, sizeof(z_stream));
	if(inflateInit(&zdata) != Z_OK)
	{
		png_error = PNG_ERROR_DECOMP;
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
		if(inflate(&zdata, Z_SYNC_FLUSH) != Z_OK)
		{
			inflateEnd(&zdata);
			png_error = PNG_ERROR_DECOMP;
			return(false);
		}
		zdata.next_out = out;
		zdata.avail_out = rowbytes;
		zerror = inflate(&zdata, Z_SYNC_FLUSH);
		if((zerror != Z_OK) && (zerror != Z_STREAM_END))
		{
			inflateEnd(&zdata);
			png_error = PNG_ERROR_DECOMP;
			return(false);
		}

		// Unfilter a row of data
		PNG_Unfilter(out, filter, lastline, rowbytes, image->bytedepth);

		lastline = out;
		out += rowbytes;
	}
	inflateEnd(&zdata);
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

	workspace = (byte *)ri.Malloc(datasize);
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
		if(crc != BigLong(*(ulong *)(next - 4)))
		{
			if(image->data)
			{
				ri.Free(image->data);
				image->data = NULL;
			}
			ri.Free(workspace);
			png_error = PNG_ERROR_FAILED_CRC;
			return(false);		
		}
		switch(type)
		{
		case PNG_IHDR:
			if(!PNG_HandleIHDR(data, image))
			{
				ri.Free(workspace);
				return(false);
			}
			image->data = (byte *)ri.Malloc(image->width * image->height * image->bytedepth);
			if(!image->data)
			{
				ri.Free(workspace);
				png_error = PNG_ERROR_MEMORY;
				return(false);
			}
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
				ri.Free(workspace);
				ri.Free(image->data);
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
	ri.Free(workspace);
	return(true);
}

// Outputs a crc'd chunk of PNG data

bool PNG_OutputChunk(FILE *fp, ulong type, byte *data, ulong size)
{
	ulong	crc, little, outcount;

	// Output a standard PNG chunk - length, type, data, crc
	little = BigLong(size);
	outcount = fwrite(&little, 1, sizeof(little), fp);

	little = BigLong(type);
	crc = crc32(0, (byte *)&little, sizeof(little));
	outcount += fwrite(&little, 1, sizeof(little), fp);

	if(size)
	{
		crc = crc32(crc, data, size);
		outcount += fwrite(data, 1, size, fp);
	}

	little = BigLong(crc);
	outcount += fwrite(&little, 1, sizeof(little), fp);

	if(outcount != (size + 12))
	{
		png_error = PNG_ERROR_WRITE;
		return(false);
	}
	return(true);
}


// Saves a PNG format compressed image
//
// this is yet more shit code from other people that doesn't work. Don't use it, it just locks up
//
bool PNG_Save(const char *name, byte *pixels, int width, int height, int bytedepth)
{
	assert(0);
	byte			*work;
	FILE			*fp;
	int				maxsize;
	ulong			size, outcount;
	png_ihdr_t		png_header;
	png_image_t		_image,*image=&_image;

	png_error = PNG_ERROR_OK;

	_image.data = pixels;
	_image.width = width;
	_image.height = height;
	_image.bytedepth = bytedepth;

	// Create the file
	fp = fopen(name, "wb");
	if(!fp)
	{
		png_error = PNG_ERROR_CREATE_FAIL;
		return(false);
	}
	// Write out the PNG signature
	outcount = fwrite(png_signature, 1, sizeof(png_signature), fp);
	if(outcount != sizeof(png_signature))
	{
		fclose(fp);
		png_error = PNG_ERROR_WRITE;
		return(false);
	}
	// Create and output a valid header
	PNG_CreateHeader(&png_header, image);
	if(!PNG_OutputChunk(fp, PNG_IHDR, (byte *)&png_header, sizeof(png_header)))
	{
		fclose(fp);
		return(false);
	}
	// Create and output the copyright info
 	if(!PNG_OutputChunk(fp, PNG_tEXt, (byte *)png_copyright, sizeof(png_copyright)))
	{
		fclose(fp);
		return(false);
	}
	// Max size of compressed image (source size + 0.1% + 12)
	maxsize = (image->width * image->height * image->bytedepth) + 4096;
	work = (byte *)ri.Malloc(maxsize);
	if(!work)
	{
		fclose(fp);
		png_error = PNG_ERROR_MEMORY;
		return(false);
	}
	// Pack up the image data
	if(!PNG_Pack(work, &size, maxsize, image))
	{
		ri.Free(work);
		fclose(fp);
		return(false);
	}
	// Write out the compressed image data
	if(!PNG_OutputChunk(fp, PNG_IDAT, (byte *)work, size))
	{
		ri.Free(work);
		fclose(fp);
		return(false);
	}
	ri.Free(work);
	// Output terminating chunk
	if(!PNG_OutputChunk(fp, PNG_IEND, NULL, 0))
	{
		fclose(fp);
		return(false);
	}
	fclose(fp);
	return(true);
}

// Prints out the relevant info regarding a PNG file

bool PNG_Info(const byte *data, ulong datasize, png_image_t *image)
{
	bool			moredata;
	const byte		*next;
	ulong			length, type, crc;

	png_error = PNG_ERROR_OK;

	printf("Checking signature.....");
	if(memcmp(data, png_signature, sizeof(png_signature)))
	{
		printf("failed\n");
		return(false);
	}
	data += sizeof(png_signature);
	printf("OK\n\n");

	moredata = true;
	while(moredata)
	{
		printf("Chunk: ");
		length = BigLong(*(ulong *)data) + sizeof(ulong);
		data += sizeof(ulong);

		type = BigLong(*(ulong *)data);
		const byte *crcbase = data;
		data += sizeof(ulong);

		printf("%c%c%c%c ", type >> 24, type >> 16, type >> 8, type);
		printf("Length: %8d ", length - sizeof(ulong));

		// CRC checksum location
		next = data + length;

		// CRC checksum includes header field
		crc = crc32(0, crcbase, length);
		printf("CRC 0x%x ", crc);
		if(crc != BigLong(*(ulong *)(next - 4)))
		{
			printf("failed\n");
			png_error = PNG_ERROR_FAILED_CRC;
			return(false);		
		}
		printf("passed\n");

		switch(type)
		{
		case PNG_IHDR:
			if(!PNG_HandleIHDR(data, image))
			{
				return(false);
			}
			break;
		case PNG_IDAT:
			break;
		case PNG_IEND:
			moredata = false;
			break;
		default:
			break;
		}
		data = next;
	}
	printf("\nDimensions: %d x %d x %d\n", image->width, image->height, image->bytedepth * 8);
	printf("\nAll chunks processed OK.\n\n");

	return(true);
}

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

	temp = (byte *)ri.Malloc(image->width * image->height * 4);
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
	ri.Free(old2);
}

/*
=============
LoadPNG32
=============
*/
bool LoadPNG32 (const char *name, byte **pixels, int *width, int *height, int *bytedepth)
{ 
	byte		*buffer;
	byte		**bufferptr = &buffer;
	int			nLen;
	png_image_t	png_image;

	if(!pixels)
	{
		bufferptr = NULL;
	}
	nLen = ri.FS_ReadFile ( ( char * ) name, (void **)bufferptr);
	if (nLen == -1) 
	{
//		Com_Printf ("Couldn't read %s\n", name);
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
	if(!PNG_Load(buffer, nLen, &png_image))
	{
		Com_Printf ("Error parsing %s: %s\n", name, PNG_GetError());
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
	ri.FS_FreeFile(buffer);
	return(true);
}

// end
