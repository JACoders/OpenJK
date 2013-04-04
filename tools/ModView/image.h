// Filename:-	image.h
//


#ifndef IMAGE_H
#define IMAGE_H

#include "textures.h"

bool	DStamp_MarkImage(Texture_t *pTexture, LPCSTR psText);	// this-app-specific
LPCSTR	DStamp_MarkImage(byte *pPixels, int iWidth, int iHeight, int iPlanes, LPCSTR psText);	// generic
LPCSTR	DStamp_ReadImage(byte *pPixels, int iWidth, int iHeight, int iPlanes);

void DStamp_AnalyseImage(byte *pPixels, int iWidth, int iHeight, int iPlanes);
void DStamp_AnalyseImage(Texture_t *pTexture);


#endif	// #ifndef IMAGE_H

//////////////// eof ////////////

