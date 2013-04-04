// cmodel.c -- model loading
#include "../qcommon/exe_headers.h"

#include "cm_local.h"
#include "cm_landscape.h" //rwwRMG - include
#include "cm_patch.h"
//#include "../rmg/rm_headers.h" //rwwRMG - include
#include "../renderer/tr_local.h"

#include "sparc.h"
#include "../zlib/zlib.h"

static SPARC<byte> visData;

void *SparcAllocator(unsigned int size)
{
	return Z_Malloc(size, TAG_BSP, false);
}

void SparcDeallocator(void *ptr)
{
	Z_Free(ptr);
}


void CM_LoadShaderText(bool forceReload);

#ifdef BSPC

#include "../bspc/l_qfiles.h"

void SetPlaneSignbits (cplane_t *out) {
	int	bits, j;

	// for fast box on planeside test
	bits = 0;
	for (j=0 ; j<3 ; j++) {
		if (out->normal[j] < 0) {
			bits |= 1<<j;
		}
	}
	out->signbits = bits;
}
#endif //BSPC

// to allow boxes to be treated as brush models, we allocate
// some extra indexes along with those needed by the map
#define	BOX_BRUSHES		1
#define	BOX_SIDES		6
#define	BOX_LEAFS		2
#define	BOX_PLANES		12

#define	LL(x) x=LittleLong(x)

clipMap_t	cmg;
int			c_pointcontents;
int			c_traces, c_brush_traces, c_patch_traces;


byte		*cmod_base;

#ifndef BSPC
cvar_t		*cm_noAreas;
cvar_t		*cm_noCurves;
cvar_t		*cm_playerCurveClip;
#endif

cmodel_t	box_model;
cplane_t	*box_planes;
cbrush_t	*box_brush;



void	CM_InitBoxHull (void);
void	CM_FloodAreaConnections (void);

/*
clipMap_t	SubBSP[MAX_SUB_BSP];
int			NumSubBSP;
*/
int TotalSubModels;

/*
===============================================================================

					MAP LOADING

===============================================================================
*/

/*
=================
CMod_LoadShaders
=================
*/
void CMod_LoadShaders( void *data, int len ) {
	dshader_t	*in;
	int			i, count;
	CCMShader	*out;

	in = (dshader_t *)(data);
	if (len % sizeof(*in)) {
		Com_Error (ERR_DROP, "CMod_LoadShaders: funny lump size");
	}
	count = len / sizeof(*in);

	if (count < 1) {
		Com_Error (ERR_DROP, "Map with no shaders");
	}
	cmg.shaders = (CCMShader *)Hunk_Alloc( (1+count) * sizeof( *cmg.shaders ), h_high );
	cmg.numShaders = count;

	out = cmg.shaders;
	for ( i = 0; i < count; i++, in++, out++ ) 
	{
		Q_strncpyz(out->shader, in->shader, MAX_QPATH);
		out->contentFlags = in->contentFlags;
		out->surfaceFlags = in->surfaceFlags;
	}
}


/*
=================
CMod_LoadSubmodels
=================
*/
void CMod_LoadSubmodels( void *data, int len ) {
	dmodel_t	*in;
	cmodel_t	*out;
	int			i, j, count;
	int			*indexes;

	in = (dmodel_t *)(data);
	if (len % sizeof(*in))
		Com_Error (ERR_DROP, "CMod_LoadSubmodels: funny lump size");
	count = len / sizeof(*in);

	if (count < 1) {
		Com_Error (ERR_DROP, "Map with no models");
	}

	if ( count > MAX_SUBMODELS ) {
		Com_Error( ERR_DROP, "MAX_SUBMODELS (%d) exceeded by %d", MAX_SUBMODELS, count-MAX_SUBMODELS );
	}

	cmg.cmodels = (struct cmodel_s *)Hunk_Alloc( count * sizeof( *cmg.cmodels ), h_high );
	cmg.numSubModels = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		out = &cmg.cmodels[i];

		for (j=0 ; j<3 ; j++)
		{	// spread the mins / maxs by a pixel
			out->mins[j] = in->mins[j] - 1;
			out->maxs[j] = in->maxs[j] + 1;
		}

		if ( i == 0 ) {
			out->firstNode = 0;
			continue;	// world model doesn't need other info
		}

		out->firstNode = -1;

		// make a "leaf" just to hold the model's brushes and surfaces
		out->leaf.numLeafBrushes = in->numBrushes;
		indexes = (int *)Hunk_Alloc( out->leaf.numLeafBrushes * 4, h_high );
		out->leaf.firstLeafBrush = indexes - cmg.leafbrushes;
		for ( j = 0 ; j < out->leaf.numLeafBrushes ; j++ ) {
			indexes[j] = in->firstBrush + j;
		}

		out->leaf.numLeafSurfaces = in->numSurfaces;
		indexes = (int *)Hunk_Alloc( out->leaf.numLeafSurfaces * 4, h_high );
		out->leaf.firstLeafSurface = indexes - cmg.leafsurfaces;
		for ( j = 0 ; j < out->leaf.numLeafSurfaces ; j++ ) {
			indexes[j] = in->firstSurface + j;
		}
	}
}

/*
=================
CMod_LoadNodes

=================
*/
void CMod_LoadNodes( void *data, int len ) {
	dnode_t		*in;
	cNode_t		*out;
	int			i, count;
	
	in = (dnode_t *)(data);
	if (len % sizeof(*in))
		Com_Error (ERR_DROP, "MOD_LoadBmodel: funny lump size");
	count = len / sizeof(*in);

	if (count < 1)
		Com_Error (ERR_DROP, "Map has no nodes");
	cmg.nodes = (cNode_t *)Hunk_Alloc( count * sizeof( *cmg.nodes ), h_high );
	cmg.numNodes = count;

	out = cmg.nodes;

	for (i=0 ; i<count ; i++, out++, in++)
	{
		out->planeNum = in->planeNum;
		out->children[0] = in->children[0];
		out->children[1] = in->children[1];
	}
}

/*
=================
CM_BoundBrush

=================
*/
void CM_BoundBrush( cbrush_t *b ) {
	b->bounds[0][0] = -cmg.planes[b->sides[0].planeNum.GetValue()].dist;
	b->bounds[1][0] = cmg.planes[b->sides[1].planeNum.GetValue()].dist;

	b->bounds[0][1] = -cmg.planes[b->sides[2].planeNum.GetValue()].dist;
	b->bounds[1][1] = cmg.planes[b->sides[3].planeNum.GetValue()].dist;

	b->bounds[0][2] = -cmg.planes[b->sides[4].planeNum.GetValue()].dist;
	b->bounds[1][2] = cmg.planes[b->sides[5].planeNum.GetValue()].dist;
}


/*
=================
CMod_LoadBrushes

=================
*/
void CMod_LoadBrushes( void *data, int len ) {
	dbrush_t	*in;
	cbrush_t	*out;
	int			i, count;

	in = (dbrush_t *)(data);
	if (len % sizeof(*in)) {
		Com_Error (ERR_DROP, "MOD_LoadBmodel: funny lump size");
	}
	count = len / sizeof(*in);

	cmg.brushes = (cbrush_t *)Hunk_Alloc( ( BOX_BRUSHES + count ) * sizeof( *cmg.brushes ), h_high );
	cmg.numBrushes = count;

	out = cmg.brushes;

	for ( i=0 ; i<count ; i++, out++, in++ ) {
		out->sides = cmg.brushsides + in->firstSide;
		out->numsides = in->numSides;

		out->shaderNum = in->shaderNum;
		if ( out->shaderNum < 0 || out->shaderNum >= cmg.numShaders ) {
			Com_Error( ERR_DROP, "CMod_LoadBrushes: bad shaderNum: %i", out->shaderNum );
		}
		out->contents = cmg.shaders[out->shaderNum].contentFlags;
		
		CM_BoundBrush( out );
	}

}

/*
=================
CMod_LoadLeafs
=================
*/
void CMod_LoadLeafs (void *data, int len)
{
	int			i;
	cLeaf_t		*out;
	dleaf_t 	*in;
	int			count;
	
	in = (dleaf_t *)(data);
	if (len % sizeof(*in))
		Com_Error (ERR_DROP, "MOD_LoadBmodel: funny lump size");
	count = len / sizeof(*in);

	if (count < 1)
		Com_Error (ERR_DROP, "Map with no leafs");

	cmg.leafs = (cLeaf_t *)Hunk_Alloc( ( BOX_LEAFS + count ) * sizeof( *cmg.leafs ), h_high );
	cmg.numLeafs = count;
	out = cmg.leafs;	

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		out->cluster = in->cluster;
		out->area = in->area;
		out->firstLeafBrush = in->firstLeafBrush;
		out->numLeafBrushes = in->numLeafBrushes;
		out->firstLeafSurface = in->firstLeafSurface;
		out->numLeafSurfaces = in->numLeafSurfaces;

		if (out->cluster >= cmg.numClusters)
			cmg.numClusters = out->cluster + 1;
		if (out->area >= cmg.numAreas)
			cmg.numAreas = out->area + 1;
	}

	cmg.areas = (cArea_t *)Hunk_Alloc( cmg.numAreas * sizeof( *cmg.areas ), h_high );
	cmg.areaPortals = (int *)Hunk_Alloc( cmg.numAreas * cmg.numAreas * sizeof( *cmg.areaPortals ), h_high );
}

/*
=================
CMod_LoadPlanes
=================
*/
void CMod_LoadPlanes (void *data, int len)
{
	int			i, j;
	cplane_t	*out;
	dplane_t 	*in;
	int			count;
	int			bits;
	
	in = (dplane_t *)(data);
	if (len % sizeof(*in))
		Com_Error (ERR_DROP, "MOD_LoadBmodel: funny lump size");
	count = len / sizeof(*in);

	if (count < 1)
		Com_Error (ERR_DROP, "Map with no planes");
	cmg.planes = (struct cplane_s *)Hunk_Alloc( ( BOX_PLANES + count ) * sizeof( *cmg.planes ), h_high );
	cmg.numPlanes = count;

	out = cmg.planes;	

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		bits = 0;
		for (j=0 ; j<3 ; j++)
		{
			out->normal[j] = in->normal[j];
			if (out->normal[j] < 0)
				bits |= 1<<j;
		}

		out->dist = in->dist;
		out->type = PlaneTypeForNormal( out->normal );
		out->signbits = bits;
	}

	RE_SetPlaneData(cmg.planes, cmg.numPlanes);
}

/*
=================
CMod_LoadLeafBrushes
=================
*/
void CMod_LoadLeafBrushes (void *data, int len)
{
	int			*out;
	int		 	*in;
	int			count;
	
	in = (int *)(data);
	if (len % sizeof(*in))
		Com_Error (ERR_DROP, "MOD_LoadBmodel: funny lump size");
	count = len / sizeof(*in);

	cmg.leafbrushes = (int *)Hunk_Alloc( (count + BOX_BRUSHES) * sizeof( *cmg.leafbrushes ), h_high );
	cmg.numLeafBrushes = count;

	out = cmg.leafbrushes;

	memcpy(out, in, len);
}


void CMod_LoadLeafSurfaces( void *data, int len )
{
	int			i;
	int			*out;
	int		 	*in;
	int			count;
	
	in = (int *)(data);
	if (len % sizeof(*in))
		Com_Error (ERR_DROP, "MOD_LoadBmodel: funny lump size");
	count = len / sizeof(*in);

	cmg.leafsurfaces = (int *)Hunk_Alloc( count * sizeof( *cmg.leafsurfaces ), h_high );
	cmg.numLeafSurfaces = count;

	out = cmg.leafsurfaces;

	memcpy(out, in, len);
}

/*
=================
CMod_LoadBrushSides
=================
*/
void CMod_LoadBrushSides (void *data, int len)
{
	int				i;
	cbrushside_t	*out;
	dbrushside_t 	*in;
	int				count;

	in = (dbrushside_t *)(data);
	if ( len % sizeof(*in) ) {
		Com_Error (ERR_DROP, "MOD_LoadBmodel: funny lump size");
	}
	count = len / sizeof(*in);

	cmg.brushsides = (cbrushside_t *)Hunk_Alloc( ( BOX_SIDES + count ) * sizeof( *cmg.brushsides ), h_high );
	cmg.numBrushSides = count;

	out = cmg.brushsides;	

	for ( i=0 ; i<count ; i++, in++, out++) {
		out->planeNum = in->planeNum;
		assert(in->planeNum == out->planeNum.GetValue());

		out->shaderNum = in->shaderNum;
		if ( out->shaderNum < 0 || out->shaderNum >= cmg.numShaders ) {
			Com_Error( ERR_DROP, "CMod_LoadBrushSides: bad shaderNum: %i", out->shaderNum );
		}
	}
}


/*
=================
CMod_LoadEntityString
=================
*/
void CMod_LoadEntityString( void *data, int len ) {
	cmg.entityString = (char *)Hunk_Alloc( len, h_high );
	cmg.numEntityChars = len;
	memcpy (cmg.entityString, data, len);
}

/*
=================
CMod_LoadVisibility
=================
*/
void RE_SetWorldVisData( SPARC<byte> *vis );

#define	VIS_HEADER	8
void CMod_LoadVisibility( void *data, int len ) {
	char	*buf;

	if ( !len ) {
		cmg.visibility = NULL;
		return;
	}
	buf = (char*)data;

	visData.SetAllocator(SparcAllocator, SparcDeallocator);

	cmg.vised = qtrue;
	cmg.numClusters = ((int *)buf)[0];
	cmg.clusterBytes = ((int *)buf)[1];
	visData.Load(buf + VIS_HEADER, len - VIS_HEADER);
	cmg.visibility = &visData;
	RE_SetWorldVisData(&visData);
}

//==================================================================


/*
=================
CMod_LoadPatches
=================
*/
#define	MAX_PATCH_VERTS		1024

void CMod_LoadPatches( void *verts, int vertlen, void *surfaces, int surfacelen, int numsurfs ) {
	mapVert_t	*dv, *dv_p;
	dpatch_t	*in;
	int			count;
	int			i, j;
	int			c;
	cPatch_t	*patch;
	vec3_t		points[MAX_PATCH_VERTS];
	int			width, height;
	int			shaderNum;

	count = surfacelen / sizeof(*in);

	cmg.numSurfaces = numsurfs;
	cmg.surfaces = (cPatch_t **)Hunk_Alloc( cmg.numSurfaces * sizeof( cmg.surfaces[0] ), h_high );

	dv = (mapVert_t *)(verts);
	if (vertlen % sizeof(*dv))
		Com_Error (ERR_DROP, "MOD_LoadBmodel: funny lump size");

	unsigned char* patchScratch = (unsigned char*)Z_Malloc( sizeof( *patch ) * count, TAG_BSP, qtrue);
	
	extern void CM_GridAlloc();
	extern void CM_PatchCollideFromGridTempAlloc();
	extern void CM_PreparePatchCollide(int num);
	extern void CM_TempPatchPlanesAlloc();
	CM_GridAlloc();
	CM_PatchCollideFromGridTempAlloc();
	CM_PreparePatchCollide(count);
	CM_TempPatchPlanesAlloc();

	facetLoad_t *facetbuf = (facetLoad_t*)Z_Malloc(
		MAX_PATCH_PLANES*sizeof(facetLoad_t), TAG_TEMP_WORKSPACE, qfalse);
	
	int *gridbuf = (int*)Z_Malloc(
		CM_MAX_GRID_SIZE*CM_MAX_GRID_SIZE*2*sizeof(int), TAG_TEMP_WORKSPACE, qfalse);

	for ( i = 0 ; i < count ; i++) {
		in = (dpatch_t *)surfaces + i;

		cmg.surfaces[ in->code ] = patch = (cPatch_t *) patchScratch;
		patchScratch += sizeof( *patch );

		// load the full drawverts onto the stack
		width = in->patchWidth;
		height = in->patchHeight;
		c = width * height;
		if ( c > MAX_PATCH_VERTS ) {
			Com_Error( ERR_DROP, "ParseMesh: MAX_PATCH_VERTS" );
		}

		dv_p = dv + (in->verts >> 12);
		for ( j = 0 ; j < c ; j++, dv_p++ ) {
			points[j][0] = dv_p->xyz[0];
			points[j][1] = dv_p->xyz[1];
			points[j][2] = dv_p->xyz[2];
		}

		shaderNum = in->shaderNum;
		patch->contents = cmg.shaders[shaderNum].contentFlags;
		patch->surfaceFlags = cmg.shaders[shaderNum].surfaceFlags;

		// create the internal facet structure
		patch->pc = CM_GeneratePatchCollide( width, height, points, facetbuf, gridbuf );
	}

	extern void CM_GridDealloc();
	extern void CM_PatchCollideFromGridTempDealloc();
	extern void CM_TempPatchPlanesDealloc();
	CM_PatchCollideFromGridTempDealloc();
	CM_GridDealloc();
	CM_TempPatchPlanesDealloc();

	Z_Free(gridbuf);
	Z_Free(facetbuf);
}


/*
==================
CM_LoadMap

Loads in the map and all submodels
==================
*/
void *gpvCachedMapDiskImage = NULL;
char  gsCachedMapDiskImage[MAX_QPATH];
qboolean gbUsingCachedMapDataRightNow = qfalse;	// if true, signifies that you can't delete this at the moment!! (used during z_malloc()-fail recovery attempt)

// called in response to a "devmapbsp blah" or "devmapall blah" command, do NOT use inside CM_Load unless you pass in qtrue
//
// new bool return used to see if anything was freed, used during z_malloc failure re-try
//
qboolean CM_DeleteCachedMap(qboolean bGuaranteedOkToDelete)
{
	qboolean bActuallyFreedSomething = qfalse;

	if (bGuaranteedOkToDelete || !gbUsingCachedMapDataRightNow)
	{
		// dump cached disk image...
		//
		if (gpvCachedMapDiskImage)
		{
			Z_Free(	gpvCachedMapDiskImage );
					gpvCachedMapDiskImage = NULL;

			bActuallyFreedSomething = qtrue;
		}
		gsCachedMapDiskImage[0] = '\0';
		
		// force map loader to ignore cached internal BSP structures for next level CM_LoadMap() call...
		//
		cmg.name[0] = '\0';
	}

	return bActuallyFreedSomething;
}

void CM_Free(void) 
{
	CM_ClearLevelPatches();
	visData.Release();
	Z_TagFree(TAG_BSP);
}


static void CM_LoadMap_Actual( const char *name, qboolean clientload, int *checksum ) {
	const int		*buf = NULL;
	const int		*surfBuf = NULL;
	static unsigned	last_checksum;
	char			lmName[MAX_QPATH];
	char			stripName[MAX_QPATH];
	Lump			outputLump;

	if ( !name || !name[0] ) {
		Com_Error( ERR_DROP, "CM_LoadMap: NULL name" );
	}

#ifndef BSPC
	cm_noAreas = Cvar_Get ("cm_noAreas", "0", CVAR_CHEAT);
	cm_noCurves = Cvar_Get ("cm_noCurves", "0", CVAR_CHEAT);
	cm_playerCurveClip = Cvar_Get ("cm_playerCurveClip", "1", CVAR_ARCHIVE|CVAR_CHEAT );
#endif
	Com_DPrintf( "CM_LoadMap( %s, %i )\n", name, clientload );

	if ( !strcmp( cmg.name, name ) && clientload ) {
		*checksum = last_checksum;
		return;
	}

	CM_ClearMap();
	CM_ClearLevelPatches();

	// free old stuff
	memset( &cmg, 0, sizeof( cmg ) );
	
	if ( !name[0] ) {
		cmg.numLeafs = 1; 
		cmg.numClusters = 1;
		cmg.numAreas = 1;
		cmg.cmodels = (struct cmodel_s *) Z_Malloc( sizeof( *cmg.cmodels ), TAG_BSP, qtrue );
		*checksum = 0;
		return;
	}
	
	last_checksum = crc32(0, (const Bytef *)name, strlen(name));
	COM_StripExtension(name, stripName);

	// load into heap
	outputLump.load(stripName, "shaders");
	CMod_LoadShaders( outputLump.data, outputLump.len );

	outputLump.load(stripName, "leafs");
	CMod_LoadLeafs (outputLump.data, outputLump.len);

	outputLump.load(stripName, "leafbrushes");
	CMod_LoadLeafBrushes (outputLump.data, outputLump.len);

	outputLump.load(stripName, "leafsurfaces");
	CMod_LoadLeafSurfaces (outputLump.data, outputLump.len);

	outputLump.load(stripName, "planes");
	CMod_LoadPlanes (outputLump.data, outputLump.len);

	outputLump.load(stripName, "brushsides");
	CMod_LoadBrushSides (outputLump.data, outputLump.len);

	outputLump.load(stripName, "brushes");
	CMod_LoadBrushes (outputLump.data, outputLump.len);

	outputLump.load(stripName, "models");
	CMod_LoadSubmodels (outputLump.data, outputLump.len);

	outputLump.load(stripName, "nodes");
	CMod_LoadNodes (outputLump.data, outputLump.len);

	outputLump.load(stripName, "entities");
	CMod_LoadEntityString (outputLump.data, outputLump.len);

	outputLump.load(stripName, "visibility");
	CMod_LoadVisibility( outputLump.data, outputLump.len);
	
	Lump misc;
	misc.load(stripName, "misc");
		
	int num_surfs = *(int*)misc.data;
	misc.clear();

	Lump verts;
	verts.load(stripName, "verts");

	Lump patches;
	patches.load(stripName, "patches");
	CMod_LoadPatches(verts.data, verts.len, patches.data, patches.len, num_surfs );
	patches.clear();

	TotalSubModels += cmg.numSubModels;
	
#if !defined(BSPC)
	CM_LoadShaderText(qfalse);
#endif
	CM_InitBoxHull ();
#if !defined(BSPC)
	CM_SetupShaderProperties();
#endif

	*checksum = last_checksum;	

	// do this whether or not the map was cached from last load...
	//
	CM_FloodAreaConnections ();

	// allow this to be cached if it is loaded by the server
	if ( !clientload ) {
		Q_strncpyz( cmg.name, name, sizeof( cmg.name ) );
	}
}

// need a wrapper function around this because of multiple returns, need to ensure bool is correct...
//
void CM_LoadMap( const char *name, qboolean clientload, int *checksum )
{
	CM_LoadMap_Actual( name, clientload, checksum );
}


/*
==================
CM_ClearMap
==================
*/
void CM_ClearMap( void ) 
{
	int		i;

#if !defined(BSPC)
	CM_ShutdownShaderProperties();
//	MAT_Shutdown();
#endif

/*
	if (TheRandomMissionManager)
	{
		delete TheRandomMissionManager;
		TheRandomMissionManager = 0;
	}
*/

/*
	if (cmg.landScape)
	{
		delete cmg.landScape;
		cmg.landScape = 0;
	}
*/

	Com_Memset( &cmg, 0, sizeof( cmg ) );
	CM_ClearLevelPatches();

/*
	for(i = 0; i < NumSubBSP; i++)
	{
		memset(&SubBSP[i], 0, sizeof(SubBSP[0]));
	}
	NumSubBSP = 0;
*/
	TotalSubModels = 0;
}

/*
==================
CM_ClipHandleToModel
==================
*/
cmodel_t	*CM_ClipHandleToModel( clipHandle_t handle, clipMap_t **clipMap )
{
	int		i;
	int		count;

	if ( handle < 0 ) 
	{
		Com_Error( ERR_DROP, "CM_ClipHandleToModel: bad handle %i", handle );
	}
	if ( handle < cmg.numSubModels ) 
	{
		if (clipMap)
		{
			*clipMap = &cmg;
		}
		return &cmg.cmodels[handle];
	}
	if ( handle == BOX_MODEL_HANDLE ) 
	{
		if (clipMap)
		{
			*clipMap = &cmg;
		}
		return &box_model;
	}

/*
	count = cmg.numSubModels;
	for(i = 0; i < NumSubBSP; i++)
	{
		if (handle < count + SubBSP[i].numSubModels)
		{
			if (clipMap)
			{
				*clipMap = &SubBSP[i];
			}
			return &SubBSP[i].cmodels[handle - count];
		}
		count += SubBSP[i].numSubModels;
	}
*/

	if ( handle < MAX_SUBMODELS ) 
	{
		Com_Error( ERR_DROP, "CM_ClipHandleToModel: bad handle %i < %i < %i", 
			cmg.numSubModels, handle, MAX_SUBMODELS );
	}
	Com_Error( ERR_DROP, "CM_ClipHandleToModel: bad handle %i", handle + MAX_SUBMODELS );

	return NULL;
}
/*
==================
CM_InlineModel
==================
*/
clipHandle_t	CM_InlineModel( int index ) {
	if ( index < 0 || index >= TotalSubModels ) {
		Com_Error (ERR_DROP, "CM_InlineModel: bad number (may need to re-BSP map?)");
	}
	return index;
}

int		CM_NumClusters( void ) {
	return cmg.numClusters;
}

int		CM_NumInlineModels( void ) {
	return cmg.numSubModels;
}

char	*CM_EntityString( void ) {
	return cmg.entityString;
}

/*
char *CM_SubBSPEntityString( int index ) 
{
	return SubBSP[index].entityString;
}
*/

int		CM_LeafCluster( int leafnum ) {
	if (leafnum < 0 || leafnum >= cmg.numLeafs) {
		Com_Error (ERR_DROP, "CM_LeafCluster: bad number");
	}
	return cmg.leafs[leafnum].cluster;
}

int		CM_LeafArea( int leafnum ) {
	if ( leafnum < 0 || leafnum >= cmg.numLeafs ) {
		Com_Error (ERR_DROP, "CM_LeafArea: bad number");
	}
	return cmg.leafs[leafnum].area;
}

//=======================================================================


/*
===================
CM_InitBoxHull

Set up the planes and nodes so that the six floats of a bounding box
can just be stored out and get a proper clipping hull structure.
===================
*/
void CM_InitBoxHull (void)
{
	int			i;
	int			side;
	cplane_t	*p;
	cbrushside_t	*s;

	box_planes = &cmg.planes[cmg.numPlanes];

	box_brush = &cmg.brushes[cmg.numBrushes];
	box_brush->numsides = 6;
	box_brush->sides = cmg.brushsides + cmg.numBrushSides;
	box_brush->contents = CONTENTS_BODY;

	box_model.firstNode = -1;
	box_model.leaf.numLeafBrushes = 1;
//	box_model.leaf.firstLeafBrush = cmg.numBrushes;
	box_model.leaf.firstLeafBrush = cmg.numLeafBrushes;
	cmg.leafbrushes[cmg.numLeafBrushes] = cmg.numBrushes;

	for (i=0 ; i<6 ; i++)
	{
		side = i&1;

		// brush sides
		s = &cmg.brushsides[cmg.numBrushSides+i];
		s->planeNum = cmg.numPlanes+i*2+side;
		s->shaderNum = cmg.numShaders;

		// planes
		p = &box_planes[i*2];
		p->type = i>>1;
		p->signbits = 0;
		VectorClear (p->normal);
		p->normal[i>>1] = 1;

		p = &box_planes[i*2+1];
		p->type = 3 + (i>>1);
		p->signbits = 0;
		VectorClear (p->normal);
		p->normal[i>>1] = -1;

		SetPlaneSignbits( p );
	}	
}



/*
===================
CM_HeadnodeForBox

To keep everything totally uniform, bounding boxes are turned into small
BSP trees instead of being compared directly.
===================
*/
clipHandle_t CM_TempBoxModel( const vec3_t mins, const vec3_t maxs, int capsule ) {
	VectorCopy( mins, box_model.mins );
	VectorCopy( maxs, box_model.maxs );

	if ( capsule ) {
		return CAPSULE_MODEL_HANDLE;
	}

	box_planes[0].dist = maxs[0];
	box_planes[1].dist = -maxs[0];
	box_planes[2].dist = mins[0];
	box_planes[3].dist = -mins[0];
	box_planes[4].dist = maxs[1];
	box_planes[5].dist = -maxs[1];
	box_planes[6].dist = mins[1];
	box_planes[7].dist = -mins[1];
	box_planes[8].dist = maxs[2];
	box_planes[9].dist = -maxs[2];
	box_planes[10].dist = mins[2];
	box_planes[11].dist = -mins[2];

	VectorCopy( mins, box_brush->bounds[0] );
	VectorCopy( maxs, box_brush->bounds[1] );

	return BOX_MODEL_HANDLE;
}



/*
===================
CM_ModelBounds
===================
*/
void CM_ModelBounds( clipHandle_t model, vec3_t mins, vec3_t maxs ) {
	cmodel_t	*cmod;

	cmod = CM_ClipHandleToModel( model );
	VectorCopy( cmod->mins, mins );
	VectorCopy( cmod->maxs, maxs );
}

/*
===================
CM_RegisterTerrain

Allows physics to examine the terrain data.
===================
*/
#if !defined(BSPC)
/*
CCMLandScape *CM_RegisterTerrain(const char *config, bool server)
{
	CCMLandScape	*ls;

	if(cmg.landScape)
	{
		// Already spawned so just return
		ls = cmg.landScape;
		ls->IncreaseRefCount();
		return(ls);
	}
	// Doesn't exist so create and link in
	ls = CM_InitTerrain(config, 0, server);

	// Increment for the next instance
	if (cmg.landScape)
	{
		Com_Error(ERR_DROP, "You cannot have more than one terrain brush.\n");
	}
	cmg.landScape = ls;
	return(ls);
}
*/

/*
===================
CM_ShutdownTerrain
===================
*/

/*
void CM_ShutdownTerrain( thandle_t terrainId)
{
	CCMLandScape	*landscape;

	landscape = cmg.landScape;
	if (landscape)
	{
		landscape->DecreaseRefCount();
		if(landscape->GetRefCount() <= 0)
		{
			delete landscape;
			cmg.landScape = NULL;
		}
	}
}
*/
#endif

/*
int CM_LoadSubBSP(const char *name, qboolean clientload)
{
	int		i;
	int		checksum;
	int		count;

	count = cmg.numSubModels;
	for(i = 0; i < NumSubBSP; i++)
	{
		if (!stricmp(name, SubBSP[i].name))
		{
			return count;
		}
		count += SubBSP[i].numSubModels;
	}

	if (NumSubBSP == MAX_SUB_BSP)
	{
		Com_Error (ERR_DROP, "CM_LoadSubBSP: too many unique sub BSPs");
	}

#ifdef _XBOX
	assert(0); // MATT! - testing now - fix this later!
#else
	CM_LoadMap_Actual( name, clientload, &checksum, SubBSP[NumSubBSP] );
#endif
	NumSubBSP++;

	return count;
}
*/

/*
int CM_FindSubBSP(int modelIndex)
{
	int		i;
	int		count;

	count = cmg.numSubModels;
	if (modelIndex < count)
	{	// belongs to the main bsp
		return -1;
	}

	for(i = 0; i < NumSubBSP; i++)
	{
		count += SubBSP[i].numSubModels;
		if (modelIndex < count)
		{
			return i;
		}
	}
	return -1;
}
*/

void CM_GetWorldBounds ( vec3_t mins, vec3_t maxs )
{
	VectorCopy ( cmg.cmodels[0].mins, mins );
	VectorCopy ( cmg.cmodels[0].maxs, maxs );
}

int CM_ModelContents_Actual( clipHandle_t model, clipMap_t *cm ) 
{
	cmodel_t	*cmod;
	int			contents = 0;
	int			i;

	if (!cm)
	{
		cm = &cmg;
	}

	cmod = CM_ClipHandleToModel( model, &cm );

	//MCG ADDED - return the contents, too
	if( cmod->leaf.numLeafBrushes )		// check for brush
	{
		int brushNum;
		for ( i = cmod->leaf.firstLeafBrush; i < cmod->leaf.firstLeafBrush+cmod->leaf.numLeafBrushes; i++ )
		{
			brushNum = cm->leafbrushes[i];
			contents |= cm->brushes[brushNum].contents;
		}
	}
	if( cmod->leaf.numLeafSurfaces )	// if not brush, check for patch
	{	
		int surfaceNum;
		for ( i = cmod->leaf.firstLeafSurface; i < cmod->leaf.firstLeafSurface+cmod->leaf.numLeafSurfaces; i++ )
		{
			surfaceNum = cm->leafsurfaces[i];
			if ( cm->surfaces[surfaceNum] != NULL )
			{//HERNH?  How could we have a null surf within our cmod->leaf.numLeafSurfaces?
				contents |= cm->surfaces[surfaceNum]->contents;
			}
		}
	}
	return contents;
}

int CM_ModelContents(  clipHandle_t model, int subBSPIndex )
{
/*
	if (subBSPIndex < 0)
	{
*/
		return CM_ModelContents_Actual(model, NULL);
/*
	}

	return CM_ModelContents_Actual(model, &SubBSP[subBSPIndex]);
*/
}

