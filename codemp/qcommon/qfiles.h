/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2013 - 2015, OpenJK contributors

This file is part of the OpenJK source code.

OpenJK is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
===========================================================================
*/

#pragma once

#include "../qcommon/q_shared.h"

//
// qfiles.h: quake file formats
// This file must be identical in the quake and utils directories
//

// surface geometry should not exceed these limits
#define	SHADER_MAX_VERTEXES	1000
#define	SHADER_MAX_INDEXES	(6*SHADER_MAX_VERTEXES)


// the maximum size of game relative pathnames
#define	MAX_QPATH		64

/*
========================================================================

PCX files are used for 8 bit images

========================================================================
*/

typedef struct pcx_s {
    char	manufacturer;
    char	version;
    char	encoding;
    char	bits_per_pixel;
    unsigned short	xmin,ymin,xmax,ymax;
    unsigned short	hres,vres;
    unsigned char	palette[48];
    char	reserved;
    char	color_planes;
    unsigned short	bytes_per_line;
    unsigned short	palette_type;
    char	filler[58];
    unsigned char	data;			// unbounded
} pcx_t;


/*
========================================================================

.MD3 triangle model file format

========================================================================
*/

#define MD3_IDENT			(('3'<<24)+('P'<<16)+('D'<<8)+'I')
#define MD3_VERSION			15

// limits
#define MD3_MAX_LODS		3
#define	MD3_MAX_TRIANGLES	8192	// per surface
#define MD3_MAX_VERTS		4096	// per surface
#define MD3_MAX_SHADERS		256		// per surface
#define MD3_MAX_FRAMES		1024	// per model
#define	MD3_MAX_SURFACES	32 + 32	// per model
#define MD3_MAX_TAGS		16		// per frame

// vertex scales
#define	MD3_XYZ_SCALE		(1.0/64)

typedef struct md3Frame_s {
	vec3_t		bounds[2];
	vec3_t		localOrigin;
	float		radius;
	char		name[16];
} md3Frame_t;

typedef struct md3Tag_s {
	char		name[MAX_QPATH];	// tag name
	vec3_t		origin;
	matrix3_t	axis;
} md3Tag_t;

/*
** md3Surface_t
**
** CHUNK			SIZE
** header			sizeof( md3Surface_t )
** shaders			sizeof( md3Shader_t ) * numShaders
** triangles[0]		sizeof( md3Triangle_t ) * numTriangles
** st				sizeof( md3St_t ) * numVerts
** XyzNormals		sizeof( md3XyzNormal_t ) * numVerts * numFrames
*/
typedef struct md3Surface_s {
	int		ident;				//

	char	name[MAX_QPATH];	// polyset name

	int		flags;
	int		numFrames;			// all surfaces in a model should have the same

	int		numShaders;			// all surfaces in a model should have the same
	int		numVerts;

	int		numTriangles;
	int		ofsTriangles;

	int		ofsShaders;			// offset from start of md3Surface_t
	int		ofsSt;				// texture coords are common for all frames
	int		ofsXyzNormals;		// numVerts * numFrames

	int		ofsEnd;				// next surface follows
} md3Surface_t;

typedef struct md3Shader_s {
	char			name[MAX_QPATH];
	int				shaderIndex;	// for in-game use
} md3Shader_t;

typedef struct md3Triangle_s {
	int			indexes[3];
} md3Triangle_t;

typedef struct md3St_s {
	float		st[2];
} md3St_t;

typedef struct md3XyzNormal_s {
	short		xyz[3];
	short		normal;
} md3XyzNormal_t;

typedef struct md3Header_s {
	int			ident;
	int			version;

	char		name[MAX_QPATH];	// model name

	int			flags;

	int			numFrames;
	int			numTags;
	int			numSurfaces;

	int			numSkins;

	int			ofsFrames;			// offset for first frame
	int			ofsTags;			// numFrames * numTags
	int			ofsSurfaces;		// first surface, others follow

	int			ofsEnd;				// end of file
} md3Header_t;


/*
==============================================================================

  .BSP file format

==============================================================================
*/


// little-endian "RBSP"
#define BSP_IDENT				(('P'<<24)+('S'<<16)+('B'<<8)+'R')

#define BSP_VERSION				1


// there shouldn't be any problem with increasing these values at the
// expense of more memory allocation in the utilities
#define	MAX_MAP_MODELS			0x400
#define	MAX_MAP_BRUSHES			0x8000
#define	MAX_MAP_ENTITIES		0x800
#define	MAX_MAP_ENTSTRING		0x40000
#define	MAX_MAP_SHADERS			0x400

#define	MAX_MAP_AREAS			0x100	// MAX_MAP_AREA_BYTES in q_shared must match!
#define	MAX_MAP_FOGS			0x100
#define	MAX_MAP_PLANES			0x20000
#define	MAX_MAP_NODES			0x20000
#define	MAX_MAP_BRUSHSIDES		0x20000
#define	MAX_MAP_LEAFS			0x20000
#define	MAX_MAP_LEAFFACES		0x20000
#define	MAX_MAP_LEAFBRUSHES		0x40000
#define	MAX_MAP_PORTALS			0x20000
#define	MAX_MAP_LIGHTING		0x800000
#define	MAX_MAP_LIGHTGRID		65535
#define	MAX_MAP_LIGHTGRID_ARRAY	0x100000
#define	MAX_MAP_VISIBILITY		0x600000

#define	MAX_MAP_DRAW_SURFS	0x20000
#define	MAX_MAP_DRAW_VERTS	0x80000
#define	MAX_MAP_DRAW_INDEXES	0x80000


// key / value pair sizes in the entities lump
#define	MAX_KEY				32
#define	MAX_VALUE			1024

// the editor uses these predefined yaw angles to orient entities up or down
#define	ANGLE_UP			-1
#define	ANGLE_DOWN			-2

#define	LIGHTMAP_WIDTH		128
#define	LIGHTMAP_HEIGHT		128

//=============================================================================

typedef struct lump_s {
	int		fileofs, filelen;
} lump_t;

#define	LUMP_ENTITIES		0
#define	LUMP_SHADERS		1
#define	LUMP_PLANES			2
#define	LUMP_NODES			3
#define	LUMP_LEAFS			4
#define	LUMP_LEAFSURFACES	5
#define	LUMP_LEAFBRUSHES	6
#define	LUMP_MODELS			7
#define	LUMP_BRUSHES		8
#define	LUMP_BRUSHSIDES		9
#define	LUMP_DRAWVERTS		10
#define	LUMP_DRAWINDEXES	11
#define	LUMP_FOGS			12
#define	LUMP_SURFACES		13
#define	LUMP_LIGHTMAPS		14
#define	LUMP_LIGHTGRID		15
#define	LUMP_VISIBILITY		16
#define LUMP_LIGHTARRAY		17
#define	HEADER_LUMPS		18

typedef struct dheader_s {
	int			ident;
	int			version;

	lump_t		lumps[HEADER_LUMPS];
} dheader_t;

typedef struct dmodel_s {
	float		mins[3], maxs[3];
	int			firstSurface, numSurfaces;
	int			firstBrush, numBrushes;
} dmodel_t;

typedef struct dshader_s {
	char		shader[MAX_QPATH];
	int			surfaceFlags;
	int			contentFlags;
} dshader_t;

// planes x^1 is allways the opposite of plane x

typedef struct dplane_s {
	float		normal[3];
	float		dist;
} dplane_t;

typedef struct dnode_s {
	int			planeNum;
	int			children[2];	// negative numbers are -(leafs+1), not nodes
	int			mins[3];		// for frustom culling
	int			maxs[3];
} dnode_t;

typedef struct dleaf_s {
	int			cluster;			// -1 = opaque cluster (do I still store these?)
	int			area;

	int			mins[3];			// for frustum culling
	int			maxs[3];

	int			firstLeafSurface;
	int			numLeafSurfaces;

	int			firstLeafBrush;
	int			numLeafBrushes;
} dleaf_t;

typedef struct dbrushside_s {
	int			planeNum;			// positive plane side faces out of the leaf
	int			shaderNum;
	int			drawSurfNum;
} dbrushside_t;

typedef struct dbrush_s {
	int			firstSide;
	int			numSides;
	int			shaderNum;		// the shader that determines the contents flags
} dbrush_t;

typedef struct dfog_s {
	char		shader[MAX_QPATH];
	int			brushNum;
	int			visibleSide;	// the brush side that ray tests need to clip against (-1 == none)
} dfog_t;

// Light Style Constants
#define	MAXLIGHTMAPS	4
#define LS_NORMAL		0x00
#define LS_UNUSED		0xfe
#define	LS_LSNONE		0xff //rww - changed name because it unhappily conflicts with a lightsaber state name and changing this is just easier
#define MAX_LIGHT_STYLES		64

typedef struct mapVert_s {
	vec3_t		xyz;
	float		st[2];
	float		lightmap[MAXLIGHTMAPS][2];
	vec3_t		normal;
	byte		color[MAXLIGHTMAPS][4];
} mapVert_t;

typedef struct drawVert_s {
	vec3_t		xyz;
	float		st[2];
	float		lightmap[MAXLIGHTMAPS][2];
	vec3_t		normal;
	byte		color[MAXLIGHTMAPS][4];
} drawVert_t;

typedef struct dgrid_s {
	byte		ambientLight[MAXLIGHTMAPS][3];
	byte		directLight[MAXLIGHTMAPS][3];
	byte		styles[MAXLIGHTMAPS];
	byte		latLong[2];
} dgrid_t;

typedef enum {
	MST_BAD,
	MST_PLANAR,
	MST_PATCH,
	MST_TRIANGLE_SOUP,
	MST_FLARE
} mapSurfaceType_t;

typedef struct dsurface_s {
	int			shaderNum;
	int			fogNum;
	int			surfaceType;

	int			firstVert;
	int			numVerts;

	int			firstIndex;
	int			numIndexes;

	byte		lightmapStyles[MAXLIGHTMAPS], vertexStyles[MAXLIGHTMAPS];
	int			lightmapNum[MAXLIGHTMAPS];
	int			lightmapX[MAXLIGHTMAPS], lightmapY[MAXLIGHTMAPS];
	int			lightmapWidth, lightmapHeight;

	vec3_t		lightmapOrigin;
	matrix3_t	lightmapVecs;	// for patches, [0] and [1] are lodbounds

	int			patchWidth;
	int			patchHeight;
} dsurface_t;

/////////////////////////////////////////////////////////////
//
// Defines and structures required for fonts

#define GLYPH_COUNT			256

// Must match define in stmparse.h
#define STYLE_DROPSHADOW	0x80000000
#define STYLE_BLINK			0x40000000
#define	SET_MASK			0x00ffffff

typedef struct
{
	short		width;					// number of pixels wide
	short		height;					// number of scan lines
	short		horizAdvance;			// number of pixels to advance to the next char
	short		horizOffset;			// x offset into space to render glyph
	int			baseline;				// y offset
	float		s;						// x start tex coord
	float		t;						// y start tex coord
	float		s2;						// x end tex coord
	float		t2;						// y end tex coord
} glyphInfo_t;


// this file corresponds 1:1 with the "*.fontdat" files, so don't change it unless you're going to
//	recompile the fontgen util and regenerate all the fonts!
//
typedef struct dfontdat_s
{
	glyphInfo_t		mGlyphs[GLYPH_COUNT];

	short			mPointSize;
	short			mHeight;				// max height of font
	short			mAscender;
	short			mDescender;

	short			mKoreanHack;
} dfontdat_t;

/////////////////// fonts end ////////////////////////////////////
