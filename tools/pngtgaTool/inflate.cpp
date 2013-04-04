//#include "../game/q_shared.h"
//#include "../qcommon/qcommon.h"

#include "zip.h"
#include "inflate.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifdef _TIMING
int		totalInflateTime;
int		totalInflateCount;
#endif

// If you use the zlib library in a product, an acknowledgment is welcome
// in the documentation of your product. If for some reason you cannot
// include such an acknowledgment, I would appreciate that you keep this
// copyright string in the executable of your product.
const char inflate_copyright[] = "Inflate 1.1.3 Copyright 1995-1998 Mark Adler ";

static const char *inflate_error = "OK";

//	int inflate(z_stream *strm);
//
//    inflate decompresses as much data as possible, and stops when the input
//  buffer becomes empty or the output buffer becomes full. It may some
//  introduce some output latency (reading input without producing any output)
//  except when forced to flush.
//
//  The detailed semantics are as follows. inflate performs one or both of the
//  following actions:
//
//  - Decompress more input starting at next_in and update next_in and avail_in
//    accordingly. If not all input can be processed (because there is not
//    enough room in the output buffer), next_in is updated and processing
//    will resume at this point for the next call of inflate().
//
//  - Provide more output starting at next_out and update next_out and avail_out
//    accordingly.  inflate() provides as much output as possible, until there
//    is no more input data or no more space in the output buffer (see below
//    about the flush parameter).
//
//  Before the call of inflate(), the application should ensure that at least
//  one of the actions is possible, by providing more input and/or consuming
//  more output, and updating the next_* and avail_* values accordingly.
//  The application can consume the uncompressed output when it wants, for
//  example when the output buffer is full (avail_out == 0), or after each
//  call of inflate(). If inflate returns Z_OK and with zero avail_out, it
//  must be called again after making room in the output buffer because there
//  might be more output pending.
//
//    If the parameter flush is set to Z_SYNC_FLUSH, inflate flushes as much
//  output as possible to the output buffer. The flushing behavior of inflate is
//  not specified for values of the flush parameter other than Z_SYNC_FLUSH
//  and Z_FINISH, but the current implementation actually flushes as much output
//  as possible anyway.
//
//    inflate() should normally be called until it returns Z_STREAM_END or an
//  error. However if all decompression is to be performed in a single step
//  (a single call of inflate), the parameter flush should be set to
//  Z_FINISH. In this case all pending input is processed and all pending
//  output is flushed; avail_out must be large enough to hold all the
//  uncompressed data. (The size of the uncompressed data may have been saved
//  by the compressor for this purpose.) The next operation on this stream must
//  be inflateEnd to deallocate the decompression state. The use of Z_FINISH
//  is never required, but can be used to inform inflate that a faster routine
//  may be used for the single inflate() call.
//
//    It sets strm->adler to the adler32 checksum of all output produced
//  so and returns Z_OK, Z_STREAM_END or
//  an error code as described below. At the end of the stream, inflate()
//  checks that its computed adler32 checksum is equal to that saved by the
//  compressor and returns Z_STREAM_END only if the checksum is correct.
//
//    inflate() returns Z_OK if some progress has been made (more input processed
//  or more output produced), Z_STREAM_END if the end of the compressed data has
//  been reached and all uncompressed output has been produced, 
//  Z_DATA_ERROR if the input data was
//  corrupted (input stream not conforming to the zlib format or incorrect
//  adler32 checksum), Z_STREAM_ERROR if the stream structure was inconsistent
//  (for example if next_in or next_out was NULL),
//  Z_BUF_ERROR if no progress is possible or if there was not
//  enough room in the output buffer when Z_FINISH is used.

//	int inflateEnd (z_stream *strm);
//
//     All dynamically allocated data structures for this stream are freed.
//   This function discards any unprocessed input and does not flush any
//   pending output.
//
//     inflateEnd returns Z_OK if success, Z_STREAM_ERROR if the stream state
//   was inconsistent. In the error case, msg may be set but then points to a
//   static string (which must not be deallocated).

//	EStatus inflateInit(z_stream *strm, EFlush flush, int noWrap = 0);
//
//      inflateInit returns Z_OK if success,
//   Z_STREAM_ERROR if a parameter is invalid.
//   msg is set to "OK" if there is no error message. inflateInit
//   does not perform any decompression apart from reading the zlib header if
//   present: this will be done by inflate(). (So next_in and avail_in may be
//   modified, but next_out and avail_out are unchanged.)

//   Notes beyond the 1.93a appnote.txt:
//
//   1. Distance pointers never point before the beginning of the output
//      stream.
//   2. Distance pointers can point back across blocks, up to 32k away.
//   3. There is an implied maximum of 7 bits for the bit length table and
//      15 bits for the actual data.
//   4. If only one code exists, then it is encoded using one bit.  (Zero
//      would be more efficient, but perhaps a little confusing.)  If two
//      codes exist, they are coded using one bit each (0 and 1).
//   5. There is no way of sending zero distance codes--a dummy must be
//      sent if there are none.  (History: a pre 2.0 version of PKZIP would
//      store blocks with no distance codes, but this was discovered to be
//      too harsh a criterion.)  Valid only for 1.93a.  2.04c does allow
//      zero distance codes, which is sent as one code of zero bits in
//      length.
//   6. There are up to 286 literal/length codes.  Code 256 represents the
//      end-of-block.  Note however that the static length tree defines
//      288 codes just to fill out the Huffman codes.  Codes 286 and 287
//      cannot be used though, since there is no length base or extra bits
//      defined for them.  Similarily, there are up to 30 distance codes.
//      However, static trees define 32 codes (all 5 bits) to fill out the
//      Huffman codes, but the last two had better not show up in the data.
//   7. Unzip can check dynamic Huffman blocks for complete code sets.
//      The exception is that a single code would not be complete (see #4).
//   8. The five bits following the block type is really the number of
//      literal codes sent minus 257.
//   9. Length codes 8,16,16 are interpreted as 13 length codes of 8 bits
//      (1+6+6).  Therefore, to output three times the length, you output
//      three codes (1+1+1), whereas to output four times the same length,
//      you only need two codes (1+3).  Hmm.
//  10. In the tree reconstruction algorithm, Code = Code + Increment
//      only if BitLength(i) is not zero.  (Pretty obvious.)
//  11. Correction: 4 Bits: # of Bit Length codes - 4     (4 - 19)
//  12. Note: length code 284 can represent 227-258, but length code 285
//      really is 258.  The last length deserves its own, short code
//      since it gets used a lot in very redundant files.  The length
//      258 is special since 258 - 3 (the min match length) is 255.
//  13. The literal/length and distance code bit lengths are read as a
//      single stream of lengths.  It is possible (and advantageous) for
//      a repeat code (16, 17, or 18) to go across the boundary between
//      the two sets of lengths.

// And'ing with mask[n] masks the lower n bits
static const ulong inflate_mask[17] = 
{
    0x0000,
    0x0001, 0x0003, 0x0007, 0x000f, 0x001f, 0x003f, 0x007f, 0x00ff,
    0x01ff, 0x03ff, 0x07ff, 0x0fff, 0x1fff, 0x3fff, 0x7fff, 0xffff
};

// Order of the bit length code lengths
static const ulong border[] = 
{ 
	16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15
};

// Copy lengths for literal codes 257..285 (see note #13 above about 258)
static const ulong cplens[31] = 
{ 
	3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
	35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258, 0, 0
};

// Extra bits for literal codes 257..285 (112 == invalid)
static const ulong cplext[31] = 
{ 
	0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
	3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0, 112, 112
}; 

// Copy offsets for distance codes 0..29
static const ulong cpdist[30] = 
{ 
	1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
	257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145,
	8193, 12289, 16385, 24577
};

static ulong fixed_bl = 9;
static ulong fixed_bd = 5;

static inflate_huft_t fixed_tl[] = 
{
    { 96, 7, 256 }, { 0, 8, 80  }, { 0, 8, 16 }, { 84,  8, 115 },
    { 82, 7, 31  }, { 0, 8, 112 }, { 0, 8, 48 }, { 0,   9, 192 },
    { 80, 7, 10  }, { 0, 8, 96  }, { 0, 8, 32 }, { 0,   9, 160 },
    { 0,  8, 0   }, { 0, 8, 128 }, { 0, 8, 64 }, { 0,   9, 224 },
    { 80, 7, 6   }, { 0, 8, 88  }, { 0, 8, 24 }, { 0,   9, 144 },
    { 83, 7, 59  }, { 0, 8, 120 }, { 0, 8, 56 }, { 0,   9, 208 },
    { 81, 7, 17  }, { 0, 8, 104 }, { 0, 8, 40 }, { 0,   9, 176 },
    { 0,  8, 8   }, { 0, 8, 136 }, { 0, 8, 72 }, { 0,   9, 240 },
    { 80, 7, 4   }, { 0, 8, 84  }, { 0, 8, 20 }, { 85,  8, 227 },
    { 83, 7, 43  }, { 0, 8, 116 }, { 0, 8, 52 }, { 0,   9, 200 },
    { 81, 7, 13  }, { 0, 8, 100 }, { 0, 8, 36 }, { 0,   9, 168 },
    { 0,  8, 4   }, { 0, 8, 132 }, { 0, 8, 68 }, { 0,   9, 232 },
    { 80, 7, 8   }, { 0, 8, 92  }, { 0, 8, 28 }, { 0,   9, 152 },
    { 84, 7, 83  }, { 0, 8, 124 }, { 0, 8, 60 }, { 0,   9, 216 },
    { 82, 7, 23  }, { 0, 8, 108 }, { 0, 8, 44 }, { 0,   9, 184 },
    { 0,  8, 12  }, { 0, 8, 140 }, { 0, 8, 76 }, { 0,   9, 248 },
    { 80, 7, 3   }, { 0, 8, 82  }, { 0, 8, 18 }, { 85,  8, 163 },
    { 83, 7, 35  }, { 0, 8, 114 }, { 0, 8, 50 }, { 0,   9, 196 },
    { 81, 7, 11  }, { 0, 8, 98  }, { 0, 8, 34 }, { 0,   9, 164 },
    { 0,  8, 2   }, { 0, 8, 130 }, { 0, 8, 66 }, { 0,   9, 228 },
    { 80, 7, 7   }, { 0, 8, 90  }, { 0, 8, 26 }, { 0,   9, 148 },
    { 84, 7, 67  }, { 0, 8, 122 }, { 0, 8, 58 }, { 0,   9, 212 },
    { 82, 7, 19  }, { 0, 8, 106 }, { 0, 8, 42 }, { 0,   9, 180 },
    { 0,  8, 10  }, { 0, 8, 138 }, { 0, 8, 74 }, { 0,   9, 244 },
    { 80, 7, 5   }, { 0, 8, 86  }, { 0, 8, 22 }, { 192, 8, 0   }, 
    { 83, 7, 51  }, { 0, 8, 118 }, { 0, 8, 54 }, { 0,   9, 204 },
    { 81, 7, 15  }, { 0, 8, 102 }, { 0, 8, 38 }, { 0,   9, 172 },
    { 0,  8, 6   }, { 0, 8, 134 }, { 0, 8, 70 }, { 0,   9, 236 },
    { 80, 7, 9   }, { 0, 8, 94  }, { 0, 8, 30 }, { 0,   9, 156 },
    { 84, 7, 99  }, { 0, 8, 126 }, { 0, 8, 62 }, { 0,   9, 220 },
    { 82, 7, 27  }, { 0, 8, 110 }, { 0, 8, 46 }, { 0,   9, 188 },
    { 0,  8, 14  }, { 0, 8, 142 }, { 0, 8, 78 }, { 0,   9, 252 },
    { 96, 7, 256 }, { 0, 8, 81  }, { 0, 8, 17 }, { 85,  8, 131 },
    { 82, 7, 31  }, { 0, 8, 113 }, { 0, 8, 49 }, { 0,   9, 194 },
    { 80, 7, 10  }, { 0, 8, 97  }, { 0, 8, 33 }, { 0,   9, 162 },
    { 0,  8, 1   }, { 0, 8, 129 }, { 0, 8, 65 }, { 0,   9, 226 },
    { 80, 7, 6   }, { 0, 8, 89  }, { 0, 8, 25 }, { 0,   9, 146 },
    { 83, 7, 59  }, { 0, 8, 121 }, { 0, 8, 57 }, { 0,   9, 210 },
    { 81, 7, 17  }, { 0, 8, 105 }, { 0, 8, 41 }, { 0,   9, 178 },
    { 0,  8, 9   }, { 0, 8, 137 }, { 0, 8, 73 }, { 0,   9, 242 },
    { 80, 7, 4   }, { 0, 8, 85  }, { 0, 8, 21 }, { 80,  8, 258 },
    { 83, 7, 43  }, { 0, 8, 117 }, { 0, 8, 53 }, { 0,   9, 202 },
    { 81, 7, 13  }, { 0, 8, 101 }, { 0, 8, 37 }, { 0,   9, 170 },
    { 0,  8, 5   }, { 0, 8, 133 }, { 0, 8, 69 }, { 0,   9, 234 },
    { 80, 7, 8   }, { 0, 8, 93  }, { 0, 8, 29 }, { 0,   9, 154 },
    { 84, 7, 83  }, { 0, 8, 125 }, { 0, 8, 61 }, { 0,   9, 218 },
    { 82, 7, 23  }, { 0, 8, 109 }, { 0, 8, 45 }, { 0,   9, 186 },
    { 0,  8, 13  }, { 0, 8, 141 }, { 0, 8, 77 }, { 0,   9, 250 },
    { 80, 7, 3   }, { 0, 8, 83  }, { 0, 8, 19 }, { 85,  8, 195 },
    { 83, 7, 35  }, { 0, 8, 115 }, { 0, 8, 51 }, { 0,   9, 198 },
    { 81, 7, 11  }, { 0, 8, 99  }, { 0, 8, 35 }, { 0,   9, 166 },
    { 0,  8, 3   }, { 0, 8, 131 }, { 0, 8, 67 }, { 0,   9, 230 },
    { 80, 7, 7   }, { 0, 8, 91  }, { 0, 8, 27 }, { 0,   9, 150 },
    { 84, 7, 67  }, { 0, 8, 123 }, { 0, 8, 59 }, { 0,   9, 214 },
    { 82, 7, 19  }, { 0, 8, 107 }, { 0, 8, 43 }, { 0,   9, 182 },
    { 0,  8, 11  }, { 0, 8, 139 }, { 0, 8, 75 }, { 0,   9, 246 },
    { 80, 7, 5   }, { 0, 8, 87  }, { 0, 8, 23 }, { 192, 8, 0   }, 
    { 83, 7, 51  }, { 0, 8, 119 }, { 0, 8, 55 }, { 0,   9, 206 },
    { 81, 7, 15  }, { 0, 8, 103 }, { 0, 8, 39 }, { 0,   9, 174 },
    { 0,  8, 7   }, { 0, 8, 135 }, { 0, 8, 71 }, { 0,   9, 238 },
    { 80, 7, 9   }, { 0, 8, 95  }, { 0, 8, 31 }, { 0,   9, 158 },
    { 84, 7, 99  }, { 0, 8, 127 }, { 0, 8, 63 }, { 0,   9, 222 },
    { 82, 7, 27  }, { 0, 8, 111 }, { 0, 8, 47 }, { 0,   9, 190 },
    { 0,  8, 15  }, { 0, 8, 143 }, { 0, 8, 79 }, { 0,   9, 254 },
    { 96, 7, 256 }, { 0, 8, 80  }, { 0, 8, 16 }, { 84,  8, 115 },
    { 82, 7, 31  }, { 0, 8, 112 }, { 0, 8, 48 }, { 0,   9, 193 },
    { 80, 7, 10  }, { 0, 8, 96  }, { 0, 8, 32 }, { 0,   9, 161 },
    { 0,  8, 0   }, { 0, 8, 128 }, { 0, 8, 64 }, { 0,   9, 225 },
    { 80, 7, 6   }, { 0, 8, 88  }, { 0, 8, 24 }, { 0,   9, 145 },
    { 83, 7, 59  }, { 0, 8, 120 }, { 0, 8, 56 }, { 0,   9, 209 },
    { 81, 7, 17  }, { 0, 8, 104 }, { 0, 8, 40 }, { 0,   9, 177 },
    { 0,  8, 8   }, { 0, 8, 136 }, { 0, 8, 72 }, { 0,   9, 241 },
    { 80, 7, 4   }, { 0, 8, 84  }, { 0, 8, 20 }, { 85,  8, 227 },
    { 83, 7, 43  }, { 0, 8, 116 }, { 0, 8, 52 }, { 0,   9, 201 },
    { 81, 7, 13  }, { 0, 8, 100 }, { 0, 8, 36 }, { 0,   9, 169 },
    { 0,  8, 4   }, { 0, 8, 132 }, { 0, 8, 68 }, { 0,   9, 233 },
    { 80, 7, 8   }, { 0, 8, 92  }, { 0, 8, 28 }, { 0,   9, 153 },
    { 84, 7, 83  }, { 0, 8, 124 }, { 0, 8, 60 }, { 0,   9, 217 },
    { 82, 7, 23  }, { 0, 8, 108 }, { 0, 8, 44 }, { 0,   9, 185 },
    { 0,  8, 12  }, { 0, 8, 140 }, { 0, 8, 76 }, { 0,   9, 249 },
    { 80, 7, 3   }, { 0, 8, 82  }, { 0, 8, 18 }, { 85,  8, 163 },
    { 83, 7, 35  }, { 0, 8, 114 }, { 0, 8, 50 }, { 0,   9, 197 },
    { 81, 7, 11  }, { 0, 8, 98  }, { 0, 8, 34 }, { 0,   9, 165 },
    { 0,  8, 2   }, { 0, 8, 130 }, { 0, 8, 66 }, { 0,   9, 229 },
    { 80, 7, 7   }, { 0, 8, 90  }, { 0, 8, 26 }, { 0,   9, 149 },
    { 84, 7, 67  }, { 0, 8, 122 }, { 0, 8, 58 }, { 0,   9, 213 },
    { 82, 7, 19  }, { 0, 8, 106 }, { 0, 8, 42 }, { 0,   9, 181 },
    { 0,  8, 10  }, { 0, 8, 138 }, { 0, 8, 74 }, { 0,   9, 245 },
    { 80, 7, 5   }, { 0, 8, 86  }, { 0, 8, 22 }, { 192, 8, 0   }, 
    { 83, 7, 51  }, { 0, 8, 118 }, { 0, 8, 54 }, { 0,   9, 205 },
    { 81, 7, 15  }, { 0, 8, 102 }, { 0, 8, 38 }, { 0,   9, 173 },
    { 0,  8, 6   }, { 0, 8, 134 }, { 0, 8, 70 }, { 0,   9, 237 },
    { 80, 7, 9   }, { 0, 8, 94  }, { 0, 8, 30 }, { 0,   9, 157 },
    { 84, 7, 99  }, { 0, 8, 126 }, { 0, 8, 62 }, { 0,   9, 221 },
    { 82, 7, 27  }, { 0, 8, 110 }, { 0, 8, 46 }, { 0,   9, 189 },
    { 0,  8, 14  }, { 0, 8, 142 }, { 0, 8, 78 }, { 0,   9, 253 },
    { 96, 7, 256 }, { 0, 8, 81  }, { 0, 8, 17 }, { 85,  8, 131 },
    { 82, 7, 31  }, { 0, 8, 113 }, { 0, 8, 49 }, { 0,   9, 195 },
    { 80, 7, 10  }, { 0, 8, 97  }, { 0, 8, 33 }, { 0,   9, 163 },
    { 0,  8, 1   }, { 0, 8, 129 }, { 0, 8, 65 }, { 0,   9, 227 },
    { 80, 7, 6   }, { 0, 8, 89  }, { 0, 8, 25 }, { 0,   9, 147 },
    { 83, 7, 59  }, { 0, 8, 121 }, { 0, 8, 57 }, { 0,   9, 211 },
    { 81, 7, 17  }, { 0, 8, 105 }, { 0, 8, 41 }, { 0,   9, 179 },
    { 0,  8, 9   }, { 0, 8, 137 }, { 0, 8, 73 }, { 0,   9, 243 },
    { 80, 7, 4   }, { 0, 8, 85  }, { 0, 8, 21 }, { 80,  8, 258 },
    { 83, 7, 43  }, { 0, 8, 117 }, { 0, 8, 53 }, { 0,   9, 203 },
    { 81, 7, 13  }, { 0, 8, 101 }, { 0, 8, 37 }, { 0,   9, 171 },
    { 0,  8, 5   }, { 0, 8, 133 }, { 0, 8, 69 }, { 0,   9, 235 },
    { 80, 7, 8   }, { 0, 8, 93  }, { 0, 8, 29 }, { 0,   9, 155 },
    { 84, 7, 83  }, { 0, 8, 125 }, { 0, 8, 61 }, { 0,   9, 219 },
    { 82, 7, 23  }, { 0, 8, 109 }, { 0, 8, 45 }, { 0,   9, 187 },
    { 0,  8, 13  }, { 0, 8, 141 }, { 0, 8, 77 }, { 0,   9, 251 },
    { 80, 7, 3   }, { 0, 8, 83  }, { 0, 8, 19 }, { 85,  8, 195 },
    { 83, 7, 35  }, { 0, 8, 115 }, { 0, 8, 51 }, { 0,   9, 199 },
    { 81, 7, 11  }, { 0, 8, 99  }, { 0, 8, 35 }, { 0,   9, 167 },
    { 0,  8, 3   }, { 0, 8, 131 }, { 0, 8, 67 }, { 0,   9, 231 },
    { 80, 7, 7   }, { 0, 8, 91  }, { 0, 8, 27 }, { 0,   9, 151 },
    { 84, 7, 67  }, { 0, 8, 123 }, { 0, 8, 59 }, { 0,   9, 215 },
    { 82, 7, 19  }, { 0, 8, 107 }, { 0, 8, 43 }, { 0,   9, 183 },
    { 0,  8, 11  }, { 0, 8, 139 }, { 0, 8, 75 }, { 0,   9, 247 },
    { 80, 7, 5   }, { 0, 8, 87  }, { 0, 8, 23 }, { 192, 8, 0   }, 
    { 83, 7, 51  }, { 0, 8, 119 }, { 0, 8, 55 }, { 0,   9, 207 },
    { 81, 7, 15  }, { 0, 8, 103 }, { 0, 8, 39 }, { 0,   9, 175 },
    { 0,  8, 7   }, { 0, 8, 135 }, { 0, 8, 71 }, { 0,   9, 239 },
    { 80, 7, 9   }, { 0, 8, 95  }, { 0, 8, 31 }, { 0,   9, 159 },
    { 84, 7, 99  }, { 0, 8, 127 }, { 0, 8, 63 }, { 0,   9, 223 },
    { 82, 7, 27  }, { 0, 8, 111 }, { 0, 8, 47 }, { 0,   9, 191 },
    { 0,  8, 15  }, { 0, 8, 143 }, { 0, 8, 79 }, { 0,   9, 255 }
};

static inflate_huft_t fixed_td[] = 
{
    { 80, 5, 1  }, { 87, 5, 257  }, { 83, 5, 17  }, { 91,  5, 4097  },
    { 81, 5, 5  }, { 89, 5, 1025 }, { 85, 5, 65  }, { 93,  5, 16385 },
    { 80, 5, 3  }, { 88, 5, 513  }, { 84, 5, 33  }, { 92,  5, 8193  },
    { 82, 5, 9  }, { 90, 5, 2049 }, { 86, 5, 129 }, { 192, 5, 24577 },
    { 80, 5, 2  }, { 87, 5, 385  }, { 83, 5, 25  }, { 91,  5, 6145  },
    { 81, 5, 7  }, { 89, 5, 1537 }, { 85, 5, 97  }, { 93,  5, 24577 },
    { 80, 5, 4  }, { 88, 5, 769  }, { 84, 5, 49  }, { 92,  5, 12289 },
    { 82, 5, 13 }, { 90, 5, 3073 }, { 86, 5, 193 }, { 192, 5, 24577 }
};

// ===============================================================================
// ===============================================================================

static void inflate_blocks_reset(z_stream *z, inflate_blocks_state_t *s)
{
	if((s->mode == BTREE) || (s->mode == DTREE))
	{
		Z_Free(s->trees.blens);
	}
	if(s->mode == CODES)
	{
		Z_Free(s->decode.codes);
	}
	s->mode = TYPE;
	s->bitk = 0;
	s->bitb = 0;
	s->write = s->window;
	s->read = s->window;
	z->istate->adler = 1;
}

// ===============================================================================
// ===============================================================================

static int inflate_blocks_free(z_stream *z, inflate_blocks_state_t *s)
{
	inflate_blocks_reset(z, s);
	Z_Free(s->hufts);
	s->hufts = NULL;
	Z_Free(s);
	return(Z_OK);
}

// ===============================================================================
// ===============================================================================

static inflate_blocks_state_t *inflate_blocks_new(z_stream *z, check_func check)
{
	inflate_blocks_state_t *s;

	s = (inflate_blocks_state_t *)Z_Malloc(sizeof(inflate_blocks_state_t), TAG_INFLATE, qtrue);
	s->hufts = (inflate_huft_t *)Z_Malloc(sizeof(inflate_huft_t) * MANY, TAG_INFLATE, qtrue);
	s->end = s->window + WINDOW_SIZE;
	s->mode = TYPE;
	inflate_blocks_reset(z, s);

	return(s);
}

// ===============================================================================
// copy as much as possible from the sliding window to the output area
// ===============================================================================

static void inflate_flush_copy(z_stream *z, inflate_blocks_state_t *s, ulong count)
{
	if(count > z->avail_out)
	{
		count = z->avail_out;
	}
	if(count && (z->error == Z_BUF_ERROR))
	{
		z->error = Z_OK;
	}

	// Calculate the checksum if required
	if(!z->istate->nowrap)
	{
		z->istate->adler = adler32(z->istate->adler, s->read, count);
	}

	// copy as as end of window
	memcpy(z->next_out, s->read, count);

	// update counters
	z->avail_out -= count;
	z->total_out += count;
	z->next_out += count;
	s->read += count;
}

// ===============================================================================
// ===============================================================================

static void inflate_flush(z_stream *z, inflate_blocks_state_t *s)
{
	ulong	count;

	// compute number of bytes to copy as as end of window
	count = (s->read <= s->write ? s->write : s->end) - s->read;

	inflate_flush_copy(z, s, count);

	// see if more to copy at beginning of window
	if(s->read == s->end)
	{
		// wrap pointers
		s->read = s->window;
		if(s->write == s->end)
		{
			s->write = s->window;
		}
		// compute bytes to copy
		count = s->write - s->read;
		inflate_flush_copy(z, s, count);
	}
}

// ===============================================================================
// get bytes and bits
// ===============================================================================

static bool getbits(z_stream *z, inflate_blocks_state_t *s, ulong bits)								 
{			
	while(s->bitk < bits) 							
	{											
		if(z->avail_in)
		{
			z->error = Z_OK;
		}
		else
		{										
			inflate_flush(z, s);
			return(false);
		}		  
		z->avail_in--;
		z->total_in++;
		s->bitb |= *z->next_in++ << s->bitk;			
		s->bitk += 8; 	 
	}		
	return(true);
}

// ===============================================================================
// output bytes
// ===============================================================================

static ulong needout(z_stream *z, inflate_blocks_state_t *s, ulong bytesToEnd) 
{ 
	if(!bytesToEnd)
	{ 
		if((s->write == s->end) && (s->read != s->window)) 
		{ 
			s->write = s->window;
			bytesToEnd = s->write < s->read ? s->read - s->write - 1 : s->end - s->write;
		}
		if(!bytesToEnd)
		{
			inflate_flush(z, s);
			bytesToEnd = s->write < s->read ? s->read - s->write - 1 : s->end - s->write;
			if((s->write == s->end) && (s->read != s->window))
			{
				s->write = s->window;
				bytesToEnd = s->write < s->read ? s->read - s->write - 1 : s->end - s->write;
			}
			if(!bytesToEnd)
			{
				inflate_flush(z, s);
				return(bytesToEnd);
			}
		}
	}
	z->error = Z_OK;
	return(bytesToEnd);
}

// ===============================================================================
// Called with number of bytes left to write in window at least 258
//	 (the maximum string length) and number of input bytes available
//	 at least ten.	The ten bytes are six bytes for the longest length/
//	 distance pair plus four bytes for overloading the bit buffer.
// ===============================================================================

inline byte *qcopy(byte *dst, byte *src, int count)
{
	byte 	*retval;

	_asm
	{
		push ecx  
		push esi
		push edi

		mov edi, [dst]
		mov esi, [src]
		mov ecx, [count]
		rep movsb

		mov [retval], edi
		pop edi
		pop esi
		pop ecx
	}
	return(retval);
}

inline ulong get_remaining(inflate_blocks_state_t *s)
{
	if(s->write < s->read)
	{
		return(s->read - s->write - 1);
	}
	return(s->end - s->write); 
}

static EStatus inflate_fast(ulong lengthMask, ulong distMask, inflate_huft_t *lengthTree, inflate_huft_t *distTree, inflate_blocks_state_t *s, z_stream *z)
{
	inflate_huft_t	*huft;			// temporary pointer
	byte			*data;			
	byte			*src; 			// copy source pointer
	byte			*dst;			
	ulong			extraBits;		// extra bits or operation
	ulong			bytesToEnd;		// bytes to end of window or read pointer
	ulong			count;			// bytes to copy
	ulong			dist;			// distance back to copy from
	ulong			bitb;
	ulong			bitk;
	ulong			availin;
	ulong			morebits;
	ulong			copymore;

	// load input, output, bit values
	data = z->next_in;
	dst = s->write;
	availin = z->avail_in;
	bitb = s->bitb;
	bitk = s->bitk;

	bytesToEnd = get_remaining(s);

	// do until not enough input or output space for fast loop
	// assume called with bytesToEnd >= 258 && availIn >= 10
	while((bytesToEnd >= 258) && (availin >= 10))
	{						   
		// get literal/length code
		while(bitk < 20)
		{
			bitb |= *data++ << bitk;
			bitk += 8;
			availin--;
		}

		huft = lengthTree + (bitb & lengthMask);
		if(!huft->Exop)
		{
			bitb >>= huft->Bits;
			bitk -= huft->Bits;
			*dst++ = (byte)huft->base;
			bytesToEnd--;
		}
		else
		{
			extraBits = huft->Exop;
			morebits = 1;
			do
			{
				bitb >>= huft->Bits; 
				bitk -= huft->Bits;
				if(extraBits & 16)
				{
					// get extra bits for length
					extraBits &= 15;
					count = huft->base + (bitb & inflate_mask[extraBits]);
					bitb >>= extraBits;
					bitk -= extraBits;
					// decode distance base of block to copy
					while(bitk < 15)
					{
						bitb |= *data++ << bitk;
						bitk += 8;
						availin--;
					}
					huft = distTree + (bitb & distMask);
					extraBits = huft->Exop;
	   				copymore = 1;
					do
					{
						bitb >>= huft->Bits;
						bitk -= huft->Bits;
						if(extraBits & 16)
						{
							// get extra bits to add to distance base
							extraBits &= 15;
							while(bitk < extraBits)
							{
								bitb |= *data++ << bitk;
								bitk += 8;
								availin--;
							}
							dist = huft->base + (bitb & inflate_mask[extraBits]);
							bitb >>= extraBits;
							bitk -= extraBits;

							// do the copy
							bytesToEnd -= count;
							// offset before dest
							if((dst - s->window) >= dist)	 
							{								   
								// just copy
								src = dst - dist;
							}
							// else offset after destination
							else						
							{
								// bytes from offset to end
								extraBits = dist - (dst - s->window);
								// pointer to offset
								src = s->end - extraBits; 		  
								// if source crosses,					  
								if(count > extraBits)				  
								{
									// copy to end of window
									dst = qcopy(dst, src, extraBits);
									// copy rest from start of window
									count -= extraBits; 				
									src = s->window;			
								}
							}
							// copy all or what's left
							dst = qcopy(dst, src, count);
							copymore = 0;
						}
						else
						{
							if(!(extraBits & 64))
							{
								huft += huft->base + (bitb & inflate_mask[extraBits]);
								extraBits = huft->Exop;
							}
							else
							{
								inflate_error = "Inflate data: Invalid distance code";
								return(Z_DATA_ERROR);
							}
						}
					} while(copymore);

					morebits = 0;
				}
				else
				{
					if(!(extraBits & 64))
					{
						huft += huft->base + (bitb & inflate_mask[extraBits]);
						extraBits = huft->Exop;
						if(!extraBits)								   
						{
							bitb >>= huft->Bits;
							bitk -= huft->Bits;
							*dst++ = (byte)huft->base;
							bytesToEnd--;
							morebits = 0;
						}
					}
					else if(extraBits & 32)									 
					{
						count = data - z->next_in;

						z->avail_in = availin;
						z->total_in += count;
						z->next_in = data;

						s->write = dst;

						count = (bitk >> 3) < count ? bitk >> 3 : count;

						s->bitb = bitb;
						s->bitk = bitk - (count << 3);
						z->avail_in += count;
						z->total_in -= count;
						z->next_in -= count ;
						return(Z_STREAM_END);
					}
					else
					{
						inflate_error = "Inflate data: Invalid literal/length code";
						return(Z_DATA_ERROR);
					}
				}
			} while(morebits);
		}
	}

	// not enough input or output--restore pointers and return
	count = data - z->next_in;

	z->avail_in = availin;
	z->total_in += count;
	z->next_in = data;

	s->write = dst;

	count = (bitk >> 3) < count ? bitk >> 3 : count;
	s->bitb = bitb;
	s->bitk = bitk - (count << 3);
	z->avail_in += count;
	z->total_in -= count;
	z->next_in -= count;

	return(Z_OK);
}

// ===============================================================================
// ===============================================================================

static void inflate_codes(z_stream *z, inflate_blocks_state_t *s)
{
	inflate_huft_t			*huft; 		// temporary pointer
	ulong					extraBits;	// extra bits or operation
	ulong					bytesToEnd;	// bytes to end of window or read pointer
	byte					*src; 		// pointer to copy strings from
	inflate_codes_state_t	*infCodes;	// codes state

	infCodes = s->decode.codes;

	// copy input/output information to locals
	bytesToEnd = get_remaining(s);

	// process input and output based on current state
	while(true)
	{
		// waiting for "i:"=input, "o:"=output, "x:"=nothing
		switch (infCodes->mode)
		{			  
		// x: set up for LEN
		case START:
			if((bytesToEnd >= 258) && (z->avail_in >= 10))
			{
				z->error = inflate_fast(inflate_mask[infCodes->lbits], inflate_mask[infCodes->dbits], infCodes->ltree, infCodes->dtree, s, z);
				bytesToEnd = get_remaining(s);
				if(z->error != Z_OK)
				{
					infCodes->mode = (z->error == Z_STREAM_END) ? WASH : BADCODE;
					break;
				}
			}
			infCodes->code.need = infCodes->lbits;
			infCodes->code.tree = infCodes->ltree;
			infCodes->mode = LEN;
		// i: get length/literal/eob next
		case LEN:			
			if(!getbits(z, s, infCodes->code.need))
			{
				// We could get here because we have run out of input data *or* the stream has ended
				if(z->status == Z_BUF_ERROR)
				{
					z->error = Z_STREAM_END;
				}
				return;
			}
			huft = infCodes->code.tree + (s->bitb & inflate_mask[infCodes->code.need]);
			s->bitb >>= huft->Bits;
			s->bitk -= huft->Bits; 
			extraBits = huft->Exop;
			// literal
			if(!extraBits)				 
			{
				infCodes->lit = huft->base;
				infCodes->mode = LIT;
				break;
			}
			// length
			if(extraBits & 16) 			  
			{
				infCodes->copy.get = extraBits & 15;
				infCodes->len = huft->base;
				infCodes->mode = LENEXT;
				break;
			}
			// next table
			if(!(extraBits & 64))
			{
				infCodes->code.need = extraBits;
				infCodes->code.tree = huft + huft->base;
				break;
			}
			// end of block
			if(extraBits & 32) 			  
			{
				infCodes->mode = WASH;
				break;
			}
			// invalid code
			infCodes->mode = BADCODE;		  
			inflate_error = "Inflate data: Invalid literal/length code";
			z->error = Z_DATA_ERROR;
			inflate_flush(z, s);
			return;
		// i: getting length extra (have base)
		case LENEXT:		
			if(!getbits(z, s, infCodes->copy.get))
			{
				return;
			}
			infCodes->len += s->bitb & inflate_mask[infCodes->copy.get];
			s->bitb >>= infCodes->copy.get;
			s->bitk -= infCodes->copy.get;
			infCodes->code.need = infCodes->dbits;
			infCodes->code.tree = infCodes->dtree;
			infCodes->mode = DIST;
		// i: get distance next
		case DIST:			
			if(!getbits(z, s, infCodes->code.need))
			{
				return;
			}
			huft = infCodes->code.tree + (s->bitb & inflate_mask[infCodes->code.need]);
			s->bitb >>= huft->Bits;
			s->bitk -= huft->Bits;
			extraBits = huft->Exop;
			// distance
			if(extraBits & 16) 			  
			{
				infCodes->copy.get = extraBits & 15;
				infCodes->copy.dist = huft->base;
				infCodes->mode = DISTEXT;
				break;
			}
			// next table
			if(!(extraBits & 64))
			{
				infCodes->code.need = extraBits;
				infCodes->code.tree = huft + huft->base;
				break;
			}
			// invalid code
			infCodes->mode = BADCODE;		  
			inflate_error = "Inflate data: Invalid distance code";
			z->error = Z_DATA_ERROR;
			inflate_flush(z, s);
			return;
		// i: getting distance extra
		case DISTEXT:		
			if(!getbits(z, s, infCodes->copy.get))
			{
				return;
			}
			infCodes->copy.dist += s->bitb & inflate_mask[infCodes->copy.get];
			s->bitb >>= infCodes->copy.get;
			s->bitk -= infCodes->copy.get;
			infCodes->mode = COPY;
		// o: copying bytes in window, waiting for space
		case COPY:			
			if(s->write - s->window < infCodes->copy.dist)
			{
				src = s->end - (infCodes->copy.dist - (s->write - s->window));
			}
			else
			{
				src = s->write - infCodes->copy.dist;
			}
			while(infCodes->len)
			{
				bytesToEnd = needout(z, s, bytesToEnd);
				if(!bytesToEnd)
				{
					return;
				}
				*s->write++ = (byte)(*src++);
				bytesToEnd--;
				if(src == s->end)
				{
					src = s->window;
				}
				infCodes->len--;
			}
			infCodes->mode = START;
			break;
		// o: got literal, waiting for output space
		case LIT:			
			bytesToEnd = needout(z, s, bytesToEnd);
			if(!bytesToEnd)
			{
				return;
			}
			*s->write++ = (byte)infCodes->lit; 
			bytesToEnd--;
			infCodes->mode = START;
			break;
		// o: got eob, possibly more output
		case WASH:			
			// return unused byte, if any
			if(s->bitk > 7)		  
			{
				s->bitk -= 8;
				z->avail_in++;
				z->total_in--;
				// can always return one
				z->next_in--;			
			}
			inflate_flush(z, s);
			bytesToEnd = get_remaining(s);
			if(s->read != s->write)
			{
				inflate_error = "Inflate data: read != write while in WASH";
				inflate_flush(z, s);
				return;
			}
			infCodes->mode = END;
		case END:
			z->error = Z_STREAM_END;
			inflate_flush(z, s);
			return;
		// x: got error
		case BADCODE:		
			z->error = Z_DATA_ERROR;
			inflate_flush(z, s);
			return;
		default:
			z->error = Z_STREAM_ERROR;
			inflate_flush(z, s);
			return;
		}
	}
}

// ===============================================================================
// ===============================================================================

static inflate_codes_state_t *inflate_codes_new(z_stream *z, ulong bl, ulong bd, inflate_huft_t *lengthTree, inflate_huft_t *distTree)
{
	inflate_codes_state_t		*c;

	c = (inflate_codes_state_t *)Z_Malloc(sizeof(inflate_codes_state_t), TAG_INFLATE, qtrue);
	c->mode = START;
	c->lbits = (byte)bl;
	c->dbits = (byte)bd;
	c->ltree = lengthTree;
	c->dtree = distTree;

	return(c);
}

// ===============================================================================
// Generate Huffman trees for efficient decoding

//	ulong b				// code lengths in bits (all assumed <= BMAX)
//	ulong n             // number of codes (assumed <= 288)
//	ulong s             // number of simple-valued codes (0..s-1)
//	const ulong *d      // list of base values for non-simple codes
//	const ulong *e      // list of extra bits for non-simple codes
//	inflate_huft ** t	// result: starting table
//	ulong *m            // maximum lookup bits, returns actual
//	inflate_huft *hp    // space for trees
//	ulong *hn           // hufts used in space
//	ulong *workspace    // working area: values in order of bit length
//
//   Given a list of code lengths and a maximum table size, make a set of
//   tables to decode that set of codes.  Return Z_OK on success, Z_BUF_ERROR
//   if the given code set is incomplete (the tables are still built in this
//   case), Z_DATA_ERROR if the input is invalid (an over-subscribed set of
//   lengths).
//
//   Huffman code decoding is performed using a multi-level table lookup.
//   The fastest way to decode is to simply build a lookup table whose
//   size is determined by the longest code.  However, the time it takes
//   to build this table can also be a factor if the data being decoded
//   is not very long.  The most common codes are necessarily the
//   shortest codes, so those codes dominate the decoding time, and hence
//   the speed.  The idea is you can have a shorter table that decodes the
//   shorter, more probable codes, and then point to subsidiary tables for
//   the longer codes.  The time it costs to decode the longer codes is
//   then traded against the time it takes to make longer tables.
//
//   This results of this trade are in the variables lbits and dbits
//   below.  lbits is the number of bits the first level table for literal/
//   length codes can decode in one step, and dbits is the same thing for
//   the distance codes.  Subsequent tables are also less than or equal to
//   those sizes.  These values may be adjusted either when all of the
//   codes are shorter than that, in which case the longest code length in
//   bits is used, or when the shortest code is *longer* than the requested
//   table size, in which case the length of the shortest code in bits is
//   used.
//
//   There are two different values for the two tables, since they code a
//   different number of possibilities each.  The literal/length table
//   codes 286 possible values, or in a flat code, a little over eight
//   bits.  The distance table codes 30 possible values, or a little less
//   than five bits, flat.  The optimum values for speed end up being
//   about one bit more than those, so lbits is 8+1 and dbits is 5+1.
//   The optimum values may differ though from machine to machine, and
//   possibly even between compilers.  Your mileage may vary.
// ===============================================================================

static EStatus huft_build(ulong *b, ulong numCodes, ulong s, const ulong *d, const ulong *e, inflate_huft_t **t, ulong *m, inflate_huft_t *hp, ulong *hn, ulong *workspace)
{
	ulong			codeCounter;					// counter for codes of length bitsPerCode
	ulong			bitLengths[BMAX + 1] = { 0 };	// bit length count table
	ulong			bitOffsets[BMAX + 1];			// bit offsets, then code stack
	ulong			f;								// i repeats in table every f entries
	int 			maxCodeLen;						// maximum code length
	int 			tableLevel;						// table level
	ulong			i;								// counter, current code
	ulong			j;								// counter
	int 			bitsPerCode;					// number of bits in current code
	ulong 			bitsPerTable;					// bits per table (returned in m)
	int 			bitsBeforeTable;				// bits before this table == (bitsPerTable * tableLevel)
	ulong			*p; 							// pointer into bitLengths[], b[], or workspace[]
	inflate_huft_t	*q; 							// points to current table
	inflate_huft_t	r;								// table entry for structure assignment
	inflate_huft_t	*tableStack[BMAX];				// table stack
	ulong			*xp;							// pointer into bitOffsets
	int 			dummyCodes;						// number of dummy codes added
	ulong			entryCount;						// number of entries in current table

	// Generate counts for each bit length
	// assume all entries <= BMAX
	p = b;	
	i = numCodes;
	do
	{
		bitLengths[*p++]++;					
	} while(--i);

	// null input--all zero length codes
	if(bitLengths[0] == numCodes)				  
	{
		*t = NULL;
		*m = 0;
		return(Z_OK);
	}

	// Find minimum and maximum length, bound *m by those
	bitsPerTable = *m;
	for(j = 1; j <= BMAX; j++)
	{
		if(bitLengths[j])
		{
			break;
		}
	}
	// minimum code length
	bitsPerCode = j;						  

	if(bitsPerTable < j)
	{
		bitsPerTable = j;
	}
	for(i = BMAX; i; i--)
	{
		if(bitLengths[i])
		{
			break;
		}
	}
	// maximum code length
	maxCodeLen = i;						  

	if(bitsPerTable > i)
	{
		bitsPerTable = i;
	}
	*m = bitsPerTable;

	// Adjust last length count to fill out codes, if needed
	for(dummyCodes = 1 << j; j < i; j++, dummyCodes <<= 1)
	{
		dummyCodes -= bitLengths[j];
		if(dummyCodes < 0)
		{
			return(Z_DATA_ERROR);
		}
	}
	dummyCodes -= bitLengths[i];
	if(dummyCodes < 0)
	{
		return(Z_DATA_ERROR);
	}
	bitLengths[i] += dummyCodes;

	// Generate starting offsets into the value table for each length
	bitOffsets[1] = 0;
	j = 0;
	p = bitLengths + 1;
	xp = bitOffsets + 2;
	// note that i == maxCodeLen from above
	while(--i) 
	{			 
		j += *p++;
		*xp++ = j;
	}

	// Make a table of values in order of bit lengths
	p = b;
	i = 0;
	do
	{
		j = *p++;
		if(j)
		{
			workspace[bitOffsets[j]++] = i;
		}
	} while(++i < numCodes);

	// set numCodes to length of workspace
	numCodes = bitOffsets[maxCodeLen];					  

	// Generate the Huffman codes and for each, make the table entries
	bitOffsets[0] = 0;						// first Huffman code is zero
	i = 0;							
	p = workspace;							// grab values in bit order
	tableLevel = -1; 						// no tables yet--level -1
	bitsBeforeTable = bitsPerTable; 						// bits decoded == (bitsPerTable * tableLevel)
	bitsBeforeTable = -bitsBeforeTable;
	tableStack[0] = NULL;	// just to keep compilers happy
	q = NULL;		// ditto
	entryCount = 0;							// ditto

	// go through the bit lengths (bitsPerCode already is bits in shortest code)
	for(; bitsPerCode <= maxCodeLen; bitsPerCode++)
	{
		codeCounter = bitLengths[bitsPerCode];
		while(codeCounter--)
		{
			// here i is the Huffman code of length bitsPerCode bits for value *p
			// make tables up to required level
			while(bitsPerCode > bitsBeforeTable + bitsPerTable)
			{
				tableLevel++;
				bitsBeforeTable += bitsPerTable; 			// previous table always bitsPerTable bits

				// compute minimum size table less than or equal to bitsPerTable bits
				entryCount = maxCodeLen - bitsBeforeTable;
				entryCount = entryCount > bitsPerTable ? bitsPerTable : entryCount;				// table size upper limit
				j = bitsPerCode - bitsBeforeTable;
				f = 1 << j;
				if(f > codeCounter + 1)							// try a bitsPerCode-bitsBeforeTable bit table
				{										// too few codes for bitsPerCode-bitsBeforeTable bit table
					f -= codeCounter + 1; 						// deduct codes from patterns left
					xp = bitLengths + bitsPerCode;
					if(j < entryCount)
					{
						while(++j < entryCount) 					// try smaller tables up to entryCount bits
						{
							f <<= 1;
							if(f <= *++xp)
							{
								break;					// enough codes to use up j bits
							}
							f -= *xp;					// else deduct codes from patterns
						}
					}
				}
				entryCount = 1 << j; 							// table entries for j-bit table

				// allocate new table
				if(*hn + entryCount > MANY) 						// (note: doesn't matter for fixed)
				{
					return(Z_DATA_ERROR);				// not enough memory
				}
				q = hp + *hn;
				tableStack[tableLevel] = q;
				*hn += entryCount;

				// connect to last table, if there is one
				if(tableLevel)
				{
					bitOffsets[tableLevel] = i;							// save pattern for backing up
					r.Bits = (byte)bitsPerTable;   	 					// bits to dump before this table
					r.Exop = (byte)j;									// bits in this table
					j = i >> (bitsBeforeTable - bitsPerTable);
					r.base = q - tableStack[tableLevel - 1] - j; // offset to this table
					tableStack[tableLevel - 1][j] = r;					// connect to last table
				}
				else						   
				{
					*t = q; 							// first table is returned result
				}
			}

			// set up table entry in r
			r.Bits = (byte)(bitsPerCode - bitsBeforeTable);
			if(p >= workspace + numCodes)
			{
				r.Exop = 128 + 64;						// out of values--invalid code
			}
			else if(*p < s)
			{
				r.Exop = (byte)(*p < 256 ? 0 : 32 + 64);		// 256 is end-of-block
				r.base = *p++;							// simple code is just the value
			}
			else
			{
				r.Exop = (byte)(e[*p - s] + 16 + 64);		 	// non-simple--look up in lists
				r.base = d[*p++ - s];
			}

			// fill code-like entries with r
			f = 1 << (bitsPerCode - bitsBeforeTable);
			for(j = i >> bitsBeforeTable; j < entryCount; j += f)
			{
				q[j] = r;
			}

			// backwards increment the bitsPerCode-bit code i
			for(j = 1 << (bitsPerCode - 1); i & j; j >>= 1)
			{
				i ^= j;
			}
			i ^= j;

			// backup over finished tables
			while((i & ((1 << bitsBeforeTable) - 1)) != bitOffsets[tableLevel])
			{
				tableLevel--;									// don't need to update q
				bitsBeforeTable -= bitsPerTable;
			}
		}
	}

	// Return Z_BUF_ERROR if we were given an incomplete table
	if(dummyCodes && (maxCodeLen != 1))
	{
		return(Z_BUF_ERROR);
	}
	return(Z_OK);
}

// ===============================================================================
// ulong *c 				19 code lengths
// ulong *bb				bits tree desired/actual depth
// inflate_huft **tb		bits tree result
// inflate_huft *hp 		space for trees
// ===============================================================================

static void inflate_trees_bits(z_stream *z, ulong *c, ulong *bb, inflate_huft_t **tb, inflate_huft_t *hp)
{
	ulong	hn = 0;				// hufts used in space
	ulong	workspace[19];		// work area for huft_build

	z->error = huft_build(c, 19, 19, NULL, NULL, tb, bb, hp, &hn, workspace);
	if(z->error == Z_DATA_ERROR)
	{
		inflate_error = "Inflate data: Oversubscribed dynamic bit lengths tree";
	}
	else if((z->error == Z_BUF_ERROR) || !*bb)
	{
		inflate_error = "Inflate data: Incomplete dynamic bit lengths tree";
		z->error = Z_DATA_ERROR;
	}
}

// ===============================================================================
// ulong *c 				// that many (total) code lengths
// ulong *bl				// literal desired/actual bit depth
// ulong *bd				// distance desired/actual bit depth
// inflate_huft **tl		// literal/length tree result
// inflate_huft **td		// distance tree result
// inflate_huft *hp 		// space for trees
// ===============================================================================

static void inflate_trees_dynamic(z_stream *z, ulong numLiteral, ulong numDist, ulong *c, ulong *bl, ulong *bd, inflate_huft_t **tl, inflate_huft_t **td, inflate_huft_t *hp)
{
	ulong		hn = 0;				// hufts used in space
	ulong		workspace[288]; 	// work area for huft_build

	// build literal/length tree
	z->error = huft_build(c, numLiteral, 257, cplens, cplext, tl, bl, hp, &hn, workspace);
	if(z->error != Z_OK || !*bl)
	{
		inflate_error = "Inflate data: Erroneous literal/length tree";
		z->error = Z_DATA_ERROR;
		return;
	}
	// build distance tree
	z->error = huft_build(c + numLiteral, numDist, 0, cpdist, extra_dbits, td, bd, hp, &hn, workspace);
	if((z->error != Z_OK) || (!*bd && numLiteral > 257))
	{
		inflate_error = "Inflate data: Erroneous distance tree";
		z->error = Z_DATA_ERROR;
		return;
	}
}

// ===============================================================================
// ulong *bl				// literal desired/actual bit depth
// ulong *bd				// distance desired/actual bit depth
// inflate_huft **tl		// literal/length tree result
// inflate_huft **td		// distance tree result
// ===============================================================================

// Fixme: Calculate dynamically

static void inflate_trees_fixed(z_stream *z, ulong *bl, ulong *bd, inflate_huft_t **tl, inflate_huft_t **td)
{
	*bl = fixed_bl;
	*bd = fixed_bd;
	*tl = fixed_tl;
	*td = fixed_td;
	z->error = Z_OK;
}

// ===============================================================================
// ===============================================================================

static void inflate_blocks(inflate_blocks_state_t *s, z_stream *z)
{
	ulong					t;				// temporary storage
	ulong					bytesToEnd;		// bytes to end of window or read pointer
	ulong					bl, bd;
	inflate_huft_t			*lengthTree = NULL;
	inflate_huft_t			*distTree = NULL;
	inflate_codes_state_t	*c;

	// copy input/output information to locals (UPDATE macro restores)
	bytesToEnd = s->write < s->read ? s->read - s->write - 1 : s->end - s->write; 

	// process input based on current state
	while(true)
	{
		switch (s->mode)
		{
		case TYPE:
			if(!getbits(z, s, 3))
			{
				return;
			}
			t = s->bitb & 7;
			s->last = !!(t & 1);

			switch (t >> 1)
			{
			case STORED_BLOCK:
				s->bitb >>= 3;
				s->bitk -= 3;
				t = s->bitk & 7;				// go to byte boundary
				s->bitb >>= t;
				s->bitk -= t;
				s->mode = LENS; 				// get length of stored block
				break;
			case STATIC_TREES:
				inflate_trees_fixed(z, &bl, &bd, &lengthTree, &distTree);
				s->decode.codes = inflate_codes_new(z, bl, bd, lengthTree, distTree);
				s->bitb >>= 3;
				s->bitk -= 3;
				s->mode = CODES;
				break;
			case DYN_TREES:
				s->bitb >>= 3;
				s->bitk -= 3;
				s->mode = TABLE;
				break;
			case MODE_ILLEGAL:
				s->bitb >>= 3;
				s->bitk -= 3;
				s->mode = BAD;
				inflate_error = "Inflate data: Invalid block type";
				z->error = Z_DATA_ERROR;
				inflate_flush(z, s);
				return;
			}
			break;
		case LENS:
			if(!getbits(z, s, 32))
			{
				return;
			}
			if(((~s->bitb) >> 16) != (s->bitb & 0xffff))
			{
				s->mode = BAD;
				inflate_error = "Inflate data: Invalid stored block lengths";
				z->error = Z_DATA_ERROR;
				inflate_flush(z, s);
				return;
			}
			s->left = s->bitb & 0xffff;
			s->bitb = 0;
			s->bitk = 0;								// dump bits
			s->mode = s->left ? STORED : (s->last ? DRY : TYPE);
			break;
		case STORED:
			if(!z->avail_in)
			{
				inflate_flush(z, s);
				return;
			}
			bytesToEnd = needout(z, s, bytesToEnd);
			if(!bytesToEnd)
			{
				return;
			}
			t = s->left;
			if(t > z->avail_in) 
			{
				t = z->avail_in;
			}
			if(t > bytesToEnd) 
			{
				t = bytesToEnd;
			}
			memcpy(s->write, z->next_in, t);
			z->next_in += t;
			z->avail_in -= t;
			z->total_in += t;
			s->write += t;
			bytesToEnd -= t;
			s->left -= t;
			if(s->left)
			{
				break;
			}
			s->mode = s->last ? DRY : TYPE;
			break;
		case TABLE:
			if(!getbits(z, s, 14))
			{
				return;
			}
			t = s->bitb & 0x3fff;
			s->trees.table = t;
			if((t & 0x1f) > 29 || ((t >> 5) & 0x1f) > 29)
			{
				s->mode = BAD;
				inflate_error = "Inflate data: Too many length or distance symbols";
				z->error = Z_DATA_ERROR;
				inflate_flush(z, s);
				return;
			}
			t = 258 + (t & 0x1f) + ((t >> 5) & 0x1f);
			s->trees.blens = (ulong *)Z_Malloc(t * sizeof(ulong), TAG_INFLATE, qfalse);
			s->bitb >>= 14;
			s->bitk -= 14;
			s->trees.index = 0;
			s->mode = BTREE;
		case BTREE:
			while(s->trees.index < 4 + (s->trees.table >> 10))
			{
				if(!getbits(z, s, 3))
				{
					return;
				}
				s->trees.blens[border[s->trees.index++]] = s->bitb & 7;
				s->bitb >>= 3;
				s->bitk -= 3;
			}
			while(s->trees.index < 19)
			{
				s->trees.blens[border[s->trees.index++]] = 0;
			}
			s->trees.bb = 7;
			inflate_trees_bits(z, s->trees.blens, &s->trees.bb, &s->trees.tb, s->hufts);
			if(z->error != Z_OK)
			{
				Z_Free(s->trees.blens);
				s->mode = BAD;
				inflate_flush(z, s);
				return;
			}
			s->trees.index = 0;
			s->mode = DTREE;
		case DTREE:
			while(t = s->trees.table, s->trees.index < 258 + (t & 0x1f) + ((t >> 5) & 0x1f))
			{
				inflate_huft_t *h;
				ulong i, j, c;

				t = s->trees.bb;
				if(!getbits(z, s, t))
				{
					return;
				}
				h = s->trees.tb + (s->bitb & inflate_mask[t]);
				t = h->Bits;
				c = h->base;
				if(c < 16)
				{
					s->bitb >>= t;
					s->bitk -= t;
					s->trees.blens[s->trees.index++] = c;
				}
				else	// c == 16..18
				{
					i = (c == 18) ? 7 : c - 14;
					j = (c == 18) ? 11 : 3;
					if(!getbits(z, s, t + i))
					{
						return;
					}
					s->bitb >>= t;
					s->bitk -= t;
					j += s->bitb & inflate_mask[i];
					s->bitb >>= i;
					s->bitk -= i;
					i = s->trees.index;
					t = s->trees.table;
					if(i + j > 258 + (t & 0x1f) + ((t >> 5) & 0x1f) || (c == 16 && i < 1))
					{
						Z_Free(s->trees.blens);
						s->mode = BAD;
						inflate_error = "Inflate data: Invalid bit length repeat";
						z->error = Z_DATA_ERROR;
						inflate_flush(z, s);
						return;
					}
					c = (c == 16) ? s->trees.blens[i - 1] : 0;
					do
					{
						s->trees.blens[i++] = c;
					} while(--j);
					s->trees.index = i;
				}
			}
			s->trees.tb = NULL;

			bl = 9; 					// must be <= 9 for lookahead assumptions
			bd = 6; 					// must be <= 9 for lookahead assumptions
			t = s->trees.table;
			inflate_trees_dynamic(z, 257 + (t & 0x1f), 1 + ((t >> 5) & 0x1f), s->trees.blens, &bl, &bd, &lengthTree, &distTree, s->hufts);
			Z_Free(s->trees.blens);
			if(z->error != Z_OK)
			{
				s->mode = BAD;
				inflate_flush(z, s);
				return;
			}
			c = inflate_codes_new(z, bl, bd, lengthTree, distTree);
			s->decode.codes = c;
			s->mode = CODES;
		case CODES:
			inflate_codes(z, s);
			if(z->error != Z_STREAM_END)
			{
				inflate_flush(z, s);
				return;
			}
			z->error = Z_OK;
			Z_Free(s->decode.codes);
			bytesToEnd = s->write < s->read ? s->read - s->write - 1 : s->end - s->write; 
			if(!s->last)
			{
				s->mode = TYPE;
				break;
			}
			s->mode = DRY;
		case DRY:
			inflate_flush(z, s);
			bytesToEnd = s->write < s->read ? s->read - s->write - 1 : s->end - s->write;
			if(s->read != s->write)
			{
				inflate_error = "Inflate data: read != write in DRY";
				inflate_flush(z, s);
				return;
			}
			s->mode = DONE;
		case DONE:
			z->error = Z_STREAM_END;
			inflate_flush(z, s);
			return;
		case BAD:
		default:
			z->error = Z_DATA_ERROR;
			inflate_flush(z, s);
			return;
		}
	}
}

// -------------------------------------------------------------------------------------------------
// Controlling routines
// -------------------------------------------------------------------------------------------------

EStatus inflateEnd(z_stream *z)
{
	assert(z);

	if(z->istate->blocks)
	{
		inflate_blocks_free(z, z->istate->blocks);
		z->istate->blocks = NULL;
	}
	if(z->istate)
	{
		Z_Free(z->istate);
		z->istate = NULL;
	}
	return(Z_OK);
}

// ===============================================================================
// ===============================================================================

EStatus inflateInit(z_stream *z, EFlush flush, int noWrap)
{
	// initialize state
	assert(z);

	inflate_error = "OK";

	z->istate = (inflate_state *)Z_Malloc(sizeof(inflate_state), TAG_INFLATE, qtrue);
	z->istate->blocks = NULL;

	// handle nowrap option (no zlib header or check)
	z->istate->nowrap = noWrap;
	z->istate->wbits = MAX_WBITS;

	// create inflate_blocks state
	z->istate->blocks = inflate_blocks_new(z, NULL);

	z->status = Z_OK;
	if(flush == Z_FINISH)
	{
		z->status = Z_BUF_ERROR;
	}

	// reset state
	z->istate->mode = imMETHOD;
	if(z->istate->nowrap)
	{
		z->istate->mode = imBLOCKS;
	}
	inflate_blocks_reset(z, z->istate->blocks);
	return(Z_OK);
}

// ===============================================================================
// ===============================================================================

EStatus inflate(z_stream *z)
{
	ulong		b;

	// Sanity check data
	assert(z);
	assert(z->istate);

	while(true)
	{
		switch (z->istate->mode)
		{
		case imMETHOD:
			if(!z->avail_in)
			{
				return(z->status);
			}
			z->istate->method = *z->next_in++;
			z->avail_in--;
			z->total_in++;
			if((z->istate->method & 0xf) != ZF_DEFLATED)
			{
				z->istate->mode = imBAD;
				inflate_error = "Inflate data: Unknown compression method";
				return(Z_DATA_ERROR);
			}
			if((z->istate->method >> 4) + 8 > z->istate->wbits)
			{
				z->istate->mode = imBAD;
				inflate_error = "Inflate data: Invalid window size";
				return(Z_DATA_ERROR);
			}
			z->istate->mode = imFLAG;
			break;
		case imFLAG:
			if(!z->avail_in)
			{
				return(z->status);
			}
			b = *z->next_in++;
			z->avail_in--;
			z->total_in++;
			if(((z->istate->method << 8) + b) % 31)
			{
				z->istate->mode = imBAD;
				inflate_error = "Inflate data: Incorrect header check";
				return(Z_DATA_ERROR);
			}
			z->istate->mode = imBLOCKS;
			break;
		case imBLOCKS:
			inflate_blocks(z->istate->blocks, z);
			
			// Make sure everything processed ok
			if(z->error == Z_DATA_ERROR)
			{
				z->istate->mode = imBAD;
				return(Z_DATA_ERROR);
			}

			if(z->error != Z_STREAM_END)
			{
				return(z->status);
			}
			z->istate->calcadler = z->istate->adler;
			inflate_blocks_reset(z, z->istate->blocks);
			if(z->istate->nowrap)
			{
				z->istate->mode = imDONE;
				break;
			}
			z->istate->mode = imCHECK4;
			break;
		case imCHECK4:
			if(!z->avail_in)
			{
				return(z->status);
			}
			z->istate->adler = *z->next_in++ << 24;
			z->avail_in--;
			z->total_in++;
			z->istate->mode = imCHECK3;
			break;
		case imCHECK3:
			if(!z->avail_in)
			{
				return(z->status);
			}
			z->istate->adler += *z->next_in++ << 16;
			z->avail_in--;
			z->total_in++;
			z->istate->mode = imCHECK2;
			break;
		case imCHECK2:
			if(!z->avail_in)
			{
				return(z->status);
			}
			z->istate->adler += *z->next_in++ << 8;
			z->avail_in--;
			z->total_in++;
			z->istate->mode = imCHECK1;
			break;
		case imCHECK1:
			if(!z->avail_in)
			{
				return(z->status);
			}
			z->istate->adler += *z->next_in++;
			z->avail_in--;
			z->total_in++;

			if(z->istate->calcadler != z->istate->adler)
			{
				inflate_error = "Inflate data: Failed Adler checksum";
				z->istate->mode = imBAD;
				break;
			}
			z->istate->mode = imDONE;
			break;
		case imDONE:
			return(Z_STREAM_END);
		case imBAD:
			return(Z_DATA_ERROR);
		default:
			return(Z_STREAM_ERROR);
		}
	}
	assert(0);
	return(Z_OK);
}

// ===============================================================================
// ===============================================================================

const char *inflateError(void)
{
	return(inflate_error);
}

// ===============================================================================
// External calls
// ===============================================================================

bool InflateFile(byte *src, ulong compressedSize, byte *dst, ulong uncompressedSize, int noWrap)
{
	z_stream	z = { 0 };

	inflateInit(&z, Z_FINISH, noWrap);

	z.next_in = src;
	z.avail_in = compressedSize;
	z.next_out = dst;
	z.avail_out = uncompressedSize;

#ifdef _TIMING
	int temp = timeGetTime();
#endif
	if(inflate(&z) != Z_STREAM_END)
	{
		inflate_error = "Inflate data: Stream did not end";
		inflateEnd(&z);
		return(false);
	}
#ifdef _TIMING
	totalInflateTime += timeGetTime() - temp;
	totalInflateCount++;
#endif

	if(z.avail_in)
	{
		inflate_error = "Inflate data: Remaining input data at stream end";
		inflateEnd(&z);
		return(false);
	}
	if(z.avail_out)
	{
		inflate_error = "Inflate data: Remaining output space at stream end";
		inflateEnd(&z);
		return(false);
	}
	if(z.total_in != compressedSize)
	{
		inflate_error = "Inflate data: Number of processed bytes != compressed size";
		inflateEnd(&z);
		return(false);
	}
	if(z.total_out != uncompressedSize)
	{
		inflate_error = "Inflate data: Number of bytes output != uncompressed size";
		inflateEnd(&z);
		return(false);
	}
	inflateEnd(&z);
	return(true);
}

// end
