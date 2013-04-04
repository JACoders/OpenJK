// Known chunk types

#define PNG_IHDR		'IHDR'
#define PNG_IDAT		'IDAT' 
#define PNG_IEND 		'IEND'
#define PNG_tEXt 		'tEXt'

#define PNG_PLTE 		'PLTE'
#define PNG_bKGD 		'bKGD'
#define PNG_cHRM 		'cHRM'
#define PNG_gAMA 		'gAMA'
#define PNG_hIST 		'hIST'
#define PNG_iCCP 		'iCCP'
#define PNG_iTXt 		'iTXt'
#define PNG_oFFs 		'oFFs'
#define PNG_pCAL 		'pCAL'
#define PNG_sCAL 		'sCAL'
#define PNG_pHYs 		'pHYs'
#define PNG_sBIT 		'sBIT'
#define PNG_sPLT 		'sPLT'
#define PNG_sRGB 		'sRGB'
#define PNG_tIME 		'tIME'
#define PNG_tRNS 		'tRNS'
#define PNG_zTXt 		'zTXt'

// Filter values 

#define PNG_FILTER_VALUE_NONE	0
#define PNG_FILTER_VALUE_SUB	1
#define PNG_FILTER_VALUE_UP		2
#define PNG_FILTER_VALUE_AVG	3
#define PNG_FILTER_VALUE_PAETH	4
#define PNG_FILTER_NUM			5

// Common defines and typedefs

typedef unsigned char	byte;
typedef unsigned short	word;
typedef unsigned long	ulong;

#pragma pack(push)
#pragma pack(1)

typedef struct png_ihdr_s
{
	ulong			width;
	ulong			height;
	byte			bitdepth;			// Bits per sample (not per pixel)
	byte			colortype;			// bit 0 - palette; bit 1 - RGB; bit 2 - alpha channel
	byte			compression;		// 0 for zip - error otherwise
	byte			filter;				// 0 for adaptive with the 5 basic types - error otherwise
	byte			interlace;			// 0 for no interlace - 1 for Adam7 interlace
} png_ihdr_t;

#pragma pack(pop)

typedef struct png_image_s
{
	byte			*data;
	ulong			width;
	ulong			height;
	ulong			bytedepth;
} png_image_t;

bool LoadPNG32 (LPCSTR name, byte **pixels, int *width, int *height, int *bytedepth);
bool PNG_Save(const char *name, byte *pixels, int width, int height, int bytedepth);

// end