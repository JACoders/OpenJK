// tr_map.c

// leave this as first line for PCH reasons...
//
#include "../server/exe_headers.h"

#include "tr_local.h"

#include "../qcommon/cm_local.h"

/*

Loads and prepares a map file for scene rendering.

A single entry point:

void RE_LoadWorldMap( const char *name );

*/

world_t		s_worldData;
byte		*fileBase;
int			c_subdivisions;
int			c_gridVerts;

static int	flareNum = 0;

void R_RMGInit(void);
//===============================================================================

// We use a special hack to prevent slight differences in channels
// from exploding into big differences, as it causes lighting problems
// later on. This is the maximum channel separation for which we
// enable the hack.
#define MAX_GREYSCALE_CHANNEL_DIFF 15

static void R_ColorShiftLightingBytes16( const byte in[4], byte out[2] ) {
	// What's the largest separation between the red, green, and blue
	// channels?
	int chanDiff = max(in[0],max(in[1],in[2])) -
		min(in[0],min(in[1],in[2]));
	if (chanDiff <= MAX_GREYSCALE_CHANNEL_DIFF)
	{
		// Ensure that all color channels compress to the same value
		byte channelAvg = (in[0] + in[1] + in[2] + 1) / 3;
		out[0] = channelAvg & 0xF0;
		out[0] |= (channelAvg & 0xF0) >> 4;
		out[1] = channelAvg & 0xF0;
		out[1] |= (in[3] & 0xF0) >> 4;

		if (channelAvg % 16 >= 8)
		{
			out[0] |= 0x10;
			out[0] |= 0x01;
			out[1] |= 0x10;
		}
		if (in[4] % 16 >= 8)
		{
			out[1] |= 0x01;
		}
		return;
	}

	// Normal case for vertex colors that are not "near" greyscale
	out[0] = in[0] & 0xF0;
	out[0] |= (in[1] & 0xF0) >> 4;
	out[1] = in[2] & 0xF0;
	out[1] |= (in[3] & 0xF0) >> 4;
	
	if(in[0] % 16 >= 8) {
		out[0] |= 0x10;
	}
	if(in[1] % 16 >= 8) {
		out[0] |= 0x1;
	}
	if(in[2] % 16 >= 8) {
		out[1] |= 0x10;
	}
	if(in[3] % 16 >= 8) {
		out[1] |= 0x1;
	}
}


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
	int		shift=0, r, g, b;

	// should NOT do it if overbrightBits is 0
	if (tr.overbrightBits)
		shift = 1 - tr.overbrightBits;

	if (!shift)
	{
		out[0] = in[0];
		out[1] = in[1];
		out[2] = in[2];
		out[3] = in[3];
		return;
	}

	// shift the data based on overbright range
	r = in[0] << shift;
	g = in[1] << shift;
	b = in[2] << shift;
	
	// normalize by color instead of saturating to white
	if ( ( r | g | b ) > 255 ) {
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
static	void R_ColorShiftLightingBytes( byte in[3]) 
{
	int		shift=0, r, g, b;

	// should NOT do it if overbrightBits is 0
	if (tr.overbrightBits)
		shift = 1 - tr.overbrightBits;

	if (!shift) {
		return;	//no need if not overbright
	}
	// shift the data based on overbright range
	r = in[0] << shift;
	g = in[1] << shift;
	b = in[2] << shift;
	
	// normalize by color instead of saturating to white
	if ( ( r | g | b ) > 255 ) {
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
void R_LoadLightmaps( void *data, int len, const char *psMapName ) {
	byte		*buf, *buf_p;
	int			i;

	if ( !len ) {
		return;
	}
	buf = (byte *)data + sizeof(int);

	// we are about to upload textures
	R_SyncRenderThread();

	// create all the lightmaps
	int size = *(int*)data;
	tr.numLightmaps = len / size;

	byte* image = (byte*)Z_Malloc(size, TAG_TEMP_WORKSPACE, qfalse, 32);

	char sMapName[MAX_QPATH];
	COM_StripExtension(psMapName,sMapName);	// will already by MAX_QPATH legal, so no length check

	for ( i = 0 ; i < tr.numLightmaps ; i++ ) {
		buf_p = buf + i * size;
		memcpy(image, buf_p, size);

		char lmapName[MAX_QPATH + 32];
		Com_sprintf(lmapName, MAX_QPATH + 32, "*%s/lightmap%d",sMapName,i);
		tr.lightmaps[i] = R_CreateImage( lmapName, image, 
			LIGHTMAP_SIZE, LIGHTMAP_SIZE,
			GL_DDS_RGB16_EXT,
			qfalse, 0, GL_CLAMP);
	}

	Z_Free(image);
}


/*
=================
RE_SetWorldVisData

This is called by the clipmodel subsystem so we can share the 1.8 megs of
space in big maps...
=================
*/
void RE_SetWorldVisData( SPARC<byte> *vis ) {
	tr.externalVisData = vis;
}


/*
=================
R_LoadVisibility
=================
*/
static	void R_LoadVisibility( void ) {
	int		len;

	len = ( s_worldData.numClusters + 63 ) & ~63;
	s_worldData.novis = ( unsigned char *) Hunk_Alloc( len, qfalse );
	memset( s_worldData.novis, 0xff, len );

	s_worldData.numClusters = cmg.numClusters;
	s_worldData.clusterBytes = cmg.clusterBytes;

	// CM_Load should have given us the vis data to share, so
	// we don't need to allocate another copy
	//if ( tr.externalVisData ) {
		s_worldData.vis = tr.externalVisData;
	/*} else {
		assert(0);
	}*/
}

//===============================================================================

qhandle_t R_GetShaderByNum(int shaderNum, world_t &worldData)
{
	qhandle_t	shader;

	if ( (shaderNum < 0) || (shaderNum >= worldData.numShaders) ) 
	{
		Com_Printf( "Warning: Bad index for R_GetShaderByNum - %i", shaderNum );
		return(0);
	}
	shader = RE_RegisterShader(worldData.shaders[ shaderNum ].shader);
	return(shader);
}

/*
===============
ShaderForShaderNum
===============
*/
static shader_t *ShaderForShaderNum( int shaderNum, const short *lightmapNum, const byte *lightmapStyles ) {
	shader_t	*shader;
	dshader_t	*dsh;

	shaderNum = shaderNum;
	if ( shaderNum < 0 || shaderNum >= s_worldData.numShaders ) {
		Com_Error( ERR_DROP, "ShaderForShaderNum: bad num %i", shaderNum );
	}
	dsh = &s_worldData.shaders[ shaderNum ];

	shader = R_FindShader( dsh->shader, lightmapNum, lightmapStyles, qtrue );

	// if the shader had errors, just use default shader
	if ( shader->defaultShader ) {
		return tr.defaultShader;
	}

	return shader;
}

bool NeedVertexColors(shader_t *shader)
{
	int i;
	shaderStage_t *stage;

	for(i=0; i<shader->numUnfoggedPasses; i++) {
		stage = &shader->stages[i];
		switch(stage->rgbGen) {
		case CGEN_EXACT_VERTEX:
		case CGEN_VERTEX:
		case CGEN_ONE_MINUS_VERTEX:
			return true;
		}
		switch(stage->alphaGen) {
		case AGEN_VERTEX:
		case AGEN_ONE_MINUS_VERTEX:
			return true;
		}
	}

	return false;
}

int NumLightMaps(shader_t *shader)
{
	int count = 0;
	int i;

	for(i=0; i<MAXLIGHTMAPS; i++) {
		if(shader->lightmapIndex[i] >= 0) {
			count++;
		} else {
			return count;
		}
	}

	return count;
}

int SurfaceFaceSize(int numVerts, int numLightMaps, bool needVertexColors,
		int numIndexes)
{
	int sfaceSize = ( int ) &((srfSurfaceFace_t *)0)->srfPoints + 
		4 /*sizeof srfPoints*/ + 
		(numVerts * sizeof(unsigned short) *
			(VERTEX_LM + numLightMaps * 2 + 
#ifdef COMPRESS_VERTEX_COLORS
			(int)needVertexColors * 4));	
#else
			(int)needVertexColors * 8));	
#endif

	// Add in tangent size - no, tangent size is included in VERTEX_LM!

	//Indices stored in 8 bits now.
	sfaceSize += numIndexes;

	return sfaceSize;
}


void BuildDrawVertTangents( drawVert_t *verts, int *indexes, int numIndexes, int numVertexes ) 
{
	int i = 0;

	for(i = 0; i < numVertexes; i++)
	{
		verts[i].tangent[0] = 0.0f;
		verts[i].tangent[1] = 0.0f;
		verts[i].tangent[2] = 0.0f;
	}

	for(i = 0; i < numIndexes; i += 3)
	{
		vec3_t vec1, vec2, du, dv, cp;
		float st0[2], st1[2], st2[2];

		Q_CastShort2FloatScale(&st0[0], &verts[indexes[i]].dvst[0], 1.f / DRAWVERT_ST_SCALE);
		Q_CastShort2FloatScale(&st0[1], &verts[indexes[i]].dvst[1], 1.f / DRAWVERT_ST_SCALE);

		Q_CastShort2FloatScale(&st1[0], &verts[indexes[i+1]].dvst[0], 1.f / DRAWVERT_ST_SCALE);
		Q_CastShort2FloatScale(&st1[1], &verts[indexes[i+1]].dvst[1], 1.f / DRAWVERT_ST_SCALE);

		Q_CastShort2FloatScale(&st2[0], &verts[indexes[i+2]].dvst[0], 1.f / DRAWVERT_ST_SCALE);
		Q_CastShort2FloatScale(&st2[1], &verts[indexes[i+2]].dvst[1], 1.f / DRAWVERT_ST_SCALE);

		vec1[0] = verts[indexes[i+1]].xyz[0] - verts[indexes[i]].xyz[0];
		vec1[1] = st1[0] - st0[0];
		vec1[2] = st1[1] - st0[1];

		vec2[0] = verts[indexes[i+2]].xyz[0] - verts[indexes[i]].xyz[0];
		vec2[1] = st2[0] - st0[0];
		vec2[2] = st2[1] - st0[1];

		CrossProduct(vec1, vec2, cp);

		if(cp[0] == 0.0f)
			cp[0] = 0.001f;

		du[0] = -cp[1] / cp[0];
		dv[0] = -cp[2] / cp[0];

		vec1[0] = verts[indexes[i+1]].xyz[1] - verts[indexes[i]].xyz[1];

		vec2[0] = verts[indexes[i+2]].xyz[1] - verts[indexes[i]].xyz[1];
	
		CrossProduct(vec1, vec2, cp);

		if(cp[0] == 0.0f)
			cp[0] = 0.001f;

		du[1] = -cp[1] / cp[0];
		dv[1] = -cp[2] / cp[0];

		vec1[0] = verts[indexes[i+1]].xyz[2] - verts[indexes[i]].xyz[2];

		vec2[0] = verts[indexes[i+2]].xyz[2] - verts[indexes[i]].xyz[2];

		CrossProduct(vec1, vec2, cp);

		if(cp[0] == 0.0f)
			cp[0] = 0.001f;

		du[2] = -cp[1] / cp[0];
		dv[2] = -cp[2] / cp[0];

		verts[indexes[i]].tangent[0] += du[0];
		verts[indexes[i]].tangent[1] += du[1];
		verts[indexes[i]].tangent[2] += du[2];

		verts[indexes[i+1]].tangent[0] += du[0];
		verts[indexes[i+1]].tangent[1] += du[1];
		verts[indexes[i+1]].tangent[2] += du[2];

		verts[indexes[i+2]].tangent[0] += du[0];
		verts[indexes[i+2]].tangent[1] += du[1];
		verts[indexes[i+2]].tangent[2] += du[2];
	}

	for(i = 0; i < numVertexes; i++)
	{
		VectorNormalizeFast(verts[i].tangent);
	}
}


void BuildMapVertTangents( mapVert_t *verts, vec3_t *tangents, short *indexes, int numIndexes, int numVertexes ) 
{
	int i = 0;

	for(i = 0; i < numVertexes; i++)
	{
		tangents[i][0] = 0.0f;
		tangents[i][1] = 0.0f;
		tangents[i][2] = 0.0f;
	}

	for(i = 0; i < numIndexes; i += 3)
	{
		vec3_t vec1, vec2, du, dv, cp;
		
		vec1[0] = verts[indexes[i+1]].xyz[0] - verts[indexes[i]].xyz[0];
		vec1[1] = (verts[indexes[i+1]].st[0] * POINTS_ST_SCALE) - 
				   (verts[indexes[i]].st[0] * POINTS_ST_SCALE);
		vec1[2] = (verts[indexes[i+1]].st[1] * POINTS_ST_SCALE) - 
				   (verts[indexes[i]].st[1] * POINTS_ST_SCALE);

		vec2[0] = verts[indexes[i+2]].xyz[0] - verts[indexes[i]].xyz[0];
		vec2[1] = (verts[indexes[i+2]].st[0] * POINTS_ST_SCALE) - 
				   (verts[indexes[i]].st[0] * POINTS_ST_SCALE);
		vec2[2] = (verts[indexes[i+2]].st[1]* POINTS_ST_SCALE) - 
				   (verts[indexes[i]].st[1] * POINTS_ST_SCALE);

		CrossProduct(vec1, vec2, cp);

		if(cp[0] == 0.0f)
			cp[0] = 0.001f;

		du[0] = -cp[1] / cp[0];
		dv[0] = -cp[2] / cp[0];

		vec1[0] = verts[indexes[i+1]].xyz[1] - verts[indexes[i]].xyz[1];

		vec2[0] = verts[indexes[i+2]].xyz[1] - verts[indexes[i]].xyz[1];

		CrossProduct(vec1, vec2, cp);

		if(cp[0] == 0.0f)
			cp[0] = 0.001f;

		du[1] = -cp[1] / cp[0];
		dv[1] = -cp[2] / cp[0];

		vec1[0] = verts[indexes[i+1]].xyz[2] - verts[indexes[i]].xyz[2];

		vec2[0] = verts[indexes[i+2]].xyz[2] - verts[indexes[i]].xyz[2];

		CrossProduct(vec1, vec2, cp);

		if(cp[0] == 0.0f)
			cp[0] = 0.001f;

		du[2] = -cp[1] / cp[0];
		dv[2] = -cp[2] / cp[0];

		tangents[indexes[i]][0] += du[0];
		tangents[indexes[i]][1] += du[1];
		tangents[indexes[i]][2] += du[2];

		tangents[indexes[i+1]][0] += du[0];
		tangents[indexes[i+1]][1] += du[1];
		tangents[indexes[i+1]][2] += du[2];

		tangents[indexes[i+2]][0] += du[0];
		tangents[indexes[i+2]][1] += du[1];
		tangents[indexes[i+2]][2] += du[2];
	}

	for(i = 0; i < numVertexes; i++)
	{
		VectorNormalizeFast(tangents[i]);
	}
}

/*
===============
ParseFace
===============
*/
static void ParseFace( dface_t *ds, mapVert_t *verts, msurface_t *surf, short *indexes, byte *&pFaceDataBuffer) 
{
	int			i, j, k;
	srfSurfaceFace_t	*cv;
	int			numPoints, numIndexes;
	short		lightmapNum[MAXLIGHTMAPS];
	int			sfaceSize, ofsIndexes;
	vec3_t		tangents[1000];

	for(i=0;i<MAXLIGHTMAPS;i++)
	{
		lightmapNum[i] = (int)ds->lightmapNum[i] - 4;
	}

	// get fog volume
	surf->fogIndex = ds->fogNum + 1;

	// get shader value
	surf->shader = ShaderForShaderNum( ds->shaderNum, lightmapNum, ds->lightmapStyles );
	if ( r_singleShader->integer && !surf->shader->sky ) {
		surf->shader = tr.defaultShader;
	}

	bool needVertexColors = NeedVertexColors(surf->shader); 
	int numLightMaps = NumLightMaps(surf->shader);
	assert(numLightMaps <= 0x7F);

	numPoints = ds->verts & 0xFFF;
	if (numPoints > MAX_FACE_POINTS) {
		VID_Printf( PRINT_DEVELOPER, "MAX_FACE_POINTS exceeded: %i\n", numPoints);
	}

	numIndexes = ds->indexes & 0xFFF;

	// create the srfSurfaceFace_t
	sfaceSize = SurfaceFaceSize(numPoints,
			numLightMaps, needVertexColors, numIndexes);
	ofsIndexes = sfaceSize - numIndexes;

	cv = (srfSurfaceFace_t *) pFaceDataBuffer;//Hunk_Alloc( sfaceSize );
	pFaceDataBuffer += sfaceSize;	// :-)

	cv->surfaceType = SF_FACE;
	cv->numPoints = numPoints;
	cv->numIndices = numIndexes;
	cv->ofsIndices = ofsIndexes;
	cv->srfPoints = (unsigned short *)(((byte*)cv) + ( int ) &((srfSurfaceFace_t *)0)->srfPoints + 4);
	if(needVertexColors) {
		cv->flags = 1 << 7;
	} else {
		cv->flags = 0;
	}
	cv->flags |= (numLightMaps & 0x7F);

	//Make sure we don't overflow storage.
	assert(numPoints < 256);
	assert(numIndexes < 65536);
	assert(ofsIndexes < 65536);

	int nextSurfPoint = NEXT_SURFPOINT(cv->flags);
	verts += ds->verts >> 12;

	indexes += ds->indexes >> 12;

	BuildMapVertTangents(verts, tangents, indexes, numIndexes, numPoints);

	for ( i = 0 ; i < numPoints ; i++ ) {
		for ( j = 0 ; j < 3 ; j++ ) {
			*(cv->srfPoints + i * nextSurfPoint + j) = verts[i].xyz[j];
		}
		
		for ( j = 0; j < 3 ; j++ ) {
			assert(tangents[i][j] >= -1 && tangents[i][j] <= 1);
			*(cv->srfPoints + i * nextSurfPoint + 3 + j) = (short)(tangents[i][j] * 32767.0f);
		}
		for ( j = 0 ; j < 2 ; j++ ) {
			*(cv->srfPoints + i * nextSurfPoint + 6 + j) = 
				(short)(verts[i].st[j] * POINTS_ST_SCALE);

			for(k=0;k<numLightMaps;k++)
			{
				*(cv->srfPoints + i * nextSurfPoint + VERTEX_LM+j+(k*2)) = 
					verts[i].lightmap[k][j];
			}
		}
		if(needVertexColors) {
			for(k=0;k<MAXLIGHTMAPS;k++)
			{
#ifdef COMPRESS_VERTEX_COLORS
				R_ColorShiftLightingBytes16(
					verts[i].color[k],
					(byte*)(cv->srfPoints + i * nextSurfPoint + 
					VERTEX_COLOR(cv->flags) + k));
#else
				R_ColorShiftLightingBytes(
					verts[i].color[k],
					(byte*)(cv->srfPoints + i * nextSurfPoint + 
					VERTEX_COLOR(cv->flags) + 2*k));
#endif
			}
		}
	}

//	indexes += ds->indexes >> 12;
	unsigned char *indexStorage = ((unsigned char*)cv) + cv->ofsIndices;
	for ( i = 0 ; i < numIndexes ; i++ ) {
		indexStorage[i] = indexes[ i ];
	}

	// take the plane information from the lightmap vector
	for ( i = 0 ; i < 3 ; i++ ) {
		cv->plane.normal[i] = (float)ds->lightmapVecs[i] / 32767.f;
	}
	vec3_t fVec;
	fVec[0] = (float)((short)cv->srfPoints[0]);
	fVec[1] = (float)((short)cv->srfPoints[1]);
	fVec[2] = (float)((short)cv->srfPoints[2]);
	cv->plane.dist = DotProduct( fVec, cv->plane.normal );
	SetPlaneSignbits( &cv->plane );
	cv->plane.type = PlaneTypeForNormal( cv->plane.normal );

	surf->data = (surfaceType_t *)cv;
}


/*
===============
ParseMesh
===============
*/
static void ParseMesh ( dpatch_t *ds, mapVert_t *verts, msurface_t *surf,
					   drawVert_t* points, drawVert_t* ctrl, float* errorTable ) {
	srfGridMesh_t	*grid;
	int				i, j, k;
	int				width, height, numPoints;
	short			lightmapNum[MAXLIGHTMAPS];
	vec3_t			bounds[2];
	vec3_t			tmpVec;
	static surfaceType_t	skipData = SF_SKIP;

	for(i=0;i<MAXLIGHTMAPS;i++)
	{
		lightmapNum[i] = (int)ds->lightmapNum[i] - 4;
	}

	// get fog volume
	surf->fogIndex = ds->fogNum + 1;

	// get shader value
	surf->shader = ShaderForShaderNum( ds->shaderNum, lightmapNum, ds->lightmapStyles );
	if ( r_singleShader->integer && !surf->shader->sky ) {
		surf->shader = tr.defaultShader;
	}

	// we may have a nodraw surface, because they might still need to
	// be around for movement clipping
	if ( s_worldData.shaders[ ds->shaderNum ].surfaceFlags & SURF_NODRAW ) {
		surf->data = &skipData;
		return;
	}

	width = ds->patchWidth;
	height = ds->patchHeight;

	verts += ds->verts >> 12;
	numPoints = width * height;
	for ( i = 0 ; i < numPoints ; i++ ) {
		for ( j = 0 ; j < 3 ; j++ ) {
			points[i].xyz[j] = (float)verts[i].xyz[j];
			points[i].normal[j] = (float)verts[i].normal[j] / 32767.f;
		}
		for ( j = 0 ; j < 2 ; j++ ) {
			// Sanity check that alternate fixed point representation
			// is good enough
			assert( verts[i].st[j] * GRID_DRAWVERT_ST_SCALE < 32767 &&
					verts[i].st[j] * GRID_DRAWVERT_ST_SCALE >= -32768 );
			points[i].dvst[j] = verts[i].st[j] * GRID_DRAWVERT_ST_SCALE;
			for(k=0;k<MAXLIGHTMAPS;k++)
			{
				points[i].dvlightmap[k][j] = 
					((float)verts[i].lightmap[k][j] / POINTS_LIGHT_SCALE) *
					DRAWVERT_LIGHTMAP_SCALE;
			}
		}
		for(k=0;k<MAXLIGHTMAPS;k++)
		{
#ifdef COMPRESS_VERTEX_COLORS
			R_ColorShiftLightingBytes16(verts[i].color[k], 
				points[i].dvcolor[k]);
#else
			R_ColorShiftLightingBytes(verts[i].color[k],
				points[i].dvcolor[k]);
#endif
		}
	}

	// pre-tesseleate
	grid = R_SubdividePatchToGrid( width, height, points, ctrl, errorTable );
	surf->data = (surfaceType_t *)grid;

	// copy the level of detail origin, which is the center
	// of the group of all curves that must subdivide the same
	// to avoid cracking
	for ( i = 0 ; i < 3 ; i++ ) {
		bounds[0][i] = ds->lightmapVecs[0][i];
		bounds[1][i] = ds->lightmapVecs[1][i];
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
static void ParseTriSurf( dtrisurf_t *ds, mapVert_t *verts, msurface_t *surf, short *indexes ) {
	srfTriangles_t	*tri;
	int				i, j, k;
	int				numVerts, numIndexes;

	// get fog volume
	surf->fogIndex = ds->fogNum + 1;

	// get shader
	surf->shader = ShaderForShaderNum( ds->shaderNum, lightmapsVertex, ds->lightmapStyles );
	if ( r_singleShader->integer && !surf->shader->sky ) {
		surf->shader = tr.defaultShader;
	}

	numVerts = ds->verts & 0xFFF;
	numIndexes = ds->indexes & 0xFFF;

	tri = (srfTriangles_t *) Hunk_Alloc( sizeof( *tri ) + numVerts * sizeof( tri->verts[0] ) 
		+ numIndexes * sizeof( tri->indexes[0] ), qtrue );
	tri->surfaceType = SF_TRIANGLES;
	tri->numVerts = numVerts;
	tri->numIndexes = numIndexes;
	tri->verts = (drawVert_t *)(tri + 1);
	tri->indexes = (int *)(tri->verts + tri->numVerts );

	surf->data = (surfaceType_t *)tri;

	// copy vertexes
	verts += ds->verts >> 12;
	ClearBounds( tri->bounds[0], tri->bounds[1] );
	for ( i = 0 ; i < numVerts ; i++ ) {
		for ( j = 0 ; j < 3 ; j++ ) {
			tri->verts[i].xyz[j] = verts[i].xyz[j];
			tri->verts[i].normal[j] = verts[i].normal[j];
		}
		AddPointToBounds( tri->verts[i].xyz, tri->bounds[0], tri->bounds[1] );
		for ( j = 0 ; j < 2 ; j++ ) {
			// Sanity check that alternate fixed point representation
			// is good enough
			// MATT! - double check this!
			assert( verts[i].st[j] * DRAWVERT_ST_SCALE <= 32767 &&
					verts[i].st[j] * DRAWVERT_ST_SCALE >= -32768 );
			tri->verts[i].dvst[j] = verts[i].st[j] * DRAWVERT_ST_SCALE;
			for(k=0;k<MAXLIGHTMAPS;k++)
			{
				tri->verts[i].dvlightmap[k][j] = 
					((float)verts[i].lightmap[k][j] / POINTS_LIGHT_SCALE) *
					DRAWVERT_LIGHTMAP_SCALE;
			}
		}
		for(k=0;k<MAXLIGHTMAPS;k++)
		{
#ifdef COMPRESS_VERTEX_COLORS
			R_ColorShiftLightingBytes16(verts[i].color[k], 
				tri->verts[i].dvcolor[k]);
#else
			R_ColorShiftLightingBytes(verts[i].color[k],
				tri->verts[i].dvcolor[k]);
#endif
		}
	}

	// copy indexes
	indexes += ds->indexes >> 12;
	for ( i = 0 ; i < numIndexes ; i++ ) {
		tri->indexes[i] = indexes[i];
		if ( tri->indexes[i] < 0 || tri->indexes[i] >= numVerts ) {
			Com_Error( ERR_DROP, "Bad index in triangle surface" );
		}
	}

	// Build the tangent vectors
	BuildDrawVertTangents(tri->verts, tri->indexes, numIndexes, numVerts);
}


/*
===============
ParseFlare
===============
*/
static void ParseFlare( dflare_t *df, msurface_t *surf )
{
	srfFlare_t		*flare;
	int i;

	surf->fogIndex = df->fogNum + 1;

	// get shader
	surf->shader = ShaderForShaderNum( df->shaderNum, lightmapsVertex, stylesDefault );

	flare = (srfFlare_t *) Hunk_Alloc( sizeof( *flare ), qtrue );
	flare->surfaceType = SF_FLARE;

	for ( i = 0 ; i < 3 ; i++ ) {
		flare->origin[i] = df->origin[i];
		flare->color[i] = df->color[i];
		flare->normal[i] = df->normal[i];
	}

	assert(flareNum <= 255);
	flare->number = flareNum++;
	flare->visible = -1;

	surf->data = (surfaceType_t *)flare;
}


void R_LoadFlares( void *surfaces, int surfacelen ) {
	int count, i;
	dflare_t	*in = NULL;
	msurface_t  *out;

	count = surfacelen / sizeof(*in);

	flareNum = 0;

	for ( i = 0 ; i < count ; i++ ) {
		in = (dflare_t *)surfaces + i;
		out = s_worldData.surfaces + in->code;
		ParseFlare( in, out );
	}
}


/*
===============
R_LoadSurfaces
===============
*/
void R_LoadSurfaces( int count ) {
	s_worldData.surfaces = (struct msurface_s *) 
		Hunk_Alloc ( count * sizeof(msurface_s), qtrue );
	s_worldData.numsurfaces = count;
}


/*
===============
R_LoadPatches
===============
*/
void R_LoadPatches( void *verts, int vertlen, 
					void *surfaces, int surfacelen ) {
	dpatch_t	*in = NULL;
	msurface_t	*out;
	mapVert_t	*dv;
	int			count;
	int			i;

	if (surfacelen == 0) {
		return;
	}

	count = surfacelen / sizeof(*in);

	dv = (mapVert_t *)(verts);
	if (vertlen % sizeof(*dv))
		Com_Error (ERR_DROP, "LoadMap: funny lump size in %s",s_worldData.name);

	drawVert_t* points = (drawVert_t*)Z_Malloc(
		MAX_PATCH_SIZE*MAX_PATCH_SIZE*sizeof(drawVert_t), 
		TAG_TEMP_WORKSPACE, qfalse);
	
	drawVert_t*	ctrl = (drawVert_t*)Z_Malloc(
		MAX_GRID_SIZE*MAX_GRID_SIZE*sizeof(drawVert_t), 
		TAG_TEMP_WORKSPACE, qfalse);
	
	float* errorTable = (float*)Z_Malloc(
		2*MAX_GRID_SIZE*sizeof(float), 
		TAG_TEMP_WORKSPACE, qfalse);

	for ( i = 0 ; i < count ; i++ ) {
		in = (dpatch_t *)surfaces + i;
		out = s_worldData.surfaces + in->code;
		ParseMesh ( in, dv, out, points, ctrl, errorTable );
	}

	Z_Free(errorTable);
	Z_Free(ctrl);
	Z_Free(points);

	VID_Printf( PRINT_ALL, "...loaded %i meshes\n", count );
}


					/*
===============
R_LoadTriSurfs
===============
*/
void R_LoadTriSurfs( void *indexdata, int indexlen, 
					void *verts, int vertlen, 
					void *surfaces, int surfacelen ) {
	dtrisurf_t	*in = NULL;
	msurface_t	*out;
	mapVert_t	*dv;
	short		*indexes;
	int			count;
	int			i;

	if (surfacelen == 0) {
		return;
	}
	
	count = surfacelen / sizeof(*in);

	dv = (mapVert_t *)(verts);
	if (vertlen % sizeof(*dv))
		Com_Error (ERR_DROP, "LoadMap: funny lump size in %s",s_worldData.name);

	indexes = (short *)(indexdata);
	if ( indexlen % sizeof(*indexes))
		Com_Error (ERR_DROP, "LoadMap: funny lump size in %s",s_worldData.name);

	for ( i = 0 ; i < count ; i++ ) {
		in = (dtrisurf_t *)surfaces + i;
		out = s_worldData.surfaces + in->code;
		ParseTriSurf( in, dv, out, indexes );
	}

	VID_Printf( PRINT_ALL, "...loaded %i trisurfs\n", count );
}


/*
===============
R_LoadFaces
===============
*/
void R_LoadFaces( void *indexdata, int indexlen, 
					void *verts, int vertlen, 
					void *surfaces, int surfacelen ) {
	dface_t		*in = NULL;
	msurface_t	*out;
	mapVert_t	*dv;
	short		*indexes;
	int			count;
	int			i;

	if (surfacelen == 0) {
		return;
	}
	
	count = surfacelen / sizeof(*in);

	dv = (mapVert_t *)(verts);
	if (vertlen % sizeof(*dv))
		Com_Error (ERR_DROP, "LoadMap: funny lump size in %s",s_worldData.name);

	indexes = (short *)(indexdata);
	if ( indexlen % sizeof(*indexes))
		Com_Error (ERR_DROP, "LoadMap: funny lump size in %s",s_worldData.name);

	// new bit, the face code on our biggest map requires over 15,000 mallocs, which was no problem on the hunk,
	//	bit hits the zone pretty bad (even the tagFree takes about 9 seconds for that many memblocks), 
	//	so special-case pre-alloc enough space for this data (the patches etc can stay as they are)...
	//
	int nTimes = count / 100;
	int nToGo = nTimes;
	int iFaceDataSizeRequired = 0;
	for ( i = 0 ; i < count ; i++) 
	{ 
		in = (dface_t *)surfaces + i;

		short lightmapNum[MAXLIGHTMAPS];
		for(int j=0; j<4; j++) {
			lightmapNum[j] = (int)in->lightmapNum[j] - 4;
		}
		shader_t *shader = ShaderForShaderNum( in->shaderNum, lightmapNum, in->lightmapStyles );
		bool needVertexColors = NeedVertexColors(shader); 
		int numLightMaps = NumLightMaps(shader);
		
		int sfaceSize = SurfaceFaceSize(in->verts & 0xFFF,
			numLightMaps, needVertexColors,
			in->indexes & 0xFFF);
		
		iFaceDataSizeRequired += sfaceSize;
		assert(sfaceSize < 100 * 1024);
		if (--nToGo <= 0)
		{
			nToGo = nTimes;
		}
	}
	in -= count;	// back it up, ready for loop-proper

	// since this ptr is to hunk data, I can pass it in and have it advanced without worrying about losing
	//	the original alloc ptr...
	//
	byte *orgFaceData;
	byte *pFaceDataBuffer	= (byte *)Hunk_Alloc( iFaceDataSizeRequired, qtrue );
	orgFaceData = pFaceDataBuffer;

	// now do regular loop...
	//
	for ( i = 0 ; i < count ; i++ ) {
		in = (dface_t *)surfaces + i;
		out = s_worldData.surfaces + in->code;
		ParseFace( in, dv, out, indexes, pFaceDataBuffer );
		if (--nToGo <= 0)
		{
			nToGo = nTimes;
		}
	}

	VID_Printf( PRINT_ALL, "...loaded %d faces\n", count );
}


/*
=================
R_LoadSubmodels
=================
*/
static	void R_LoadSubmodels( void *data, int len ) {
	dmodel_t	*in;
	bmodel_t	*out;
	int			i, j, count;

	in = (dmodel_t *)(data);
	if (len % sizeof(*in))
		Com_Error (ERR_DROP, "LoadMap: funny lump size in %s",s_worldData.name);
	count = len / sizeof(*in);

	s_worldData.bmodels = out = (bmodel_t *) Hunk_Alloc( count * sizeof(*out), qtrue );

	for ( i=0 ; i<count ; i++, in++, out++ ) {
		model_t *model;

		model = R_AllocModel();

		assert( model != NULL );			// this should never happen

		model->type = MOD_BRUSH;
		model->bmodel = out;
		Com_sprintf( model->name, sizeof( model->name ), "*%d", i );

		for (j=0 ; j<3 ; j++) {
			out->bounds[0][j] = in->mins[j];
			out->bounds[1][j] = in->maxs[j];
		}

		RE_InsertModelIntoHash(model->name, model);

		out->firstSurface = s_worldData.surfaces + in->firstSurface;
		out->numSurfaces = in->numSurfaces;
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
static void R_LoadNodesAndLeafs (void *nodes, int nodelen, void *leafs, int leaflen) {
	int			i, j, p;
	dnode_t		*in;
	dleaf_t		*inLeaf;
	mnode_t 	*outNode;
	mleaf_s 	*outLeaf;
	int			numNodes, numLeafs;

	in = (dnode_t *)(nodes);
	if (nodelen % sizeof(dnode_t) ||
		leaflen % sizeof(dleaf_t) ) {
		Com_Error (ERR_DROP, "LoadMap: funny lump size in %s",s_worldData.name);
	}
	numNodes = nodelen / sizeof(dnode_t);
	numLeafs = leaflen / sizeof(dleaf_t);

	outNode = (struct mnode_s *) Hunk_Alloc ( (numNodes) * sizeof(*outNode), qtrue );	
	outLeaf = (struct mleaf_s *) Hunk_Alloc ( (numLeafs) * sizeof(*outLeaf), qtrue );	

	s_worldData.nodes = outNode;
	s_worldData.leafs = outLeaf;
	s_worldData.numnodes = numNodes;
	s_worldData.numleafs = numLeafs;

	// load nodes
	for ( i=0 ; i<numNodes; i++, in++, outNode++)
	{
		for (j=0 ; j<3 ; j++)
		{
			outNode->mins[j] = in->mins[j];
			outNode->maxs[j] = in->maxs[j];
		}
	
		outNode->planeNum = in->planeNum;
		outNode->contents = CONTENTS_NODE;	// differentiate from leafs

		for (j=0 ; j<2 ; j++)
		{
			p = in->children[j];
			if (p >= 0) {
				if(p < numNodes) {
					outNode->children[j] = s_worldData.nodes + p;
				} else {
					outNode->children[j] = (mnode_s*)
						(s_worldData.leafs + (p - numNodes));
				}
			} else {
				if(numNodes + (-1 - p) < numNodes) {
					outNode->children[j] = s_worldData.nodes + numNodes + (-1 - p);
				} else {
					outNode->children[j] = (mnode_s*)
						(s_worldData.leafs + (-1 - p));
				}
			}
		}
	}
	
	// load leafs
	inLeaf = (dleaf_t *)(leafs);
	for ( i=0 ; i<numLeafs ; i++, inLeaf++, outLeaf++)
	{
		for (j=0 ; j<3 ; j++)
		{
			outLeaf->mins[j] = inLeaf->mins[j];
			outLeaf->maxs[j] = inLeaf->maxs[j];
		}

		outLeaf->cluster = inLeaf->cluster;
		outLeaf->area = inLeaf->area;

		if ( outLeaf->cluster >= s_worldData.numClusters ) {
			s_worldData.numClusters = outLeaf->cluster + 1;
		}

		outLeaf->firstMarkSurfNum = inLeaf->firstLeafSurface;
		outLeaf->nummarksurfaces = inLeaf->numLeafSurfaces;
	}	

	// chain decendants
	R_SetParent (s_worldData.nodes, NULL);
}

//=============================================================================

/*
=================
R_LoadShaders
=================
*/
void R_LoadShaders( void ) {	
	/*s_worldData.shaders = cm.shaders;
	s_worldData.numShaders = cm.numShaders;*/
}

/*
=================
R_LoadMarksurfaces
=================
*/
static	void R_LoadMarksurfaces (void *data, int len)
{	
	int		i, count;
	int		*in;
	msurface_t **out;
	
	in = (int *)(data);
	if (len % sizeof(*in))
		Com_Error (ERR_DROP, "LoadMap: funny lump size in %s",s_worldData.name);
	count = len / sizeof(*in);
	out = (struct msurface_s **) Hunk_Alloc ( count*sizeof(*out), qtrue );	

	s_worldData.marksurfaces = out;
	s_worldData.nummarksurfaces = count;

	for ( i=0 ; i<count ; i++)
	{
		if(in[i] > s_worldData.numsurfaces)
			assert(0);

		out[i] = s_worldData.surfaces + in[i];

		if (out[i]->shader && out[i]->shader->sort == SS_PORTAL)
		{
			s_worldData.portalPresent = qtrue;
		}
	}
}

/*
=================
R_LoadPlanes
=================
*/
static	void R_LoadPlanes( void ) {
	//New method - share with server.
	s_worldData.planes = cmg.planes;
	s_worldData.numplanes = cmg.numPlanes;
}

/*
=================
R_LoadFogs

=================
*/
static void R_LoadFogs( void *fogdata, int foglen,
					   void *brushdata, int brushlen,
					   void *sidedata, int sidelen ) {
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
	short		lightmaps[MAXLIGHTMAPS] = { LIGHTMAP_NONE } ;

	fogs = (dfog_t *)(fogdata);
	if (foglen % sizeof(*fogs)) {
		Com_Error (ERR_DROP, "LoadMap: funny lump size in %s",s_worldData.name);
	}
	count = foglen / sizeof(*fogs);

	// create fog structres for them
	// NOTE: we allocate memory for an extra one so that the LA goggles can turn on their own fog
	s_worldData.numfogs = count + 1;
	s_worldData.fogs = (fog_t *)Hunk_Alloc (( s_worldData.numfogs + 1)*sizeof(*out), qtrue );
	s_worldData.globalFog = -1;
	out = s_worldData.fogs + 1;

	if ( !count ) {
		return;
	}

	brushes = (dbrush_t *)(brushdata);
	if (brushlen % sizeof(*brushes)) {
		Com_Error (ERR_DROP, "LoadMap: funny lump size in %s",s_worldData.name);
	}
	brushesCount = brushlen / sizeof(*brushes);

	sides = (dbrushside_t *)(sidedata);
	if (sidelen % sizeof(*sides)) {
		Com_Error (ERR_DROP, "LoadMap: funny lump size in %s",s_worldData.name);
	}
	sidesCount = sidelen / sizeof(*sides);

	for ( i=0 ; i<count ; i++, fogs++) {
		out->originalBrushNumber = fogs->brushNum;
		if (out->originalBrushNumber == -1)
		{
			out->bounds[0][0] = out->bounds[0][1] = out->bounds[0][2] = MIN_WORLD_COORD;
			out->bounds[1][0] = out->bounds[1][1] = out->bounds[1][2] = MAX_WORLD_COORD;
			s_worldData.globalFog = i+1;
		}
		else
		{
			if ( (unsigned)out->originalBrushNumber >= brushesCount ) {
				Com_Error( ERR_DROP, "fog brushNumber out of range" );
			}
			brush = brushes + out->originalBrushNumber;
			
			firstSide = brush->firstSide;
			
			if ( (unsigned)firstSide > sidesCount - 6 ) {
				Com_Error( ERR_DROP, "fog brush sideNumber out of range" );
			}
			
			// brushes are always sorted with the axial sides first
			sideNum = firstSide + 0;
			planeNum = sides[ sideNum ].planeNum;
			out->bounds[0][0] = -s_worldData.planes[ planeNum ].dist;
			
			sideNum = firstSide + 1;
			planeNum = sides[ sideNum ].planeNum;
			out->bounds[1][0] = s_worldData.planes[ planeNum ].dist;
			
			sideNum = firstSide + 2;
			planeNum = sides[ sideNum ].planeNum;
			out->bounds[0][1] = -s_worldData.planes[ planeNum ].dist;
			
			sideNum = firstSide + 3;
			planeNum = sides[ sideNum ].planeNum;
			out->bounds[1][1] = s_worldData.planes[ planeNum ].dist;
			
			sideNum = firstSide + 4;
			planeNum = sides[ sideNum ].planeNum;
			out->bounds[0][2] = -s_worldData.planes[ planeNum ].dist;
			
			sideNum = firstSide + 5;
			planeNum = sides[ sideNum ].planeNum;
			out->bounds[1][2] = s_worldData.planes[ planeNum ].dist;
		}
		
		// get information from the shader for fog parameters
		shader = R_FindShader( fogs->shader, lightmaps, stylesDefault, qtrue );
		
		out->parms = *shader->fogParms;
		out->colorInt = ColorBytes4 ( shader->fogParms->color[0] * tr.identityLight, 
			shader->fogParms->color[1] * tr.identityLight, 
			shader->fogParms->color[2] * tr.identityLight, 1.0 );
		
		d = shader->fogParms->depthForOpaque < 1 ? 1 : shader->fogParms->depthForOpaque;
		out->tcScale = 1.0 / ( d * 8 );
		
		// set the gradient vector
		sideNum = fogs->visibleSide;
		
		if ( sideNum == -1 ) {
			out->hasSurface = qfalse;
		} else {
			out->hasSurface = qtrue;
			planeNum = sides[ firstSide + sideNum ].planeNum;
			VectorSubtract( vec3_origin, s_worldData.planes[ planeNum ].normal, out->surface );
			out->surface[3] = -s_worldData.planes[ planeNum ].dist;
		}
		
		out++;
	}

	// Initialise the last fog so we can use it with the LA Goggles
	// NOTE: We are might appear to be off the end of the array, but we allocated an extra memory slot above but [purposely] didn't 
	//	increment the total world numFogs to match our array size
	VectorSet(out->bounds[0], MIN_WORLD_COORD, MIN_WORLD_COORD, MIN_WORLD_COORD);
	VectorSet(out->bounds[1], MAX_WORLD_COORD, MAX_WORLD_COORD, MAX_WORLD_COORD);
	out->originalBrushNumber = -1;
	out->parms.color[0] = 0.0f;
	out->parms.color[1] = 0.0f;
	out->parms.color[2] = 0.0f;
	out->parms.color[3] = 0.0f;
	out->parms.depthForOpaque = 0.0f;
	out->colorInt = 0x00000000;
	out->tcScale = 0.0f;
	out->hasSurface = false;
}

/*
================
R_LoadLightGrid

================
*/
void R_LoadLightGrid( void *data, int len ) {
	vec3_t	maxs;
	world_t	*w;
	int		i;
	float	*wMins, *wMaxs;

	w = &s_worldData;

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

	w->lightGridData = (mgrid_t *)Hunk_Alloc( len, qfalse );
	memcpy( w->lightGridData, data, len );
}

/*
================
R_LoadLightGridArray

================
*/
void R_LoadLightGridArray( void *data, int len ) {
	world_t	*w;

	w = &s_worldData;

	w->numGridArrayElements = w->lightGridBounds[0] * w->lightGridBounds[1] * w->lightGridBounds[2];

	if ( len != w->numGridArrayElements * sizeof(*w->lightGridArray) ) {
		if (len>0)//don't warn if not even lit
			VID_Printf( PRINT_WARNING, "WARNING: light grid array mismatch\n" );
		w->lightGridData = NULL;
		return;
	}

	w->lightGridArray = (unsigned short *)Hunk_Alloc( len, qfalse );
	memcpy( w->lightGridArray, data, len );
}

/*
================
R_LoadEntities
================
*/
void R_LoadEntities( void *data, int len ) {
	const char *p, *token;
	char keyname[MAX_TOKEN_CHARS];
	char value[MAX_TOKEN_CHARS];
	world_t	*w;
	float ambient = 1;

	w = &s_worldData;
	w->lightGridSize[0] = 64;
	w->lightGridSize[1] = 64;
	w->lightGridSize[2] = 128;

	VectorSet(tr.sunAmbient, 1, 1, 1);
	tr.distanceCull = 12000;//DEFAULT_DISTANCE_CULL;

	p = (char *)(data);

	token = COM_ParseExt( &p, qtrue );
	if (!*token || *token != '{') {
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

		if (!Q_stricmp(keyname, "distanceCull")) {
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
}


/*
=================
RE_LoadWorldMap

Called directly from cgame
=================
*/
void RE_LoadWorldMap_Actual( const char *name, world_t &worldData, int index ) {
	char		stripName[MAX_QPATH];
	Lump outputLumps[3];

	// This is no longer correct. The new code supports sub-models, apparently BSPs in
	// several chunks. If any map tries to use them, the following COM_Error will go
	// off. We haven't hit it yet, but if (when) we do, check out tr_bsp.cpp for changes.
	if ( tr.worldMapLoaded ) {
		Com_Error( ERR_DROP, "ERROR: attempted to redundantly load world map\n" );
	}

	// set default sun direction to be used if it isn't
	// overridden by a shader
	skyboxportal = 0;

	tr.sunDirection[0] = 0.45f;
	tr.sunDirection[1] = 0.3f;
	tr.sunDirection[2] = 0.9f;

	VectorNormalize( tr.sunDirection );

	Cvar_SetValue( "r_sundir_x", tr.sunDirection[0] );
	Cvar_SetValue( "r_sundir_y", tr.sunDirection[1] );
	Cvar_SetValue( "r_sundir_z", tr.sunDirection[2] );

	tr.worldMapLoaded = qtrue;

	// clear tr.world so if the level fails to load, the next
	// try will not look at the partially loaded version
	tr.world = NULL;

	//Preserve data which was already set in cm_load
	msurface_t *surfacePtr = s_worldData.surfaces;
	int numSurfaces = s_worldData.numsurfaces;
	memset( &s_worldData, 0, sizeof( s_worldData ) );
	s_worldData.surfaces = surfacePtr;
	s_worldData.numsurfaces = numSurfaces;
	//s_worldData.shaders = cm.shaders;
	s_worldData.numShaders = cmg.numShaders;

	Q_strncpyz( s_worldData.name, name, sizeof( s_worldData.name ) );

	Q_strncpyz( s_worldData.baseName, COM_SkipPath( s_worldData.name ), sizeof( s_worldData.name ) );
	COM_StripExtension( s_worldData.baseName, s_worldData.baseName );

	COM_StripExtension(name, stripName);
	
	c_gridVerts = 0;

	// load into heap
	R_LoadPlanes ();

	outputLumps[0].load(stripName, "fogs");
	outputLumps[1].load(stripName, "brushes");
	outputLumps[2].load(stripName, "brushsides");
	R_LoadFogs( outputLumps[0].data, outputLumps[0].len,
		outputLumps[1].data, outputLumps[1].len,
		outputLumps[2].data, outputLumps[2].len );
	outputLumps[2].clear();
	outputLumps[1].clear();

	outputLumps[0].load(stripName, "leafsurfaces");
	R_LoadMarksurfaces (outputLumps[0].data, outputLumps[0].len);

	outputLumps[0].load(stripName, "nodes");
	outputLumps[1].load(stripName, "leafs");
	R_LoadNodesAndLeafs (outputLumps[0].data, outputLumps[0].len,
		outputLumps[1].data, outputLumps[1].len);
	outputLumps[1].clear();
	
	outputLumps[0].load(stripName, "models");
	R_LoadSubmodels (outputLumps[0].data, outputLumps[0].len);

	R_LoadVisibility();

	outputLumps[0].load(stripName, "entities");
	R_LoadEntities( outputLumps[0].data, outputLumps[0].len );
	outputLumps[0].load(stripName, "lightgrid");
	R_LoadLightGrid( outputLumps[0].data, outputLumps[0].len );
	outputLumps[0].load(stripName, "lightarray");
	R_LoadLightGridArray( outputLumps[0].data, outputLumps[0].len );

	// only set tr.world now that we know the entire level has loaded properly
	tr.world = &s_worldData;

	// Load the light parms for this level
	R_LoadLevelLightParms();
	R_GetLightParmsForLevel();
}


// new wrapper used for convenience to tell z_malloc()-fail recovery code whether it's safe to dump the cached-bsp or not.
//
extern qboolean gbUsingCachedMapDataRightNow;
void RE_LoadWorldMap( const char *name )
{
	memset(entityVisList, -1, sizeof(entityVisList));

	gbUsingCachedMapDataRightNow = qtrue;	// !!!!!!!!!!!!

		RE_LoadWorldMap_Actual( name, s_worldData, 0 );

	gbUsingCachedMapDataRightNow = qfalse;	// !!!!!!!!!!!!
}


//A nasty looking function which loops through all images used by all surfaces
//and returns the number of matches for the given image.
#ifndef FINAL_BUILD
int R_SurfaceImageCount(const image_t *image1)
{
	int count = 0;

	for(int i=0; i<s_worldData.numsurfaces; i++) {
		for(int j=0; j<s_worldData.surfaces[i].shader->numUnfoggedPasses; j++){
			for(int k=0; k<NUM_TEXTURE_BUNDLES; k++) {
				image_t *image2 = s_worldData.surfaces[i].shader->stages[j].bundle[k].image;
				if(image2 != NULL && !Q_stricmp(image1->imgName, image2->imgName)) {
					count++;
				}
							
			}
		}
	}

	return count;
}
#endif
