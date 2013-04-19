// Maximum size of dynamic tree.  The maximum found in a long but non-
// exhaustive search was 1004 huft structures (850 for length/literals
// and 154 for distances, the latter actually the result of an
// exhaustive search).  The actual maximum is not known, but the
// value below is more than safe.

#define MANY					1440

// maximum bit length of any code (if BMAX needs to be larger than 16, then h and x[] should be ulong.)
#define BMAX					15

typedef ulong (*check_func) (ulong check, const byte *buf, ulong len);

typedef enum 
{
	TYPE,		// get type bits (3, including end bit)
	LENS,		// get lengths for stored
	STORED,		// processing stored block
	TABLE,		// get table lengths
	BTREE,		// get bit lengths tree for a dynamic block
	DTREE,		// get length, distance trees for a dynamic block
	CODES,		// processing fixed or dynamic block
	DRY,		// output remaining window bytes
	DONE,		// finished last block, done
	BAD			// got a data error--stuck here
} inflate_block_mode;

// waiting for "i:"=input, "o:"=output, "x:"=nothing
typedef enum 
{        
	START,		// x: set up for LEN
	LEN,		// i: get length/literal/eob next
	LENEXT,		// i: getting length extra (have base)
	DIST,		// i: get distance next
	DISTEXT,	// i: getting distance extra
	COPY,		// o: copying bytes in window, waiting for space
	LIT,		// o: got literal, waiting for output space
	WASH,		// o: got eob, possibly still output waiting
	END,		// x: got eob and all data flushed
	BADCODE		// x: got error
} inflate_codes_mode;

typedef enum 
{
	imMETHOD,	// waiting for method byte
	imFLAG,		// waiting for flag byte
	imBLOCKS,	// decompressing blocks
	imCHECK4,	// four check bytes to go
	imCHECK3,	// three check bytes to go
	imCHECK2,	// two check bytes to go
	imCHECK1,	// one check byte to go
	imDONE,		// finished check, done
	imBAD		// got an error--stay here
} inflate_mode;

typedef struct inflate_huft_s 
{
	byte	Exop;							// number of extra bits or operation
	byte	Bits;							// number of bits in this code or subcode
	ulong	base;							// literal, length base, distance base, or table offset
} inflate_huft_t;

// inflate codes private state
typedef struct inflate_codes_state_s 
{
	inflate_codes_mode	mode;				// current inflate_codes mode
											
	// mode dependent information			
	ulong len;								
	union									
	{										
		struct								
		{									
			inflate_huft_t	*tree;			// pointer into tree
			ulong			need;			// bits needed
		} code;								// if LEN or DIST, where in tree
		ulong			lit;				// if LIT, literal
		struct								
		{									
			ulong			get;			// bits to get for extra
			ulong			dist;			// distance back to copy from
		} copy;								// if EXT or COPY, where and how much
	};										// submode
											
	// mode independent information			
	byte			lbits;					// ltree bits decoded per branch
	byte			dbits;					// dtree bits decoder per branch
	inflate_huft_t	*ltree;					// literal/length/eob tree
	inflate_huft_t	*dtree;					// distance tree
} inflate_codes_state_t;

// inflate blocks semi-private state
typedef struct inflate_blocks_state_s 
{
	// mode
	inflate_block_mode		mode;			// current inflate_block mode

	// mode dependent information
	union
	{
		ulong		left;					// if STORED, bytes left to copy
		struct								
		{									
			ulong			table;			// table lengths (14 bits)
			ulong			index;			// index into blens (or border)
			ulong			*blens;			// bit lengths of codes
			ulong			bb;				// bit length tree depth
			inflate_huft_t	*tb;			// bit length decoding tree
		} trees;							// if DTREE, decoding info for trees
		struct 
		{
			inflate_codes_state_t *codes;
		} decode;							// if CODES, current state
	};										// submode
	bool			last;					// true if this block is the last block
											
	// mode independent information			
	ulong			bitk;					// bits in bit buffer
	ulong			bitb;					// bit buffer
	inflate_huft_t	*hufts;					// single malloc for tree space
	byte			window[WINDOW_SIZE];	// sliding window
	byte			*end;					// one byte after sliding window
	byte			*read;					// window read pointer
	byte			*write;					// window write pointer
	ulong			check;					// check on output
} inflate_blocks_state_t;

// inflate private state
typedef struct inflate_state_s 
{
	inflate_mode			mode;	  		// current inflate mode
											
	ulong					method;			// if FLAGS, method byte
											
	// mode independent information			
	int						nowrap;			// flag for no wrapper
	ulong					wbits;			// log2(window size)  (8..15, defaults to 15)
	inflate_blocks_state_t	*blocks;		// current inflate_blocks state

	ulong					adler;
	ulong					calcadler;
} inflate_state;


// end
