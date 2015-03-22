/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2005 - 2015, ioquake3 contributors
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

// tr_map.c

#include "../server/exe_headers.h"

#include "tr_local.h"

/*

Loads and prepares a map file for scene rendering.

A single entry point:

void RE_LoadWorldMap( const char *name );

*/

static	world_t		s_worldData;
static	byte		*fileBase;

int			c_subdivisions;
int			c_gridVerts;

//===============================================================================

static void HSVtoRGB( float h, float s, float v, float rgb[3] )
{
	int i;
	float f;
	float p, q, t;

	h *= 5;

	i = floor( h );
	f = h - i;

	p = v * ( 1 - s );
	q = v * ( 1 - s * f );
	t = v * ( 1 - s * ( 1 - f ) );

	switch ( i )
	{
	case 0:
		rgb[0] = v;
		rgb[1] = t;
		rgb[2] = p;
		break;
	case 1:
		rgb[0] = q;
		rgb[1] = v;
		rgb[2] = p;
		break;
	case 2:
		rgb[0] = p;
		rgb[1] = v;
		rgb[2] = t;
		break;
	case 3:
		rgb[0] = p;
		rgb[1] = q;
		rgb[2] = v;
		break;
	case 4:
		rgb[0] = t;
		rgb[1] = p;
		rgb[2] = v;
		break;
	case 5:
		rgb[0] = v;
		rgb[1] = p;
		rgb[2] = q;
		break;
	}
}

/*
===============
R_ColorShiftLightingBytes

===============
*/
void R_ColorShiftLightingBytes( byte in[4], byte out[4] ) {
	int shift, r, g, b;

	// shift the color data based on overbright range
	shift = Q_max( 0, r_mapOverBrightBits->integer - tr.overbrightBits );

	// shift the data based on overbright range
	r = in[0] << shift;
	g = in[1] << shift;
	b = in[2] << shift;

	// normalize by color instead of saturating to white
	if ( (r|g|b) > 255 ) {
		int		max;

		max = r > g ? r : g;
		max = max > b ? max : b;
		r = r * 255 / max;
		g = g * 255 / max;
		b = b * 255 / max;
	}

	out[0] = r;
	out[1] = g;
	out[2] = b;
	out[3] = in[3];
}

/*
===============
R_ColorShiftLightingBytes

===============
*/
void R_ColorShiftLightingBytes( byte in[3] ) {
	int shift, r, g, b;

	// shift the color data based on overbright range
	shift = Q_max( 0, r_mapOverBrightBits->integer - tr.overbrightBits );

	// shift the data based on overbright range
	r = in[0] << shift;
	g = in[1] << shift;
	b = in[2] << shift;

	// normalize by color instead of saturating to white
	if ( (r|g|b) > 255 ) {
		int		max;

		max = r > g ? r : g;
		max = max > b ? max : b;
		r = r * 255 / max;
		g = g * 255 / max;
		b = b * 255 / max;
	}

	in[0] = r;
	in[1] = g;
	in[2] = b;
}

/*
===============
R_LoadLightmaps

===============
*/
#define	LIGHTMAP_SIZE	128
static	void R_LoadLightmaps( lump_t *l, const char *psMapName, world_t &worldData ) 
{
	byte				*buf, *buf_p;
	int					len;
	byte		image[LIGHTMAP_SIZE*LIGHTMAP_SIZE*4];
	int					i, j;
	float				maxIntensity = 0;
	double				sumIntensity = 0;
	int					count;

	if (&worldData == &s_worldData)
	{
		tr.numLightmaps = 0;
	}

    len = l->filelen;
	if ( !len ) {
		return;
	}
	buf = fileBase + l->fileofs;

	// we are about to upload textures
	R_IssuePendingRenderCommands(); //

	// create all the lightmaps
	worldData.startLightMapIndex = tr.numLightmaps;
	count = len / (LIGHTMAP_SIZE * LIGHTMAP_SIZE * 3);
	tr.numLightmaps += count;

	// if we are in r_vertexLight mode, we don't need the lightmaps at all
	if ( r_vertexLight->integer ) {
		return;
	}

	char sMapName[MAX_QPATH];
	COM_StripExtension(psMapName,sMapName, sizeof(sMapName));

	for ( i = 0 ; i < count ; i++ ) {
		// expand the 24 bit on-disk to 32 bit
		buf_p = buf + i * LIGHTMAP_SIZE*LIGHTMAP_SIZE * 3;

		if ( r_lightmap->integer == 2 )
		{	// color code by intensity as development tool	(FIXME: check range)
			for ( j = 0; j < LIGHTMAP_SIZE * LIGHTMAP_SIZE; j++ )
			{
				float r = buf_p[j*3+0];
				float g = buf_p[j*3+1];
				float b = buf_p[j*3+2];
				float intensity;
				float out[3] = {0.0f, 0.0f, 0.0f};

				intensity = 0.33f * r + 0.685f * g + 0.063f * b;

				if ( intensity > 255 )
					intensity = 1.0f;
				else
					intensity /= 255.0f;

				if ( intensity > maxIntensity )
					maxIntensity = intensity;

				HSVtoRGB( intensity, 1.00, 0.50, out );

				image[j*4+0] = out[0] * 255;
				image[j*4+1] = out[1] * 255;
				image[j*4+2] = out[2] * 255;
				image[j*4+3] = 255;

				sumIntensity += intensity;
			}
		} else {
			for ( j = 0 ; j < LIGHTMAP_SIZE * LIGHTMAP_SIZE; j++ ) {
				R_ColorShiftLightingBytes( &buf_p[j*3], &image[j*4] );
				image[j*4+3] = 255;
			}
		}
		tr.lightmaps[worldData.startLightMapIndex+i] = R_CreateImage( va("$%s/lightmap%d",sMapName,worldData.startLightMapIndex+i), image, 
			LIGHTMAP_SIZE, LIGHTMAP_SIZE, GL_RGBA, qfalse, qfalse, r_ext_compressed_lightmaps->integer, GL_CLAMP );
	}

	if ( r_lightmap->integer == 2 )	{
		ri.Printf( PRINT_ALL, "Brightest lightmap value: %d\n", ( int ) ( maxIntensity * 255 ) );
	}
}



/*
=================
RE_SetWorldVisData

This is called by the clipmodel subsystem so we can share the 1.8 megs of
space in big maps...
=================
*/
void		RE_SetWorldVisData( const byte *vis ) {
	tr.externalVisData = vis;
}


/*
=================
R_LoadVisibility
=================
*/
static	void R_LoadVisibility( lump_t *l, world_t &worldData ) {
	int		len;
	byte	*buf;

	len = ( worldData.numClusters + 63 ) & ~63;
	worldData.novis = ( unsigned char *) Hunk_Alloc( len, qfalse );
	memset( worldData.novis, 0xff, len );

    len = l->filelen;
	if ( !len ) {
		return;
	}
	buf = fileBase + l->fileofs;

	worldData.numClusters = LittleLong( ((int *)buf)[0] );
	worldData.clusterBytes = LittleLong( ((int *)buf)[1] );

	// CM_Load should have given us the vis data to share, so
	// we don't need to allocate another copy
	if ( tr.externalVisData ) {
		worldData.vis = tr.externalVisData;
	} else {
		byte	*dest;

		dest = (byte *) Hunk_Alloc( len - 8, qfalse );
		memcpy( dest, buf + 8, len - 8 );
		worldData.vis = dest;
	}
}

//===============================================================================

/*
===============
ShaderForShaderNum
===============
*/
static shader_t *ShaderForShaderNum( int shaderNum, const int *lightmapNum, const byte *lightmapStyles, const byte *vertexStyles, world_t &worldData ) {
	shader_t	*shader;
	dshader_t	*dsh;
	const byte	*styles;

	styles = lightmapStyles;

	shaderNum = LittleLong( shaderNum );
	if ( shaderNum < 0 || shaderNum >= worldData.numShaders ) {
		Com_Error( ERR_DROP, "ShaderForShaderNum: bad num %i", shaderNum );
	}
	dsh = &worldData.shaders[ shaderNum ];

	if (lightmapNum[0] == LIGHTMAP_BY_VERTEX)
	{
		styles = vertexStyles;
	}

	if ( r_vertexLight->integer ) 
	{
		lightmapNum = lightmapsVertex;
		styles = vertexStyles;
	}

/*	if ( r_fullbright->integer ) 
	{
		lightmapNum = lightmapsFullBright;
		styles = vertexStyles;
	}
*/
	shader = R_FindShader( dsh->shader, lightmapNum, styles, qtrue );

	// if the shader had errors, just use default shader
	if ( shader->defaultShader ) {
		return tr.defaultShader;
	}

	return shader;
}

/*
===============
ParseFace
===============
*/
static void ParseFace( dsurface_t *ds, mapVert_t *verts, msurface_t *surf, int *indexes, byte *&pFaceDataBuffer, world_t &worldData, int index )
{
	int			i, j, k;
	srfSurfaceFace_t	*cv;
	int			numPoints, numIndexes;
	int			lightmapNum[MAXLIGHTMAPS];
	int			sfaceSize, ofsIndexes;

	for(i=0;i<MAXLIGHTMAPS;i++)
	{
		lightmapNum[i] = LittleLong( ds->lightmapNum[i] );
		if (lightmapNum[i] >= 0)
		{
			lightmapNum[i] += worldData.startLightMapIndex;
		}
	}

	// get fog volume
	surf->fogIndex = LittleLong( ds->fogNum ) + 1;
	if (index && !surf->fogIndex && tr.world && tr.world->globalFog != -1)
	{
		surf->fogIndex = worldData.globalFog;
	}

	// get shader value
	surf->shader = ShaderForShaderNum( ds->shaderNum, lightmapNum, ds->lightmapStyles, ds->vertexStyles, worldData );
	if ( r_singleShader->integer && !surf->shader->sky ) {
		surf->shader = tr.defaultShader;
	}

	numPoints = LittleLong( ds->numVerts );
	numIndexes = LittleLong( ds->numIndexes );

	// create the srfSurfaceFace_t
	sfaceSize = ( intptr_t ) &((srfSurfaceFace_t *)0)->points[numPoints];
	ofsIndexes = sfaceSize;
	sfaceSize += sizeof( int ) * numIndexes;

	cv = (srfSurfaceFace_t *) pFaceDataBuffer;//Hunk_Alloc( sfaceSize );
	pFaceDataBuffer += sfaceSize;	// :-)

	cv->surfaceType = SF_FACE;
	cv->numPoints = numPoints;
	cv->numIndices = numIndexes;
	cv->ofsIndices = ofsIndexes;

	verts += LittleLong( ds->firstVert );
	for ( i = 0 ; i < numPoints ; i++ ) {
		for ( j = 0 ; j < 3 ; j++ ) {
			cv->points[i][j] = LittleFloat( verts[i].xyz[j] );
		}
		for ( j = 0 ; j < 2 ; j++ ) {
			cv->points[i][3+j] = LittleFloat( verts[i].st[j] );
			for(k=0;k<MAXLIGHTMAPS;k++)
			{
				cv->points[i][VERTEX_LM+j+(k*2)] = LittleFloat( verts[i].lightmap[k][j] );
			}
		}
		for(k=0;k<MAXLIGHTMAPS;k++)
		{
			R_ColorShiftLightingBytes( verts[i].color[k], (byte *)&cv->points[i][VERTEX_COLOR+k] );
		}
	}

	indexes += LittleLong( ds->firstIndex );
	for ( i = 0 ; i < numIndexes ; i++ ) {
		((int *)((byte *)cv + cv->ofsIndices ))[i] = LittleLong( indexes[ i ] );
	}

	// take the plane information from the lightmap vector
	for ( i = 0 ; i < 3 ; i++ ) {
		cv->plane.normal[i] = LittleFloat( ds->lightmapVecs[2][i] );
	}
	cv->plane.dist = DotProduct( cv->points[0], cv->plane.normal );
	SetPlaneSignbits( &cv->plane );
	cv->plane.type = PlaneTypeForNormal( cv->plane.normal );

	surf->data = (surfaceType_t *)cv;
}


/*
===============
ParseMesh
===============
*/
static void ParseMesh ( dsurface_t *ds, mapVert_t *verts, msurface_t *surf, world_t &worldData, int index) {
	srfGridMesh_t	*grid;
	int				i, j, k;
	int				width, height, numPoints;
	drawVert_t points[MAX_PATCH_SIZE*MAX_PATCH_SIZE];
	int				lightmapNum[MAXLIGHTMAPS];
	vec3_t			bounds[2];
	vec3_t			tmpVec;
	static surfaceType_t	skipData = SF_SKIP;

	for(i=0;i<MAXLIGHTMAPS;i++)
	{
		lightmapNum[i] = LittleLong( ds->lightmapNum[i] );
		if (lightmapNum[i] >= 0)
		{
			lightmapNum[i] += worldData.startLightMapIndex;
		}
	}

	// get fog volume
	surf->fogIndex = LittleLong( ds->fogNum ) + 1;
	if (index && !surf->fogIndex && tr.world && tr.world->globalFog != -1)
	{
		surf->fogIndex = worldData.globalFog;
	}

	// get shader value
	surf->shader = ShaderForShaderNum( ds->shaderNum, lightmapNum, ds->lightmapStyles, ds->vertexStyles, worldData );
	if ( r_singleShader->integer && !surf->shader->sky ) {
		surf->shader = tr.defaultShader;
	}

	// we may have a nodraw surface, because they might still need to
	// be around for movement clipping
	if ( worldData.shaders[ LittleLong( ds->shaderNum ) ].surfaceFlags & SURF_NODRAW ) {
		surf->data = &skipData;
		return;
	}

	width = LittleLong( ds->patchWidth );
	height = LittleLong( ds->patchHeight );

	verts += LittleLong( ds->firstVert );
	numPoints = width * height;
	for ( i = 0 ; i < numPoints ; i++ ) {
		for ( j = 0 ; j < 3 ; j++ ) {
			points[i].xyz[j] = LittleFloat( verts[i].xyz[j] );
			points[i].normal[j] = LittleFloat( verts[i].normal[j] );
		}
		for ( j = 0 ; j < 2 ; j++ ) {
			points[i].st[j] = LittleFloat( verts[i].st[j] );
			for(k=0;k<MAXLIGHTMAPS;k++)
			{
				points[i].lightmap[k][j] = LittleFloat( verts[i].lightmap[k][j] );
			}
		}
		for(k=0;k<MAXLIGHTMAPS;k++)
		{
			R_ColorShiftLightingBytes( verts[i].color[k], points[i].color[k] );
		}
	}

	// pre-tesseleate
	grid = R_SubdividePatchToGrid( width, height, points );
	surf->data = (surfaceType_t *)grid;

	// copy the level of detail origin, which is the center
	// of the group of all curves that must subdivide the same
	// to avoid cracking
	for ( i = 0 ; i < 3 ; i++ ) {
		bounds[0][i] = LittleFloat( ds->lightmapVecs[0][i] );
		bounds[1][i] = LittleFloat( ds->lightmapVecs[1][i] );
	}
	VectorAdd( bounds[0], bounds[1], bounds[1] );
	VectorScale( bounds[1], 0.5f, grid->lodOrigin );
	VectorSubtract( bounds[0], grid->lodOrigin, tmpVec );
	grid->lodRadius = VectorLength( tmpVec );
}

/*
===============
ParseTriSurf
===============
*/
static void ParseTriSurf( dsurface_t *ds, mapVert_t *verts, msurface_t *surf, int *indexes, world_t &worldData, int index ) {
	srfTriangles_t	*tri;
	int				i, j, k;
	int				numVerts, numIndexes;

	// get fog volume
	surf->fogIndex = LittleLong( ds->fogNum ) + 1;
	if (index && !surf->fogIndex && tr.world && tr.world->globalFog != -1)
	{
		surf->fogIndex = worldData.globalFog;
	}

	// get shader
	surf->shader = ShaderForShaderNum( ds->shaderNum, lightmapsVertex, ds->lightmapStyles, ds->vertexStyles, worldData );
	if ( r_singleShader->integer && !surf->shader->sky ) {
		surf->shader = tr.defaultShader;
	}

	numVerts = LittleLong( ds->numVerts );
	numIndexes = LittleLong( ds->numIndexes );

	if ( numVerts >= SHADER_MAX_VERTEXES ) {
		Com_Error(ERR_DROP, "ParseTriSurf: verts > MAX (%d > %d) on misc_model %s", numVerts, SHADER_MAX_VERTEXES, surf->shader->name );
	}
	if ( numIndexes >= SHADER_MAX_INDEXES ) {
		Com_Error(ERR_DROP, "ParseTriSurf: indices > MAX (%d > %d) on misc_model %s", numIndexes, SHADER_MAX_INDEXES, surf->shader->name );
	}

	tri = (srfTriangles_t *) Z_Malloc( sizeof( *tri ) + numVerts * sizeof( tri->verts[0] ) + numIndexes * sizeof( tri->indexes[0] ), TAG_HUNKMISCMODELS, qfalse );
	tri->dlightBits = 0; //JIC
	tri->surfaceType = SF_TRIANGLES;
	tri->numVerts = numVerts;
	tri->numIndexes = numIndexes;
	tri->verts = (drawVert_t *)(tri + 1);
	tri->indexes = (int *)(tri->verts + tri->numVerts );

	surf->data = (surfaceType_t *)tri;

	// copy vertexes
	verts += LittleLong( ds->firstVert );
	ClearBounds( tri->bounds[0], tri->bounds[1] );
	for ( i = 0 ; i < numVerts ; i++ ) {
		for ( j = 0 ; j < 3 ; j++ ) {
			tri->verts[i].xyz[j] = LittleFloat( verts[i].xyz[j] );
			tri->verts[i].normal[j] = LittleFloat( verts[i].normal[j] );
		}
		AddPointToBounds( tri->verts[i].xyz, tri->bounds[0], tri->bounds[1] );
		for ( j = 0 ; j < 2 ; j++ ) {
			tri->verts[i].st[j] = LittleFloat( verts[i].st[j] );
			for(k=0;k<MAXLIGHTMAPS;k++)
			{
				tri->verts[i].lightmap[k][j] = LittleFloat( verts[i].lightmap[k][j] );
			}
		}
		for(k=0;k<MAXLIGHTMAPS;k++)
		{
			R_ColorShiftLightingBytes( verts[i].color[k], tri->verts[i].color[k] );
		}
	}

	// copy indexes
	indexes += LittleLong( ds->firstIndex );
	for ( i = 0 ; i < numIndexes ; i++ ) {
		tri->indexes[i] = LittleLong( indexes[i] );
		if ( tri->indexes[i] < 0 || tri->indexes[i] >= numVerts ) {
			Com_Error( ERR_DROP, "Bad index in triangle surface" );
		}
	}
}

/*
===============
ParseFlare
===============
*/
static void ParseFlare( dsurface_t *ds, mapVert_t *verts, msurface_t *surf, int *indexes, world_t &worldData, int index ) {
	srfFlare_t		*flare;
	int				i;
	int		lightmaps[MAXLIGHTMAPS] = { LIGHTMAP_BY_VERTEX };

	// get fog volume
	surf->fogIndex = LittleLong( ds->fogNum ) + 1;
	if (index && !surf->fogIndex && tr.world->globalFog != -1)
	{
		surf->fogIndex = worldData.globalFog;
	}

	// get shader
	surf->shader = ShaderForShaderNum( ds->shaderNum, lightmaps, ds->lightmapStyles, ds->vertexStyles, worldData );
	if ( r_singleShader->integer && !surf->shader->sky ) {
		surf->shader = tr.defaultShader;
	}

	flare = (srfFlare_t *) Hunk_Alloc( sizeof( *flare ), qtrue );
	flare->surfaceType = SF_FLARE;

	surf->data = (surfaceType_t *)flare;

	for ( i = 0 ; i < 3 ; i++ ) {
		flare->origin[i] = LittleFloat( ds->lightmapOrigin[i] );
		flare->color[i] = LittleFloat( ds->lightmapVecs[0][i] );
		flare->normal[i] = LittleFloat( ds->lightmapVecs[2][i] );
	}
}

/*
===============
R_LoadSurfaces
===============
*/
static	void R_LoadSurfaces( lump_t *surfs, lump_t *verts, lump_t *indexLump, world_t &worldData, int index ) {
	dsurface_t	*in;
	msurface_t	*out;
	mapVert_t	*dv;
	int			*indexes;
	int			count;
	int			numFaces, numMeshes, numTriSurfs, numFlares;
	int			i;

	numFaces = 0;
	numMeshes = 0;
	numTriSurfs = 0;
	numFlares = 0;

	in = (dsurface_t *)(fileBase + surfs->fileofs);
	if (surfs->filelen % sizeof(*in))
		Com_Error (ERR_DROP, "LoadMap: funny lump size in %s",worldData.name);
	count = surfs->filelen / sizeof(*in);

	dv = (mapVert_t *)(fileBase + verts->fileofs);
	if (verts->filelen % sizeof(*dv))
		Com_Error (ERR_DROP, "LoadMap: funny lump size in %s",worldData.name);

	indexes = (int *)(fileBase + indexLump->fileofs);
	if ( indexLump->filelen % sizeof(*indexes))
		Com_Error (ERR_DROP, "LoadMap: funny lump size in %s",worldData.name);

	out = (struct msurface_s *) Hunk_Alloc ( count * sizeof(*out), qtrue );	

	worldData.surfaces = out;
	worldData.numsurfaces = count;

	// new bit, the face code on our biggest map requires over 15,000 mallocs, which was no problem on the hunk,
	//	bit hits the zone pretty bad (even the tagFree takes about 9 seconds for that many memblocks), 
	//	so special-case pre-alloc enough space for this data (the patches etc can stay as they are)...
	//
	int iFaceDataSizeRequired = 0;
	for ( i = 0 ; i < count ; i++, in++) 
	{
		switch ( LittleLong( in->surfaceType ) ) 
		{
			case MST_PLANAR:
				
				int sfaceSize = ( intptr_t ) &((srfSurfaceFace_t *)0)->points[LittleLong(in->numVerts)];	
					sfaceSize += sizeof( int ) * LittleLong(in->numIndexes);

				iFaceDataSizeRequired += sfaceSize;
				break;
		}
	}
	in -= count;	// back it up, ready for loop-proper

	// since this ptr is to hunk data, I can pass it in and have it advanced without worrying about losing
	//	the original alloc ptr...
	//
	byte *pFaceDataBuffer	= (byte *)Hunk_Alloc( iFaceDataSizeRequired, qtrue );	

	// now do regular loop...
	//
	for ( i = 0 ; i < count ; i++, in++, out++ ) {
		switch ( LittleLong( in->surfaceType ) ) {
		case MST_PATCH:
			ParseMesh ( in, dv, out, worldData, index );
			numMeshes++;
			break;
		case MST_TRIANGLE_SOUP:
			ParseTriSurf( in, dv, out, indexes, worldData, index );
			numTriSurfs++;
			break;
		case MST_PLANAR:
			ParseFace( in, dv, out, indexes, pFaceDataBuffer, worldData, index );
			numFaces++;
			break;
		case MST_FLARE:
			ParseFlare( in, dv, out, indexes, worldData, index );
			numFlares++;
			break;
		default:
			Com_Error( ERR_DROP, "Bad surfaceType" );
		}
	}
	
	ri.Printf( PRINT_ALL, "...loaded %d faces, %i meshes, %i trisurfs, %i flares\n", 
		numFaces, numMeshes, numTriSurfs, numFlares );
}



/*
=================
R_LoadSubmodels
=================
*/
static	void R_LoadSubmodels( lump_t *l, world_t &worldData, int index  ) {
	dmodel_t	*in;
	bmodel_t	*out;
	int			i, j, count;

	in = (dmodel_t *)(fileBase + l->fileofs);
	if (l->filelen % sizeof(*in))
		Com_Error (ERR_DROP, "LoadMap: funny lump size in %s",worldData.name);
	count = l->filelen / sizeof(*in);

	worldData.bmodels = out = (bmodel_t *) Hunk_Alloc( count * sizeof(*out), qtrue );

	for ( i=0 ; i<count ; i++, in++, out++ ) {
		model_t *model;

		model = R_AllocModel();

		assert( model != NULL );			// this should never happen
		if ( model == NULL ) {
			ri.Error(ERR_DROP, "R_LoadSubmodels: R_AllocModel() failed");
		}

		model->type = MOD_BRUSH;
		model->bmodel = out;
		if (index)
		{
			Com_sprintf( model->name, sizeof( model->name ), "*%d-%d", index, i );
			model->bspInstance = true;
		}
		else
		{
			Com_sprintf( model->name, sizeof( model->name ), "*%d", i);
		}

		for (j=0 ; j<3 ; j++) {
			out->bounds[0][j] = LittleFloat (in->mins[j]);
			out->bounds[1][j] = LittleFloat (in->maxs[j]);
		}
/*
Ghoul2 Insert Start
*/

		RE_InsertModelIntoHash(model->name, model);
/*
Ghoul2 Insert End
*/

		out->firstSurface = worldData.surfaces + LittleLong( in->firstSurface );
		out->numSurfaces = LittleLong( in->numSurfaces );
	}
}



//==================================================================

/*
=================
R_SetParent
=================
*/
static	void R_SetParent (mnode_t *node, mnode_t *parent)
{
	node->parent = parent;
	if (node->contents != -1)
		return;
	R_SetParent (node->children[0], node);
	R_SetParent (node->children[1], node);
}

/*
=================
R_LoadNodesAndLeafs
=================
*/
static	void R_LoadNodesAndLeafs (lump_t *nodeLump, lump_t *leafLump, world_t &worldData) {
	int			i, j, p;
	dnode_t		*in;
	dleaf_t		*inLeaf;
	mnode_t 	*out;
	int			numNodes, numLeafs;

	in = (dnode_t *)(fileBase + nodeLump->fileofs);
	if (nodeLump->filelen % sizeof(dnode_t) ||
		leafLump->filelen % sizeof(dleaf_t) ) {
		Com_Error (ERR_DROP, "LoadMap: funny lump size in %s",worldData.name);
	}
	numNodes = nodeLump->filelen / sizeof(dnode_t);
	numLeafs = leafLump->filelen / sizeof(dleaf_t);

	out = (struct mnode_s *) Hunk_Alloc ( (numNodes + numLeafs) * sizeof(*out), qtrue );	

	worldData.nodes = out;
	worldData.numnodes = numNodes + numLeafs;
	worldData.numDecisionNodes = numNodes;

	// load nodes
	for ( i=0 ; i<numNodes; i++, in++, out++)
	{
		for (j=0 ; j<3 ; j++)
		{
			out->mins[j] = LittleLong (in->mins[j]);
			out->maxs[j] = LittleLong (in->maxs[j]);
		}

		p = LittleLong(in->planeNum);
		out->plane = worldData.planes + p;

		out->contents = CONTENTS_NODE;	// differentiate from leafs

		for (j=0 ; j<2 ; j++)
		{
			p = LittleLong (in->children[j]);
			if (p >= 0)
				out->children[j] = worldData.nodes + p;
			else
				out->children[j] = worldData.nodes + numNodes + (-1 - p);
		}
	}

	// load leafs
	inLeaf = (dleaf_t *)(fileBase + leafLump->fileofs);
	for ( i=0 ; i<numLeafs ; i++, inLeaf++, out++)
	{
		for (j=0 ; j<3 ; j++)
		{
			out->mins[j] = LittleLong (inLeaf->mins[j]);
			out->maxs[j] = LittleLong (inLeaf->maxs[j]);
		}

		out->cluster = LittleLong(inLeaf->cluster);
		out->area = LittleLong(inLeaf->area);

		if ( out->cluster >= worldData.numClusters ) {
			worldData.numClusters = out->cluster + 1;
		}

		out->firstmarksurface = worldData.marksurfaces +
			LittleLong(inLeaf->firstLeafSurface);
		out->nummarksurfaces = LittleLong(inLeaf->numLeafSurfaces);
	}

	// chain decendants
	R_SetParent (worldData.nodes, NULL);
}

//=============================================================================

/*
=================
R_LoadShaders
=================
*/
static	void R_LoadShaders( lump_t *l, world_t &worldData ) {	
	int		i, count;
	dshader_t	*in, *out;

	in = (dshader_t *)(fileBase + l->fileofs);
	if (l->filelen % sizeof(*in))
		Com_Error (ERR_DROP, "LoadMap: funny lump size in %s",worldData.name);
	count = l->filelen / sizeof(*in);
	out = (dshader_t *) Hunk_Alloc ( count*sizeof(*out), qfalse );

	worldData.shaders = out;
	worldData.numShaders = count;

	memcpy( out, in, count*sizeof(*out) );

	for ( i=0 ; i<count ; i++ ) {
		out[i].surfaceFlags = LittleLong( out[i].surfaceFlags );
		out[i].contentFlags = LittleLong( out[i].contentFlags );
	}
}


/*
=================
R_LoadMarksurfaces
=================
*/
static	void R_LoadMarksurfaces (lump_t *l, world_t &worldData)
{
	int		i, j, count;
	int		*in;
	msurface_t **out;

	in = (int *)(fileBase + l->fileofs);
	if (l->filelen % sizeof(*in))
		Com_Error (ERR_DROP, "LoadMap: funny lump size in %s",worldData.name);
	count = l->filelen / sizeof(*in);
	out = (struct msurface_s **) Hunk_Alloc ( count*sizeof(*out), qtrue );	

	worldData.marksurfaces = out;
	worldData.nummarksurfaces = count;

	for ( i=0 ; i<count ; i++)
	{
		j = LittleLong(in[i]);
		out[i] = worldData.surfaces + j;
	}
}


/*
=================
R_LoadPlanes
=================
*/
static	void R_LoadPlanes( lump_t *l, world_t &worldData ) {
	int			i, j;
	cplane_t	*out;
	dplane_t 	*in;
	int			count;
	int			bits;

	in = (dplane_t *)(fileBase + l->fileofs);
	if (l->filelen % sizeof(*in))
		Com_Error (ERR_DROP, "LoadMap: funny lump size in %s",worldData.name);
	count = l->filelen / sizeof(*in);
	out = (struct cplane_s *) Hunk_Alloc ( count*2*sizeof(*out), qtrue );	

	worldData.planes = out;
	worldData.numplanes = count;

	for ( i=0 ; i<count ; i++, in++, out++) {
		bits = 0;
		for (j=0 ; j<3 ; j++) {
			out->normal[j] = LittleFloat (in->normal[j]);
			if (out->normal[j] < 0) {
				bits |= 1<<j;
			}
		}

		out->dist = LittleFloat (in->dist);
		out->type = PlaneTypeForNormal( out->normal );
		out->signbits = bits;
	}
}

/*
=================
R_LoadFogs

=================
*/
static	void R_LoadFogs( lump_t *l, lump_t *brushesLump, lump_t *sidesLump, world_t &worldData, int index ) {
	int			i;
	fog_t		*out;
	dfog_t		*fogs;
	dbrush_t 	*brushes, *brush;
	dbrushside_t	*sides;
	int			count, brushesCount, sidesCount;
	int			sideNum;
	int			planeNum;
	shader_t	*shader;
	float		d;
	int			firstSide=0;
	int			lightmaps[MAXLIGHTMAPS] = { LIGHTMAP_NONE } ;

	fogs = (dfog_t *)(fileBase + l->fileofs);
	if (l->filelen % sizeof(*fogs)) {
		Com_Error (ERR_DROP, "LoadMap: funny lump size in %s",worldData.name);
	}
	count = l->filelen / sizeof(*fogs);

	// create fog strucutres for them
	worldData.numfogs = count + 1;
	worldData.fogs = (fog_t *)Hunk_Alloc ( (worldData.numfogs+1)*sizeof(*out), qtrue);
	worldData.globalFog = -1;
	out = worldData.fogs + 1;

	// Copy the global fog from the main world into the bsp instance
	if(index)
	{
		if(tr.world && (tr.world->globalFog != -1))
		{
			// Use the nightvision fog slot
			worldData.fogs[worldData.numfogs] = tr.world->fogs[tr.world->globalFog];
			worldData.globalFog = worldData.numfogs;
			worldData.numfogs++;
		}
	}

	if ( !count ) {
		return;
	}

	brushes = (dbrush_t *)(fileBase + brushesLump->fileofs);
	if (brushesLump->filelen % sizeof(*brushes)) {
		Com_Error (ERR_DROP, "LoadMap: funny lump size in %s",worldData.name);
	}
	brushesCount = brushesLump->filelen / sizeof(*brushes);

	sides = (dbrushside_t *)(fileBase + sidesLump->fileofs);
	if (sidesLump->filelen % sizeof(*sides)) {
		Com_Error (ERR_DROP, "LoadMap: funny lump size in %s",worldData.name);
	}
	sidesCount = sidesLump->filelen / sizeof(*sides);

	for ( i=0 ; i<count ; i++, fogs++) {
		out->originalBrushNumber = LittleLong( fogs->brushNum );
		if (out->originalBrushNumber == -1)
		{
			if(index)
			{
				Com_Error (ERR_DROP, "LoadMap: global fog not allowed in bsp instances - %s", worldData.name);
			}
			VectorSet(out->bounds[0], MIN_WORLD_COORD, MIN_WORLD_COORD, MIN_WORLD_COORD);
			VectorSet(out->bounds[1], MAX_WORLD_COORD, MAX_WORLD_COORD, MAX_WORLD_COORD);
			worldData.globalFog = i + 1;
		}
		else
		{
			if ( (unsigned)out->originalBrushNumber >= (unsigned)brushesCount ) {
				Com_Error( ERR_DROP, "fog brushNumber out of range" );
			}
			brush = brushes + out->originalBrushNumber;

			firstSide = LittleLong( brush->firstSide );

				if ( (unsigned)firstSide > (unsigned)(sidesCount - 6) ) {
				Com_Error( ERR_DROP, "fog brush sideNumber out of range" );
			}

			// brushes are always sorted with the axial sides first
			sideNum = firstSide + 0;
			planeNum = LittleLong( sides[ sideNum ].planeNum );
			out->bounds[0][0] = -worldData.planes[ planeNum ].dist;

			sideNum = firstSide + 1;
			planeNum = LittleLong( sides[ sideNum ].planeNum );
			out->bounds[1][0] = worldData.planes[ planeNum ].dist;

			sideNum = firstSide + 2;
			planeNum = LittleLong( sides[ sideNum ].planeNum );
			out->bounds[0][1] = -worldData.planes[ planeNum ].dist;

			sideNum = firstSide + 3;
			planeNum = LittleLong( sides[ sideNum ].planeNum );
			out->bounds[1][1] = worldData.planes[ planeNum ].dist;

			sideNum = firstSide + 4;
			planeNum = LittleLong( sides[ sideNum ].planeNum );
			out->bounds[0][2] = -worldData.planes[ planeNum ].dist;

			sideNum = firstSide + 5;
			planeNum = LittleLong( sides[ sideNum ].planeNum );
			out->bounds[1][2] = worldData.planes[ planeNum ].dist;
		}

		// get information from the shader for fog parameters
		shader = R_FindShader( fogs->shader, lightmaps, stylesDefault, qtrue );

		assert(shader->fogParms);
		if (!shader->fogParms)
		{//bad shader!!
			out->parms.color[0] = 1.0f;
			out->parms.color[1] = 0.0f;
			out->parms.color[2] = 0.0f;
			out->parms.depthForOpaque = 250.0f;
		}
		else
		{
			out->parms = *shader->fogParms;
		}
		out->colorInt = ColorBytes4 ( out->parms.color[0], 
			out->parms.color[1], 
			out->parms.color[2], 1.0 );

		d = out->parms.depthForOpaque < 1 ? 1 : out->parms.depthForOpaque;
		out->tcScale = 1.0f / ( d * 8 );

		// set the gradient vector
		sideNum = LittleLong( fogs->visibleSide );

		if ( sideNum == -1 ) {
			out->hasSurface = qfalse;
		} else {
			out->hasSurface = qtrue;
			planeNum = LittleLong( sides[ firstSide + sideNum ].planeNum );
			VectorSubtract( vec3_origin, worldData.planes[ planeNum ].normal, out->surface );
			out->surface[3] = -worldData.planes[ planeNum ].dist;
		}

		out++;
	}

	if (!index)
	{
		// Initialise the last fog so we can use it with the LA Goggles
		// NOTE: We are might appear to be off the end of the array, but we allocated an extra memory slot above but [purposely] didn't 
		//	increment the total world numFogs to match our array size
		VectorSet(out->bounds[0], MIN_WORLD_COORD, MIN_WORLD_COORD, MIN_WORLD_COORD);
		VectorSet(out->bounds[1], MAX_WORLD_COORD, MAX_WORLD_COORD, MAX_WORLD_COORD);
		out->originalBrushNumber = -1;
		out->parms.color[0] = 0.0f;
		out->parms.color[1] = 0.0f;
		out->parms.color[2] = 0.0f;
		out->parms.depthForOpaque = 0.0f;
		out->colorInt = 0x00000000;
		out->tcScale = 0.0f;
		out->hasSurface = false;
	}
}


/*
================
R_LoadLightGrid

================
*/
void R_LoadLightGrid( lump_t *l, world_t &worldData ) {
	int		i, j;
	vec3_t	maxs;
	world_t	*w;
	float	*wMins, *wMaxs;

	w = &worldData;

	w->lightGridInverseSize[0] = 1.0 / w->lightGridSize[0];
	w->lightGridInverseSize[1] = 1.0 / w->lightGridSize[1];
	w->lightGridInverseSize[2] = 1.0 / w->lightGridSize[2];

	wMins = w->bmodels[0].bounds[0];
	wMaxs = w->bmodels[0].bounds[1];

	for ( i = 0 ; i < 3 ; i++ ) {
		w->lightGridOrigin[i] = w->lightGridSize[i] * ceil( wMins[i] / w->lightGridSize[i] );
		maxs[i] = w->lightGridSize[i] * floor( wMaxs[i] / w->lightGridSize[i] );
		w->lightGridBounds[i] = (maxs[i] - w->lightGridOrigin[i])/w->lightGridSize[i] + 1;
	}

	int numGridDataElements = l->filelen / sizeof(*w->lightGridData);

	w->lightGridData = (mgrid_t *)Hunk_Alloc( l->filelen, qfalse );
	memcpy( w->lightGridData, (void *)(fileBase + l->fileofs), l->filelen );

	// deal with overbright bits
	for ( i = 0 ; i < numGridDataElements ; i++ ) {
		for(j=0;j<MAXLIGHTMAPS;j++)
		{
			R_ColorShiftLightingBytes(w->lightGridData[i].ambientLight[j]);
			R_ColorShiftLightingBytes(w->lightGridData[i].directLight[j]);
		}
	}
}

/*
================
R_LoadLightGridArray

================
*/
void R_LoadLightGridArray( lump_t *l, world_t &worldData ) {
	world_t	*w;
#ifdef Q3_BIG_ENDIAN
	int i;
#endif

	w = &worldData;

	w->numGridArrayElements = w->lightGridBounds[0] * w->lightGridBounds[1] * w->lightGridBounds[2];

	if ( l->filelen != (int)(w->numGridArrayElements * sizeof(*w->lightGridArray)) ) {
		if (l->filelen>0)//don't warn if not even lit
			ri.Printf( PRINT_WARNING, "WARNING: light grid array mismatch\n" );
		w->lightGridData = NULL;
		return;
	}

	w->lightGridArray = (unsigned short *)Hunk_Alloc( l->filelen, qfalse );
	memcpy( w->lightGridArray, (void *)(fileBase + l->fileofs), l->filelen );
#ifdef Q3_BIG_ENDIAN
	for ( i = 0 ; i < w->numGridArrayElements ; i++ ) {
		w->lightGridArray[i] = LittleShort(w->lightGridArray[i]);
	}
#endif
}


/*
================
R_LoadEntities
================
*/
void R_LoadEntities( lump_t *l, world_t &worldData ) {
	const char *p, *token;
	char keyname[MAX_TOKEN_CHARS];
	char value[MAX_TOKEN_CHARS];
	world_t	*w;
	float ambient = 1;

	COM_BeginParseSession();

	w = &worldData;
	w->lightGridSize[0] = 64;
	w->lightGridSize[1] = 64;
	w->lightGridSize[2] = 128;

	VectorSet(tr.sunAmbient, 1, 1, 1);
	tr.distanceCull = 12000;//DEFAULT_DISTANCE_CULL;

	p = (char *)(fileBase + l->fileofs);

	token = COM_ParseExt( &p, qtrue );
	if (!*token || *token != '{') {
		COM_EndParseSession();
		return;
	}

	// only parse the world spawn
	while ( 1 ) {	
		// parse key
		token = COM_ParseExt( &p, qtrue );

		if ( !*token || *token == '}' ) {
			break;
		}
		Q_strncpyz(keyname, token, sizeof(keyname));

		// parse value
		token = COM_ParseExt( &p, qtrue );

		if ( !*token || *token == '}' ) {
			break;
		}
		Q_strncpyz(value, token, sizeof(value));

		// check for remapping of shaders for vertex lighting
/*		s = "vertexremapshader";
		if (!Q_strncmp(keyname, s, strlen(s)) ) {
			s = strchr(value, ';');
			if (!s) {
				ri.Printf( S_COLOR_YELLOW "WARNING: no semi colon in vertexshaderremap '%s'\n", value );
				break;
			}
			*s++ = 0;
			if (r_vertexLight->integer) {
				R_RemapShader(value, s, "0");
			}
			continue;
		}
		// check for remapping of shaders
		s = "remapshader";
		if (!Q_strncmp(keyname, s, strlen(s)) ) {
			s = strchr(value, ';');
			if (!s) {
				ri.Printf( S_COLOR_YELLOW "WARNING: no semi colon in shaderremap '%s'\n", value );
				break;
			}
			*s++ = 0;
			R_RemapShader(value, s, "0");
			continue;
		}
*/		if (!Q_stricmp(keyname, "distanceCull")) {
			sscanf(value, "%f", &tr.distanceCull );
			continue;
		}
		//check for linear fog -rww
		if (!Q_stricmp(keyname, "linFogStart")) {
			sscanf(value, "%f", &tr.rangedFog );
			tr.rangedFog = -tr.rangedFog;
			continue;
		}
		// check for a different grid size
		if (!Q_stricmp(keyname, "gridsize")) {
			sscanf(value, "%f %f %f", &w->lightGridSize[0], &w->lightGridSize[1], &w->lightGridSize[2] );
			continue;
		}
	// find the optional world ambient for arioche
		if (!Q_stricmp(keyname, "_color")) {
			sscanf(value, "%f %f %f", &tr.sunAmbient[0], &tr.sunAmbient[1], &tr.sunAmbient[2] );
			continue;
		}
		if (!Q_stricmp(keyname, "ambient")) {
			sscanf(value, "%f", &ambient);
			continue;
		}
	}
	//both default to 1 so no harm if not present.
	VectorScale( tr.sunAmbient, ambient, tr.sunAmbient);

	COM_EndParseSession();
}


/*
=================
RE_LoadWorldMap

Called directly from cgame
=================
*/
void RE_LoadWorldMap_Actual( const char *name, world_t &worldData, int index ) {
	dheader_t	*header;
	byte		*buffer = NULL;
	qboolean	loadedSubBSP = qfalse;

	if ( tr.worldMapLoaded && !index ) {
		Com_Error( ERR_DROP, "ERROR: attempted to redundantly load world map\n" );
	}

	// set default sun direction to be used if it isn't
	// overridden by a shader
	if (!index)
	{
		skyboxportal = 0;

		tr.sunDirection[0] = 0.45f;
		tr.sunDirection[1] = 0.3f;
		tr.sunDirection[2] = 0.9f;

		VectorNormalize( tr.sunDirection );

		tr.worldMapLoaded = qtrue;

		// clear tr.world so if the level fails to load, the next
		// try will not look at the partially loaded version
		tr.world = NULL;
	}

	// check for cached disk file from the server first...
	//
	if (ri.gpvCachedMapDiskImage())
	{
		if (!strcmp(name, ri.gsCachedMapDiskImage()))
		{
			// we should always get here, if inside the first IF...
			//
			buffer = (byte *)ri.gpvCachedMapDiskImage();
		}
		else
		{
			// this should never happen (ie renderer loading a different map than the server), but just in case...
			//
	//		assert(0);
	//		Z_Free(gpvCachedMapDiskImage);
	//			   gpvCachedMapDiskImage = NULL;
			//rww - this is a valid possibility now because of sub-bsp loading.\
			//it's alright, just keep the current cache
			loadedSubBSP = qtrue;
		}
	}

	tr.worldDir[0] = '\0';
	
	if (buffer == NULL)
	{
		// still needs loading...
		//
		ri.FS_ReadFile( name, (void **)&buffer );
		if ( !buffer ) {
			Com_Error (ERR_DROP, "RE_LoadWorldMap: %s not found", name);
		}
	}

	memset( &worldData, 0, sizeof( worldData ) );
	Q_strncpyz( worldData.name, name, sizeof( worldData.name ) );
	Q_strncpyz( tr.worldDir, name, sizeof( tr.worldDir ) );
	Q_strncpyz( worldData.baseName, COM_SkipPath( worldData.name ), sizeof( worldData.name ) );

	COM_StripExtension( worldData.baseName, worldData.baseName, sizeof( worldData.baseName ) );
	COM_StripExtension( tr.worldDir, tr.worldDir, sizeof( tr.worldDir ) );

	c_gridVerts = 0;

	header = (dheader_t *)buffer;
	fileBase = (byte *)header;

	header->version = LittleLong (header->version);
	
	if ( header->version != BSP_VERSION ) 
	{
		Com_Error (ERR_DROP, "RE_LoadWorldMap: %s has wrong version number (%i should be %i)", name, header->version, BSP_VERSION);
	}

	// swap all the lumps
	for (size_t i=0 ; i<sizeof(dheader_t)/4 ; i++) {
		((int *)header)[i] = LittleLong ( ((int *)header)[i]);
	}

	// load into heap
	R_LoadShaders( &header->lumps[LUMP_SHADERS], worldData );
	R_LoadLightmaps( &header->lumps[LUMP_LIGHTMAPS], name, worldData );
	R_LoadPlanes (&header->lumps[LUMP_PLANES], worldData);
	R_LoadFogs( &header->lumps[LUMP_FOGS], &header->lumps[LUMP_BRUSHES], &header->lumps[LUMP_BRUSHSIDES], worldData, index );
	R_LoadSurfaces( &header->lumps[LUMP_SURFACES], &header->lumps[LUMP_DRAWVERTS], &header->lumps[LUMP_DRAWINDEXES], worldData, index );
	R_LoadMarksurfaces (&header->lumps[LUMP_LEAFSURFACES], worldData);
	R_LoadNodesAndLeafs (&header->lumps[LUMP_NODES], &header->lumps[LUMP_LEAFS], worldData);
	R_LoadSubmodels (&header->lumps[LUMP_MODELS], worldData, index);
	R_LoadVisibility( &header->lumps[LUMP_VISIBILITY], worldData );

	if (!index)
	{
		R_LoadEntities( &header->lumps[LUMP_ENTITIES], worldData );
		R_LoadLightGrid( &header->lumps[LUMP_LIGHTGRID], worldData );
		R_LoadLightGridArray( &header->lumps[LUMP_LIGHTARRAY], worldData );

		// only set tr.world now that we know the entire level has loaded properly
		tr.world = &worldData;
	}


	if (ri.gpvCachedMapDiskImage() && !loadedSubBSP)
	{
		// For the moment, I'm going to keep this disk image around in case we need it to respawn.
		//  No problem for memory, since it'll only be a NZ ptr if we're not low on physical memory
		//	( ie we've got > 96MB).
		//
		//  So don't do this...
		//
		//		Z_Free( gpvCachedMapDiskImage );
		//				gpvCachedMapDiskImage = NULL;
	}
	else
	{
		ri.FS_FreeFile( buffer );
	}
}


// new wrapper used for convenience to tell z_malloc()-fail recovery code whether it's safe to dump the cached-bsp or not.
//
void RE_LoadWorldMap( const char *name )
{
	*(ri.gbUsingCachedMapDataRightNow()) = qtrue;	// !!!!!!!!!!!!

		RE_LoadWorldMap_Actual( name, s_worldData, 0 );

	*(ri.gbUsingCachedMapDataRightNow()) = qfalse;	// !!!!!!!!!!!!
}
