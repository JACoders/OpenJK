//#include "../game/q_shared.h"
//#include "../qcommon/qcommon.h"

#include "zip.h"
#include "deflate.h"
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#ifdef _TIMING
int		totalDeflateTime[Z_MAX_COMPRESSION + 1];
int		totalDeflateCount[Z_MAX_COMPRESSION + 1];
#endif

// If you use the zlib library in a product, an acknowledgment is welcome
// in the documentation of your product. If for some reason you cannot
// include such an acknowledgment, I would appreciate that you keep this
// copyright string in the executable of your product.
const char deflate_copyright[] = "Deflate 1.1.3 Copyright 1995-1998 Jean-loup Gailly ";

static const char *deflate_error = "OK";

//  ALGORITHM
// 
//  The "deflation" process depends on being able to identify portions
//  of the input text which are identical to earlier input (within a
//  sliding window trailing behind the input currently being processed).
// 
//  The most straightforward technique turns out to be the fastest for
//  most input files: try all possible matches and select the longest.
//  The key feature of this algorithm is that insertions into the string
//  dictionary are very simple and thus fast, and deletions are avoided
//  completely. Insertions are performed at each input character, whereas
//  string matches are performed only when the previous match ends. So it
//  is preferable to spend more time in matches to allow very fast string
//  insertions and avoid deletions. The matching algorithm for small
//  strings is inspired from that of Rabin & Karp. A brute force approach
//  is used to find longer strings when a small match has been found.
//  A similar algorithm is used in comic (by Jan-Mark Wams) and freeze
//  (by Leonid Broukhis).
//
// ACKNOWLEDGEMENTS
//
//  The idea of lazy evaluation of matches is due to Jan-Mark Wams, and
//  I found it in 'freeze' written by Leonid Broukhis.
//  Thanks to many people for bug reports and testing.
//
// REFERENCES
//
//  Deutsch, L.P.,"DEFLATE Compressed Data Format Specification".
//  Available in ftp://ds.internic.net/rfc/rfc1951.txt
//
//  A description of the Rabin and Karp algorithm is given in the book
//     "Algorithms" by R. Sedgewick, Addison-Wesley, p252.
//
//  Fiala,E.R., and Greene,D.H.
//     Data Compression with Finite Windows, Comm.ACM, 32,4 (1989) 490-595

// ===============================================================================
//  A word is an index in the character window. We use short instead of int to
//  save space in the various tables. ulong is used only for parameter passing.

//  The static literal tree. Since the bit lengths are imposed, there is no
//  need for the L_CODES extra codes used during heap construction. However
//  The codes 286 and 287 are needed to build a canonical tree (see _tr_init
//  below).
static const ct_data static_ltree[L_CODES + 2] = 
{
	{{  12 }, { 8 }}, {{ 140 }, { 8 }}, {{  76 }, { 8 }}, {{ 204 }, { 8 }}, {{  44 }, { 8 }},
	{{ 172 }, { 8 }}, {{ 108 }, { 8 }}, {{ 236 }, { 8 }}, {{  28 }, { 8 }}, {{ 156 }, { 8 }},
	{{  92 }, { 8 }}, {{ 220 }, { 8 }}, {{  60 }, { 8 }}, {{ 188 }, { 8 }}, {{ 124 }, { 8 }},
	{{ 252 }, { 8 }}, {{   2 }, { 8 }}, {{ 130 }, { 8 }}, {{  66 }, { 8 }}, {{ 194 }, { 8 }},
	{{  34 }, { 8 }}, {{ 162 }, { 8 }}, {{  98 }, { 8 }}, {{ 226 }, { 8 }}, {{  18 }, { 8 }},
	{{ 146 }, { 8 }}, {{  82 }, { 8 }}, {{ 210 }, { 8 }}, {{  50 }, { 8 }}, {{ 178 }, { 8 }},
	{{ 114 }, { 8 }}, {{ 242 }, { 8 }}, {{  10 }, { 8 }}, {{ 138 }, { 8 }}, {{  74 }, { 8 }},
	{{ 202 }, { 8 }}, {{  42 }, { 8 }}, {{ 170 }, { 8 }}, {{ 106 }, { 8 }}, {{ 234 }, { 8 }},
	{{  26 }, { 8 }}, {{ 154 }, { 8 }}, {{  90 }, { 8 }}, {{ 218 }, { 8 }}, {{  58 }, { 8 }},
	{{ 186 }, { 8 }}, {{ 122 }, { 8 }}, {{ 250 }, { 8 }}, {{   6 }, { 8 }}, {{ 134 }, { 8 }},
	{{  70 }, { 8 }}, {{ 198 }, { 8 }}, {{  38 }, { 8 }}, {{ 166 }, { 8 }}, {{ 102 }, { 8 }},
	{{ 230 }, { 8 }}, {{  22 }, { 8 }}, {{ 150 }, { 8 }}, {{  86 }, { 8 }}, {{ 214 }, { 8 }},
	{{  54 }, { 8 }}, {{ 182 }, { 8 }}, {{ 118 }, { 8 }}, {{ 246 }, { 8 }}, {{  14 }, { 8 }},
	{{ 142 }, { 8 }}, {{  78 }, { 8 }}, {{ 206 }, { 8 }}, {{  46 }, { 8 }}, {{ 174 }, { 8 }},
	{{ 110 }, { 8 }}, {{ 238 }, { 8 }}, {{  30 }, { 8 }}, {{ 158 }, { 8 }}, {{  94 }, { 8 }},
	{{ 222 }, { 8 }}, {{  62 }, { 8 }}, {{ 190 }, { 8 }}, {{ 126 }, { 8 }}, {{ 254 }, { 8 }},
	{{   1 }, { 8 }}, {{ 129 }, { 8 }}, {{  65 }, { 8 }}, {{ 193 }, { 8 }}, {{  33 }, { 8 }},
	{{ 161 }, { 8 }}, {{  97 }, { 8 }}, {{ 225 }, { 8 }}, {{  17 }, { 8 }}, {{ 145 }, { 8 }},
	{{  81 }, { 8 }}, {{ 209 }, { 8 }}, {{  49 }, { 8 }}, {{ 177 }, { 8 }}, {{ 113 }, { 8 }},
	{{ 241 }, { 8 }}, {{   9 }, { 8 }}, {{ 137 }, { 8 }}, {{  73 }, { 8 }}, {{ 201 }, { 8 }},
	{{  41 }, { 8 }}, {{ 169 }, { 8 }}, {{ 105 }, { 8 }}, {{ 233 }, { 8 }}, {{  25 }, { 8 }},
	{{ 153 }, { 8 }}, {{  89 }, { 8 }}, {{ 217 }, { 8 }}, {{  57 }, { 8 }}, {{ 185 }, { 8 }},
	{{ 121 }, { 8 }}, {{ 249 }, { 8 }}, {{   5 }, { 8 }}, {{ 133 }, { 8 }}, {{  69 }, { 8 }},
	{{ 197 }, { 8 }}, {{  37 }, { 8 }}, {{ 165 }, { 8 }}, {{ 101 }, { 8 }}, {{ 229 }, { 8 }},
	{{  21 }, { 8 }}, {{ 149 }, { 8 }}, {{  85 }, { 8 }}, {{ 213 }, { 8 }}, {{  53 }, { 8 }},
	{{ 181 }, { 8 }}, {{ 117 }, { 8 }}, {{ 245 }, { 8 }}, {{  13 }, { 8 }}, {{ 141 }, { 8 }},
	{{  77 }, { 8 }}, {{ 205 }, { 8 }}, {{  45 }, { 8 }}, {{ 173 }, { 8 }}, {{ 109 }, { 8 }},
	{{ 237 }, { 8 }}, {{  29 }, { 8 }}, {{ 157 }, { 8 }}, {{  93 }, { 8 }}, {{ 221 }, { 8 }},
	{{  61 }, { 8 }}, {{ 189 }, { 8 }}, {{ 125 }, { 8 }}, {{ 253 }, { 8 }}, {{  19 }, { 9 }},
	{{ 275 }, { 9 }}, {{ 147 }, { 9 }}, {{ 403 }, { 9 }}, {{  83 }, { 9 }}, {{ 339 }, { 9 }},
	{{ 211 }, { 9 }}, {{ 467 }, { 9 }}, {{  51 }, { 9 }}, {{ 307 }, { 9 }}, {{ 179 }, { 9 }},
	{{ 435 }, { 9 }}, {{ 115 }, { 9 }}, {{ 371 }, { 9 }}, {{ 243 }, { 9 }}, {{ 499 }, { 9 }},
	{{  11 }, { 9 }}, {{ 267 }, { 9 }}, {{ 139 }, { 9 }}, {{ 395 }, { 9 }}, {{  75 }, { 9 }},
	{{ 331 }, { 9 }}, {{ 203 }, { 9 }}, {{ 459 }, { 9 }}, {{  43 }, { 9 }}, {{ 299 }, { 9 }},
	{{ 171 }, { 9 }}, {{ 427 }, { 9 }}, {{ 107 }, { 9 }}, {{ 363 }, { 9 }}, {{ 235 }, { 9 }},
	{{ 491 }, { 9 }}, {{  27 }, { 9 }}, {{ 283 }, { 9 }}, {{ 155 }, { 9 }}, {{ 411 }, { 9 }},
	{{  91 }, { 9 }}, {{ 347 }, { 9 }}, {{ 219 }, { 9 }}, {{ 475 }, { 9 }}, {{  59 }, { 9 }},
	{{ 315 }, { 9 }}, {{ 187 }, { 9 }}, {{ 443 }, { 9 }}, {{ 123 }, { 9 }}, {{ 379 }, { 9 }},
	{{ 251 }, { 9 }}, {{ 507 }, { 9 }}, {{   7 }, { 9 }}, {{ 263 }, { 9 }}, {{ 135 }, { 9 }},
	{{ 391 }, { 9 }}, {{  71 }, { 9 }}, {{ 327 }, { 9 }}, {{ 199 }, { 9 }}, {{ 455 }, { 9 }},
	{{  39 }, { 9 }}, {{ 295 }, { 9 }}, {{ 167 }, { 9 }}, {{ 423 }, { 9 }}, {{ 103 }, { 9 }},
	{{ 359 }, { 9 }}, {{ 231 }, { 9 }}, {{ 487 }, { 9 }}, {{  23 }, { 9 }}, {{ 279 }, { 9 }},
	{{ 151 }, { 9 }}, {{ 407 }, { 9 }}, {{  87 }, { 9 }}, {{ 343 }, { 9 }}, {{ 215 }, { 9 }},
	{{ 471 }, { 9 }}, {{  55 }, { 9 }}, {{ 311 }, { 9 }}, {{ 183 }, { 9 }}, {{ 439 }, { 9 }},
	{{ 119 }, { 9 }}, {{ 375 }, { 9 }}, {{ 247 }, { 9 }}, {{ 503 }, { 9 }}, {{  15 }, { 9 }},
	{{ 271 }, { 9 }}, {{ 143 }, { 9 }}, {{ 399 }, { 9 }}, {{  79 }, { 9 }}, {{ 335 }, { 9 }},
	{{ 207 }, { 9 }}, {{ 463 }, { 9 }}, {{  47 }, { 9 }}, {{ 303 }, { 9 }}, {{ 175 }, { 9 }},
	{{ 431 }, { 9 }}, {{ 111 }, { 9 }}, {{ 367 }, { 9 }}, {{ 239 }, { 9 }}, {{ 495 }, { 9 }},
	{{  31 }, { 9 }}, {{ 287 }, { 9 }}, {{ 159 }, { 9 }}, {{ 415 }, { 9 }}, {{  95 }, { 9 }},
	{{ 351 }, { 9 }}, {{ 223 }, { 9 }}, {{ 479 }, { 9 }}, {{  63 }, { 9 }}, {{ 319 }, { 9 }},
	{{ 191 }, { 9 }}, {{ 447 }, { 9 }}, {{ 127 }, { 9 }}, {{ 383 }, { 9 }}, {{ 255 }, { 9 }},
	{{ 511 }, { 9 }}, {{   0 }, { 7 }}, {{  64 }, { 7 }}, {{  32 }, { 7 }}, {{  96 }, { 7 }},
	{{  16 }, { 7 }}, {{  80 }, { 7 }}, {{  48 }, { 7 }}, {{ 112 }, { 7 }}, {{   8 }, { 7 }},
	{{  72 }, { 7 }}, {{  40 }, { 7 }}, {{ 104 }, { 7 }}, {{  24 }, { 7 }}, {{  88 }, { 7 }},
	{{  56 }, { 7 }}, {{ 120 }, { 7 }}, {{   4 }, { 7 }}, {{  68 }, { 7 }}, {{  36 }, { 7 }},
	{{ 100 }, { 7 }}, {{  20 }, { 7 }}, {{  84 }, { 7 }}, {{  52 }, { 7 }}, {{ 116 }, { 7 }},
	{{   3 }, { 8 }}, {{ 131 }, { 8 }}, {{  67 }, { 8 }}, {{ 195 }, { 8 }}, {{  35 }, { 8 }},
	{{ 163 }, { 8 }}, {{  99 }, { 8 }}, {{ 227 }, { 8 }}	 	 	    	   	   		  
};

// The static distance tree. (Actually a trivial tree since all codes use 5 bits.)
static const ct_data static_dtree[D_CODES] = 
{
	{{  0 }, { 5 }}, {{ 16 }, { 5 }}, {{  8 },{ 5 }}, {{ 24 },{ 5 }}, {{  4 },{ 5 }},
	{{ 20 }, { 5 }}, {{ 12 }, { 5 }}, {{ 28 },{ 5 }}, {{  2 },{ 5 }}, {{ 18 },{ 5 }},
	{{ 10 }, { 5 }}, {{ 26 }, { 5 }}, {{  6 },{ 5 }}, {{ 22 },{ 5 }}, {{ 14 },{ 5 }},
	{{ 30 }, { 5 }}, {{  1 }, { 5 }}, {{ 17 },{ 5 }}, {{  9 },{ 5 }}, {{ 25 },{ 5 }},
	{{  5 }, { 5 }}, {{ 21 }, { 5 }}, {{ 13 },{ 5 }}, {{ 29 },{ 5 }}, {{  3 },{ 5 }},
	{{ 19 }, { 5 }}, {{ 11 }, { 5 }}, {{ 27 },{ 5 }}, {{  7 },{ 5 }}, {{ 23 },{ 5 }}
};

// Distance codes. The first 256 values correspond to the distances
// 3 .. 258, the last 256 values correspond to the top 8 bits of
// the 15 bit distances.
static const byte tr_dist_code[DIST_CODE_LEN] = 
{
	 0,  1,  2,  3,  4,  4,  5,  5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  8,
	 8,  8,  8,  8,  9,  9,  9,  9,  9,  9,  9,  9, 10, 10, 10, 10, 10, 10, 10, 10,
	10, 10, 10, 10, 10, 10, 10, 10, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
	11, 11, 11, 11, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
	12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 13, 13, 13, 13,
	13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
	13, 13, 13, 13, 13, 13, 13, 13, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,
	14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,
	14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,
	14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,  0,  0, 16, 17,
	18, 18, 19, 19, 20, 20, 20, 20, 21, 21, 21, 21, 22, 22, 22, 22, 22, 22, 22, 22,
	23, 23, 23, 23, 23, 23, 23, 23, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
	24, 24, 24, 24, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
	26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26,
	26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 27, 27, 27, 27, 27, 27, 27, 27,
	27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
	27, 27, 27, 27, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
	28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
	28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
	28, 28, 28, 28, 28, 28, 28, 28, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
	29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
	29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
	29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29
};

// length code for each normalized match length (0 == MIN_MATCH)
static const byte tr_length_code[MAX_MATCH - MIN_MATCH + 1] = 
{
	 0,  1,  2,  3,  4,  5,  6,  7,  8,  8,  9,  9, 10, 10, 11, 11, 12, 12, 12, 12,
	13, 13, 13, 13, 14, 14, 14, 14, 15, 15, 15, 15, 16, 16, 16, 16, 16, 16, 16, 16,
	17, 17, 17, 17, 17, 17, 17, 17, 18, 18, 18, 18, 18, 18, 18, 18, 19, 19, 19, 19,
	19, 19, 19, 19, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
	21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 22, 22, 22, 22,
	22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 23, 23, 23, 23, 23, 23, 23, 23,
	23, 23, 23, 23, 23, 23, 23, 23, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
	24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
	25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
	25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 26, 26, 26, 26, 26, 26, 26, 26,
	26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26,
	26, 26, 26, 26, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
	27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 28
};

// First normalized length for each code (0 = MIN_MATCH)
static const int base_length[LENGTH_CODES] = 
{
	0, 1, 2, 3, 4, 5, 6, 7, 8, 10, 12, 14, 16, 20, 24, 28, 32, 40, 48, 56,
	64, 80, 96, 112, 128, 160, 192, 224, 0
};

// First normalized distance for each code (0 = distance of 1)
static const int base_dist[D_CODES] = 
{
    0,     1,     2,     3,     4,     6,     8,    12,    16,    24,
   32,    48,    64,    96,   128,   192,   256,   384,   512,   768,
 1024,  1536,  2048,  3072,  4096,  6144,  8192, 12288, 16384, 24576
};

// Note: the deflate() code requires max_lazy >= MIN_MATCH and max_chain >= 4
// For deflate_fast() (levels <= 3) good is ignored and lazy has a different
// meaning.
static block_state deflate_stored(deflate_state *s, EFlush flush);
static block_state deflate_fast(deflate_state *s, EFlush flush);
static block_state deflate_slow(deflate_state *s, EFlush flush);

// Values for max_lazy_match, good_match and max_chain_length, depending on
// the desired pack level (0..9). The values given below have been tuned to
// exclude worst case performance for pathological files. Better values may be
// found for specific files.
static const config configuration_table[10] = 
{
  // good lazy nice chain
	{ 0,    0,   0,    0, deflate_stored },		// store only

	{ 4,    4,   8,    4, deflate_fast },		// maximum speed, no lazy matches
	{ 4,    5,  16,    8, deflate_fast },
	{ 4,    6,  32,   32, deflate_fast },
		  
	{ 4,    4,  16,   16, deflate_slow },		// lazy matches
	{ 8,   16,  32,   32, deflate_slow },
	{ 8,   16, 128,  128, deflate_slow },
	{ 8,   32, 128,  256, deflate_slow },
	{ 32, 128, 258, 1024, deflate_slow },
	{ 32, 258, 258, 4096, deflate_slow }		// maximum compression
}; 

// extra bits for each length code
static ulong extra_lbits[LENGTH_CODES] = 
{
	0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0
};

// Extra bits for distance codes
const ulong extra_dbits[D_CODES] = 
{ 
	0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,	7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13
};

// extra bits for each bit length code
static ulong extra_blbits[BL_CODES] = 
{
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 3, 7
};

// The lengths of the bit length codes are sent in order of decreasing
// probability, to avoid transmitting the lengths for unused bit length codes.
static const byte bl_order[BL_CODES] = 
{
	16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15
};

static static_tree_desc static_l_desc = 
{
	static_ltree, extra_lbits, LITERALS + 1, L_CODES, MAX_WBITS
};

static static_tree_desc static_d_desc = 
{
	static_dtree, extra_dbits, 0, D_CODES, MAX_WBITS
};

static static_tree_desc static_bl_desc = 
{
	NULL, extra_blbits, 0, BL_CODES, MAX_BL_BITS
};

// ===============================================================================
// Output bytes to the output stream. Inlined for speed
// ===============================================================================

inline void put_byte(deflate_state *s, const byte c)
{
	s->pending_buf[s->pending++] = c;
}

// Fixme: write as 1 short
inline void put_short(deflate_state *s, const word w)
{
	s->pending_buf[s->pending++] = (byte)(w & 0xff);
	s->pending_buf[s->pending++] = (byte)(w >> 8);
}

inline void put_shortMSB(deflate_state *s, const word w)
{
	s->pending_buf[s->pending++] = (byte)(w >> 8);
	s->pending_buf[s->pending++] = (byte)(w & 0xff);
}   

inline void put_longMSB(deflate_state *s, const ulong l)
{
    s->pending_buf[s->pending++] = (byte)(l >> 24);
    s->pending_buf[s->pending++] = (byte)(l >> 16);
    s->pending_buf[s->pending++] = (byte)(l >> 8);
    s->pending_buf[s->pending++] = (byte)(l & 0xff);
}   

// ===============================================================================
// Send a value on a given number of bits.
// IN assertion: length <= 16 and value fits in length bits.
// ===============================================================================

static void send_bits(deflate_state *s, const ulong val, const ulong len)
{
	assert(len <= 16);
	assert(val <= 65536);

	if(s->bi_valid > (BUF_SIZE - len)) 
	{
		s->bi_buf |= val << s->bi_valid;
		put_short(s, s->bi_buf);
		s->bi_buf = (word)(val >> (BUF_SIZE - s->bi_valid));
		s->bi_valid += len - BUF_SIZE;
	}
	else
	{
		s->bi_buf |= val << s->bi_valid;
		s->bi_valid += len;
	}
}

// ===============================================================================
// Initialize a new block.
// ===============================================================================

static void init_block(deflate_state *s)
{
    int		n;						// iterates over tree elements

    // Initialize the trees.
    for(n = 0; n < L_CODES;  n++) 
	{
		s->dyn_ltree[n].fc.freq = 0;
	}
    for(n = 0; n < D_CODES;  n++) 
	{
		s->dyn_dtree[n].fc.freq = 0;
	}
    for(n = 0; n < BL_CODES; n++) 
	{
		s->bl_tree[n].fc.freq = 0;
	}
    s->dyn_ltree[END_BLOCK].fc.freq = 1;
    s->opt_len = 0;
	s->static_len = 0;
    s->last_lit = 0;
	s->matches = 0;
}

// ===============================================================================
// Initialize the tree data structures for a new zlib stream.
// ===============================================================================

static void tr_init(deflate_state *s)
{
    s->l_desc.dyn_tree = s->dyn_ltree;
    s->l_desc.stat_desc = &static_l_desc;

    s->d_desc.dyn_tree = s->dyn_dtree;
    s->d_desc.stat_desc = &static_d_desc;

    s->bl_desc.dyn_tree = s->bl_tree;
    s->bl_desc.stat_desc = &static_bl_desc;

    s->bi_buf = 0;
    s->bi_valid = 0;
	// enough lookahead for inflate
    s->last_eob_len = 8; 

    // Initialize the first block of the first file:
    init_block(s);
}

// ===============================================================================
// Compares to subtrees, using the tree depth as tie breaker when
// the subtrees have equal frequency. This minimizes the worst case length.
// ===============================================================================

static bool smaller(ct_data *tree, ulong son, ulong daughter, byte *depth)
{
	if(tree[son].fc.freq < tree[daughter].fc.freq)
	{
		return(true);
	}
	if((tree[son].fc.freq == tree[daughter].fc.freq) && (depth[son] <= depth[daughter]))
	{
		return(true);
	}
	return(false);
}

// ===============================================================================
// Restore the heap property by moving down the tree starting at node k,
// exchanging a node with the smallest of its two sons if necessary, stopping
// when the heap property is re-established (each father smaller than its
// two sons).
// ===============================================================================

static void pqdownheap(deflate_state *s, ct_data *tree, ulong node)
{
    ulong	base; 
    ulong	sibling;				// left son of node

	base = s->heap[node];
	sibling = node << 1;

    while(sibling <= s->heap_len) 
	{
        // Set sibling to the smallest of the two children
        if((sibling < s->heap_len) && smaller(tree, s->heap[sibling + 1], s->heap[sibling], s->depth)) 
		{
            sibling++;
        }
        // Exit if base is smaller than both sons
        if(smaller(tree, base, s->heap[sibling], s->depth)) 
		{
			break;
		}
        // Exchange base with the smallest son
        s->heap[node] = s->heap[sibling];  
		node = sibling;

        // And continue down the tree, setting sibling to the left son of base
        sibling <<= 1;
    }
    s->heap[node] = base;
}

// ===============================================================================
// Compute the optimal bit lengths for a tree and update the total bit length
// for the current block.
// IN assertion: the fields freq and dad are set, heap[heap_max] and
//    above are the tree nodes sorted by increasing frequency.
// OUT assertions: the field len is set to the optimal bit length, the
//     array bl_count contains the frequencies for each bit length.
//     The length opt_len is updated; static_len is also updated if stree is
//     not null.
// ===============================================================================

static void gen_bitlen(deflate_state *s, tree_desc *desc)
{
    const ct_data	*stree;
    const ulong		*extra;
    ulong			base;
    ulong			max_length;
    ulong			heapIdx;		// heap index
    ulong			n, m;			// iterate over the tree elements
    ulong			bits;			// bit length
    ulong			xbits;			// extra bits
    word			freq;			// frequency
    ulong			overflow;		// number of elements with bit length too large

    stree = desc->stat_desc->static_tree;
    extra = desc->stat_desc->extra_bits;
    base = desc->stat_desc->extra_base;
    max_length = desc->stat_desc->max_length;
	overflow = 0;

    for(bits = 0; bits <= MAX_WBITS; bits++) 
	{
		s->bl_count[bits] = 0;
	}

    // In a first pass, compute the optimal bit lengths (which may
    // overflow in the case of the bit length tree).
	// root of the heap
    desc->dyn_tree[s->heap[s->heap_max]].dl.len = 0;

    for(heapIdx = s->heap_max + 1; heapIdx < HEAP_SIZE; heapIdx++) 
	{
        n = s->heap[heapIdx];
        bits = desc->dyn_tree[desc->dyn_tree[n].dl.dad].dl.len + 1;
        if(bits > max_length)
		{
			bits = max_length;
			overflow++;
		}
        // We overwrite tree[n].dl.dad which is no longer needed
        desc->dyn_tree[n].dl.len = (word)bits;

		// not a leaf node
        if(n > desc->max_code) 
		{
			continue; 
		}

        s->bl_count[bits]++;
        xbits = 0;
        if(n >= base) 
		{
			xbits = extra[n - base];
		}
        freq = desc->dyn_tree[n].fc.freq;
        s->opt_len += freq * (bits + xbits);
        if(stree) 
		{
			s->static_len += freq * (stree[n].dl.len + xbits);
		}
    }
    if(!overflow) 
	{
		return;
	}

    // Find the first bit length which could increase
    do 
	{
        bits = max_length - 1;
        while(!s->bl_count[bits]) 
		{
			bits--;
		}
		// move one leaf down the tree
        s->bl_count[bits]--;      
		// move one overflow item as its brother
        s->bl_count[bits + 1] += 2;
        // The brother of the overflow item also moves one step up,
        // but this does not affect bl_count[max_length]
        s->bl_count[max_length]--;
        overflow -= 2;
    } 
	while(overflow > 0);

    // Now recompute all bit lengths, scanning in increasing frequency.
    // heapIdx is still equal to HEAP_SIZE. (It is simpler to reconstruct all
    // lengths instead of fixing only the wrong ones. This idea is taken
    // from 'ar' written by Haruhiko Okumura.)
    for(bits = max_length; bits; bits--) 
	{
        n = s->bl_count[bits];
        while(n) 
		{
            m = s->heap[--heapIdx];
            if(m > desc->max_code) 
			{
				continue;
			}
            if(desc->dyn_tree[m].dl.len != bits) 
			{
                s->opt_len += (bits - desc->dyn_tree[m].dl.len) * desc->dyn_tree[m].fc.freq;
                desc->dyn_tree[m].dl.len = (word)bits;
            }
            n--;
        }
    }
}

// ===============================================================================
// Flush the bit buffer and align the output on a byte boundary
// ===============================================================================

static void bi_windup(deflate_state *s)
{
    if(s->bi_valid > 8) 
	{
        put_short(s, s->bi_buf);
    } 
	else if(s->bi_valid > 0) 
	{
        put_byte(s, (byte)s->bi_buf);
    }
    s->bi_buf = 0;
    s->bi_valid = 0;
}

// ===============================================================================
// Reverse the first len bits of a code, using straightforward code (a faster
// method would use a table)
// ===============================================================================

static ulong bi_reverse(ulong code, ulong len)
{
    ulong	res;

	assert(1 <= len);
	assert(len <= 15);

    res = 0;
    do 
	{
        res |= code & 1;
        code >>= 1;
		res <<= 1;
    } 
	while(--len > 0);

    return(res >> 1);
}

// ===============================================================================
// Generate the codes for a given tree and bit counts (which need not be optimal).
// IN assertion: the array bl_count contains the bit length statistics for
// the given tree and the field len is set for all tree elements.
// OUT assertion: the field code is set for all tree elements of non zero code length.
// ===============================================================================

static void gen_codes(ct_data *tree, ulong max_code, word *bl_count)
{
    word	next_code[MAX_WBITS + 1];	// next code value for each bit length
    word	code;						// running code value
    ulong	bits;						// bit index
    ulong	codes;						// code index
	ulong	len;

    // The distribution counts are first used to generate the code values
    // without bit reversal.
	code = 0;
    for(bits = 1; bits <= MAX_WBITS; bits++) 
	{
		code = (word)((code + bl_count[bits - 1]) << 1);
        next_code[bits] = code;
    }

    // Check that the bit counts in bl_count are consistent. The last code
    // must be all ones.
    for(codes = 0; codes <= max_code; codes++) 
	{
        len = tree[codes].dl.len;

        if(!len) 
		{
			continue;
		}
        // Now reverse the bits
        tree[codes].fc.code = (word)bi_reverse(next_code[len]++, len);
    }
}

// ===============================================================================
// Construct one Huffman tree and assigns the code bit strings and lengths.
// Update the total bit length for the current block.
// IN assertion: the field freq is set for all tree elements.
// OUT assertions: the fields len and code are set to the optimal bit length
//     and corresponding code. The length opt_len is updated; static_len is
//     also updated if stree is not null. The field max_code is set.
// ===============================================================================

static void build_tree(deflate_state *s, tree_desc *desc)
{
    ct_data			*tree;
    const ct_data	*stree;
    ulong			elems;
    ulong			n, m;	 		// iterate over heap elements
    ulong			max_code;		// largest code with non zero frequency
    ulong			node;			// new node being created

    tree = desc->dyn_tree;
    stree = desc->stat_desc->static_tree;
    elems = desc->stat_desc->elems;
	max_code = 0;

    // Construct the initial heap, with least frequent element in
    // heap[SMALLEST]. The sons of heap[n] are heap[2*n] and heap[2*n+1].
    // heap[0] is not used.
    s->heap_len = 0;
	s->heap_max = HEAP_SIZE;

    for(n = 0; n < elems; n++) 
	{
        if(tree[n].fc.freq) 
		{
			max_code = n;
            s->heap[++s->heap_len] = n;
            s->depth[n] = 0;
        } 
		else 
		{
            tree[n].dl.len = 0;
        }
    }

    // The pkzip format requires that at least one distance code exists,
    // and that at least one bit should be sent even if there is only one
    // possible code. So to avoid special checks later on we force at least
    // two codes of non zero frequency.
    while(s->heap_len < 2) 
	{
		s->heap[++s->heap_len] = (max_code < 2 ? ++max_code : 0);
        node = s->heap[s->heap_len];
        tree[node].fc.freq = 1;
        s->depth[node] = 0;
        s->opt_len--; 
		if(stree) 
		{
			s->static_len -= stree[node].dl.len;
		}
        // node is 0 or 1 so it does not have extra bits
    }
    desc->max_code = max_code;

    // The elements heap[heap_len/2+1 .. heap_len] are leaves of the tree,
    // establish sub-heaps of increasing lengths:
    for(n = s->heap_len >> 1; n >= 1; n--) 
	{
		pqdownheap(s, tree, n);
	}

    // Construct the Huffman tree by repeatedly combining the least two
    // frequent nodes.

	// next internal node of the tree
    node = elems;              
    do 
	{
	    n = s->heap[SMALLEST];
		s->heap[SMALLEST] = s->heap[s->heap_len--];
		pqdownheap(s, tree, SMALLEST);
        m = s->heap[SMALLEST];						// m = node of next least frequency

        s->heap[--s->heap_max] = n;					// keep the nodes sorted by frequency
        s->heap[--s->heap_max] = m;

        // Create a new node father of n and m
        tree[node].fc.freq = (word)(tree[n].fc.freq + tree[m].fc.freq);
		if(s->depth[n] > s->depth[m])
		{
			s->depth[node] = s->depth[n];
		}
		else
		{
			s->depth[node] = s->depth[m];
			s->depth[node]++;
		}
		tree[m].dl.dad = (word)node;
        tree[n].dl.dad = (word)node;

        // and insert the new node in the heap
        s->heap[SMALLEST] = node++;
        pqdownheap(s, tree, SMALLEST);
    } 
	while(s->heap_len >= 2);

    s->heap[--s->heap_max] = s->heap[SMALLEST];

    // At this point, the fields freq and dad are set. We can now
    // generate the bit lengths.
    gen_bitlen(s, desc);

    // The field len is now set, we can generate the bit codes
    gen_codes (tree, max_code, s->bl_count);
}

// ===============================================================================
// Scan a literal or distance tree to determine the frequencies of the codes
// in the bit length tree.
// ===============================================================================

static void scan_tree (deflate_state *s, ct_data *tree, ulong max_code)
{
    ulong 	n;						// iterates over all tree elements
    ulong	prevlen;				// last emitted length
    ulong	curlen;					// length of current code
    ulong	nextlen;				// length of next code
    ulong	count;					// repeat count of the current code
    ulong	max_count;				// max repeat count
    ulong	min_count;				// min repeat count

    prevlen = 0xffff;
    nextlen = tree[0].dl.len;
    count = 0;
    max_count = 7;
    min_count = 4;

    if(!nextlen) 
	{
		max_count = 138;
		min_count = 3;
	}
	// guard
    tree[max_code + 1].dl.len = (word)prevlen; 

    for(n = 0; n <= max_code; n++) 
	{
        curlen = nextlen; 
		nextlen = tree[n + 1].dl.len;
        if((++count < max_count) && (curlen == nextlen)) 
		{
            continue;
        }
		else if(count < min_count) 
		{
            s->bl_tree[curlen].fc.freq += (word)count;
        } 
		else if(curlen) 
		{
            if(curlen != prevlen) 
			{
				s->bl_tree[curlen].fc.freq++;
			}
            s->bl_tree[REP_3_6].fc.freq++;
        } 
		else if(count <= 10) 
		{
            s->bl_tree[REPZ_3_10].fc.freq++;
        } 
		else 
		{
            s->bl_tree[REPZ_11_138].fc.freq++;
        }
        count = 0; 
		prevlen = curlen;
        if(!nextlen) 
		{
            max_count = 138;
			min_count = 3;
        } 
		else if(curlen == nextlen) 
		{
            max_count = 6;
			min_count = 3;
        } 
		else 
		{
            max_count = 7;
			min_count = 4;
        }
    }
}

// ===============================================================================
// Send a literal or distance tree in compressed form, using the codes in bl_tree.
// ===============================================================================

static void send_tree(deflate_state *s, ct_data *tree, ulong max_code)
{
    ulong	n;						// iterates over all tree elements
    ulong	prevlen;				// last emitted length
    ulong	curlen;					// length of current code
    ulong	nextlen;				// length of next code
    ulong	count;					// repeat count of the current code
    ulong	max_count;				// max repeat count
    ulong	min_count;				// min repeat count

    prevlen = 0xffff;
    nextlen = tree[0].dl.len;
    count = 0;
    max_count = 7;
    min_count = 4;

    if(!nextlen)
	{
		max_count = 138;
		min_count = 3;
	}

    for(n = 0; n <= max_code; n++) 
	{
        curlen = nextlen; 
		nextlen = tree[n + 1].dl.len;
        if((++count < max_count) && (curlen == nextlen))
		{
            continue;
        } 
		else if(count < min_count) 
		{
            do 
			{ 
				send_bits(s, s->bl_tree[curlen].fc.code, s->bl_tree[curlen].dl.len);
			} 
			while(--count);

        } 
		else if(curlen) 
		{
            if(curlen != prevlen) 
			{
				send_bits(s, s->bl_tree[curlen].fc.code, s->bl_tree[curlen].dl.len);
				count--;
            }
			send_bits(s, s->bl_tree[REP_3_6].fc.code, s->bl_tree[REP_3_6].dl.len);
			send_bits(s, count - 3, 2);

        } 
		else if(count <= 10) 
		{
			send_bits(s, s->bl_tree[REPZ_3_10].fc.code, s->bl_tree[REPZ_3_10].dl.len);
			send_bits(s, count - 3, 3);

        } 
		else 
		{
			send_bits(s, s->bl_tree[REPZ_11_138].fc.code, s->bl_tree[REPZ_11_138].dl.len);
			send_bits(s, count - 11, 7);
        }
        count = 0; 
		prevlen = curlen;
        if(!nextlen) 
		{
            max_count = 138;
			min_count = 3;
        } 
		else if(curlen == nextlen) 
		{
            max_count = 6;
			min_count = 3;
        } 
		else 
		{
            max_count = 7;
			min_count = 4;
        }
    }
}

// ===============================================================================
// Construct the Huffman tree for the bit lengths and return the index in
// bl_order of the last bit length code to send.
// ===============================================================================

static ulong build_bl_tree(deflate_state *s)
{
    ulong	max_blindex;			// index of last bit length code of non zero freq

    // Determine the bit length frequencies for literal and distance trees
    scan_tree(s, s->dyn_ltree, s->l_desc.max_code);
    scan_tree(s, s->dyn_dtree, s->d_desc.max_code);

    // Build the bit length tree
    build_tree(s, &s->bl_desc);
    // opt_len now includes the length of the tree representations, except
    // the lengths of the bit lengths codes and the 5+5+4 bits for the counts.

    // Determine the number of bit length codes to send. The pkzip format
    // requires that at least 4 bit length codes be sent. (appnote.txt says
    // 3 but the actual value used is 4.)
    for(max_blindex = BL_CODES - 1; max_blindex >= 3; max_blindex--) 
	{
        if(s->bl_tree[bl_order[max_blindex]].dl.len)
		{
			break;
		}
    }
    // Update opt_len to include the bit length tree and counts
    s->opt_len += 3 * (max_blindex + 1) + 5 + 5 + 4;
    return(max_blindex);
}

// ===========================================================================
// Send the header for a block using dynamic Huffman trees: the counts, the
// lengths of the bit length codes, the literal tree and the distance tree.
// IN assertion: lcodes >= 257, dcodes >= 1, blcodes >= 4.
// ===========================================================================

static void send_all_trees(deflate_state *s, ulong lcodes, ulong dcodes, ulong blcodes)
{
    ulong	rank;				// index in bl_order

	// not +255 as stated in appnote.txt
    send_bits(s, lcodes - 257, 5); 
    send_bits(s, dcodes - 1, 5);
	// not -3 as stated in appnote.txt
    send_bits(s, blcodes - 4, 4); 

    for(rank = 0; rank < blcodes; rank++) 
	{
        send_bits(s, s->bl_tree[bl_order[rank]].dl.len, 3);
    }

	// literal tree
    send_tree(s, (ct_data *)s->dyn_ltree, lcodes - 1); 
	// distance tree
    send_tree(s, (ct_data *)s->dyn_dtree, dcodes - 1); 
}

// ===========================================================================
// Send the block data compressed using the given Huffman trees
// ===========================================================================

static void compress_block(deflate_state *s, const ct_data *ltree, const ct_data *dtree)
{
    ulong	dist;					// distance of matched string
    ulong	lenCount;				// match length or unmatched char (if dist == 0)
    ulong	lenIdx;					// running index in l_buf
    ulong	code;					// the code to send
    ulong	extra;					// number of extra bits to send

	lenIdx = 0;
    if(s->last_lit) 
	{
		do 
		{
			dist = s->d_buf[lenIdx];
			lenCount = s->l_buf[lenIdx++];
			if(!dist) 
			{
				// send a literal byte
				send_bits(s, ltree[lenCount].fc.code, ltree[lenCount].dl.len);
			} 
			else 
			{
				// Here, lenCount is the match length - MIN_MATCH
				code = tr_length_code[lenCount];
				// send the length code
				send_bits(s, ltree[code + LITERALS + 1].fc.code, ltree[code + LITERALS + 1].dl.len);
				extra = extra_lbits[code];
				if(extra) 
				{
					lenCount -= base_length[code];
					// send the extra length bits
					send_bits(s, lenCount, extra);       
				}
				// dist is now the match distance - 1
				dist--; 
				code = (dist < 256 ? tr_dist_code[dist] : tr_dist_code[256 + (dist >> 7)]);

				// send the distance code
				send_bits(s, dtree[code].fc.code, dtree[code].dl.len);
				extra = extra_dbits[code];
				if(extra) 
				{
					dist -= base_dist[code];
					// send the extra distance bits
					send_bits(s, dist, extra);   
				}
			}
		} 
		while(lenIdx < s->last_lit);
	}

	send_bits(s, ltree[END_BLOCK].fc.code, ltree[END_BLOCK].dl.len);
    s->last_eob_len = ltree[END_BLOCK].dl.len;
}

// ===========================================================================
// Send a stored block
// ===========================================================================

static void tr_stored_block(deflate_state *s, const byte *buf, ulong stored_len, bool eof)
{
	// send block type
    send_bits(s, (STORED_BLOCK << 1) + (ulong)eof, 3);

	// align on byte boundary
    bi_windup(s);        
	// enough lookahead for inflate
    s->last_eob_len = 8; 

	put_short(s, (word)stored_len);   
	put_short(s, (word)~stored_len);

    while(stored_len--)
	{
        put_byte(s, *buf++);
    }
}

// ===========================================================================
// Determine the best encoding for the current block: dynamic trees, static
// trees or store, and output the encoded block to the zip file.
// ===========================================================================

static void tr_flush_block(deflate_state *s, const byte *buf, ulong stored_len, bool eof)
{
    ulong	opt_lenb;
    ulong	static_lenb;
    ulong	max_blindex;			// index of last bit length code of non zero freq

    max_blindex = 0;

    // Build the Huffman trees unless a stored block is forced
    if(s->level > 0) 
	{
		// Construct the literal and distance trees
		build_tree(s, &s->l_desc);
		build_tree(s, &s->d_desc);
		// At this point, opt_len and static_len are the total bit lengths of
		// the compressed block data, excluding the tree representations.

		// Build the bit length tree for the above two trees, and get the index
		// in bl_order of the last bit length code to send.
		max_blindex = build_bl_tree(s);

		// Determine the best encoding. Compute first the block length in bytes
		opt_lenb = (s->opt_len + 3 + 7) >> 3;
		static_lenb = (s->static_len + 3 + 7) >> 3;

		if(static_lenb <= opt_lenb) 
		{
			opt_lenb = static_lenb;
		}
    } 
	else 
	{
		static_lenb = stored_len + 5;
		// force a stored block
		opt_lenb = static_lenb;		
    }

    if(stored_len + 4 <= opt_lenb && buf) 
	{
		// 4: two words for the lengths
        // The test buf != NULL is only necessary if LIT_BUFSIZE > WSIZE.
        // Otherwise we can't have processed more than WSIZE input bytes since
        // the last block flush, because compression would have been
        // successful. If LIT_BUFSIZE <= WSIZE, it is never too late to
        // transform a block into a stored block.
        tr_stored_block(s, buf, stored_len, eof);

    } 
	else if(static_lenb == opt_lenb) 
	{
        send_bits(s, (STATIC_TREES << 1) + (ulong)eof, 3);
        compress_block(s, static_ltree, static_dtree);
    } 
	else 
	{
        send_bits(s, (DYN_TREES << 1) + (ulong)eof, 3);
        send_all_trees(s, s->l_desc.max_code + 1, s->d_desc.max_code + 1, max_blindex + 1);
        compress_block(s, s->dyn_ltree, s->dyn_dtree);
    }

    // The above check is made mod 2^32, for files larger than 512 MB
    // and uLong implemented on 32 bits.
    init_block(s);

    if(eof) 
	{
        bi_windup(s);
    }
}

// ===============================================================================
// ===============================================================================

inline bool tr_tally_lit(deflate_state *s, byte c)
{
	s->d_buf[s->last_lit] = 0;
	s->l_buf[s->last_lit++] = c;
	s->dyn_ltree[c].fc.freq++;

	return(s->last_lit == LIT_BUFSIZE - 1);
}

// ===============================================================================
// ===============================================================================

inline bool tr_tally_dist(deflate_state *s, ulong dist, ulong len)
{ 
	assert(dist < 65536);
	assert(len < 256);

	dist &= 0xffff;
	len &= 0xff;

    s->d_buf[s->last_lit] = (word)dist;
    s->l_buf[s->last_lit++] = (byte)len;
    dist--;
    s->dyn_ltree[tr_length_code[len] + LITERALS + 1].fc.freq++;
    s->dyn_dtree[(dist < 256 ? tr_dist_code[dist] : tr_dist_code[256 + (dist >> 7)])].fc.freq++;

    return(s->last_lit == LIT_BUFSIZE - 1);
}

// ===============================================================================
// Insert string str in the dictionary and set match_head to the previous head
// of the hash chain (the most recent string with same hash key). Return
// the previous length of the hash chain.
// If this file is compiled with -DFASTEST, the compression level is forced
// to 1, and no hash chains are maintained.
// IN  assertion: all calls to to INSERT_STRING are made with consecutive
//    input characters and the first MIN_MATCH bytes of str are valid
//    (except for the last MIN_MATCH-1 bytes of the input file).
// ===============================================================================

inline void insert_string(deflate_state *s, ulong str, ulong &match_head)
{
	s->ins_h = ((s->ins_h << HASH_SHIFT) ^ s->window[str + (MIN_MATCH - 1)]) & HASH_MASK;
	match_head = s->head[s->ins_h];
    s->prev[str & WINDOW_MASK] = s->head[s->ins_h]; 
    s->head[s->ins_h] = (word)str;
}

// ===========================================================================
// Initialize the "longest match" routines for a new zlib stream
// ===========================================================================

static void lm_init(deflate_state *s)
{
    s->head[HASH_SIZE - 1] = NULL;
    memset(s->head, 0, (HASH_SIZE - 1) * sizeof(*s->head));

    // Set the default configuration parameters:
    s->max_lazy_match = configuration_table[s->level].max_lazy;
    s->good_match = configuration_table[s->level].good_length;
    s->nice_match = configuration_table[s->level].nice_length;
    s->max_chain_length = configuration_table[s->level].max_chain;

    s->strstart = 0;
    s->block_start = 0;
    s->lookahead = 0;
	s->prev_length = MIN_MATCH - 1;
    s->match_length = MIN_MATCH - 1;
    s->match_available = 0;
    s->ins_h = 0;
}

// ===========================================================================
// Set match_start to the longest match starting at the given string and
// return its length. Matches shorter or equal to prev_length are discarded,
// in which case the result is equal to prev_length and match_start is
// garbage.
// IN assertions: cur_match is the head of the hash chain for the current
//  string (strstart) and its distance is <= MAX_DIST, and prev_length >= 1
// OUT assertion: the match length is not greater than s->lookahead.
// ===========================================================================

inline byte *qcmp(byte *scan, byte *match, ulong count)
{
	byte	*retval;

	_asm
	{
		push	esi
		push	edi
		push	ecx

		mov		esi, [scan]
		mov		edi, [match]
		mov		ecx, [count]
		repe	cmpsb

		pop		ecx
		pop		edi
		mov		[retval], esi
		pop		esi
	}
	return(--retval);
}

static ulong longest_match(deflate_state *s, ulong cur_match)
{
	ulong	chain_length;			// max hash chain length
	ulong	limit;
	byte	*scan;					// current string
	byte	*match;					// matched string
	ulong	len;					// length of current match
	ulong	best_len;				// best match length so far
	ulong	nice_match;				// stop if match long enough
    byte	scan_end1;
    byte	scan_end;

	chain_length = s->max_chain_length;
	scan = s->window + s->strstart;
	best_len = s->prev_length;
	nice_match = s->nice_match;

    // Stop when cur_match becomes <= limit. To simplify the code,
    // we prevent matches with the string of window index 0.
	limit = s->strstart > (WINDOW_SIZE - MIN_LOOKAHEAD) ? s->strstart - (WINDOW_SIZE - MIN_LOOKAHEAD) : NULL;

    scan_end1 = scan[best_len - 1];
    scan_end = scan[best_len];

    // Do not waste too much time if we already have a good match:
    if(s->prev_length >= s->good_match) 
	{
        chain_length >>= 2;
    }
    // Do not look for matches beyond the end of the input. This is necessary
    // to make deflate deterministic.
    if(nice_match > s->lookahead) 
	{
		nice_match = s->lookahead;
	}
    do 
	{
        match = s->window + cur_match;

        // Skip to next match if the match length cannot increase
        // or if the match length is less than 2:
        if((match[best_len] != scan_end) || (match[best_len - 1] != scan_end1) || (match[0] != scan[0]) || (match[1] != scan[1]))
		{
			continue;
		}

        // The check at best_len-1 can be removed because it will be made
        // again later. (This heuristic is not always a win.)
        // It is not necessary to compare scan[2] and match[2] since they
        // are always equal when the other bytes match, given that
        // the hash keys are equal
		scan = qcmp(scan + 3, match + 3, MAX_MATCH - 2);

        len = scan - (s->window + s->strstart);
        scan = s->window + s->strstart;

        if(len > best_len) 
		{
            s->match_start = cur_match;
            best_len = len;
            if(len >= nice_match) 
			{
				break;
			}
            scan_end1 = scan[best_len - 1];
            scan_end = scan[best_len];
        }
    } while((cur_match = s->prev[cur_match & WINDOW_MASK]) > limit && --chain_length);

    if(best_len <= s->lookahead) 
	{
		return(best_len);
	}
    return(s->lookahead);
}

// ===========================================================================
// Flush as much pending output as possible. All deflate() output goes
// through this function so some applications may wish to modify it
// to avoid allocating a large z->next_out buffer and copying into it.
// (See also read_buf()).
// ===========================================================================

static void flush_pending(z_stream *z)
{
    ulong	len = z->dstate->pending;

    if(len > z->avail_out) 
	{
		len = z->avail_out;
	}
    if(!len) 
	{
		return;
	}
	assert(len <= MAX_BLOCK_SIZE + 5);
	assert(z->dstate->pending_out + len <= z->dstate->pending_buf + MAX_BLOCK_SIZE + 5);

    memcpy(z->next_out, z->dstate->pending_out, len);
    z->next_out += len;
	z->total_out += len;
    z->dstate->pending_out += len;
    z->avail_out -= len;
    z->dstate->pending -= len;
    if(!z->dstate->pending) 
	{
        z->dstate->pending_out = z->dstate->pending_buf;
    }
}

// ===========================================================================
// Read a new buffer from the current input stream, update the adler32
// and total number of bytes read.  All deflate() input goes through
// this function so some applications may wish to modify it to avoid
// allocating a large z->next_in buffer and copying from it.
// (See also flush_pending()).
// ===========================================================================

static ulong read_buf(z_stream *z, byte *buf, ulong size)
{
    ulong	len;

    len = z->avail_in;
    if(len > size)
	{
		len = size;
	}
    if(!len) 
	{
		return(0);
	}
    z->avail_in -= len;

	if(!z->dstate->noheader)
	{
        z->dstate->adler = adler32(z->dstate->adler, z->next_in, len);
	}
    memcpy(buf, z->next_in, len);
    z->next_in += len;
    return(len);
}

// ===========================================================================
// Fill the window when the lookahead becomes insufficient.
// Updates strstart and lookahead.
//
// IN assertion: lookahead < MIN_LOOKAHEAD
// OUT assertions: strstart <= BIG_WINDOW_SIZE - MIN_LOOKAHEAD
//    At least one byte has been read, or avail_in == 0; reads are
//    performed for at least two bytes (required for the zip translate_eol
//    option -- not supported here).
// ===========================================================================

static void fill_window(deflate_state *s)
{
	ulong		n, m;
	word		*p;
    ulong		more;				// Amount of free space at the end of the window.

    do 
	{
        more = BIG_WINDOW_SIZE - s->lookahead - s->strstart;

		if(s->strstart >= WINDOW_SIZE + (WINDOW_SIZE - MIN_LOOKAHEAD)) 
		{
            memcpy(s->window, s->window + WINDOW_SIZE, WINDOW_SIZE);
            s->match_start -= WINDOW_SIZE;
			// Make strstart >= MAX_DIST
            s->strstart -= WINDOW_SIZE;					
            s->block_start -= WINDOW_SIZE;

			// Slide the hash table (could be avoided with 32 bit values
            // at the expense of memory usage). We slide even when level == 0
            // to keep the hash table consistent if we switch back to level > 0
            // later. (Using level 0 permanently is not an optimal usage of
            // zlib, so we don't care about this pathological case.)
			n = HASH_SIZE;
			p = &s->head[n];
			do
			{
				m = *--p;
				*p = (word)(m >= WINDOW_SIZE ? m - WINDOW_SIZE : 0);
			}
			while(--n);

			n = WINDOW_SIZE;
			p = &s->prev[n];
			do
			{
				m = *--p;
				*p = (word)(m >= WINDOW_SIZE ? m - WINDOW_SIZE : 0);
				// If n is not on any hash chain, prev[n] is garbage but
				// its value will never be used.
			}
			while(--n);

            more += WINDOW_SIZE;
        }
		if(!s->z->avail_in)
		{
			return;
		}

        // If there was no sliding:
        //    strstart <= WSIZE+MAX_DIST-1 && lookahead <= MIN_LOOKAHEAD - 1 &&
        //    more == BIG_WINDOW_SIZE - lookahead - strstart
        // => more >= BIG_WINDOW_SIZE - (MIN_LOOKAHEAD-1 + WSIZE + MAX_DIST-1)
        // => more >= BIG_WINDOW_SIZE- 2*WSIZE + 2
        // If there was sliding, more >= WSIZE. So in all cases, more >= 2.
        n = read_buf(s->z, s->window + s->strstart + s->lookahead, more);
        s->lookahead += n;

        // Initialize the hash value now that we have some input:
        if(s->lookahead >= MIN_MATCH) 
		{
			s->ins_h = ((s->window[s->strstart] << HASH_SHIFT) ^ s->window[s->strstart + 1]) & HASH_MASK;
        }
        // If the whole input has less than MIN_MATCH bytes, ins_h is garbage,
        // but this is not important since only literal bytes will be emitted.
    } 
	while(s->lookahead < MIN_LOOKAHEAD && s->z->avail_in);
}

// ===========================================================================
// Flush the current block, with given end-of-file flag.
// IN assertion: strstart is set to the end of the current match.
// ===========================================================================

inline void flush_block_only(deflate_state *s, bool eof)
{
	if(s->block_start >= 0)
	{
		tr_flush_block(s, &s->window[s->block_start], s->strstart - s->block_start, eof);
	}
	else
	{
		tr_flush_block(s, 0, s->strstart - s->block_start, eof);
	}
   s->block_start = s->strstart;
   flush_pending(s->z);
}

// ===========================================================================
// Copy without compression as much as possible from the input stream, return
// the current block state.
// This function does not insert new strings in the dictionary since
// uncompressible data is probably not useful. This function is used
// only for the level=0 compression option.
// NOTE: this function should be optimized to avoid extra copying from
// window to pending_buf.
// ===========================================================================

static block_state deflate_stored(deflate_state *s, EFlush flush)
{
    // Stored blocks are limited to 0xffff bytes, pending_buf is limited
    // to pending_buf_size, and each stored block has a 5 byte header:
    ulong	max_start;

    // Copy as much as possible from input to output:
    while(true) 
	{
        // Fill the window as much as possible
        if(s->lookahead <= 1) 
		{
            fill_window(s);
            if(!s->lookahead && (flush == Z_NO_FLUSH))
			{
				return(NEED_MORE);
			}
            if(!s->lookahead) 
			{
				// flush the current block
				break;
			}
        }
		s->strstart += s->lookahead;
		s->lookahead = 0;

		// Emit a stored block if pending_buf will be full
		max_start = s->block_start + MAX_BLOCK_SIZE;
        if(!s->strstart || (s->strstart >= max_start))
		{
			// strstart == 0 is possible when wraparound on 16-bit machine
			s->lookahead = s->strstart - max_start;
			s->strstart = max_start;
			flush_block_only(s, false);
			if(!s->z->avail_out)
			{
				return(NEED_MORE);
			}
		}
		// Flush if we may have to slide, otherwise block_start may become
        // negative and the data will be gone:
		if(s->strstart - s->block_start >= WINDOW_SIZE - MIN_LOOKAHEAD) 
		{
			flush_block_only(s, false);
			if(!s->z->avail_out)
			{
				return(NEED_MORE);
			}
		}
    }
	flush_block_only(s, flush == Z_FINISH);
	if(!s->z->avail_out)
	{
		return((flush == Z_FINISH) ? FINISH_STARTED : NEED_MORE);
	}
    return(flush == Z_FINISH ? FINISH_DONE : BLOCK_DONE);
}

// ===========================================================================
// Compress as much as possible from the input stream, return the current block state.
// This function does not perform lazy evaluation of matches and inserts
// new strings in the dictionary only for unmatched strings or for short
// matches. It is used only for the fast compression options.
// ===========================================================================

static block_state deflate_fast(deflate_state *s, EFlush flush)
{
    ulong		hash_head;			// head of the hash chain
    bool		bflush;	 			// set if current block must be flushed

    hash_head = 0;
    while(true)
	{
        // Make sure that we always have enough lookahead, except
        // at the end of the input file. We need MAX_MATCH bytes
        // for the next match, plus MIN_MATCH bytes to insert the
        // string following the next match.
        if(s->lookahead < MIN_LOOKAHEAD) 
		{
            fill_window(s);
            if((s->lookahead < MIN_LOOKAHEAD) && (flush == Z_NO_FLUSH))
			{
		        return(NEED_MORE);
			}
            if(!s->lookahead) 
			{
				// flush the current block
				break; 
			}
        }

        // Insert the string window[strstart .. strstart+2] in the
        // dictionary, and set hash_head to the head of the hash chain:
        if(s->lookahead >= MIN_MATCH) 
		{
            insert_string(s, s->strstart, hash_head);
        }

        // Find the longest match, discarding those <= prev_length.
        // At this point we have always match_length < MIN_MATCH
        if(hash_head && (s->strstart - hash_head <= WINDOW_SIZE - MIN_LOOKAHEAD)) 
		{
            // To simplify the code, we prevent matches with the string
            // of window index 0 (in particular we have to avoid a match
            // of the string with itself at the start of the input file).
			s->match_length = longest_match(s, hash_head);
            // longest_match() sets match_start
        }
        if(s->match_length >= MIN_MATCH) 
		{
			s->z->quality++;

            bflush = tr_tally_dist(s, s->strstart - s->match_start, s->match_length - MIN_MATCH);
            s->lookahead -= s->match_length;

            // Insert new strings in the hash table only if the match length
            // is not too large. This saves time but degrades compression.
            if((s->match_length <= s->max_lazy_match) && (s->lookahead >= MIN_MATCH))
			{
				// string at strstart already in hash table
                s->match_length--; 
                do 
				{
                    // strstart never exceeds WSIZE-MAX_MATCH, so there are
                    // always MIN_MATCH bytes ahead.
                    s->strstart++;
                    insert_string(s, s->strstart, hash_head);
                } 
				while(--s->match_length);
                s->strstart++; 
            } 
			else
			{
				s->z->quality++;

                s->strstart += s->match_length;
                s->match_length = 0;
				s->ins_h = ((s->window[s->strstart] << HASH_SHIFT) ^ s->window[s->strstart + 1]) & HASH_MASK;
                // If lookahead < MIN_MATCH, ins_h is garbage, but it does not
                // matter since it will be recomputed at next deflate call.
            }
        } 
		else 
		{
            // No match, output a literal byte
            bflush = tr_tally_lit(s, s->window[s->strstart]);
            s->lookahead--;
            s->strstart++; 
        }
        if(bflush) 
		{
			flush_block_only(s, false);
			if(!s->z->avail_out)
			{
				return(NEED_MORE);
			}
		}
    }
	flush_block_only(s, flush == Z_FINISH);
	if(!s->z->avail_out)
	{
		return(flush == Z_FINISH ? FINISH_STARTED : NEED_MORE);
	}
    return(flush == Z_FINISH ? FINISH_DONE : BLOCK_DONE);
}

// ===========================================================================
// Same as above, but achieves better compression. We use a lazy
// evaluation for matches: a match is finally adopted only if there is
// no better match at the next window position.
// ===========================================================================

static block_state deflate_slow(deflate_state *s, EFlush flush)
{
    ulong		hash_head;			// head of hash chain
	ulong		max_insert;
    bool		bflush;				// set if current block must be flushed

    hash_head = 0;
    // Process the input block.
    while(true) 
	{
        // Make sure that we always have enough lookahead, except
        // at the end of the input file. We need MAX_MATCH bytes
        // for the next match, plus MIN_MATCH bytes to insert the
        // string following the next match.
        if(s->lookahead < MIN_LOOKAHEAD) 
		{
            fill_window(s);
            if((s->lookahead < MIN_LOOKAHEAD) && (flush == Z_NO_FLUSH))
			{
				return(NEED_MORE);
			}
            if(!s->lookahead) 
			{
				// flush the current block
				break; 
			}
        }

        // Insert the string window[strstart .. strstart+2] in the
        // dictionary, and set hash_head to the head of the hash chain:
        if(s->lookahead >= MIN_MATCH) 
		{
            insert_string(s, s->strstart, hash_head);
        }

        // Find the longest match, discarding those <= prev_length.
        s->prev_length = s->match_length;
		s->prev_match = s->match_start;
        s->match_length = MIN_MATCH - 1;

        if(hash_head && (s->prev_length < s->max_lazy_match) && (s->strstart - hash_head <= WINDOW_SIZE - MIN_LOOKAHEAD)) 
		{
            // To simplify the code, we prevent matches with the string
            // of window index 0 (in particular we have to avoid a match
            // of the string with itself at the start of the input file).
			// longest_match() sets match_start
			s->match_length = longest_match(s, hash_head);

            if((s->match_length <= 5) && ((s->match_length == MIN_MATCH) && (s->strstart - s->match_start > TOO_FAR))) 
			{
                // If prev_match is also MIN_MATCH, match_start is garbage
                // but we will ignore the current match anyway.
                s->match_length = MIN_MATCH - 1;
            }
        }
        // If there was a match at the previous step and the current
        // match is not better, output the previous match:
        if((s->prev_length >= MIN_MATCH) && (s->match_length <= s->prev_length))
		{
            // Do not insert strings in hash table beyond this.
			max_insert = s->strstart + s->lookahead - MIN_MATCH;

            bflush = tr_tally_dist(s, s->strstart - 1 - s->prev_match, s->prev_length - MIN_MATCH);

            // Insert in hash table all strings up to the end of the match.
            // strstart-1 and strstart are already inserted. If there is not
            // enough lookahead, the last two strings are not inserted in
            // the hash table.
            s->lookahead -= s->prev_length - 1;
            s->prev_length -= 2;
            do 
			{
                if(++s->strstart <= max_insert) 
				{
                    insert_string(s, s->strstart, hash_head);
                }
            } 
			while(--s->prev_length);

            s->match_available = 0;
            s->match_length = MIN_MATCH - 1;
            s->strstart++;

            if(bflush) 
			{
				flush_block_only(s, false);
				if(!s->z->avail_out)
				{
					return(NEED_MORE);
				}
			}
        } 
		else if(s->match_available)
		{
            // If there was no match at the previous position, output a
            // single literal. If there was a match but the current match
            // is longer, truncate the previous match to a single literal.
		    bflush = tr_tally_lit(s, s->window[s->strstart - 1]);
		    if(bflush) 
			{
                flush_block_only(s, false);
            }
            s->strstart++;
            s->lookahead--;
            if(!s->z->avail_out) 
			{
				return(NEED_MORE);
			}
        } 
		else 
		{
            // There is no previous match to compare with, wait for
            // the next step to decide.
            s->match_available = 1;
            s->strstart++;
            s->lookahead--;
        }
    }
    if(s->match_available) 
	{
        bflush = tr_tally_lit(s, s->window[s->strstart - 1]);
        s->match_available = 0;
    }
	flush_block_only(s, flush == Z_FINISH);
	if(!s->z->avail_out)
	{
		return(flush == Z_FINISH ? FINISH_STARTED : NEED_MORE);
	}
    return(flush == Z_FINISH ? FINISH_DONE : BLOCK_DONE);
}

// -------------------------------------------------------------------------------------------------
// Controlling routines
// -------------------------------------------------------------------------------------------------

EStatus deflateInit(z_stream *z, ELevel level, int noWrap)
{
    deflate_state	*s;

	assert(z);

    deflate_error = "OK";
    if((level < Z_STORE_COMPRESSION) || (level > Z_MAX_COMPRESSION)) 
	{
		deflate_error = "Invalid compression level";
        return(Z_STREAM_ERROR);
    }
    s = (deflate_state *)Z_Malloc(sizeof(deflate_state), TAG_DEFLATE, qtrue);
    z->dstate = (deflate_state *)s;
    s->z = z;

  	// undocumented feature: suppress zlib header
	s->noheader = noWrap;
    s->level = level;

	z->total_out = 0;
	z->quality = 0;

    s->pending = 0;
    s->pending_out = s->pending_buf;

    s->status = s->noheader ? BUSY_STATE : INIT_STATE;
    s->adler = 1;
    s->last_flush = Z_NO_FLUSH;

    tr_init(s);
    lm_init(s);
	return(Z_OK);
}

// ===========================================================================
// Copy the source state to the destination state.
// To simplify the source, this is not supported for 16-bit MSDOS (which
// doesn't have enough memory anyway to duplicate compression states).
// ===========================================================================

EStatus deflateCopy(z_stream *dest, z_stream *source)
{
    deflate_state	*ds;
    deflate_state	*ss;

	assert(source);
	assert(dest);
	assert(source->dstate);
	assert(!dest->dstate);

    *dest = *source;

    ss = source->dstate;
    ds = (deflate_state *)Z_Malloc(sizeof(deflate_state), TAG_DEFLATE, qtrue);
    dest->dstate = ds;
    *ds = *ss;
    ds->z = dest;

    ds->pending_out = ds->pending_buf + (ss->pending_out - ss->pending_buf);

    ds->l_desc.dyn_tree = ds->dyn_ltree;
    ds->d_desc.dyn_tree = ds->dyn_dtree;
    ds->bl_desc.dyn_tree = ds->bl_tree;

    return(Z_OK);
}

// ===========================================================================
// ===========================================================================

EStatus deflate(z_stream *z, EFlush flush)
{
    EFlush			old_flush;		// value of flush param for previous deflate call
    deflate_state	*s;
	ulong			header;
	ulong			level_flags;

	assert(z);
	assert(z->dstate);

    if((flush > Z_FINISH) || (flush < Z_NO_FLUSH)) 
	{
		deflate_error = "Invalid flush type"; 
        return(Z_STREAM_ERROR);
    }
    s = z->dstate;

    if(!z->next_out || (!z->next_in && z->avail_in) || (s->status == FINISH_STATE && flush != Z_FINISH)) 
	{
		deflate_error = "Invalid output data"; 
		return (Z_STREAM_ERROR);
    }
    if(!z->avail_out)
	{
		deflate_error = "No output space"; 
		return (Z_BUF_ERROR);
	}

    old_flush = s->last_flush;
    s->last_flush = flush;

    // Write the zlib header
    if(s->status == INIT_STATE) 
	{
		header = (ZF_DEFLATED + ((MAX_WBITS - 8) << 4)) << 8;
        level_flags = (s->level - 1) >> 1;

        if(level_flags > 3)
		{
			level_flags = 3;
		}
        header |= (level_flags << 6);

        header += 31 - (header % 31);

        s->status = BUSY_STATE;
        put_shortMSB(s, (word)header);
		s->adler = 1;
    }

	// Flush as much pending output as possible
	if(s->pending)
	{
        flush_pending(z);
        if(!z->avail_out)
		{
			// Since avail_out is 0, deflate will be called again with
			// more output space, but possibly with both pending and
			// avail_in equal to zero. There won't be anything to do,
			// but this is not an error situation so make sure we
			// return OK instead of BUF_ERROR at next call of deflate:
			s->last_flush = Z_NEED_MORE;
			return(Z_OK);
		}

		// Make sure there is something to do and avoid duplicate consecutive
		// flushes. For repeated and useless calls with Z_FINISH, we keep
		// returning Z_STREAM_END instead of Z_BUFF_ERROR.
	}
	else if(!z->avail_in && (flush <= old_flush) && (flush != Z_FINISH))
	{
		deflate_error = "No available input"; 
		return(Z_BUF_ERROR);
    }

    // User must not provide more input after the first FINISH
    if((s->status == FINISH_STATE) && z->avail_in)
	{        
		deflate_error = "Trying to finish while input available"; 
		return(Z_BUF_ERROR);
    }

    // Start a new block or continue the current one.
    if(z->avail_in || s->lookahead || ((flush != Z_NO_FLUSH) && (s->status != FINISH_STATE)))
	{
        block_state	bstate;

		bstate = (*(configuration_table[s->level].func))(s, flush);

        if((bstate == FINISH_STARTED) || (bstate == FINISH_DONE))
		{
            s->status = FINISH_STATE;
        }
        if((bstate == NEED_MORE) || (bstate == FINISH_STARTED))
		{
			if(!z->avail_out) 
			{
				// avoid BUF_ERROR next call, see above
				s->last_flush = Z_NEED_MORE; 
			}
			return(Z_OK);

			// If flush != Z_NO_FLUSH && avail_out == 0, the next call
			// of deflate should use the same flush parameter to make sure
			// that the flush is complete. So we don't have to output an
			// empty block here, this will be done at next call. This also
			// ensures that for a very small output buffer, we emit at most
			// one empty block.
		}
		if(bstate == BLOCK_DONE) 
		{
			// FULL_FLUSH or SYNC_FLUSH
            tr_stored_block(s, NULL, 0, false);

            flush_pending(z);
			if(!z->avail_out)
			{
				// avoid BUF_ERROR at next call, see above
				s->last_flush = Z_NEED_MORE; 
				return(Z_OK);
			}
		}
	}

    if(flush != Z_FINISH)
	{
		return(Z_OK);
	}
    if(s->noheader) 
	{
		return(Z_STREAM_END);
	}

    // Write the zlib trailer (adler32)
    put_longMSB(s, s->adler);
    flush_pending(z);

    // If avail_out is zero, the application will call deflate again
    // to flush the rest. Write the trailer only once!
    s->noheader = -1; 
    return(!!s->pending ? Z_OK : Z_STREAM_END);
}

// ===========================================================================
// ===========================================================================

EStatus deflateEnd(z_stream *z)
{
    int		status;

	assert(z);
	assert(z->dstate);

    status = z->dstate->status;
    if((status != INIT_STATE) && (status != BUSY_STATE) && (status != FINISH_STATE))
	{
		deflate_error = "Invalid state while ending";
		return(Z_STREAM_ERROR);
    }

    Z_Free(z->dstate);
    z->dstate = NULL;

	if(status == BUSY_STATE)
	{
		deflate_error = "Ending while in busy state";
		return(Z_DATA_ERROR);
	}
    return(Z_OK);
}

// ===========================================================================
// ===========================================================================

const char *deflateError(void)
{
	return(deflate_error);
}

// ===============================================================================
// External calls
// ===============================================================================

bool DeflateFile(byte *src, ulong uncompressedSize, byte *dst, ulong maxCompressedSize, ulong *compressedSize, ELevel level, int noWrap)
{
	z_stream	z = { 0 };

	if(deflateInit(&z, level, noWrap) != Z_OK)
	{
		return(false);
	}

	z.next_in = src;
	z.avail_in = uncompressedSize;
	z.next_out = dst;
	z.avail_out = maxCompressedSize;
#ifdef _TIMING
	int temp = timeGetTime();
#endif
	if(deflate(&z, Z_FINISH) != Z_STREAM_END)
	{
		deflateEnd(&z);
		return(false);
	}
#ifdef _TIMING
	totalDeflateTime[level] += timeGetTime() - temp;
	totalDeflateCount[level]++;
#endif
	if(deflateEnd(&z) != Z_OK)
	{
		return(false);
	}
	*compressedSize = z.total_out;
	return(true);
}

// end