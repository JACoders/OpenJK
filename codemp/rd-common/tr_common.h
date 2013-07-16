#ifndef TR_COMMON_H
#define TR_COMMON_H

#include "../rd-common/tr_public.h"
#include "../rd-common/tr_font.h"

// Refresh module function imports
extern refimport_t ri;

/*
================================================================================
 Image Loading
================================================================================
*/
// Load an image from file.
void R_LoadImage( const char *shortname, byte **pic, int *width, int *height );

// Load an image from file.
// xyc: How does this differ from R_LoadImage (except it doesn't load PNG files)?
void R_LoadDataImage( const char *name, byte **pic, int *width, int *height );

// Load raw image data from pallette-colored TGA image.
bool LoadTGAPalletteImage ( const char *name, byte **pic, int *width, int *height);

// Load raw image data from TGA image.
void LoadTGA ( const char *name, byte **pic, int *width, int *height);

// Load raw image data from JPEG image.
void LoadJPG ( const char *filename, unsigned char **pic, int *width, int *height );

// Load raw image data from PNG image.
int LoadPNG ( const char *filename, byte **data, unsigned int *width, unsigned int *height );


/*
================================================================================
 Image saving
================================================================================
*/
// Convert raw image data to JPEG format and store in buffer.
size_t RE_SaveJPGToBuffer(byte *buffer, size_t bufSize, int quality, int image_width, int image_height, byte *image_buffer, int padding);

// Save raw image data as JPEG image file.
void RE_SaveJPG (const char * filename, int quality, int image_width, int image_height, byte *image_buffer, int padding);

// Save raw image data as PNG image file.
int RE_SavePNG( const char *filename, byte *buf, size_t width, size_t height, int byteDepth );


/*
================================================================================
 Image manipulation
================================================================================
*/
// Flip an image along its y-axis.
void R_InvertImage(byte *data, int width, int height, int depth);

// Resize an image by resampling the image.
void R_Resample(byte *source, int swidth, int sheight, byte *dest, int dwidth, int dheight, int components);

#endif
