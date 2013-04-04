// Filename:-	R_Image.h
//

#ifndef R_IMAGE_H
#define R_IMAGE_H


extern char g_sImageExtension[MAX_QPATH];

//image_t	*R_FindImageFile( const char *name );
void R_LoadImage( const char *name, byte **pic, int *width, int *height );

// externalised for DMark code...
//
bool TGA_Write(LPCSTR psFullPathedFilename, byte *pPixels, int iWidth, int iHeight, int iPlanes);
void LoadTGA ( const char *name, byte **pic, int *width, int *height);

#endif