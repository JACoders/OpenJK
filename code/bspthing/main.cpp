#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <direct.h>
#include <io.h>

#include <windows.h>

#include "bsp.h"
#include "pbsp.h"

#include "../qcommon/sparc.h"

#define SWAP16(x)														\
	big_endian ?														\
		((short)( ( ((x) & 0xff00) >> 8 ) + ( ((x) & 0xff) << 8 ) ))	\
		:																\
		x;

#define SWAP32(x)												\
	big_endian ?												\
		((int)( ( ((x) & 0xff000000) >> 24)						\
			+ ( ((x) & 0x00ff0000) >> 8 )						\
			+ ( ((x) & 0x0000ff00) << 8 )						\
			+ ( ((x) & 0x000000ff) << 24 ) ))					\
		:														\
		x;

static float SWAPF(float x, bool big_endian)
{
	int temp = SWAP32(*(int*)&x);
	return *(float*)&temp;
}

#define CHECKED_READ(BUFFER, SIZE, OFFSET)			\
	if (fseek(in, OFFSET, SEEK_SET) < 0 ||			\
		!fread(BUFFER, SIZE, 1, in))				\
	{												\
		fprintf(stderr, "Error reading BSP.\n");	\
		assert(false);								\
		exit(-1);									\
	}

#define CHECKED_WRITE(NAME, BUFFER, SIZE)						\
	{															\
		char fullname[256];										\
		sprintf(fullname, "%s\\%s.%s",							\
			path, NAME, big_endian ? "mbe" : "mle");			\
		FILE* out = fopen(fullname, "wb");						\
		if (!out)												\
		{														\
			fprintf(stderr, "Error opening %s.\n", fullname);	\
			assert(false);										\
			exit(-1);											\
		}														\
		if (!fwrite(BUFFER, SIZE, 1, out))						\
		{														\
			fprintf(stderr, "Error writing %s.\n", fullname);	\
			assert(false);										\
			exit(-1);											\
		}														\
		fclose(out);											\
	}

static void convert_fogs(const lump_t& lump, FILE* in, bool big_endian, const char* path)
{
	int num = lump.filelen / sizeof(pdfog_t);
	if (num == 0)
	{
		return;
	}

	pdfog_t* fogs = new pdfog_t[num];
	CHECKED_READ(fogs, lump.filelen, lump.fileofs);

	for (int i = 0; i < num; ++i)
	{
		fogs[i].brushNum = SWAP32(fogs[i].brushNum);
		fogs[i].visibleSide = SWAP32(fogs[i].visibleSide);
	}

	CHECKED_WRITE("fogs", fogs, lump.filelen);
	
	delete [] fogs;
}

static void convert_brushes(const lump_t& lump, FILE* in, bool big_endian, const char* path)
{
	int num = lump.filelen / sizeof(dbrush_t);
	dbrush_t* in_brushes = new dbrush_t[num];
	CHECKED_READ(in_brushes, lump.filelen, lump.fileofs);

	pdbrush_t* out_brushes = new pdbrush_t[num];

	for (int i = 0; i < num; ++i)
	{
		assert(in_brushes[i].numSides >= 0 && in_brushes[i].numSides < 256);
		assert(in_brushes[i].shaderNum >= 0 && in_brushes[i].shaderNum < 65536);
		
		out_brushes[i].firstSide = SWAP32(in_brushes[i].firstSide);
		out_brushes[i].numSides = in_brushes[i].numSides;
		out_brushes[i].shaderNum = SWAP16((unsigned short)in_brushes[i].shaderNum);
	}

	int size = num * sizeof(pdbrush_t);
	CHECKED_WRITE("brushes", out_brushes, size);

	delete [] out_brushes;
	delete [] in_brushes;
}

static void convert_brushsides(const lump_t& lump, FILE* in, bool big_endian, const char* path)
{
	int num = lump.filelen / sizeof(dbrushside_t);
	dbrushside_t* in_brushesides = new dbrushside_t[num];
	CHECKED_READ(in_brushesides, lump.filelen, lump.fileofs);

	pdbrushside_t* out_brushesides = new pdbrushside_t[num];

	for (int i = 0; i < num; ++i)
	{
		assert(in_brushesides[i].shaderNum >= 0 && in_brushesides[i].shaderNum < 256);
		
		out_brushesides[i].planeNum = SWAP32(in_brushesides[i].planeNum);
		out_brushesides[i].shaderNum = in_brushesides[i].shaderNum;
	}

	int size = num * sizeof(pdbrushside_t);
	CHECKED_WRITE("brushsides", out_brushesides, size);

	delete [] out_brushesides;
	delete [] in_brushesides;
}

static void convert_leafsurfaces(const lump_t& lump, FILE* in, bool big_endian, const char* path)
{
	int num = lump.filelen / sizeof(int);
	int* leafsurfaces = new int[num];
	CHECKED_READ(leafsurfaces, lump.filelen, lump.fileofs);

	for (int i = 0; i < num; ++i)
	{
		leafsurfaces[i] = SWAP32(leafsurfaces[i]);
	}

	CHECKED_WRITE("leafsurfaces", leafsurfaces, lump.filelen);

	delete [] leafsurfaces;
}

static void convert_nodes(const lump_t& lump, FILE* in, bool big_endian, const char* path)
{
	int num = lump.filelen / sizeof(dnode_t);
	dnode_t* in_nodes = new dnode_t[num];
	CHECKED_READ(in_nodes, lump.filelen, lump.fileofs);

	pdnode_t* out_nodes = new pdnode_t[num];

	for (int i = 0; i < num; ++i)
	{
		out_nodes[i].planeNum = SWAP32(in_nodes[i].planeNum);

		for (int j = 0; j < 2; ++j)
		{
			assert(in_nodes[i].children[j] > -32768 && in_nodes[i].children[j] < 32768);
			out_nodes[i].children[j] = SWAP16((short)in_nodes[i].children[j]);
		}

		for (int k = 0; k < 3; ++k)
		{
			assert(in_nodes[i].mins[k] > -32768 && in_nodes[i].mins[k] < 32768);
			out_nodes[i].mins[k] = SWAP16((short)in_nodes[i].mins[k]);

			assert(in_nodes[i].maxs[k] > -32768 && in_nodes[i].maxs[k] < 32768);
			out_nodes[i].maxs[k] = SWAP16((short)in_nodes[i].maxs[k]);
		}
	}

	int size = num * sizeof(pdnode_t);
	CHECKED_WRITE("nodes", out_nodes, size);

	delete [] out_nodes;
	delete [] in_nodes;
}

static void convert_leafs(const lump_t& lump, FILE* in, bool big_endian, const char* path)
{
	int num = lump.filelen / sizeof(dleaf_t);
	dleaf_t* in_leafs = new dleaf_t[num];
	CHECKED_READ(in_leafs, lump.filelen, lump.fileofs);

	pdleaf_t* out_leafs = new pdleaf_t[num];

	for (int i = 0; i < num; ++i)
	{
		assert(in_leafs[i].cluster > -32768 && in_leafs[i].cluster < 32768);
		out_leafs[i].cluster = SWAP16((short)in_leafs[i].cluster);

		assert(in_leafs[i].area > -128 && in_leafs[i].area < 128);
		out_leafs[i].area = in_leafs[i].area;

		assert(in_leafs[i].firstLeafSurface >= 0 && in_leafs[i].firstLeafSurface < 65536);
		out_leafs[i].firstLeafSurface = SWAP16((short)in_leafs[i].firstLeafSurface);

		assert(in_leafs[i].numLeafSurfaces >= 0 && in_leafs[i].numLeafSurfaces < 65536);
		out_leafs[i].numLeafSurfaces = SWAP16((short)in_leafs[i].numLeafSurfaces);

		assert(in_leafs[i].firstLeafBrush >= 0 && in_leafs[i].firstLeafBrush < 65536);
		out_leafs[i].firstLeafBrush = SWAP16((short)in_leafs[i].firstLeafBrush);

		assert(in_leafs[i].numLeafBrushes >= 0 && in_leafs[i].numLeafBrushes < 65536);
		out_leafs[i].numLeafBrushes = SWAP16((short)in_leafs[i].numLeafBrushes);

		for (int k = 0; k < 3; ++k)
		{
			//assert(in_leafs[i].mins[k] > -32768 && in_leafs[i].mins[k] < 32768);
			out_leafs[i].mins[k] = SWAP16((short)in_leafs[i].mins[k]);

			//assert(in_leafs[i].maxs[k] > -32768 && in_leafs[i].maxs[k] < 32768);
			out_leafs[i].maxs[k] = SWAP16((short)in_leafs[i].maxs[k]);
		}		
	}

	int size = num * sizeof(pdleaf_t);
	CHECKED_WRITE("leafs", out_leafs, size);

	delete [] out_leafs;
	delete [] in_leafs;
}

static void convert_models(const lump_t& lump, FILE* in, bool big_endian, const char* path)
{
	int num = lump.filelen / sizeof(dmodel_t);
	if (num == 0)
	{
		return;
	}

	dmodel_t* in_models = new dmodel_t[num];
	CHECKED_READ(in_models, lump.filelen, lump.fileofs);

	pdmodel_t* out_models = new pdmodel_t[num];

	for (int i = 0; i < num; ++i)
	{
		out_models[i].firstSurface = SWAP32(in_models[i].firstSurface);

		assert(in_models[i].numSurfaces >= 0 && in_models[i].numSurfaces < 65536);
		out_models[i].numSurfaces = SWAP16((short)in_models[i].numSurfaces);

		out_models[i].firstBrush = SWAP32(in_models[i].firstBrush);

		assert(in_models[i].numBrushes >= 0 && in_models[i].numBrushes < 65536);
		out_models[i].numBrushes = SWAP16((short)in_models[i].numBrushes);

		for (int k = 0; k < 3; ++k)
		{
			out_models[i].mins[k] = SWAPF(in_models[i].mins[k], big_endian);
			out_models[i].maxs[k] = SWAPF(in_models[i].maxs[k], big_endian);
		}		
	}

	int size = num * sizeof(pdmodel_t);
	CHECKED_WRITE("models", out_models, size);

	delete [] out_models;
	delete [] in_models;
}

static void convert_entities(const lump_t& lump, FILE* in, bool big_endian, const char* path)
{
	if (lump.filelen == 0)
	{
		return;
	}

	char* entities = new char[lump.filelen];

	CHECKED_READ(entities, lump.filelen, lump.fileofs);
	CHECKED_WRITE("entities", entities, lump.filelen);

	delete [] entities;
}

static void convert_lightgrid(const lump_t& lump, FILE* in, bool big_endian, const char* path)
{
	int num = lump.filelen / sizeof(dgrid_t);
	if (num == 0)
	{
		return;
	}

	dgrid_t* in_grid = new dgrid_t[num];
	CHECKED_READ(in_grid, lump.filelen, lump.fileofs);

	//figure out how much memory we really need.
	int memory = 0;
	int i, j;
	for(i=0; i<num; i++) {
		for(j=0; j<MAXLIGHTMAPS; j++) {
			if(in_grid[i].styles[j] != LS_NONE) {
				memory++;
			}
			if(
					in_grid[i].ambientLight[j][0] != 0 ||
					in_grid[i].ambientLight[j][1] != 0 ||
					in_grid[i].ambientLight[j][2] != 0 ||
					in_grid[i].directLight[j][0] != 0 ||
					in_grid[i].directLight[j][1] != 0 ||
					in_grid[i].directLight[j][2] != 0) {
				memory += 6;
			}
		}
	}

	//Make sure we're going to use less memory than the old system.
	int size = sizeof(pdgrid_t) * num + memory;
//	assert(lump.filelen >= size);

	char* out_grid = new char[size];
	char* data = out_grid + (sizeof(pdgrid_t) * num);

	//Copy data from old array into new array.
	memory = 0;
	for(i=0; i<num; i++) {
		pdgrid_t* g = (pdgrid_t*)out_grid + i;
		g->latLong[0] = in_grid[i].latLong[0];
		g->latLong[1] = in_grid[i].latLong[1];
		g->flags = 0;
		g->data = SWAP32(sizeof(pdgrid_t) * num + memory);
		for(j=0; j<MAXLIGHTMAPS; j++) {
			if(in_grid[i].styles[j] != LS_NONE) {
				data[memory++] = in_grid[i].styles[j];
				g->flags |= 1 << (j + 4);
			}
			if(
					in_grid[i].ambientLight[j][0] != 0 ||
					in_grid[i].ambientLight[j][1] != 0 ||
					in_grid[i].ambientLight[j][2] != 0 ||
					in_grid[i].directLight[j][0] != 0 ||
					in_grid[i].directLight[j][1] != 0 ||
					in_grid[i].directLight[j][2] != 0) {
				data[memory++] = in_grid[i].ambientLight[j][0];
				data[memory++] = in_grid[i].ambientLight[j][1];
				data[memory++] = in_grid[i].ambientLight[j][2];
				data[memory++] = in_grid[i].directLight[j][0];
				data[memory++] = in_grid[i].directLight[j][1];
				data[memory++] = in_grid[i].directLight[j][2];
				g->flags |= 1 << j;
			}
		}
	}

	CHECKED_WRITE("lightgrid", out_grid, size);

	delete [] out_grid;
	delete [] in_grid;
}

static void convert_lightarray(const lump_t& lump, FILE* in, bool big_endian, const char* path)
{
	int num = lump.filelen / sizeof(short);
	if (num == 0)
	{
		return;
	}

	short* lightarray = new short[num];
	CHECKED_READ(lightarray, lump.filelen, lump.fileofs);

	for (int i = 0; i < num; ++i)
	{
		lightarray[i] = SWAP16(lightarray[i]);
	}

	CHECKED_WRITE("lightarray", lightarray, lump.filelen);

	delete [] lightarray;
}

static void convert_shaders(const lump_t& lump, FILE* in, bool big_endian, const char* path)
{
	int num = lump.filelen / sizeof(pdshader_t);
	if (num == 0)
	{
		return;
	}

	pdshader_t* shaders = new pdshader_t[num];
	CHECKED_READ(shaders, lump.filelen, lump.fileofs);

	for (int i = 0; i < num; ++i)
	{
		shaders[i].surfaceFlags = SWAP32(shaders[i].surfaceFlags);
		shaders[i].contentFlags = SWAP32(shaders[i].contentFlags);
	}

	CHECKED_WRITE("shaders", shaders, lump.filelen);

	delete [] shaders;
}

static void convert_planes(const lump_t& lump, FILE* in, bool big_endian, const char* path)
{
	int num = lump.filelen / sizeof(pdplane_t);
	pdplane_t* planes = new pdplane_t[num];
	CHECKED_READ(planes, lump.filelen, lump.fileofs);

	for (int i = 0; i < num; ++i)
	{
		planes[i].normal[0] = SWAPF(planes[i].normal[0], big_endian);
		planes[i].normal[1] = SWAPF(planes[i].normal[1], big_endian);
		planes[i].normal[2] = SWAPF(planes[i].normal[2], big_endian);
		planes[i].dist = SWAPF(planes[i].dist, big_endian);
	}

	CHECKED_WRITE("planes", planes, lump.filelen);

	delete [] planes;
}

static void convert_leafbrushes(const lump_t& lump, FILE* in, bool big_endian, const char* path)
{
	int num = lump.filelen / sizeof(int);
	int* leafbrushes = new int[num];
	CHECKED_READ(leafbrushes, lump.filelen, lump.fileofs);

	for (int i = 0; i < num; ++i)
	{
		leafbrushes[i] = SWAP32(leafbrushes[i]);
	}

	CHECKED_WRITE("leafbrushes", leafbrushes, lump.filelen);

	delete [] leafbrushes;
}

static void convert_verts(const lump_t& lump, FILE* in, bool big_endian, const char* path)
{
	int num = lump.filelen / sizeof(mapVert_t);
	mapVert_t* in_verts = new mapVert_t[num];
	CHECKED_READ(in_verts, lump.filelen, lump.fileofs);

	pmapVert_t* out_verts = new pmapVert_t[num];

	for (int i = 0; i < num; ++i)
	{
		for (int j = 0; j < 3; ++j)
		{
			assert(in_verts[i].xyz[j] > -32768 && in_verts[i].xyz[j] < 32768);
			out_verts[i].xyz[j] = SWAP16((short)in_verts[i].xyz[j]);

			assert(in_verts[i].normal[j] >= -1 && in_verts[i].normal[j] <= 1);
			out_verts[i].normal[j] = SWAP16((short)(in_verts[i].normal[j] * 32767.f));
		}

		for (int k = 0; k < 2; ++k)
		{
			out_verts[i].st[k] = SWAPF(in_verts[i].st[k], big_endian);
		}

		for (int m = 0; m < MAXLIGHTMAPS; ++m)
		{
			for (int n = 0; n < 2; ++n)
			{
				out_verts[i].lightmap[m][n] = SWAPF(
					(in_verts[i].lightmap[m][n] * POINTS_LIGHT_SCALE),
					big_endian);
			}

			for (int p = 0; p < 4; ++p)
			{
				out_verts[i].color[m][p] = in_verts[i].color[m][p];
			}
		}
	}

	int size = num * sizeof(pmapVert_t);
	CHECKED_WRITE("verts", out_verts, size);

	delete [] out_verts;
	delete [] in_verts;
}

static void convert_indexes(const lump_t& lump, FILE* in, bool big_endian, const char* path)
{
	int num = lump.filelen / sizeof(int);
	int* in_indexes = new int[num];
	CHECKED_READ(in_indexes, lump.filelen, lump.fileofs);

	short* out_indexes = new short[num];

	for (int i = 0; i < num; ++i)
	{
		assert(in_indexes[i] > -32768 && in_indexes[i] < 32768);
		out_indexes[i] = SWAP16((short)in_indexes[i]);
	}

	int size = num * sizeof(short);
	CHECKED_WRITE("indexes", out_indexes, size);

	delete [] out_indexes;
	delete [] in_indexes;
}

static void scale_color(byte* dst, const byte* src, float factor)
{
	byte hichan = 0;
	int hiindex = 0;
	for (int c = 0; c < 3; ++c)
	{
		if (hichan < src[c])
		{
			hichan = src[c];
			hiindex = c;
		}
	}
	
	float test = (float)src[hiindex] * factor;
	if (test > 255.f) test = 255.f;

	factor = test / (float)src[hiindex];

	dst[0] = (byte)((float)src[2] * factor);
	dst[1] = (byte)((float)src[1] * factor);
	dst[2] = (byte)((float)src[0] * factor);
}

static void convert_lightmaps(const lump_t& lump, FILE* in, bool big_endian, const char* path)
{
	int in_map_size = LIGHTMAP_SIZE * LIGHTMAP_SIZE * 3;
	int bmp_size = in_map_size + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

	int num = lump.filelen / in_map_size;
	if (num == 0)
	{
		return;
	}
		
	unsigned char* in_lightmaps = new unsigned char[lump.filelen];
	CHECKED_READ(in_lightmaps, lump.filelen, lump.fileofs);

	// Setup a BMP file for conversion
	char* bmp = new char[bmp_size];

	BITMAPFILEHEADER* header = (BITMAPFILEHEADER*)bmp;
	((char*)&header->bfType)[0] = 'B';
	((char*)&header->bfType)[1] = 'M';
	header->bfSize = bmp_size;
	header->bfReserved1 = 0;
	header->bfReserved2 = 0;
	header->bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

	BITMAPINFOHEADER* info = (BITMAPINFOHEADER*)(bmp + sizeof(BITMAPFILEHEADER));
	info->biSize = sizeof(BITMAPINFOHEADER);
	info->biWidth = LIGHTMAP_SIZE;
	info->biHeight = LIGHTMAP_SIZE;
	info->biPlanes = 1;
	info->biBitCount = 24;
	info->biCompression = BI_RGB;
	info->biSizeImage = 0;
	info->biXPelsPerMeter = 1;
	info->biYPelsPerMeter = 1;
	info->biClrUsed = 0;
	info->biClrImportant = 0;

	// Open the lightmaps output file
	char fullname[256];
	sprintf(fullname, "%s\\lightmaps.%s", path, big_endian ? "mbe" : "mle");
	FILE* flightmaps = fopen(fullname, "wb");
	if (!flightmaps)
	{
		fprintf(stderr, "Error opening %s.\n", fullname);
		assert(false);
		exit(-1);
	}

	// For each lightmap...
	for (int i = 0; i < num; ++i)
	{
		// Update the BMP
		unsigned char* in_map = in_lightmaps + i * in_map_size;
		for (int y = 0; y < LIGHTMAP_SIZE; ++y)
		{
			for (int x = 0; x < LIGHTMAP_SIZE; ++x)
			{
				byte* dst = (byte*)bmp + header->bfOffBits + 
					((LIGHTMAP_SIZE - y - 1) * LIGHTMAP_SIZE * 3) +
					x * 3;

				byte* src = (byte*)in_map + 
					y * LIGHTMAP_SIZE * 3 + 
					x * 3;

				scale_color(dst, src, big_endian ? 1.225f : 1.1f);
			}
		}

		FILE* fbmp = fopen("~temp.bmp", "wb");
		if (!fbmp)
		{
			fprintf(stderr, "Error opening ~temp.bmp.\n");
			assert(false);
			exit(-1);
		}

		fwrite(bmp, bmp_size, 1, fbmp);
		fclose(fbmp);

		char* out_map = NULL;
		int out_map_size = 0;

	/*	if (big_endian)
		{
			// Use the "texconvpro" GC conversion tool to convert
			// ~temp.bmp to s3tc ~temp.out
			FILE* ftcs = fopen("~temp.tcs", "wt");
			if (!ftcs)
			{
				fprintf(stderr, "Error opening ~temp.tcs.\n");
				assert(false);
				exit(-1);
			}

			fprintf(ftcs, "path = 0\n");
			fprintf(ftcs, "file 0 = ~temp.bmp\n");
			fprintf(ftcs, "image 0 = 0, 0, RGB5A3, 0, 0, 0\n");
			fprintf(ftcs, "texture 0 = 0, x\n");

			fclose(ftcs);

			const char* cmdline = "texconvpro ~temp.tcs ~temp.tpl";
			system(cmdline);

			rename("~temp.tpl", "~temp.out");
			unlink("~temp.tcs");
		}
		else
		*/
		{
			// Use the "nvdxt" conversion tool to convert
			// ~temp.bmp to s3tc ~temp.out
			const char* cmdline = "nvdxt -file ~temp.bmp -u565 -nomipmap";
			system(cmdline);

			rename("~temp.dds", "~temp.out");
		}

		// Read compressed ~temp.out
		FILE* fout = fopen("~temp.out", "rb");
		if (!fout)
		{
			fprintf(stderr, "Error opening ~temp.out.\n");
			assert(false);
			exit(-1);
		}
		
		fseek(fout, 0, SEEK_END);
		out_map_size = ftell(fout);
		fseek(fout, 0, SEEK_SET);
		
		out_map = new char[out_map_size];
		fread(out_map, out_map_size, 1, fout);
		
		fclose(fout);
		
		unlink("~temp.out");

		// Append compressed data to lightmaps
		if (i == 0)
		{
			int temp = SWAP32(out_map_size);
			fwrite(&temp, sizeof(temp), 1, flightmaps);
		}

		fwrite(out_map, out_map_size, 1, flightmaps);

		unlink("~temp.bmp");
	}

	fclose(flightmaps);

	delete [] bmp;
	delete [] in_lightmaps;
}

static void convert_visibility(const lump_t& lump, FILE* in, bool big_endian, const char* path)
{
	if(lump.filelen == 0)
		return;

	unsigned char* in_visibility = new unsigned char[lump.filelen];
	CHECKED_READ(in_visibility, lump.filelen, lump.fileofs);

	char* out_visibility = new char[lump.filelen*2];

	*((int*)out_visibility + 0) = SWAP32(*((int*)in_visibility + 0));
	*((int*)out_visibility + 1) = SWAP32(*((int*)in_visibility + 1));

	SPARC<unsigned char> vis;
	vis.Compress(in_visibility + 8, lump.filelen - 8, 0);
	int size = vis.Save(out_visibility + 8, lump.filelen*2 - 8, big_endian) + 8;

	CHECKED_WRITE("visibility", out_visibility, size);

	delete [] out_visibility;
	delete [] in_visibility;
}

static void convert_faces(const lump_t& lump, FILE* in, bool big_endian, const char* path)
{
	bool warning = false;
	
	int num = lump.filelen / sizeof(dsurface_t);
	if (num == 0)
	{
		return;
	}

	dsurface_t* in_surfaces = new dsurface_t[num];
	CHECKED_READ(in_surfaces, lump.filelen, lump.fileofs);

	pdface_t* out_surfaces = new pdface_t[num];

	int counter = 0;

	for (int i = 0; i < num; ++i)
	{
		if (in_surfaces[i].surfaceType == MST_PLANAR)
		{
			out_surfaces[counter].code = SWAP32(i);
			
			assert(in_surfaces[i].shaderNum >= 0 && in_surfaces[i].shaderNum < 256);
			out_surfaces[counter].shaderNum = in_surfaces[i].shaderNum;
			
			assert(in_surfaces[i].fogNum > -128 && in_surfaces[i].fogNum < 128);
			out_surfaces[counter].fogNum = in_surfaces[i].fogNum;
			
			assert(in_surfaces[i].firstVert >= 0 && in_surfaces[i].firstVert < 1048576);
			assert(in_surfaces[i].numVerts >= 0 && in_surfaces[i].numVerts < 4096);
			out_surfaces[counter].verts = SWAP32((in_surfaces[i].firstVert << 12) | 
				(in_surfaces[i].numVerts & 0xfff));
			
			assert(in_surfaces[i].firstIndex >= 0 && in_surfaces[i].firstIndex < 1048576);
			assert(in_surfaces[i].numIndexes >= 0 && in_surfaces[i].numIndexes < 4096);
			out_surfaces[counter].indexes = SWAP32((in_surfaces[i].firstIndex << 12) | 
				(in_surfaces[i].numIndexes & 0xfff));
			
			for (int j = 0; j < MAXLIGHTMAPS; ++j)
			{
				if (!warning &&
					(in_surfaces[i].lightmapNum[j] < -4 || 
					in_surfaces[i].lightmapNum[j] >= 252))
				{
					printf("WARNING: Lightmap index out of range!\n");
					warning = true;
				}

				out_surfaces[counter].lightmapNum[j] = in_surfaces[i].lightmapNum[j] + 4;
				
				if (in_surfaces[i].lightmapNum[0] == LIGHTMAP_BY_VERTEX)
				{
					out_surfaces[counter].lightmapStyles[j] = in_surfaces[i].vertexStyles[j];
				}
				else
				{
					out_surfaces[counter].lightmapStyles[j] = in_surfaces[i].lightmapStyles[j];
				}
			}
			
			for (int m = 0; m < 3; ++m)
			{
				assert(in_surfaces[i].lightmapVecs[2][m] >= -1 && 
					in_surfaces[i].lightmapVecs[2][m] <= 1);
				out_surfaces[counter].lightmapVecs[m] = 
					SWAP16((short)(in_surfaces[i].lightmapVecs[2][m] * 32767.f));
			}

			++counter;
		}
	}

	if (counter == 0) return;

	int size = counter * sizeof(pdface_t);
	CHECKED_WRITE("faces", out_surfaces, size);

	delete [] out_surfaces;
	delete [] in_surfaces;
}

static void convert_patches(const lump_t& lump, FILE* in, bool big_endian, const char* path)
{
	int num = lump.filelen / sizeof(dsurface_t);
	if (num == 0)
	{
		return;
	}

	dsurface_t* in_surfaces = new dsurface_t[num];
	CHECKED_READ(in_surfaces, lump.filelen, lump.fileofs);

	pdpatch_t* out_surfaces = new pdpatch_t[num];

	int counter = 0;

	for (int i = 0; i < num; ++i)
	{
		if (in_surfaces[i].surfaceType == MST_PATCH)
		{
			out_surfaces[counter].code = SWAP32(i);

			assert(in_surfaces[i].shaderNum >= 0 && in_surfaces[i].shaderNum < 256);
			out_surfaces[counter].shaderNum = in_surfaces[i].shaderNum;

			assert(in_surfaces[i].fogNum > -128 && in_surfaces[i].fogNum < 128);
			out_surfaces[counter].fogNum = in_surfaces[i].fogNum;
			
			assert(in_surfaces[i].firstVert >= 0 && in_surfaces[i].firstVert < 1048576);
			assert(in_surfaces[i].numVerts >= 0 && in_surfaces[i].numVerts < 4096);
			out_surfaces[counter].verts = SWAP32((in_surfaces[i].firstVert << 12) | 
				(in_surfaces[i].numVerts & 0xfff));
			
			assert(in_surfaces[i].patchWidth >= 0 && in_surfaces[i].patchWidth < 256);
			out_surfaces[counter].patchWidth = in_surfaces[i].patchWidth;
			
			assert(in_surfaces[i].patchHeight >= 0 && in_surfaces[i].patchHeight < 256);
			out_surfaces[counter].patchHeight = in_surfaces[i].patchHeight;
			
			for (int j = 0; j < MAXLIGHTMAPS; ++j)
			{
				assert(in_surfaces[i].lightmapNum[j] >= -4 && in_surfaces[i].lightmapNum[j] < 252);
				out_surfaces[counter].lightmapNum[j] = in_surfaces[i].lightmapNum[j] + 4;
				
				if (in_surfaces[i].lightmapNum[0] == LIGHTMAP_BY_VERTEX)
				{
					out_surfaces[counter].lightmapStyles[j] = in_surfaces[i].vertexStyles[j];
				}
				else
				{
					out_surfaces[counter].lightmapStyles[j] = in_surfaces[i].lightmapStyles[j];
				}
			}
			
			for (int m = 0; m < 3; ++m)
			{
				for (int k = 0; k < 2; ++k)
				{
					assert(in_surfaces[i].lightmapVecs[k][m] > -32768.f && 
						in_surfaces[i].lightmapVecs[k][m] < 32768.f);
					out_surfaces[counter].lightmapVecs[k][m] = 
						SWAP16((short)in_surfaces[i].lightmapVecs[k][m]);
				}
			}

			++counter;
		}
	}

	if (counter == 0) return;

	int size = counter * sizeof(pdpatch_t);
	CHECKED_WRITE("patches", out_surfaces, size);

	delete [] out_surfaces;
	delete [] in_surfaces;
}

static void convert_trisurfs(const lump_t& lump, FILE* in, bool big_endian, const char* path)
{
	int num = lump.filelen / sizeof(dsurface_t);
	if (num == 0)
	{
		return;
	}

	dsurface_t* in_surfaces = new dsurface_t[num];
	CHECKED_READ(in_surfaces, lump.filelen, lump.fileofs);

	pdtrisurf_t* out_surfaces = new pdtrisurf_t[num];

	int counter = 0;

	for (int i = 0; i < num; ++i)
	{
		if (in_surfaces[i].surfaceType == MST_TRIANGLE_SOUP)
		{
			out_surfaces[counter].code = SWAP32(i);

			assert(in_surfaces[i].shaderNum >= 0 && in_surfaces[i].shaderNum < 256);
			out_surfaces[counter].shaderNum = in_surfaces[i].shaderNum;
			
			assert(in_surfaces[i].fogNum > -128 && in_surfaces[i].fogNum < 128);
			out_surfaces[counter].fogNum = in_surfaces[i].fogNum;
			
			assert(in_surfaces[i].firstVert >= 0 && in_surfaces[i].firstVert < 1048576);
			assert(in_surfaces[i].numVerts >= 0 && in_surfaces[i].numVerts < 4096);
			out_surfaces[counter].verts = SWAP32((in_surfaces[i].firstVert << 12) | 
				(in_surfaces[i].numVerts & 0xfff));
			
			assert(in_surfaces[i].firstIndex >= 0 && in_surfaces[i].firstIndex < 1048576);
			assert(in_surfaces[i].numIndexes >= 0 && in_surfaces[i].numIndexes < 4096);
			out_surfaces[counter].indexes = SWAP32((in_surfaces[i].firstIndex << 12) | 
				(in_surfaces[i].numIndexes & 0xfff));
			
			for (int j = 0; j < MAXLIGHTMAPS; ++j)
			{
				if (in_surfaces[i].lightmapNum[0] == LIGHTMAP_BY_VERTEX)
				{
					out_surfaces[counter].lightmapStyles[j] = in_surfaces[i].vertexStyles[j];
				}
				else
				{
					out_surfaces[counter].lightmapStyles[j] = in_surfaces[i].lightmapStyles[j];
				}
			}

			++counter;
		}
	}

	if (counter == 0) return;

	int size = counter * sizeof(pdtrisurf_t);
	CHECKED_WRITE("trisurfs", out_surfaces, size);

	delete [] out_surfaces;
	delete [] in_surfaces;
}

static void convert_flares(const lump_t& lump, FILE* in, bool big_endian, const char* path)
{
	int num = lump.filelen / sizeof(dsurface_t);
	if (num == 0)
	{
		return;
	}

	dsurface_t* in_surfaces = new dsurface_t[num];
	CHECKED_READ(in_surfaces, lump.filelen, lump.fileofs);

	pdflare_t* out_surfaces = new pdflare_t[num];

	int counter = 0;

	for (int i = 0; i < num; ++i)
	{
		if (in_surfaces[i].surfaceType == MST_FLARE)
		{
			out_surfaces[counter].code = SWAP32(i);

			assert(in_surfaces[i].shaderNum >= 0 && in_surfaces[i].shaderNum < 256);
			out_surfaces[counter].shaderNum = in_surfaces[i].shaderNum;
			
			assert(in_surfaces[i].fogNum > -128 && in_surfaces[i].fogNum < 128);
			out_surfaces[counter].fogNum = in_surfaces[i].fogNum;

			for (int j = 0; j < 3; ++j)
			{
				assert(in_surfaces[i].lightmapOrigin[j] > -32768 && in_surfaces[i].lightmapOrigin[j] < 32768);
				out_surfaces[counter].origin[j] = SWAP16((short)in_surfaces[i].lightmapOrigin[j]);

				assert(in_surfaces[i].lightmapVecs[2][j] >= -1 && in_surfaces[i].lightmapVecs[2][j] <= 1);
				out_surfaces[counter].normal[j] = SWAP16((short)(in_surfaces[i].lightmapVecs[2][j] * 32767.f));

				out_surfaces[counter].color[j] = (byte)(in_surfaces[i].lightmapVecs[0][j]); 
			}
			++counter;
		}
	}

	if (counter == 0) return;

	int size = counter * sizeof(pdflare_t);
	CHECKED_WRITE("flares", out_surfaces, size);

	delete [] out_surfaces;
	delete [] in_surfaces;
}

static void convert_surfaces(const lump_t& lump, FILE* in, bool big_endian, const char* path)
{
	convert_faces(lump, in, big_endian, path);
	convert_patches(lump, in, big_endian, path);
	convert_trisurfs(lump, in, big_endian, path);
	convert_flares(lump, in, big_endian, path);
}

typedef void (*convertfunc_t)(const lump_t& lump, FILE* in, bool big_endian, const char* path);
static convertfunc_t Converters[HEADER_LUMPS] = 
{
	convert_entities,
	convert_shaders,
	convert_planes,
	convert_nodes,
	convert_leafs,
	convert_leafsurfaces,
	convert_leafbrushes,
	convert_models,
	convert_brushes,
	convert_brushsides,
	convert_verts,
	convert_indexes,
	convert_fogs,
	convert_surfaces,
	convert_lightmaps,
	convert_lightgrid,
	convert_visibility,
	convert_lightarray,
};

static void write_misc(const dheader_t& header, FILE* in, bool big_endian, const char* path)
{
	int num = header.lumps[LUMP_SURFACES].filelen / sizeof(dsurface_t);
	num = SWAP32(num);
	
	CHECKED_WRITE("misc", &num, sizeof(int));
}

/*static void optimize_lightmaps(const dheader_t* header, FILE* in)
{
	// get the vert data
	int num_verts = header->lumps[LUMP_DRAWVERTS].filelen / sizeof(mapVert_t);
	mapVert_t* verts = new mapVert_t[num_verts];
	CHECKED_READ(verts, 
		header->lumps[LUMP_DRAWVERTS].filelen, 
		header->lumps[LUMP_DRAWVERTS].fileofs);

	// get the surface data
	int num_surfs = header->lumps[LUMP_SURFACES].filelen / sizeof(dsurface_t);
	dsurface_t* surfs = new dsurface_t[num_surfs];
	CHECKED_READ(surfs, 
		header->lumps[LUMP_SURFACES].filelen, 
		header->lumps[LUMP_SURFACES].fileofs);
	
	// get the lightmap data
	const int map_size = LIGHTMAP_SIZE * LIGHTMAP_SIZE * 3;
	int num_lightmaps = header->lumps[LUMP_LIGHTMAPS].filelen / map_size;
	byte* lightmaps = new byte[header->lumps[LUMP_LIGHTMAPS].filelen];
	CHECKED_READ(lightmaps, 
		header->lumps[LUMP_LIGHTMAPS].filelen, 
		header->lumps[LUMP_LIGHTMAPS].fileofs);
	
	LMOptimizer optimizer;
	optimizer.optimize(verts, num_verts, surfs, num_surfs, lightmaps, num_lightmaps);
}*/

static void process(const char* name, bool big_endian)
{
	// open the bsp
	FILE* in = fopen(name, "rb");
	if (!in)
	{
		fprintf(stderr, "Unable to open %s\n", name);
		exit(-1);
	}

	// get the new path name
	char* path = new char[strlen(name) + 1];
	strcpy(path, name);
	path[strlen(path) - 4] = '\0';

	mkdir(path);
	
	dheader_t in_header;
	fread(&in_header, sizeof(in_header), 1, in);

	for (int i = 0; i < HEADER_LUMPS; ++i)
	{
		Converters[i](in_header.lumps[i], in, big_endian, path);
	}

	write_misc(in_header, in, big_endian, path);

	delete [] path;

	fclose(in);
}

int main(int argc, const char** argv)
{
	// check command line
	if (argc != 2)
	{
		fprintf(stderr, "USAGE: %s PATH\n", argv[0]);
		return -1;
	}

	// find all the BSP files in the path
	char spec[256];
	strcpy(spec, argv[1]);
	strcat(spec, "\\*.bsp");
	
	_finddata_t data;
	int h = _findfirst(spec, &data);
	while (h != -1)
	{
		printf("Processing %s...\n", data.name);

		char name[256];
		sprintf(name, "%s\\%s", argv[1], data.name);
		
		process(name, false);
		
		if (_findnext(h, &data)) break;
	}
	_findclose(h);

	return 0;
}