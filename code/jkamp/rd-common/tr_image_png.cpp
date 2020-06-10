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
#include <png.h>

void user_write_data( png_structp png_ptr, png_bytep data, png_size_t length ) {
	fileHandle_t fp = *(fileHandle_t*)png_get_io_ptr( png_ptr );
	ri.FS_Write( data, length, fp );
}
void user_flush_data( png_structp png_ptr ) {
	//TODO: ri.FS_Flush?
}

int RE_SavePNG( const char *filename, byte *buf, size_t width, size_t height, int byteDepth ) {
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

	fp = ri.FS_FOpenFileWrite( filename, qtrue );
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
	png_set_write_fn( png_ptr, (png_voidp)&fp, user_write_data, user_flush_data );
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
		png_destroy_read_struct (&png_ptr, &info_ptr, NULL);
	}

	int Read ( byte **data, int *width, int *height )
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
#ifdef PNG_HANDLE_AS_UNKNOWN_SUPPORTED
		// This generic "ignore all, except required chunks" requires 1.6.0 or newer"
		png_set_keep_unknown_chunks (png_ptr, PNG_HANDLE_CHUNK_NEVER, NULL, -1);
#endif
		png_set_sig_bytes (png_ptr, SIGNATURE_LEN);
		png_read_info (png_ptr, info_ptr);

		png_uint_32 width_;
		png_uint_32 height_;
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
void LoadPNG ( const char *filename, byte **data, int *width, int *height )
{
	char *buf = NULL;
	int len = ri.FS_ReadFile (filename, (void **)&buf);
	if ( len < 0 || buf == NULL )
	{
		return;
	}

	PNGFileReader reader (buf);
	reader.Read (data, width, height);
}

