//Anything above this #include will be ignored by the compiler
#include "qcommon/exe_headers.h"

#include "cm_local.h"
#include "cm_patch.h"
#include "cm_landscape.h"
#include "qcommon/GenericParser2.h"
#include "cm_randomterrain.h"
//#include "client/client.h" // this will do for now. we're not a lib
#include "rd-common/tr_public.h"

extern	refexport_t		*re;					// interface to refresh .dll

#if defined(_WIN32) && defined(_MSC_VER) && (_MSC_VER < 1600)
#pragma optimize("p", on)
#endif

#define _SMOOTH_TERXEL_BRUSH

#ifdef _SMOOTH_TERXEL_BRUSH
#define BRUSH_SIDES_PER_TERXEL	8
#else
#define BRUSH_SIDES_PER_TERXEL	5
#endif

void CCMLandScape::SetShaders(int height, CCMShader *shader)
{
	int		i;

	for(i = height; shader && (i < HEIGHT_RESOLUTION); i++)
	{
		if(!mHeightDetails[i].GetSurfaceFlags())
		{
			mHeightDetails[i].SetFlags(shader->contentFlags, shader->surfaceFlags);
		}
	}
}

void CCMLandScape::LoadTerrainDef(const char *td)
{
	char				terrainDef[MAX_QPATH];
	CGenericParser2		parse;
	CGPGroup			*basegroup, *classes, *items;

	Com_sprintf(terrainDef, MAX_QPATH, "ext_data/RMG/%s.terrain", Info_ValueForKey(td, "terrainDef"));
	Com_DPrintf("CM_Terrain: Loading and parsing terrainDef %s.....\n", Info_ValueForKey(td, "terrainDef"));

	if(!Com_ParseTextFile(terrainDef, parse))
	{
		Com_sprintf(terrainDef, MAX_QPATH, "ext_data/arioche/%s.terrain", Info_ValueForKey(td, "terrainDef"));
		if(!Com_ParseTextFile(terrainDef, parse))
		{
			Com_Printf("Could not open %s\n", terrainDef);
			return;
		}
	}
	// The whole file....
	basegroup = parse.GetBaseParseGroup();

	// The root { } struct
	classes = basegroup->GetSubGroups();
	while(classes)
	{
		items = classes->GetSubGroups();
		while(items)
		{
			if(!Q_stricmp(items->GetName(), "altitudetexture"))
			{
				int			height;
				const char	*shaderName;
				CCMShader	*shader;

				// Height must exist - the rest are optional
				height = atol(items->FindPairValue("height", "0"));

				// Shader for this height
				shaderName = items->FindPairValue("shader", "");
				if(strlen(shaderName))
				{
					shader = CM_GetShaderInfo(shaderName);
					if(shader)
					{
						SetShaders(height, shader); 
					}
				}
			}
			else if(!Q_stricmp(items->GetName(), "water"))
			{
				const char	*shaderName;
				CCMShader	*shader;

				// Grab the height of the water
				mBaseWaterHeight = atol(items->FindPairValue("height", "0"));
				SetRealWaterHeight(mBaseWaterHeight);

				// Grab the material of the water
				shaderName = items->FindPairValue("shader", "");
				shader = CM_GetShaderInfo(shaderName);
				if(shader)
				{
					mWaterContents = shader->contentFlags;
					mWaterSurfaceFlags = shader->surfaceFlags;
				}
			}
			items = (CGPGroup *)items->GetNext();
		}
		classes = (CGPGroup *)classes->GetNext();
	}
	Com_ParseTextFileDestroy(parse);
}

CCMPatch::~CCMPatch(void)
{
}

CCMLandScape::CCMLandScape(const char *configstring, bool server)
{
	int			numPatches, numBrushesPerPatch, size;//, seed;
	char		heightMap[MAX_QPATH];
	//char		*ptr;

	holdrand = 0x89abcdef;

	// Clear out the height details
	memset(mHeightDetails, 0, sizeof(CCMHeightDetails) * HEIGHT_RESOLUTION);
	mBaseWaterHeight = 0;
	mWaterHeight = 0.0f;

	// When constructed, referenced once
	mRefCount = 1;

	// Extract the relevant data from the config string
	Com_sprintf(heightMap, MAX_QPATH, "%s", Info_ValueForKey(configstring, "heightMap"));
	numPatches = atol(Info_ValueForKey(configstring, "numPatches"));
	mTerxels = atol(Info_ValueForKey(configstring, "terxels"));
	mHasPhysics = !!atol(Info_ValueForKey(configstring, "physics"));
	//seed = strtoul(Info_ValueForKey(configstring, "seed"), &ptr, 10);

	mBounds[0][0] = (float)atof(Info_ValueForKey(configstring, "minx"));
	mBounds[0][1] = (float)atof(Info_ValueForKey(configstring, "miny"));
	mBounds[0][2] = (float)atof(Info_ValueForKey(configstring, "minz"));
	mBounds[1][0] = (float)atof(Info_ValueForKey(configstring, "maxx"));
	mBounds[1][1] = (float)atof(Info_ValueForKey(configstring, "maxy"));
	mBounds[1][2] = (float)atof(Info_ValueForKey(configstring, "maxz"));

	// Calculate size of the brush
	VectorSubtract(mBounds[1], mBounds[0], mSize);

	// Work out the dimensions of the brush in blocks - the object is to make the blocks as square as possible
	mBlockWidth = Round(sqrtf(numPatches * mSize[0] / mSize[1]));
	mBlockHeight = Round(sqrtf(numPatches * mSize[1] / mSize[0]));

	// ...which lets us get the size of the heightmap
	mWidth = mBlockWidth * mTerxels;
	mHeight = mBlockHeight * mTerxels;

	mHeightMap = (byte *)Z_Malloc(GetRealArea(), TAG_CM_TERRAIN);
	mFlattenMap = (byte *)Z_Malloc(GetRealArea(), TAG_CM_TERRAIN);
	
	// Zero means unused.
	memset ( mFlattenMap, 0, GetRealArea() );

	if(strlen(heightMap))
	{
#ifndef DEDICATED
		int iWidth, iHeight;
		byte *imageData;
#endif

		Com_DPrintf("CM_Terrain: Loading heightmap %s.....\n", heightMap);
		mRandomTerrain = 0;
#ifndef DEDICATED
		re->LoadDataImage(heightMap, &imageData, &iWidth, &iHeight); 
		if(imageData)
		{
			if(strstr(heightMap, "random_"))
			{
				mRandomTerrain = CreateRandomTerrain ( configstring, this, mHeightMap, GetRealWidth(), GetRealHeight());
			}
			else
			{
				// Flip to make the same as GenSurf
				re->InvertImage(imageData, iWidth, iHeight, 1);
				re->Resample(imageData, iWidth, iHeight, mHeightMap, GetRealWidth(), GetRealHeight(), 1);
			}
			Z_Free(imageData);
		}
#endif
	}
	else
	{
		Com_Error(ERR_FATAL, "Terrain has no heightmap specified\n");
	}

	// Work out the dimensions of the terxel - should be almost square
	mTerxelSize[0] = mSize[0] / mWidth;
	mTerxelSize[1] = mSize[1] / mHeight;
	mTerxelSize[2] = mSize[2] / 255.0f;

	// Work out the patchsize
	mPatchSize[0] = mSize[0] / mBlockWidth;
	mPatchSize[1] = mSize[1] / mBlockHeight;
	mPatchSize[2] = 1.0f;
	mPatchScalarSize = VectorLength(mPatchSize);

	// Loads in the water height and properties
	// Gets the shader properties for the blended shaders
	LoadTerrainDef(configstring);

	Com_DPrintf("CM_Terrain: Creating patches.....\n");
	mPatches = (CCMPatch *)Z_Malloc(sizeof(CCMPatch) * GetBlockCount(), TAG_CM_TERRAIN);

	numBrushesPerPatch = mTerxels * mTerxels * 2;
	size = (numBrushesPerPatch * sizeof(cbrush_t)) + (numBrushesPerPatch * BRUSH_SIDES_PER_TERXEL * 2 * (sizeof(cbrushside_t) + sizeof(cplane_t)));
	mPatchBrushData = (byte *)Z_Malloc(size * GetBlockCount(), TAG_CM_TERRAIN);

	// Initialize all terrain patches
	UpdatePatches();
}

// Initialise a plane from 3 coords

void CCMPatch::InitPlane(struct cbrushside_s *side, cplane_t *plane, vec3_t p0, vec3_t p1, vec3_t p2)
{
	vec3_t	dx, dy;

	VectorSubtract(p1, p0, dx);
	VectorSubtract(p2, p0, dy);
	CrossProduct(dx, dy, plane->normal);
	VectorNormalize(plane->normal);

	plane->dist = DotProduct(p0, plane->normal);
	plane->type = PlaneTypeForNormal(plane->normal);
	SetPlaneSignbits(plane);

	side->plane = plane;
}

// Create the planes required for collision detection
// 2 brushes per terxel - each brush has 5 sides and 5 planes

void* CCMPatch::GetAdjacentBrushY ( int x, int y )
{
	int			yo1 = y % owner->GetTerxels();
	int			yo2 = (y-1) % owner->GetTerxels();
	int			xo  = x % owner->GetTerxels();
	CCMPatch*	patch;
	
	// Different patch
	if ( yo2 > yo1 )
	{
		patch = owner->GetPatch ( x / owner->GetTerxels(), (y-1) / owner->GetTerxels() );
	}
	else
	{
		patch = this;
	}

	cbrush_t *brush;
	
	brush =  patch->mPatchBrushData;
	brush += ((yo2 * owner->GetTerxels ( ) + xo) * 2);
	brush ++;

	return brush;
}

void* CCMPatch::GetAdjacentBrushX ( int x, int y )
{
	int			xo1 = x % owner->GetTerxels();
	int			xo2 = (x-1) % owner->GetTerxels();
	int			yo  = y % owner->GetTerxels();
	CCMPatch*	patch;
	
	// Different patch
	if ( xo2 > xo1 )
	{
		patch = owner->GetPatch ( (x-1) / owner->GetTerxels(), y / owner->GetTerxels() );
	}
	else
	{
		patch = this;
	}

	cbrush_t *brush;
	
	brush =  patch->mPatchBrushData;
	brush += ((yo * owner->GetTerxels ( ) + xo2) * 2);

	if ( ! ((x+y) & 1) )
	{		
		brush ++;
	}

	return brush;
}

void CCMPatch::CreatePatchPlaneData(void)
{				
	int				realWidth;
	int				x, y, i, j;
#if	0
	int				n;
#endif
	cbrush_t		*brush;
	cbrushside_t	*side;
	cplane_t		*plane;
	vec3_t			*coords;
	vec3_t			localCoords[8];

	mNumBrushes = owner->GetTerxels() * owner->GetTerxels() * 2;
	realWidth = owner->GetRealWidth();
	coords = owner->GetCoords();

	brush = mPatchBrushData;
	side = (cbrushside_t *)(mPatchBrushData + mNumBrushes);
	plane = (cplane_t *)(side + (mNumBrushes * BRUSH_SIDES_PER_TERXEL * 2));
	for(y = mHy; y < mHy + owner->GetTerxels(); y++)
	{
		for(x = mHx; x < mHx + owner->GetTerxels(); x++)
		{
			int		offsets[4];

			if ( (x+y)&1 )
			{
				offsets[0] = (y * realWidth) + x;				// TL
				offsets[1] = (y * realWidth) + x + 1;			// TR
				offsets[2] = ((y + 1) * realWidth) + x;			// BL
				offsets[3] = ((y + 1) * realWidth) + x + 1;		// BR
			}
			else
			{
				offsets[2] = (y * realWidth) + x;				// TL
				offsets[0] = (y * realWidth) + x + 1;			// TR
				offsets[3] = ((y + 1) * realWidth) + x;			// BL
				offsets[1] = ((y + 1) * realWidth) + x + 1;		// BR
			}

			for(i = 0; i < 4; i++)
			{
				VectorCopy(coords[offsets[i]], localCoords[i]);
				VectorCopy(coords[offsets[i]], localCoords[i + 4]);
				// Set z of base of brush to bottom of landscape brush
				localCoords[i + 4][2] = owner->GetMins()[2];
			}

			// Set the bounds of the terxel
			VectorSet(brush[0].bounds[0], MAX_WORLD_COORD, MAX_WORLD_COORD, MAX_WORLD_COORD);
			VectorSet(brush[0].bounds[1], MIN_WORLD_COORD, MIN_WORLD_COORD, MIN_WORLD_COORD);

			for(i = 0; i < 8; i++)
			{
				for(j = 0; j < 3; j++)
				{
					// mins
					if(localCoords[i][j] < brush[0].bounds[0][j])
					{
						brush[0].bounds[0][j] = localCoords[i][j];
					}
					// maxs
					if(localCoords[i][j] > brush[0].bounds[1][j])
					{
						brush[0].bounds[1][j] = localCoords[i][j];
					}
				}
			}
			VectorDec(brush[0].bounds[0]);
			VectorInc(brush[0].bounds[1]);
			VectorCopy(brush[0].bounds[0], brush[1].bounds[0]);
			VectorCopy(brush[0].bounds[1], brush[1].bounds[1]);

			brush[0].contents = mContentFlags;
			brush[1].contents = mContentFlags;

#ifndef _SMOOTH_TERXEL_BRUSH

			// Set up sides of the brushes
			brush[0].numsides = 5;
			brush[0].sides = side;
			brush[1].numsides = 5;
			brush[1].sides = side + 5;
								  			
			for ( i = 0; i < 8 ; i ++ )
			{
				localCoords[i][0] = (int)localCoords[i][0];
				localCoords[i][1] = (int)localCoords[i][1];
				localCoords[i][2] = (int)localCoords[i][2];
			}
	
			// Create the planes of the 2 triangles that make up the tops of the brushes
			InitPlane(side + 0, plane + 0, localCoords[0], localCoords[1], localCoords[2]);
			InitPlane(side + 5, plane + 5, localCoords[3], localCoords[2], localCoords[1]);

			// Create the bottom face of the brushes
			InitPlane(side + 1, plane + 1, localCoords[6], localCoords[5], localCoords[4]);
			InitPlane(side + 6, plane + 6, localCoords[5], localCoords[6], localCoords[7]);

			// Create the 3 vertical faces
			InitPlane(side + 2, plane + 2, localCoords[0], localCoords[2], localCoords[4]);
			InitPlane(side + 7, plane + 7, localCoords[3], localCoords[1], localCoords[7]);

			InitPlane(side + 3, plane + 3, localCoords[0], localCoords[4], localCoords[1]);
			InitPlane(side + 8, plane + 8, localCoords[3], localCoords[7], localCoords[2]);

			InitPlane(side + 4, plane + 4, localCoords[2], localCoords[1], localCoords[6]);
			InitPlane(side + 9, plane + 9, localCoords[5], localCoords[1], localCoords[6]);

			// Increment to next terxel
			brush += 2;
			side += 10;
			plane += 10;

			

#else

			// Set up sides of the brushes
			brush[0].numsides = 5;
			brush[0].sides = side;
			brush[1].numsides = 5;
			brush[1].sides = side + 8;


			// Create the planes of the 2 triangles that make up the tops of the brushes
			InitPlane(side + 0, plane + 0, localCoords[0], localCoords[1], localCoords[2]);
			InitPlane(side + 8, plane + 8, localCoords[3], localCoords[2], localCoords[1]);

			// Create the bottom face of the brushes
			InitPlane(side + 1, plane + 1, localCoords[4], localCoords[6], localCoords[5]);
			InitPlane(side + 9, plane + 9, localCoords[7], localCoords[5], localCoords[6]);

			// Create the 3 vertical faces
			InitPlane(side + 2, plane + 2, localCoords[0], localCoords[2], localCoords[4]);
			InitPlane(side + 10, plane + 10, localCoords[3], localCoords[1], localCoords[7]);

			InitPlane(side + 3, plane + 3, localCoords[0], localCoords[4], localCoords[1]);
			InitPlane(side + 11, plane + 11, localCoords[3], localCoords[7], localCoords[2]);

			InitPlane(side + 4, plane + 4, localCoords[2], localCoords[1], localCoords[6]);
			InitPlane(side + 12, plane + 12, localCoords[5], localCoords[1], localCoords[6]);

			float V = DotProduct ( (plane + 8)->normal, localCoords[0] ) - (plane + 8)->dist;

			if ( V < 0 )
			{
				InitPlane ( brush[0].sides + brush[0].numsides, plane + brush[0].numsides, localCoords[3], localCoords[2], localCoords[1]);
				brush[0].numsides++;			

				InitPlane ( brush[1].sides + brush[1].numsides, plane + 8 + brush[1].numsides, localCoords[0], localCoords[1], localCoords[2]);
				brush[1].numsides++;			
			}

			// Determine if we need to smooth the brush transition from the brush above us
			if ( y > 0 && y < owner->GetPatchHeight ( ) - 1 )
			{
				cbrush_t* abovebrush = (cbrush_t*)GetAdjacentBrushY ( x, y );
				cplane_t* aboveplane = abovebrush->sides->plane;

				V = DotProduct ( aboveplane->normal, ((y+x)&1)?(localCoords[2]):(localCoords[1]) ) - aboveplane->dist;

				if ( V < 0 )
				{
					memcpy ( brush[0].sides + brush[0].numsides, abovebrush->sides, sizeof(cbrushside_t) );
					brush[0].numsides++;

					memcpy ( abovebrush->sides + abovebrush->numsides, side + 0, sizeof(cbrushside_t) );
					abovebrush->numsides++;
				}
			}
			
			// Determine if we need to smooth the brush transition from the brush to the left of us
			if ( x > 0 && x < owner->GetPatchWidth ( ) - 1 )
			{
				cbrush_t* abovebrush = (cbrush_t*)GetAdjacentBrushX ( x, y );
				cplane_t* aboveplane = abovebrush->sides->plane;

				V = DotProduct ( aboveplane->normal, localCoords[1] ) - aboveplane->dist;

				if ( V < 0 )
				{
					if ( (x+y)&1 )
					{
						memcpy ( brush[0].sides + brush[0].numsides, abovebrush->sides, sizeof(cbrushside_t) );
						brush[0].numsides++;

						memcpy ( abovebrush->sides + abovebrush->numsides, side + 0, sizeof(cbrushside_t) );
						abovebrush->numsides++;
					}
					else
					{
						memcpy ( brush[1].sides + brush[1].numsides, abovebrush->sides, sizeof(cbrushside_t) );
						brush[1].numsides++;

						memcpy ( abovebrush->sides + abovebrush->numsides, side + 8, sizeof(cbrushside_t) );
						abovebrush->numsides++;
					}
				}
			}

			// Increment to next terxel
			brush += 2;
			side += 16;
			plane += 16;
#endif
		}
	}
}

void CCMPatch::Init(CCMLandScape *ls, int heightX, int heightY, vec3_t world, byte *hMap, byte *patchBrushData)
{
	int		min, max, x, y, height;

	// Set owning landscape
	owner = ls;

	// Store the base of the top left corner
	VectorCopy(world, mWorldCoords);

	// Store pointer to first byte of the height data for this patch.
	mHx = heightX;
	mHy = heightY;
	mHeightMap = hMap + ((heightY * owner->GetRealWidth()) + heightX);

	// Calculate the bounds for culling 
	// Use the dimensions 1 terxel outside the patch to allow for sloping of edge terxels
	min = 256;
	max = -1;
	for(y = heightY - 1; y < heightY + owner->GetTerxels() + 1; y++)
	{
		if(y >= 0)
		{
			for(x = heightX - 1; x < heightX + owner->GetTerxels() + 1; x++)
			{
				if(x >= 0)
				{
					height = hMap[(y * owner->GetRealWidth()) + x];

					if(height > max)
					{
						max = height;
					}
					if(height < min)
					{
						min = height;
					}
				}
			}
		}
	}

	// Mins
	mBounds[0][0] = world[0];
	mBounds[0][1] = world[1];
	mBounds[0][2] = world[2] + (min * owner->GetTerxelSize()[2]);

	// Maxs
	mBounds[1][0] = world[0] + (owner->GetPatchSize()[0]);
	mBounds[1][1] = world[1] + (owner->GetPatchSize()[1]);
	mBounds[1][2] = world[2] + (max * owner->GetTerxelSize()[2]);

	// Corner heights
	mCornerHeights[0] = mHeightMap[0];
	mCornerHeights[1] = mHeightMap[owner->GetTerxels()];
	mCornerHeights[2] = mHeightMap[owner->GetTerxels() * owner->GetRealWidth()];
	mCornerHeights[3] = mHeightMap[(owner->GetTerxels() * owner->GetRealWidth()) + owner->GetTerxels()];

	// Set the surfaceFlags using average height (may want a more complex algo here)
	mSurfaceFlags = owner->GetSurfaceFlags((min + max) >> 1);
	mContentFlags = owner->GetContentFlags((min + max) >> 1);

	// Set base of brush data from big array
	mPatchBrushData = (cbrush_t *)patchBrushData; 
	CreatePatchPlaneData();
}

CCMPatch *CCMLandScape::GetPatch(int x, int y)
{
	return(mPatches + ((y * mBlockWidth) + x));
}

extern cvar_t	*com_newtrace;		

void CCMLandScape::PatchCollide(struct traceWork_s *tw, trace_t &trace, const vec3_t start, const vec3_t end, int checkcount)
{
	vec3pair_t	tBounds;

	// Convert to valid bounding box
	CM_CalcExtents(start, end, tw, tBounds);

	//if (com_newtrace->integer)
	if (1)
	{
		float		slope, offset;
		float		startPatchLoc, endPatchLoc, startPos, endPos;
		float		patchDirection = 1;
		float		checkDirection = 1;
		int			countPatches, count;
		CCMPatch	*patch;
		float		fraction = trace.fraction;

		if (fabs(end[0]-start[0]) >= fabs(fabs(end[1]-start[1])))
		{	// x travels more than y
			// calculate line slope and offset
			if (end[0] - start[0])
			{
				slope = (end[1] - start[1]) / (end[0] - start[0]);
			}
			else
			{
				slope = 0;
			}
			offset = start[1] - (start[0] * slope);

			// find the starting 
			startPatchLoc = floor((start[0] - mBounds[0][0]) / mPatchSize[0]);
			endPatchLoc = floor((end[0] - mBounds[0][0]) / mPatchSize[0]);

			if (startPatchLoc <= endPatchLoc)
			{	// moving along slope in a positive direction
				endPatchLoc++;
				startPatchLoc--;
				countPatches = endPatchLoc - startPatchLoc + 1;
			}
			else
			{	// moving along slope in a negative direction
				endPatchLoc--;
				startPatchLoc++;
				patchDirection = -1;
				countPatches = startPatchLoc - endPatchLoc + 1;
			}
			if (slope < 0.0)
			{
				checkDirection = -1;
			}
			
			// first calculate the real world location
			startPos = ((startPatchLoc * mPatchSize[0] + mBounds[0][0]) * slope) + offset;
			// calculate it back into patch coords
			startPos = floor((startPos - mBounds[0][1] + tw->size[0][1]) / mPatchSize[1]);
			do
			{
				if (startPatchLoc >= 0 && startPatchLoc < mBlockWidth)
				{	// valid location
					// first calculate the real world location
					endPos = (((startPatchLoc+patchDirection) * mPatchSize[0] + mBounds[0][0]) * slope) + offset;
					// calculate it back into patch coords
					endPos = floor((endPos - mBounds[0][1] + tw->size[1][1]) / mPatchSize[1]);
					
					if (checkDirection < 0)
					{
						startPos++;
						endPos--;
					}
					else
					{
						startPos--;
						endPos++;
					}
					count = fabs(endPos - startPos) + 1;
					while(count)
					{
						if (startPos  >= 0 && startPos < mBlockHeight)
						{	// valid location
							patch = GetPatch(startPatchLoc, startPos);
							// Collide with every patch to find the minimum fraction
							CM_HandlePatchCollision(tw, trace, tBounds[0], tBounds[1], patch, checkcount);

							if (trace.fraction <= 0.0)
							{
								return;
							}
						}
						startPos += checkDirection;
						count--;
					}

					if (trace.fraction < fraction)
					{
						return;
					}
				}
				// move to the next spot
				// we still stay one behind, to get the opposite edge of the terrain patch
				startPos = ((startPatchLoc * mPatchSize[0] + mBounds[0][0]) * slope) + offset;
				startPatchLoc += patchDirection;
				// first calculate the real world location
				// calculate it back into patch coords
				startPos = floor((startPos - mBounds[0][1] + tw->size[0][1]) / mPatchSize[1]);

				countPatches--;
			}
			while (countPatches);
		}
		else
		{
			// calculate line slope and offset
			slope = (end[0] - start[0]) / (end[1] - start[1]);
			offset = start[0] - (start[1] * slope);

			// find the starting 
			startPatchLoc = floor((start[1] - mBounds[0][1]) / mPatchSize[1]);
			endPatchLoc = floor((end[1] - mBounds[0][1]) / mPatchSize[1]);

			if (startPatchLoc <= endPatchLoc)
			{	// moving along slope in a positive direction
				endPatchLoc++;
				startPatchLoc--;
				countPatches = endPatchLoc - startPatchLoc + 1;
			}
			else
			{	// moving along slope in a negative direction
				endPatchLoc--;
				startPatchLoc++;
				patchDirection = -1;
				countPatches = startPatchLoc - endPatchLoc + 1;
			}
			if (slope < 0.0)
			{
				checkDirection = -1;
			}
			
			// first calculate the real world location
			startPos = ((startPatchLoc * mPatchSize[1] + mBounds[0][1]) * slope) + offset;
			// calculate it back into patch coords
			startPos = floor((startPos - mBounds[0][0] + tw->size[0][0]) / mPatchSize[0]);
			do
			{
				if (startPatchLoc >= 0 && startPatchLoc < mBlockHeight)
				{	// valid location
					// first calculate the real world location
					endPos = (((startPatchLoc+patchDirection) * mPatchSize[1] + mBounds[0][1]) * slope) + offset;
					// calculate it back into patch coords
					endPos = floor((endPos - mBounds[0][0] + tw->size[1][0]) / mPatchSize[0]);

					if (checkDirection < 0)
					{
						startPos++;
						endPos--;
					}
					else
					{
						startPos--;
						endPos++;
					}
					
					count = fabs(endPos - startPos) + 1;
					while(count)
					{
						if (startPos >= 0 && startPos < mBlockWidth)
						{	// valid location
							patch = GetPatch(startPos, startPatchLoc);
							// Collide with every patch to find the minimum fraction
							CM_HandlePatchCollision(tw, trace, tBounds[0], tBounds[1], patch, checkcount);

							if (trace.fraction <= 0.0)
							{
								return;
							}
						}
						startPos += checkDirection;
						count--;
					}

					if (trace.fraction < fraction)
					{
						return;
					}
				}

				// move to the next spot
				// we still stay one behind, to get the opposite edge of the terrain patch
				startPos = ((startPatchLoc * mPatchSize[1] + mBounds[0][1]) * slope) + offset;
				startPatchLoc += patchDirection;
				// first calculate the real world location
				// calculate it back into patch coords
				startPos = floor((startPos - mBounds[0][0] + tw->size[0][0]) / mPatchSize[0]);
				countPatches--;
			}
			while (countPatches);
		}
	}
	else
	{
		int			x, y;
		vec3_t		tWork;
		vec3_t		pStart, pEnd;
		int			minx, maxx, miny, maxy;
		CCMPatch	*patch;

		// Work out and grab the relevant patches
		VectorSubtract(tBounds[0], mBounds[0], tWork);
		VectorInverseScaleVector(tWork, mPatchSize, pStart);
		VectorSubtract(tBounds[1], mBounds[0], tWork);
		VectorInverseScaleVector(tWork, mPatchSize, pEnd);

		minx = Com_Clamp(0, mBlockWidth - 1, floorf(pStart[0]));
		maxx = Com_Clamp(0, mBlockWidth - 1, ceilf(pEnd[0]));
		miny = Com_Clamp(0, mBlockHeight - 1, floorf(pStart[1]));
		maxy = Com_Clamp(0, mBlockHeight - 1, ceilf(pEnd[1]));

		// generic box collide with each one
		for(y = miny; y <= maxy; y++)
		{
			for(x = minx; x <= maxx; x++)
			{
				patch = GetPatch(x, y);
				// Collide with every patch to find the minimum fraction
				CM_HandlePatchCollision(tw, trace, tBounds[0], tBounds[1], patch, checkcount);

				if (trace.fraction <= 0.0)
				{
					break;
				}
			}
		}
	}
}

float CCMLandScape::WaterCollide(const vec3_t begin, const vec3_t end, float fraction) const
{
	// Check for completely above water
	if((begin[2] > mWaterHeight) && (end[2] > mWaterHeight))
	{
		return(fraction);
	}
	// Check for completely below water
	if((begin[2] < mWaterHeight) && (end[2] < mWaterHeight))
	{
		return(fraction);
	}
	// Check for starting in water and leaving
	if(begin[2] < mWaterHeight - SURFACE_CLIP_EPSILON)
	{
		fraction = ((mWaterHeight - SURFACE_CLIP_EPSILON) - begin[2]) / (end[2] - begin[2]);
		return(fraction);
	}
	// Now the trace must be entering the water
	if(begin[2] > mWaterHeight + SURFACE_CLIP_EPSILON)
	{
		fraction = (begin[2] - (mWaterHeight + SURFACE_CLIP_EPSILON)) / (begin[2] - end[2]);
	}
	return(fraction);
}

void CCMLandScape::GetTerxelLocalCoords ( int x, int y, vec3_t localCoords[8] )
{
	int		realWidth;
	vec3_t* coords;
	int		offsets[4];
	int		i;

	coords = GetCoords ( );
	realWidth = GetRealWidth ( );

	if ( (x+y)&1 )
	{
		offsets[0] = (y * realWidth) + x;				// TL
		offsets[1] = (y * realWidth) + x + 1;			// TR
		offsets[2] = ((y + 1) * realWidth) + x;			// BL
		offsets[3] = ((y + 1) * realWidth) + x + 1;		// BR
	}
	else
	{
		offsets[2] = (y * realWidth) + x;				// TL
		offsets[0] = (y * realWidth) + x + 1;			// TR
		offsets[3] = ((y + 1) * realWidth) + x;			// BL
		offsets[1] = ((y + 1) * realWidth) + x + 1;		// BR
	}

	for( i = 0; i < 4; i++ )
	{
		VectorCopy(coords[offsets[i]], localCoords[i]);
		VectorCopy(coords[offsets[i]], localCoords[i + 4]);
		
		// Set z of base of brush to bottom of landscape brush
		localCoords[i + 4][2] = GetMins()[2];
	}
}


void CCMLandScape::UpdatePatches(void)
{
	CCMPatch	*patch;
	int			x, y, ix, iy, numBrushesPerPatch;
	vec3_t		world;
	int			size;

/*	for(y=0;y<GetRealHeight();y++)
	{
		for(x=0;x<GetRealWidth();x++)
		{
			Com_Printf("(%d,%d) = %u\n", x, y, (unsigned)mHeightMap[y*GetRealWidth() + x]);
		}
	}
*/
	// Calculate real world coordinates from the heightmap
	CalcRealCoords();

	numBrushesPerPatch = mTerxels * mTerxels * 2;
	size = (numBrushesPerPatch * sizeof(cbrush_t)) + (numBrushesPerPatch * BRUSH_SIDES_PER_TERXEL * 2 * (sizeof(cbrushside_t) + sizeof(cplane_t)));

	patch = mPatches;
	for(y = 0, iy = 0; y < mHeight; y += mTerxels, iy++)
	{
		for(x = 0, ix = 0; x < mWidth; x += mTerxels, ix++, patch++)
		{
			VectorSet(world, mBounds[0][0] + (x * mTerxelSize[0]), mBounds[0][1] + (y * mTerxelSize[1]), mBounds[0][2]);
			patch->Init(this, x, y, world, mHeightMap, mPatchBrushData + (size * (ix + (iy * mBlockWidth))));
		}
	}

/*	
	for ( y = mTerxels; y < mHeight - mTerxels; y ++ )
	{
		for ( x = mTerxels; x < mWidth - mTerxels; x ++ )
		{
			int xo     = x % mTerxels;
			int yo     = y % mTerxels;
			int xor    = (x + 1) % mTerxels;
			int yob    = (y + 1) % mTerxels;

			CCMPatch*	patch  = mPatches + (mWidth / mTerxels) * y + (x / mTerxels);
			CCMPatch*	rpatch = mPatches + (mWidth / mTerxels) * y + ((x+1) / mTerxels);
			CCMPatch*	bpatch = mPatches + (mWidth / mTerxels) * (y + 1) + (x / mTerxels);

			int		offsets[4];
			vec3_t	localCoords[8];
			vec3_t	localCoordsR[8];
			vec3_t	localCoordsL[8];

			GetTerxelLocalCoords ( x, y, localCoords );
			GetTerxelLocalCoords ( x + 1, y, localCoordsR );
			GetTerxelLocalCoords ( x, y + 1, localCoordsB );

			brush = patch->GetCollisionData ( );;
			side  = (cbrushside_t *)(mPatchBrushData + patch->GetNumBrushes ( ) );
			plane = (cplane_t *)(side + (mNumBrushes * BRUSH_SIDES_PER_TERXEL * 2));
			

			float V = DotProduct ( (plane + 8)->normal, localCoords[0] ) + plane->dist;

			if ( V < 0 )
			{
				InitPlane ( brush[0].sides + brush[0].numsides, plane + brush[0].numsides, localCoords[3], localCoords[2], localCoords[1]);
				brush[0].numsides++;			

				InitPlane ( brush[1].sides + brush[1].numsides, plane + 8 + brush[1].numsides, localCoords[0], localCoords[1], localCoords[2]);
				brush[1].numsides++;			
			}			
		}
	}
*/

	// Cleanup coord array
	Z_Free(mCoords);
}

void CCMLandScape::CalcRealCoords(void)
{
	int		x, y;

	mCoords = (vec3_t *)Z_Malloc(sizeof(vec3_t) * GetRealWidth() * GetRealHeight(), TAG_CM_TERRAIN_TEMP);

	// Work out the real world coordinates of each heightmap entry
	for(y = 0; y < GetRealHeight(); y++)
	{
		for(x = 0; x < GetRealWidth(); x++)
		{
			ivec3_t		icoords;
			int			offset;

			offset = (y * GetRealWidth()) + x;

			VectorSetM(icoords, x, y, mHeightMap[offset]);
			VectorScaleVectorAdd(GetMins(), icoords, GetTerxelSize(), mCoords[offset]);
		}
	}
}

void CCMLandScape::TerrainPatchIterate(void (*IterateFunc)( CCMPatch *, void * ), void *userdata) const
{
	int			i;
	CCMPatch	*patch;

	patch = mPatches;
	for(i = 0; i < GetBlockCount(); i++, patch++)
	{
		IterateFunc(patch, userdata);
	}
}

#define LERP(t, a, b) (((b)-(a))*(t) + (a))

float CCMLandScape::GetWorldHeight(vec3_t origin, const vec3pair_t bounds, bool aboveGround) const
{ 
	vec3_t		work;
	int			minx, maxx, miny, maxy;
	int			TL, TR, BL, BR;
	int			final;

	VectorSubtract(origin, mBounds[0], work);
	VectorInverseScaleVector(work, mTerxelSize, work);

	// Presume the bases of all misc models are less than 1 terxel square
	minx = Com_Clamp(0, GetWidth(), (int)floorf(work[0]));
	maxx = Com_Clamp(0, GetWidth(), (int)ceilf(work[0]));
	miny = Com_Clamp(0, GetHeight(), (int)floorf(work[1]));
	maxy = Com_Clamp(0, GetHeight(), (int)ceilf(work[1]));

	TL = mHeightMap[(miny * GetRealWidth()) + minx];
	TR = mHeightMap[(miny * GetRealWidth()) + maxx];
	BL = mHeightMap[(maxy * GetRealWidth()) + minx];
	BR = mHeightMap[(maxy * GetRealWidth()) + maxx];

	if(aboveGround)
	{
//		int			max1, max2;
//		max1 = maximum(TL, TR);
//		max2 = maximum(BL, BR);
//		final = maximum(max1, max2);
		float h1, h2;
		float tx, ty;
		tx = (work[0] - minx)/((float)(maxx-minx));
		ty = (work[1] - miny)/((float)(maxy-miny));
		h1 = LERP(tx, TL, TR);
		h2 = LERP(tx, BL, BR);
		final = LERP(ty, h1, h2);
	}
	else
	{
		int			min1, min2;

		min1 = minimum(TL, TR);
		min2 = minimum(BL, BR);
		final = minimum(min1, min2);
	}
	origin[2] = (final * mTerxelSize[2]) + mBounds[0][2]; 

	// compute slope at this spot
	if (maxx == minx)
		maxx = Com_Clamp(0, GetWidth(), minx+1);
	if (maxy == miny)
		maxy = Com_Clamp(0, GetHeight(), miny+1);
	BR = mHeightMap[(maxy * GetRealWidth()) + maxx];

	// rise over run
	return (fabs((float)(BR - TL)) * mTerxelSize[2]) / mTerxelSize[0];
}

void CM_CircularIterate(byte *data, int width, int height, int xo, int yo, int insideRadius, int outsideRadius, int *user, cm_iterateFunc callback)
{
	int		x, y, offset;
	byte	*work;

	for(y = -outsideRadius; y < outsideRadius + 1; y++)
	{
		if(y + yo >= 0 && y + yo < height)
		{
			offset = sqrtf((outsideRadius * outsideRadius) - (y * y));
			for(x = -offset; x < offset + 1; x++)
			{
		 		if(x + xo >= 0 && x + xo < width)
		 		{
					float radius = sqrt((float)(x*x+y*y));

					if ( radius >= insideRadius )
					{
		 				work = data + (x + xo) + ((y + yo) * width);
		 				callback( work, (radius - (float)insideRadius) / (float)(outsideRadius - insideRadius), user);
					}
		 		}
			}
		}
	}
}

void CM_ForceHeight( byte *work, float lerp, int *user)
{
	*work = (byte)Com_Clamp(0, 255, (int)*user);
}


void CM_GetAverage( byte *work, float lerp, int *user)
{
	user[0] += *work;
	user[1]++;
}

void CM_Smooth ( byte* work, float lerp, int *user )
{
	float smooth = sin ( M_PI/2*3 + (1.0f-lerp) * (M_PI / 2) ) + 1.0f;
//	float smooth = (1.0f - lerp);

	*work = *work + (int)((float)(*user - *work) * smooth);
}

void CM_MakeAverage( byte *work, float lerp, int *user)
{
	int		height, diff;

	height = (int)*work;
	diff = *user - height;
	if(abs(diff) > 3)
	{
		diff >>= 2;
	}
	height += diff;
	*work = (byte)Com_Clamp(0, 255, height);
}

void CCMLandScape::SaveArea(CArea *area)
{
	mAreas.push_back(area);
}

void CCMLandScape::CarveLine ( vec3_t start, vec3_t end, int depth, int width )
{
	int	x, x1, x2, deltax;
	int	y, y1, y2, deltay;
	int	xinc1, xinc2;
	int yinc1, yinc2;
	int	den, num;
	int count, add;
	int	i;
	float	heightStart;
	float	heightEnd;
	float	heightStep;

	x1 = (int) start[0];
	y1 = (int) start[1];
	x2 = (int) end[0];
	y2 = (int) end[1];	

	deltax = abs(x2 - x1);
	deltay = abs(y2 - y1);
	x = x1;
	y = y1;

	// The x-values are increasing
	if (x2 >= x1)
	{
		xinc1 = 1;
		xinc2 = 1;
	}
	// The x-values are decreasing
	else                          
	{
		xinc1 = -1;
		xinc2 = -1;
	}

	// The y-values are increasing
	if (y2 >= y1)                 
	{
		yinc1 = 1;
		yinc2 = 1;
	}
	// The y-values are decreasing
	else                          
	{
		yinc1 = -1;
		yinc2 = -1;
	}

	if (deltax >= deltay)         // There is at least one x-value for every y-value
	{
		xinc1 = 0;                  // Don't change the x when numerator >= denominator
		yinc2 = 0;                  // Don't change the y for every iteration
		den   = deltax;
		num   = deltax / 2;
		add   = deltay;
		count = deltax;				// There are more x-values than y-values
	}
	else                          // There is at least one y-value for every x-value
	{
		xinc2 = 0;                  // Don't change the x for every iteration
		yinc1 = 0;                  // Don't change the y when numerator >= denominator
		den   = deltay;
		num   = deltay / 2;
		add   = deltax;
		count = deltay;         // There are more y-values than x-values
	}

	vec3_t pt;
	vec3_t bounds[2] = {{-1,-1,-1},{1,1,1}};
	
	pt[0] = start[0];
	pt[1] = start[1];
	GetWorldHeight ( pt, bounds, false );
	heightStart = pt[2];

	pt[0] = end[0];
	pt[1] = end[1];
	GetWorldHeight ( pt, bounds, false );
	heightEnd = pt[2];

	heightStep = (heightEnd-heightStart) / count;

	for ( i = 0; i <= count; i++ )
	{
		// Flatten the current location
		CArea area;

		pt[0] = x;
		pt[1] = y;
		area.Init ( pt, width / 2 + (irand(0, width/2)) );
	 	FlattenArea ( &area, heightStart + (heightStep * i) - (depth/2 - (irand(0, depth/2))), false, true, true );

		// Increase the numerator by the top of the fraction
		num += add;					
  
		if (num >= den)             
		{
			// Calculate the new numerator value
			num -= den;
			
			// Change the x and y as appropriate
			x += xinc1;
			y += yinc1;
		}
		
		// Change the x and y as appropriate
		x += xinc2;                 
		y += yinc2;                 
	}
}

void CCMLandScape::CarveBezierCurve ( int numCtlPoints, vec3_t* ctlPoints, int steps, int depth, int size )
{
	int		i;
	int		choose;
	int		n;
	float	u;
	float	t;
	float	tt;
	float	t1;
	float	step;
	vec3_t	pt;
	vec3_t  lastpt;
	vec3_t	b[10];

	n      = numCtlPoints - 1;
	choose = 1;

	for ( i = 1; i <= n; i ++ )
	{
		if ( i == 1 )
			choose = n;
		else
			choose = choose * (n-i+1) / i;

		(*(ctlPoints+i))[0] *= choose;
		(*(ctlPoints+i))[1] *= choose;
	}
	
	step = 1.0f / (float)steps;
	for ( choose = 0, t = step; t < 1; t += step, choose++ )
	{
		b[0][0] = (*(ctlPoints+0))[0];
		b[0][1] = (*(ctlPoints+0))[1];

		for ( u = t, i = 1; i <= n; i ++ )
		{
			b[i][0] = (*(ctlPoints+i))[0] * u;
			b[i][1] = (*(ctlPoints+i))[1] * u;

			u = u * t;
		}

		pt[0] = b[n][0];
		pt[1] = b[n][1];

		t1 = 1 - t;
		tt = t1;

		for ( i = n - 1; i >= 0; i -- )
		{
			pt[0] += b[i][0] * tt;
			pt[1] += b[i][1] * tt;

			tt = tt * t1;
		}

		if ( choose != 0 )
		{
			CarveLine ( lastpt, pt, depth, size );
		}

		// Save this point for next time around
		lastpt[0] = pt[0];
		lastpt[1] = pt[1];
	}
}

void CCMLandScape::FlattenArea(CArea *area, int height, bool save, bool forceHeight, bool smooth )
{
	vec3_t	temp;
	ivec3_t	icoords;
	int		radius;
	int		height2;

	if(save)
	{
		SaveArea(area);
	//	mAreas.push_back(*area);
	}

	// Work out coords in the heightmap
	VectorSubtract(area->GetPosition(), mBounds[0], temp);
	icoords[0] = temp[0] / (mBounds[1][0] - mBounds[0][0]) * (float)GetRealWidth ( );
	icoords[1] = temp[1] / (mBounds[1][1] - mBounds[0][1]) * (float)GetRealHeight ( );

//	VectorInverseScaleVector(temp, mTerxelSize, icoords);

	// round up, we'd rather have a little more area flattened than have less then what was requested
	radius = (int)ceilf( (area->GetRadius() / mTerxelSize[1]) );

	// Work out the average height of the surrounding terrain
	height2 = height;
	if(height < 0)
	{
		ivec3_t info;

		info[0] = 0;
		info[1] = 0;
		CM_CircularIterate(mHeightMap, GetRealWidth(), GetRealHeight(), icoords[0], icoords[1], 0, radius, info, CM_GetAverage);
		if(info[1])
		{
			height = info[0] / info[1];
		}
	}
	else
	{
		height = height & 0x7F;
	}

	if ( smooth )
	{
		CM_CircularIterate(mHeightMap, GetRealWidth(), GetRealHeight(), icoords[0], icoords[1], radius, radius * 3, &height, CM_Smooth);
	}

	if ( forceHeight )
	{		
		CM_CircularIterate(mHeightMap, GetRealWidth(), GetRealHeight(), icoords[0], icoords[1], 0, radius + 1, &height, CM_ForceHeight );
		CM_CircularIterate(mFlattenMap, GetRealWidth(), GetRealHeight(), icoords[0], icoords[1], 0, radius + 1, &height2, CM_ForceHeight );
	}
	else if ( smooth ) 
	{
		CM_CircularIterate(mHeightMap, GetRealWidth(), GetRealHeight(), icoords[0], icoords[1], 0, radius, &height, CM_Smooth);
	}
}

void CM_BelowLevel(byte *data, float lerp, int *info)
{
	info[1]++;
	if(*data < info[2])
	{
		info[0]++;
	}
}

float CCMLandScape::FractionBelowLevel(CArea *area, int height)
{
	vec3_t	temp;
	ivec3_t	icoords, info;
	int		count;
	float	level;

	// Work out coords in the heightmap
	VectorSubtract(area->GetPosition(), mBounds[0], temp);
	VectorInverseScaleVector(temp, mTerxelSize, icoords);

	// Work out radius of area in heightmap entries
	count = area->GetRadius() / mTerxelSize[1];

	info[0] = 0;
	info[1] = 0;

	info[2] = height;
	if(height < 0)
	{
		info[2] = mBaseWaterHeight;
	}
	CM_CircularIterate(mHeightMap, GetRealWidth(), GetRealHeight(), icoords[0], icoords[1], 0, count, info, CM_BelowLevel);

	level = 0.0f;
	if(info[1])
	{
		level = (float)info[0] / info[1];
	}

	return(level);
}

CArea *CCMLandScape::GetFirstArea(void)
{
	if(!mAreas.size())
	{
		return(NULL);
	}
	mAreasIt = mAreas.begin();
	return (*mAreasIt);
}

CArea *CCMLandScape::GetFirstObjectiveArea(void)
{
	if(!mAreas.size())
	{
		return(NULL);
	}
	mAreasIt = mAreas.begin();

	while (mAreasIt != mAreas.end())
	{
		// run through the areas to find the player area
		if((*mAreasIt)->GetType() == AT_OBJECTIVE)
		{
			return (*mAreasIt);
		}
		++mAreasIt;
	}
	return(NULL);
}

CArea *CCMLandScape::GetPlayerArea(void)
{ // do me
	if(!mAreas.size())
	{
		return(NULL);
	}
	mAreasIt = mAreas.begin();

	while (mAreasIt != mAreas.end())
	{
		// run through the areas to find the player area
		if((*mAreasIt)->GetType() == AT_PLAYER)
		{
			return (*mAreasIt);
		}
		++mAreasIt;
	}
	return(NULL);
}

CArea *CCMLandScape::GetNextArea(void)
{
	if(++mAreasIt == mAreas.end())
	{
		return(NULL);
	}
	return (*mAreasIt);
}

CArea *CCMLandScape::GetNextObjectiveArea(void)
{
	++mAreasIt;

	while (mAreasIt != mAreas.end())
	{
		// run through the areas to find the player area
		if((*mAreasIt)->GetType() == AT_OBJECTIVE)
		{
			return (*mAreasIt);
		}
		++mAreasIt;
	}
	return(NULL);
}

bool CCMLandScape::AreaCollision(CArea *area, int *areaTypes, int areaTypeCount)
{
	CArea	*areas;
	int		i;
	float	segment;
	bool	collision;

	areas = GetFirstArea();
	while(areas)
	{
		collision = false;

		if(area->GetVillageID() == areas->GetVillageID())
		{
			// Check for being too close angularly
			if(area->GetAngleDiff() && areas->GetAngleDiff())
			{
				segment = areas->GetAngle() - area->GetAngle();
				if(segment < M_PI)
				{
					segment += 2 * M_PI;
				}						
				if(segment > M_PI)
				{
					segment -= 2 * M_PI;
				}						
				if(fabsf(segment) < areas->GetAngleDiff() + area->GetAngleDiff())
				{
					collision = true;
				}
			}
		}

		// Check for buildings being too close together
		if(Distance(areas->GetPosition(), area->GetPosition()) < areas->GetRadius() + area->GetRadius())
		{
			collision = true;
		}

		if(collision)
		{
			// If no area type list was specified then all areas are fair game
			if ( !areaTypes )
			{
				return true;
			}

			for(i = 0; i < areaTypeCount; i++)
			{
				if(areas->GetType() == areaTypes[i])
				{
					return(true);
				}
			}
		}
		areas = GetNextArea();
	}
	return(false);
}

void CCMLandScape::rand_seed(int seed)
{
	holdrand = seed;
	Com_Printf("rand_seed = %d\n", holdrand);
} 

float CCMLandScape::flrand(float min, float max)
{
	float	result;

	assert((max - min) < 32768);

	holdrand = (holdrand * 214013L) + 2531011L;
	result = (float)(holdrand >> 17);						// 0 - 32767 range
	result = ((result * (max - min)) / 32768.0F) + min;
//	Com_Printf("flrand: Seed = %d\n", holdrand);

	return(result);
}

int CCMLandScape::irand(int min, int max)
{
	int		result;

	assert((max - min) < 32768);

	max++;
	holdrand = (holdrand * 214013L) + 2531011L;
	result = holdrand >> 17;
	result = ((result * (max - min)) >> 15) + min;
//	Com_Printf("irand: Seed = %d\n", holdrand);

	return(result);
}

CCMLandScape::~CCMLandScape(void)
{
	if(mHeightMap) 
	{ 
		Z_Free(mHeightMap); 
		mHeightMap = NULL;
	}
	if(mFlattenMap)
	{
		Z_Free(mFlattenMap);
		mFlattenMap = NULL;
	}
	if(mPatchBrushData)
	{
		Z_Free(mPatchBrushData);
		mPatchBrushData = NULL;
	}
	if(mPatches) 
	{ 
		Z_Free(mPatches); 
		mPatches = NULL;
	}
	if (mRandomTerrain)
	{
		delete mRandomTerrain;
	}

	for(mAreasIt=mAreas.begin(); mAreasIt != mAreas.end(); ++mAreasIt)
	{
		delete (*mAreasIt);
	}

	mAreas.clear();
}

class CCMLandScape *CM_InitTerrain(const char *configstring, thandle_t terrainId, bool server)
{
	CCMLandScape	*ls;

	ls = new CCMLandScape(configstring, server);
	ls->SetTerrainId(terrainId);

	return(ls);
}

void CM_TerrainPatchIterate(const class CCMLandScape *landscape, void (*IterateFunc)( CCMPatch *, void * ), void *userdata)
{
	landscape->TerrainPatchIterate(IterateFunc, userdata);
}

float CM_GetWorldHeight(const CCMLandScape *landscape, vec3_t origin, const vec3pair_t bounds, bool aboveGround)
{
	return landscape->GetWorldHeight(origin, bounds, aboveGround);
}

void CM_FlattenArea(CCMLandScape *landscape, CArea *area, int height, bool save, bool forceHeight, bool smooth )
{
	landscape->FlattenArea(area, height, save, forceHeight, smooth );
}

void CM_CarveBezierCurve(CCMLandScape *landscape, int numCtls, vec3_t* ctls, int steps, int depth, int size )
{
	landscape->CarveBezierCurve(numCtls, ctls, steps, depth, size );
}

void CM_SaveArea(CCMLandScape *landscape, CArea *area)
{
	landscape->SaveArea(area);
}

float CM_FractionBelowLevel(CCMLandScape *landscape, CArea *area, int height)
{
	return(landscape->FractionBelowLevel(area, height));
}

bool CM_AreaCollision(class CCMLandScape *landscape, class CArea *area, int *areaTypes, int areaTypeCount)
{
	return(landscape->AreaCollision(area, areaTypes, areaTypeCount));
}

CArea *CM_GetFirstArea(CCMLandScape *landscape)
{
	return(landscape->GetFirstArea());
}

CArea *CM_GetFirstObjectiveArea(CCMLandScape *landscape)
{
	return(landscape->GetFirstObjectiveArea());
}

CArea *CM_GetPlayerArea(CCMLandScape *landscape)
{
	return(landscape->GetPlayerArea());
}

CArea *CM_GetNextArea(CCMLandScape *landscape)
{
	return(landscape->GetNextArea());
}

CArea *CM_GetNextObjectiveArea(CCMLandScape *landscape)
{
	return(landscape->GetNextObjectiveArea());
}

CRandomTerrain *CreateRandomTerrain(const char *config, CCMLandScape *landscape, byte *heightmap, int width, int height)
{
	CRandomTerrain	*RandomTerrain = 0;

	char			*ptr;
	unsigned long	seed;

	seed = strtoul(Info_ValueForKey(config, "seed"), &ptr, 10);

	landscape->rand_seed(seed);
	
	RandomTerrain = new CRandomTerrain;
	RandomTerrain->Init(landscape, heightmap, width, height);

/*
	RandomTerrain->CreatePath(0, -1, 0, 9, 0.1, 0.5, 0.5, 0.5, 0.05, 0.08, 0.31, 0.1, 3);
	RandomTerrain->CreatePath(1, 0, 0, 6, 0.5, 0.5, 0.9, 0.1, 0.08, 0.1, 0.31, 0.1, 0.9);
	RandomTerrain->CreatePath(2, 0, 0, 6, 0.5, 0.5, 0.9, 0.9, 0.08, 0.1, 0.31, 0.1, 0.9);

	RandomTerrain->Generate();
*/

	return RandomTerrain;
}


// end

#if defined(_WIN32) && defined(_MSC_VER) && (_MSC_VER < 1600)
#pragma optimize("p", off)
#endif
