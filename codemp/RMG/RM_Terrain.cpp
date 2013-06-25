//Anything above this #include will be ignored by the compiler
#include "qcommon/exe_headers.h"

#include "qcommon/cm_local.h"
#include "renderer/tr_types.h"
#include "RM_Headers.h"

#ifdef _MSC_VER
#pragma optimize("", off)

// The above optmization triggers this warning:
// "/GS can not protect parameters and local variables from local buffer overrun because optimizations are disabled in function"
// We don't give a rats ass.
#pragma warning(disable: 4748)
#endif

static CRMLandScape		*rm_landscape;
static CCMLandScape		*origin_land;

CRMLandScape::CRMLandScape(void)
{
	common = NULL;
	mDensityMap = NULL;
}

CRMLandScape::~CRMLandScape(void)
{
	if(mDensityMap)
	{
		Z_Free(mDensityMap);
		mDensityMap = NULL;
	}
}

void CCGHeightDetails::AddModel(const CRandomModel *hd)
{
	if(mNumModels < MAX_RANDOM_MODELS)
	{
		mTotalFrequency += hd->GetFrequency();
		mModels[mNumModels++] = *hd;
	}
}

void CRMLandScape::AddModel(const int height, int maxheight, const CRandomModel *hd)
{
	int		i;

	if(maxheight > HEIGHT_RESOLUTION)
	{
		maxheight = HEIGHT_RESOLUTION;
	}

	for(i = height; hd->GetModel() && (i < maxheight); i++)
	{
		mHeightDetails[i].AddModel(hd);
	}
}

void CRMLandScape::LoadMiscentDef(const char *td)
{
	char				miscentDef[MAX_QPATH];
	CGenericParser2		parse;
	CGPGroup			*basegroup, *classes, *items, *model;
	CGPValue			*pair;

	Com_sprintf(miscentDef, MAX_QPATH, "ext_data/RMG/%s.miscents", Info_ValueForKey(td, "miscentDef"));
	Com_DPrintf("CG_Terrain: Loading and parsing miscentDef %s.....\n", Info_ValueForKey(td, "miscentDef"));

	if(!Com_ParseTextFile(miscentDef, parse))
	{
		Com_sprintf(miscentDef, MAX_QPATH, "ext_data/arioche/%s.miscents", Info_ValueForKey(td, "miscentDef"));
		if(!Com_ParseTextFile(miscentDef, parse))
		{
			Com_Printf("Could not open %s\n", miscentDef);
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
			if(!stricmp(items->GetName(), "miscent"))
			{
				int			height, maxheight;

				// Height must exist - the rest are optional
				height = atol(items->FindPairValue("height", "0"));
				maxheight = atol(items->FindPairValue("maxheight", "255"));

				model = items->GetSubGroups();
				while(model)
				{
					if(!stricmp(model->GetName(), "model"))
					{
						CRandomModel	hd;

						// Set defaults
						hd.SetModel("");
						hd.SetFrequency(1.0f);
						hd.SetMinScale(1.0f);
						hd.SetMaxScale(1.0f);

						pair = model->GetPairs();
						while(pair)
						{
							if(!stricmp(pair->GetName(), "name"))
							{
								hd.SetModel(pair->GetTopValue());
							}
							else if(!stricmp(pair->GetName(), "frequency"))
							{
								hd.SetFrequency((float)atof(pair->GetTopValue()));
							}
							else if(!stricmp(pair->GetName(), "minscale"))
							{
								hd.SetMinScale((float)atof(pair->GetTopValue()));
							}
							else if(!stricmp(pair->GetName(), "maxscale"))
							{
								hd.SetMaxScale((float)atof(pair->GetTopValue()));
							}
							pair = (CGPValue *)pair->GetNext();
						}
						AddModel(height, maxheight, &hd);
					}
 					model = (CGPGroup *)model->GetNext();
				}
			}
			items = (CGPGroup *)items->GetNext();
		}
		classes = (CGPGroup *)classes->GetNext();
	}
	Com_ParseTextFileDestroy(parse);
}

void CG_Decrease(byte *work, float lerp, int *info)
{
	int		val;

	val = *work - origin_land->irand(2, 5);
	*work = (byte)Com_Clampi(1, 255, val);
}

void CRMLandScape::CreateRandomDensityMap(byte *density, int width, int height, int seed)
{
//	int			i, border, inc;
	int			x, y, count;
//	byte		*work, *work2;
	CArea		*area;
	vec3_t		derxelSize, pos;
	ivec3_t		dmappos;
	byte		*hm_map = common->GetHeightMap();
	int			hm_width = common->GetRealWidth();
	int			hm_height = common->GetRealHeight();
	int			xpos, ypos, dx, dy;
	byte		*densityPos = density;
	bool		foundUneven;

	// Init to linear spread
	memset(density, 0, width * height);

/*	// Make more prevalent towards the edges
	border = Com_Clamp(6, 12, (width + height) >> 4);

	for(i = 0; i < border; i++)
	{
		inc = (border - i + 1) * 9;

		// Top line
		work = density + i + (i * width);
		for(x = i; x < width - i; x++, work++)
		{
			*work += (byte)common->irand(inc >> 1, inc);
		}

		// Left and right edges
		work = density + i + ((i + 1) * width);
		work2 = density + (width - i) + ((i + 1) * width);
		for(y = i + 1; y < height - i - 2; y++, work += width, work2 += width)
		{
			*work += (byte)common->irand(inc >> 1, inc);
			*work2 += (byte)common->irand(inc >> 1, inc);
		}

		// Bottom line
		work = density + i + ((height - i - 1) * width);
		for(x = i; x < width - i; x++, work++)
		{
			*work += (byte)common->irand(inc >> 1, inc);
		}
	}
*/
	count = 0;

	for(y=0;y<height;y++)
	{
		for(x=0;x<width;x++,densityPos++)
		{
			xpos = (x * hm_width / width);
			ypos = (y * hm_height / height);
			ypos = hm_height - ypos - 1;

			if (hm_map[ypos*hm_width + xpos] < 150)
			{
				continue;
			}

			foundUneven = false;
			for(dx=-4;(dx<=4 && !foundUneven);dx++)
			{
				for(dy=-4;(dy<=4 && !foundUneven);dy++)
				{
					if (dx == 0 && dy == 0)
					{
						continue;
					}
					if ((xpos+dx) >= 0 && (xpos+dx) < hm_width && (ypos+dy) >= 0 && (ypos+dy) < hm_height)
					{
						if (hm_map[(ypos+dy)*hm_width + (xpos+dx)] < 190)
						{
							*densityPos = 205;
							count++;
							foundUneven = true;
						}
					}
				}
			}
		}
	}

/*	FILE	*FH;

	FH = fopen("c:\o.raw", "wb");
	fwrite(hm_map, 1, common->GetRealWidth() * common->GetRealHeight(), FH);
	fclose(FH);

	FH = fopen("c:\d.raw", "wb");
	fwrite(density, 1, width*height, FH);
	fclose(FH);
*/
	// Reduce severely for any settlements/buildings/objectives
	VectorScale(common->GetSize(), 1.0f / width, derxelSize);

	origin_land = common;
	area = common->GetFirstArea();
	while(area)
	{
		// Skip group types since they encompass to much open area
		if ( area->GetType ( ) == AT_GROUP )
		{
			area = common->GetNextArea();
			continue;
		}

		VectorSubtract(area->GetPosition(), common->GetMins(), pos);
		VectorInverseScaleVector(pos, derxelSize, dmappos);
		// Damn upside down gensurf
		dmappos[1] = height - dmappos[1];

		count = ceilf(area->GetRadius() / derxelSize[1]);

		while(count > 0)
		{
			CM_CircularIterate(density, width, height, dmappos[0], dmappos[1], 0, count, NULL, CG_Decrease);
			count--;
		}
		area = common->GetNextArea();
	}
}

void CRMLandScape::LoadDensityMap(const char *td)
{
	char		densityMap[MAX_QPATH];
	byte		*imageData;
#ifndef DEDICATED
	int			iWidth, iHeight, seed;
	char 		*ptr;
#endif

	// Fill in with default values
	mDensityMap = (byte *)Z_Malloc(common->GetBlockCount(), TAG_TERRAIN);
	memset(mDensityMap, 128, common->GetBlockCount());

	// Load in density map (if any)
	Com_sprintf(densityMap, MAX_QPATH, "%s", Info_ValueForKey(td, "densityMap"));
	if(strlen(densityMap))
	{
		Com_DPrintf("CG_Terrain: Loading density map %s.....\n", densityMap);
#ifdef DEDICATED
		imageData = NULL;
#else
		re.LoadDataImage(densityMap, &imageData, &iWidth, &iHeight);
		if(imageData)
		{
			if(strstr(densityMap, "density_"))
			{
				seed = strtoul(Info_ValueForKey(td, "seed"),&ptr,10);
				CreateRandomDensityMap(imageData, iWidth, iHeight, seed);
			}
			re.Resample(imageData, iWidth, iHeight, mDensityMap, common->GetBlockWidth(), common->GetBlockHeight(), 1);
			re.InvertImage(mDensityMap, common->GetBlockWidth(), common->GetBlockHeight(), 1);
			Z_Free(imageData);
		}
#endif
	}
}

CRandomModel *CCGHeightDetails::GetRandomModel(CCMLandScape *land)
{
	int		seek, i;

	seek = land->irand(0, mTotalFrequency);
	for(i = 0; i < mNumModels; i++)
	{
		seek -= mModels[i].GetFrequency();
		if(seek <= 0)
		{
			return(mModels + i);
		}
	}
	assert(0);
	return(NULL);
}
#ifndef DEDICATED
void CRMLandScape::Sprinkle(CCMPatch *patch, CCGHeightDetails *hd, int level)
{
	int				i, count, px, py;
	float			density;
	vec3_t			origin, scale, angles, bounds[2];
	refEntity_t		refEnt;
	CRandomModel	*rm;
	CArea			area;
//	int				areaTypes[] = { AT_BSP, AT_OBJECTIVE };
	TCGMiscEnt		*data = (TCGMiscEnt *)cl.mSharedMemory;
	TCGTrace		*td = (TCGTrace *)cl.mSharedMemory;

//	memset(&refEnt, 0, sizeof(refEntity_t));

	px = patch->GetHeightMapX() / common->GetTerxels();
	py = patch->GetHeightMapY() / common->GetTerxels();
	// Get a number -5.3f to 5.3f
	density = (mDensityMap[px + (common->GetBlockWidth() * py)] - 128) / 24.0f;
	// ..and multiply that into the count
	count = Round(common->GetPatchScalarSize() * hd->GetAverageFrequency() * powf(2.0f, density) * 0.001);

	for(i = 0; i < count; i++)
	{
		if(!common->irand(0, 10))
		{
			vec3_t temp;
			float  average;

			rm = hd->GetRandomModel(common);

			refEnt.hModel = re.RegisterModel(rm->GetModelName());
			refEnt.frame = 0;
			re.ModelBoundsRef(&refEnt, bounds[0], bounds[1]);

			// Calculate the scale using some magic to help ensure that the
			// scales are never too different from eachother.  Otherwise you
			// could get an entity that is really small on one axis but huge 
			// on another.
			temp[0] = common->flrand(rm->GetMinScale(), rm->GetMaxScale());
			temp[1] = common->flrand(rm->GetMinScale(), rm->GetMaxScale());
			temp[2] = common->flrand(rm->GetMinScale(), rm->GetMaxScale());

			// Average of the three random numbers and divide that by two
			average = ( ( temp[0] + temp[1] + temp[2] ) / 3) / 2;

			// Add in half of the other two numbers and then subtract half the average to prevent.
			// any number from going beyond the range. If all three numbers were the same then
			// they would remain unchanged after this calculation.
			scale[0] = temp[0] + (temp[1]+temp[2]) / 2 - average;
			scale[1] = temp[1] + (temp[0]+temp[2]) / 2 - average;
			scale[2] = temp[2] + (temp[0]+temp[1]) / 2 - average;

			angles[0] = 0.0f;
			angles[1] = common->flrand(-M_PI, M_PI);
			angles[2] = 0.0f;

			VectorCopy(patch->GetMins(), origin);
			origin[0] += common->flrand(0.0f, common->GetPatchWidth());
			origin[1] += common->flrand(0.0f, common->GetPatchHeight());
			// Get above world height
			float slope = common->GetWorldHeight(origin, bounds, true);

			if (slope > 1.33)
			{	// spot has too steep of a slope
				continue;
			}
			if(origin[2] < common->GetWaterHeight())
			{
				continue;
			}
			// very that we aren't dropped too low
			if (origin[2] < common->CalcWorldHeight(level))
			{
				continue;
			}

			// Hack-ariffic, don't allow them to drop below the big player clip brush.
			if (origin[2] < 1280 )
			{
				continue;
			}
			// FIXME: shouldn't be using a hard-coded 1280 number, only allow to spawn if inside player clip brush? 
	//		if( !(CONTENTS_PLAYERCLIP & VM_Call( cgvm, CG_POINT_CONTENTS )) )
	//		{
	//			continue;
	//		}
			// Simple radius check for buildings
/*			area.Init(origin, VectorLength(bounds[0]));
			if(common->AreaCollision(&area, areaTypes, sizeof(areaTypes) / sizeof(int)))
			{
				continue;
			}*/
			// Make sure there is no architecture around - doesn't work for ents though =(

			memset( td, 0, sizeof( *td ) );
			VectorCopy(origin, td->mStart);
			VectorCopy(bounds[0], td->mMins);
			VectorCopy(bounds[1], td->mMaxs);
			VectorCopy(origin, td->mEnd);
			td->mSkipNumber = -1;
			td->mMask = MASK_PLAYERSOLID;

			VM_Call( cgvm, CG_TRACE );
			if(td->mResult.surfaceFlags & SURF_NOMISCENTS)
			{
				continue;
			}
			if(td->mResult.startsolid)
			{
//				continue;
			}
			// Get minimum height of area
			common->GetWorldHeight(origin, bounds, false);
			// Account for relative origin
			origin[2] -= bounds[0][2] * scale[2];
			origin[2] -= common->flrand(2.0, (bounds[1][2] - bounds[0][2]) / 4);
			
			// Spawn the client model
			strcpy(data->mModel, rm->GetModelName());
			VectorCopy(origin, data->mOrigin);
			VectorCopy(angles, data->mAngles);
			VectorCopy(scale, data->mScale);
			VM_Call( cgvm, CG_MISC_ENT);
			mModelCount++;
		}
	}
}
#endif // !DEDICATED

void CRMLandScape::SpawnPatchModels(CCMPatch *patch)
{
#ifndef DEDICATED
	int					i;
	CCGHeightDetails	*hd;

//	Rand_Init(10);
	for(i = 0; i < 4; i++)
	{
		hd = mHeightDetails + patch->GetHeight(i);
		if(hd->GetNumModels())
		{
			Sprinkle(patch, hd, patch->GetHeight(i));
		}
	}
#endif // !DEDICATED
}

void SpawnPatchModelsWrapper(CCMPatch *patch, void *userdata)
{
	CRMLandScape *landscape = (CRMLandScape *)userdata;
	landscape->SpawnPatchModels(patch);
}

void RM_CreateRandomModels(int terrainId, const char *terrainInfo)
{
	CRMLandScape	*landscape;

	landscape = rm_landscape = new CRMLandScape;
	landscape->SetCommon(cmg.landScape);

	Com_DPrintf("CG_Terrain: Creating random models.....\n");
	landscape->LoadMiscentDef(terrainInfo);
	landscape->LoadDensityMap(terrainInfo);
	landscape->ClearModelCount();
	CM_TerrainPatchIterate(landscape->GetCommon(), SpawnPatchModelsWrapper, landscape);

	Com_DPrintf(".....%d random client models spawned\n", landscape->GetModelCount());
}

void RM_InitTerrain(void)
{
	rm_landscape = NULL;
}

void RM_ShutdownTerrain(void)
{
	CRMLandScape 	*landscape;

	landscape = rm_landscape;
	if(landscape)
	{
		delete landscape;
		rm_landscape = NULL;
	}
}

// end

#ifdef _MSC_VER
#pragma warning(default: 4748)

#pragma optimize("", on)
#endif
