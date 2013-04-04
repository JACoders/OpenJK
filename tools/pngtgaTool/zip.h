//
// zlib.h -- interface of the 'zlib' general purpose compression library
//  version 1.1.3, July 9th, 1998
//
//  Copyright (C) 1995-1998 Jean-loup Gailly and Mark Adler
//
//  This software is provided 'as-is', without any express or implied
//  warranty.  In no event will the authors be held liable for any damages
//  arising from the use of this software.
//
//  Permission is granted to anyone to use this software for any purpose,
//  including commercial applications, and to alter it and redistribute it
//  freely, subject to the following restrictions:
//
//  1. The origin of this software must not be misrepresented; you must not
//     claim that you wrote the original software. If you use this software
//     in a product, an acknowledgment in the product documentation would be
//     appreciated but is not required.
//  2. Altered source versions must be plainly marked as such, and must not be
//     misrepresented as being the original software.
//  3. This notice may not be removed or altered from any source distribution.
//
//  Jean-loup Gailly        Mark Adler
//  jloup@gzip.org          madler@alumni.caltech.edu
//
//  The data format used by the zlib library is described by RFCs (Request for
//  Comments) 1950 to 1952 in the files ftp://ds.internic.net/rfc/rfc1950.txt
//  (zlib format), rfc1951.txt (deflate format) and rfc1952.txt (gzip format).
//

//     The 'zlib' compression library provides in-memory compression and
//  decompression functions, including integrity checks of the uncompressed
//  data.  This version of the library supports only one compression method
//  (deflation) but other algorithms will be added later and will have the same
//  stream interface.
//
//     Compression can be done in a single step if the buffers are large
//  enough (for example if an input file is mmap'ed), or can be done by
//  repeated calls of the compression function.  In the latter case, the
//  application must provide more input and/or consume the output
//  (providing more output space) before each call.
//
//     The library does not install any signal handler. The decoder checks
//  the consistency of the compressed data, so the library should never
//  crash even in case of corrupted input.

// This particular implementation has been heavily modified by jscott@ravensoft.com
// to increase inflate/deflate speeds on 32 bit machines.

// for more info about .ZIP format, see 
//    ftp://ftp.cdrom.com/pub/infozip/doc/appnote-970311-iz.zip
// PkWare has also a specification at :
//    ftp://ftp.pkware.com/probdesc.zip

// ========================================================================================
// External calls and defines required for the zlib
// ========================================================================================

// The deflate compression method
#define ZF_STORED					0
#define ZF_DEFLATED					8

// Compression levels
typedef enum
{ 
	Z_STORE_COMPRESSION,
	Z_FAST_COMPRESSION_LOW,
	Z_FAST_COMPRESSION,
	Z_FAST_COMPRESSION_HIGH,
	Z_SLOW_COMPRESSION_LOWEST,
	Z_SLOW_COMPRESSION_LOW,
	Z_DEFAULT_COMPRESSION,
	Z_SLOW_COMPRESSION_HIGH,
	Z_SLOW_COMPRESSION_HIGHEST,
	Z_MAX_COMPRESSION,
} ELevel;

// Allowed flush values
typedef enum
{
	Z_NEED_MORE = -1,					// Special case when finishing up the stream 
	Z_NO_FLUSH,
	Z_SYNC_FLUSH,						// Sync up the stream ready for another call
	Z_FINISH							// Finish up the stream
} EFlush;

// Return codes for the compression/decompression functions. Negative
// values are errors, positive values are used for special but normal events.
typedef enum
{
	Z_STREAM_ERROR = -3,				// Basic error from failed sanity checks
	Z_BUF_ERROR,						// Not enough input or output
	Z_DATA_ERROR,						// Invalid data in the stream
	Z_OK,								
	Z_STREAM_END						// End of stream
} EStatus;

// Maximum value for windowBits in deflateInit and inflateInit.
// The memory requirements for inflate are (in bytes) 1 << windowBits
// that is, 32K for windowBits=15 (default value) plus a few kilobytes
// for small objects.
#define MAX_WBITS				15					// 32K LZ77 window
#define WINDOW_SIZE				(1 << MAX_WBITS)
#define	BIG_WINDOW_SIZE			(WINDOW_SIZE << 1)
#define WINDOW_MASK				(WINDOW_SIZE - 1)

// The three kinds of block type
#define STORED_BLOCK			0
#define STATIC_TREES			1
#define DYN_TREES				2
#define MODE_ILLEGAL			3

// The minimum and maximum match lengths
#define MIN_MATCH				3
#define MAX_MATCH				258

// number of distance codes
#define D_CODES					30

typedef unsigned long ulong;
typedef unsigned char byte;
typedef unsigned short word;
extern const ulong extra_dbits[D_CODES];

// Structure to be used by external applications

#define Z_Malloc(s, x1, x2) malloc((s))
#define Z_Free(p) free((p))

//  The application must update next_in and avail_in when avail_in has
//  dropped to zero. It must update next_out and avail_out when avail_out
//  has dropped to zero. All other fields are set by the
//  compression library and must not be updated by the application.

typedef struct z_stream_s 
{
	byte			*next_in;			// next input unsigned char
	ulong			avail_in;			// number of unsigned chars available at next_in
	ulong			total_in;			// total number of bytes processed so far
										  
	byte			*next_out;			// next output unsigned char should be put there
	ulong			avail_out;			// remaining free space at next_out
	ulong			total_out;			// total number of bytes output
						  
	EStatus			status;
	EStatus			error;				// error code

	struct inflate_state_s	*istate;	// not visible by applications
	struct deflate_state_s	*dstate;	// not visible by applications

	ulong			quality;
} z_stream;

//     Update a running crc with the bytes buf[0..len-1] and return the updated
//   crc. If buf is NULL, this function returns the required initial value
//   for the crc. Pre- and post-conditioning (one's complement) is performed
//   within this function so it shouldn't be done by the application.
//   Usage example:
//
//     ulong crc = crc32(0L, NULL, 0);
//
//     while (read_buffer(buffer, length) != EOF) {
//       crc = crc32(crc, buffer, length);
//     }
//     if (crc != original_crc) error();

ulong crc32(ulong crc, const byte *buf, ulong len);

//     Update a running Adler-32 checksum with the bytes buf[0..len-1] and
//   return the updated checksum. If buf is NULL, this function returns
//   the required initial value for the checksum.
//   An Adler-32 checksum is almost as reliable as a CRC32 but can be computed
//   much faster. Usage example:
//
//     ulong adler = adler32(0L, NULL, 0);
//
//     while (read_buffer(buffer, length) != EOF) {
//       adler = adler32(adler, buffer, length);
//     }
//     if (adler != original_adler) error();

ulong adler32(ulong adler, const byte *buf, ulong len);

// External calls to the deflate code
EStatus deflateInit(z_stream *strm, ELevel level, int noWrap = 0);
EStatus deflateCopy(z_stream *dest, z_stream *source);
EStatus deflate(z_stream *strm, EFlush flush);
EStatus deflateEnd(z_stream *strm);
const char *deflateError(void);

// External calls to the deflate code
EStatus inflateInit(z_stream *strm, EFlush flush, int noWrap = 0);
EStatus inflate(z_stream *z);
EStatus inflateEnd(z_stream *strm);
const char *inflateError(void);

// External calls to the zipfile code
bool InflateFile(byte *src, ulong compressedSize, byte *dst, ulong uncompressedSize, int noWrap = 0);
bool DeflateFile(byte *src, ulong uncompressedSize, byte *dst, ulong maxCompressedSize, ulong *compressedSize, ELevel level, int noWrap = 0);

// end
