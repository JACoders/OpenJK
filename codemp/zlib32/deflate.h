// Stream status
#define INIT_STATE					42
#define BUSY_STATE					113
#define FINISH_STATE				666

#define HASH_BITS					15
#define HASH_SIZE					(1 << HASH_BITS)
#define HASH_MASK					(HASH_SIZE - 1)

// Size of match buffer for literals/lengths.  There are 4 reasons for
// limiting lit_bufsize to 64K:
//   - frequencies can be kept in 16 bit counters
//   - if compression is not successful for the first block, all input
//     data is still in the window so we can still emit a stored block even
//     when input comes from standard input.  (This can also be done for
//     all blocks if lit_bufsize is not greater than 32K.)
//   - if compression is not successful for a file smaller than 64K, we can
//     even emit a stored file instead of a stored block (saving 5 bytes).
//     This is applicable only for zip (not gzip or zlib).
//   - creating new Huffman trees less frequently may not provide fast
//     adaptation to changes in the input data statistics. (Take for
//     example a binary file with poorly compressible code followed by
//     a highly compressible string table.) Smaller buffer sizes give
//     fast adaptation but have of course the overhead of transmitting
//     trees more frequently.
//   - I can't count above 4
#define LIT_BUFSIZE					(1 << 14)

#define MAX_BLOCK_SIZE				0xffff

// Number of bits by which ins_h must be shifted at each input
// step. It must be such that after MIN_MATCH steps, the oldest
// byte no longer takes part in the hash key.
#define HASH_SHIFT					((HASH_BITS + MIN_MATCH - 1) / MIN_MATCH)

// Matches of length 3 are discarded if their distance exceeds TOO_FAR
#define TOO_FAR						32767

// Number of length codes, not counting the special END_BLOCK code
#define LENGTH_CODES				29

// Number of codes used to transfer the bit lengths
#define BL_CODES					19

// Number of literal bytes 0..255
#define LITERALS					256

// Number of Literal or Length codes, including the END_BLOCK code
#define L_CODES						(LITERALS + 1 + LENGTH_CODES)

// See definition of array dist_code below
#define DIST_CODE_LEN				512 

// Maximum heap size
#define HEAP_SIZE					(2 * L_CODES + 1)

// Index within the heap array of least frequent node in the Huffman tree
#define SMALLEST					1

// Bit length codes must not exceed MAX_BL_BITS bits
#define MAX_BL_BITS					7

// End of block literal code
#define END_BLOCK					256

// Repeat previous bit length 3-6 times (2 bits of repeat count)
#define REP_3_6						16

// Repeat a zero length 3-10 times  (3 bits of repeat count)
#define REPZ_3_10					17

// Repeat a zero length 11-138 times  (7 bits of repeat count)
#define REPZ_11_138					18

// Number of bits used within bi_buf. (bi_buf might be implemented on
// more than 16 bits on some systems.)
#define BUF_SIZE					(8 * 2)

// Minimum amount of lookahead, except at the end of the input file.
// See deflate.c for comments about the MIN_MATCH+1.
#define MIN_LOOKAHEAD				(MAX_MATCH + MIN_MATCH + 1)

typedef enum 
{
    NEED_MORE,									// block not completed, need more input or more output
    BLOCK_DONE,									// block flush performed
    FINISH_STARTED,								// finish started, need only more output at next deflate
    FINISH_DONE									// finish done, accept no more input or output
} block_state;

// Data structure describing a single value and its code string.
typedef struct ct_data_s 
{
    union 
	{
        word  freq;								// frequency count
        word  code;								// bit string
    } fc;										
    union										
	{											
        word  dad;								// father node in Huffman tree
        word  len;								// length of bit string
    } dl;										
} ct_data;										
												
typedef struct static_tree_desc_s				
{												
    const ct_data	*static_tree; 				// static tree or NULL
    const ulong		*extra_bits;  				// extra bits for each code or NULL
    ulong			extra_base;	  				// base index for extra_bits
    ulong			elems;		  				// max number of elements in the tree
    ulong			max_length;	  				// max bit length for the codes
} static_tree_desc;

typedef struct tree_desc_s 
{
    ct_data 			*dyn_tree;				// the dynamic tree
    ulong		 		max_code;				// largest code with non zero frequency
    static_tree_desc	*stat_desc;				// the corresponding static tree
} tree_desc;

// Main structure which the deflate algorithm works from
typedef struct deflate_state_s
{
    z_stream	*z;								// pointer back to this zlib stream
    ulong		status;							// as the name implies
												
    EFlush		last_flush;						// value of flush param for previous deflate call
    int 		noheader;						// suppress zlib header and adler32

    byte		pending_buf[MAX_BLOCK_SIZE + 5];// output still pending
    byte		*pending_out;					// next pending byte to output to the stream
    ulong 		pending;						// nb of bytes in the pending buffer

    // Sliding window. Input bytes are read into the second half of the window,
    // and move to the first half later to keep a dictionary of at least wSize
    // bytes. With this organization, matches are limited to a distance of
    // wSize-MAX_MATCH bytes, but this ensures that IO is always
    // performed with a length multiple of the block size. Also, it limits
    // the window size to 64K, which is quite useful on MSDOS.
    // To do: use the user input buffer as sliding window.
    byte		window[WINDOW_SIZE * 2];

    // Link to older string with same hash index. To limit the size of this
    // array to 64K, this link is maintained only for the last 32K strings.
    // An index in this array is thus a window index modulo 32K.
    word		prev[WINDOW_SIZE];

    word		head[HASH_SIZE];				// Heads of the hash chains or NULL.

    ulong		ins_h;							// hash index of string to be inserted

    // Window position at the beginning of the current output block. Gets
    // negative when the window is moved backwards.
    int			block_start;

    ulong		match_length;					// length of best match
    ulong		prev_match;						// previous match
    ulong		match_available;				// set if previous match exists
    ulong		strstart;						// start of string to insert
    ulong		match_start;					// start of matching string
    ulong		lookahead;						// number of valid bytes ahead in window

    // Length of the best match at previous step. Matches not greater than this
    // are discarded. This is used in the lazy match evaluation.
    ulong		prev_length;

    // Attempt to find a better match only when the current match is strictly
    // smaller than this value. This mechanism is used only for compression	levels >= 4.
    ulong		max_lazy_match;

    ulong		good_match;						// Use a faster search when the previous match is longer than this
    ulong		nice_match;						// Stop searching when current match exceeds this

    // To speed up deflation, hash chains are never searched beyond this
    // length.  A higher limit improves compression ratio but degrades the speed.
    ulong		max_chain_length;

    ELevel		level;							// compression level (0..9)
												
    ct_data		dyn_ltree[HEAP_SIZE];			// literal and length tree
    ct_data		dyn_dtree[(2 * D_CODES) + 1]; 	// distance tree
    ct_data		bl_tree[(2 * BL_CODES) + 1];  	// Huffman tree for bit lengths

    tree_desc	l_desc;							// desc. for literal tree
    tree_desc	d_desc;							// desc. for distance tree
    tree_desc	bl_desc;						// desc. for bit length tree

    word		bl_count[MAX_WBITS + 1];		// number of codes at each bit length for an optimal tree

    // The sons of heap[n] are heap[2*n] and heap[2*n+1]. heap[0] is not used.
    // The same heap array is used to build all trees.
    ulong		heap[(2 * L_CODES) + 1];		// heap used to build the Huffman trees
    ulong		heap_len;						// number of elements in the heap
    ulong		heap_max;						// element of largest frequency

    byte		depth[(2 * L_CODES) + 1];		// Depth of each subtree used as tie breaker for trees of equal frequency

    byte		l_buf[LIT_BUFSIZE];				// buffer for literals or lengths

    ulong		last_lit;						// running index in l_buf

    // Buffer for distances. To simplify the code, d_buf and l_buf have
    // the same number of elements. To use different lengths, an extra flag
    // array would be necessary.
    word		d_buf[LIT_BUFSIZE];
 
	ulong		opt_len;						// bit length of current block with optimal trees
    ulong		static_len;						// bit length of current block with static trees
    ulong		matches;						// number of string matches in current block
    ulong		last_eob_len;					// bit length of EOB code for last block

    word		bi_buf;							// Output buffer. bits are inserted starting at the bottom (least significant bits).
    ulong		bi_valid;						// Number of valid bits in bi_buf.  All bits above the last valid bit are always zero.

	ulong		adler;
} deflate_state;

// Compression function. Returns the block state after the call.
typedef block_state (*compress_func) (deflate_state *s, EFlush flush);

typedef struct config_s 
{
   word				good_length;				// reduce lazy search above this match length
   word				max_lazy;					// do not perform lazy search above this match length
   word				nice_length;				// quit search above this match length
   word				max_chain;
   compress_func	func;
} config;

// end						