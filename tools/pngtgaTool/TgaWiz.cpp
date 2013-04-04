// TgaWiz.cpp : Defines the entry point for the console application.
//
#include <windows.h>
#include <cstdio>
#include "png.h"

typedef unsigned char byte;

void R_MipMap (byte *in, int width, int height, bool simple = true);
void LoadTGA ( const char *name, byte **pic, int *width, int *height);
void SaveTGA( byte *pic, int width, int height, char *fileName );

int main(int argc, char *argv[])
{
	unsigned char *img;
	int w, h;

	if (argc != 3)
	{
		printf("USAGE: %s infile outfile\n", argv[0]);
		return 0;
	}

	// Load the PNG file, make sure that we have an alpha channel in memory
	if (!LoadPNG32(argv[1], &img, &w, &h, NULL))
	{
		printf("ERROR: Couldn't load %s\n", argv[1]);
		return 0;
	}

	// Save it out as TGA
	SaveTGA(img, w, h, argv[2]);
	return 0;
}

#if 0
int main(int argc, char* argv[])
{
	unsigned char *img;
	int w, h;

	if (argc != 4)
	{
		printf("USAGE: %s infile outfile N\n");
		printf("       N is largest allowed dimensions\n");
		return 0;
	}

	int maxSize = atoi(argv[3]);

	// Load it
	LoadTGA(argv[1], &img, &w, &h);

	while (w > maxSize || h > maxSize)
	{
		R_MipMap(img, w, h, true);
		w /= 2;
		h /= 2;
	}

	SaveTGA(img, w, h, argv[2]);
	return 0;
}
#endif

