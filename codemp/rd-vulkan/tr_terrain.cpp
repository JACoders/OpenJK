// this include must remain at the top of every CPP file
#include "tr_local.h"

#include "qcommon/GenericParser2.h"

// To do:
// Alter variance dependent on global distance from player (colour code this for cg_terrainCollisionDebug)
// Improve texture blending on edge conditions
// Link to neightbouring terrains or architecture (edge conditions)
// Post process generated light data to make sure there are no bands within a patch

#include "qcommon/cm_landscape.h"
#include "tr_landscape.h"

cvar_t		*r_drawTerrain;
cvar_t		*r_showFrameVariance;
cvar_t		*r_terrainTessellate;
cvar_t		*r_terrainWaterOffset;

static int		TerrainFog = 0;
static float	TerrainDistanceCull;

//
// Render the tree.
//
void CTRPatch::RenderCorner(ivec5_t corner)
{
	if((corner[3] < 0) || (tess.registration != corner[4]))
	{
		CTerVert	*vert;

		vert = mRenderMap + (corner[1] * owner->GetRealWidth()) + corner[0];

		VectorCopy(vert->coords, tess.xyz[tess.numVertexes]);
		VectorCopy(vert->normal, tess.normal[tess.numVertexes]);

		*(ulong *)tess.vertexColors[tess.numVertexes] = *(ulong *)vert->tint;
		*(ulong *)tess.vertexAlphas[tess.numVertexes] = corner[2];

		tess.texCoords[tess.numVertexes][0][0] = vert->tex[0]; //rwwRMG - reverse coords array from sof2
		tess.texCoords[tess.numVertexes][0][1] = vert->tex[1];

		tess.indexes[tess.numIndexes++] = tess.numVertexes;
		corner[3] = tess.numVertexes++;
		corner[4] = tess.registration;
	}
	else
	{
		tess.indexes[tess.numIndexes++] = corner[3];
	}
}

void CTRPatch::RecurseRender(int depth, ivec5_t left, ivec5_t right, ivec5_t apex)
{
	// All non-leaf nodes have both children, so just check for one
	if (depth >= 0)
	{
		ivec5_t		center;
		byte		*centerAlphas;
		byte		*leftAlphas;
		byte		*rightAlphas;

		// Work out the centre of the hypoteneuse
		center[0] = (left[0] + right[0]) >> 1;
		center[1] = (left[1] + right[1]) >> 1;

		// Work out the relevant texture coefficients at that point
		leftAlphas = (byte *)&left[2];
		rightAlphas = (byte *)&right[2];
		centerAlphas = (byte *)&center[2];

		centerAlphas[0] = (leftAlphas[0] + rightAlphas[0]) >> 1;
		centerAlphas[1] = (leftAlphas[1] + rightAlphas[1]) >> 1;
		centerAlphas[2] = (leftAlphas[2] + rightAlphas[2]) >> 1;
		centerAlphas[3] = (leftAlphas[3] + rightAlphas[3]) >> 1;

		// Make sure the vert index and tesselation registration are not set
		center[3] = -1;
		center[4] = 0;

		if (apex[0] == left[0] && apex[0] == center[0])
		{
			depth = 0;
		}

		RecurseRender(depth-1, apex, left, center);
		RecurseRender(depth-1, right, apex, center);
	}
	else
	{
		if (left[0] == right[0] && left[0] == apex[0])
		{
			return;
		}
		if (left[1] == right[1] && left[1] == apex[1])
		{
			return;
		}
		// A leaf node!  Output a triangle to be rendered.
		RB_CheckOverflow(4, 4);

//		assert(left[0] != right[0] || left[1] != right[1]);
//		assert(left[0] != apex[0] || left[1] != apex[1]);

	  	RenderCorner(left);
		RenderCorner(right);
		RenderCorner(apex);
	}
}

//
// Render the mesh.
//
// The order of triangles is critical to the subdivision working

void CTRPatch::Render(int Part)
{
	ivec5_t		TL, TR, BL, BR;

	VectorSet5M(TL, 0, 0, TEXTURE_ALPHA_TL, -1, 0);
	VectorSet5M(TR, owner->GetTerxels(), 0, TEXTURE_ALPHA_TR, -1, 0);
	VectorSet5M(BL, 0, owner->GetTerxels(), TEXTURE_ALPHA_BL, -1, 0);
	VectorSet5M(BR, owner->GetTerxels(), owner->GetTerxels(), TEXTURE_ALPHA_BR, -1, 0);

	if ((Part & PI_TOP) && mTLShader)
	{
/*		float		d;

		d = DotProduct (backEnd.refdef.vieworg, mNormal[0]) - mDistance[0];

		if (d <= 0.0)*/
		{
			RecurseRender(r_terrainTessellate->integer, BL, TR, TL);
		}
	}

	if ((Part & PI_BOTTOM) && mBRShader)
	{
/*		float		d;

		d = DotProduct (backEnd.refdef.vieworg, mNormal[1]) - mDistance[1];

		if (d >= 0.0)*/
		{
			RecurseRender(r_terrainTessellate->integer, TR, BL, BR);
		}
	}
}

//
// At this point the patch is visible and at least part of it is below water level
//
int CTRPatch::RenderWaterVert(int x, int y)
{
	CTerVert	*vert;

	vert = mRenderMap + x + (y * owner->GetRealWidth());

	if(vert->tessRegistration == tess.registration)
	{
		return(vert->tessIndex);
	}
	tess.xyz[tess.numVertexes][0] = vert->coords[0];
	tess.xyz[tess.numVertexes][1] = vert->coords[1];
	tess.xyz[tess.numVertexes][2] = owner->GetWaterHeight();

	*(ulong *)tess.vertexColors[tess.numVertexes] = 0xffffffff;

	tess.texCoords[tess.numVertexes][0][0] = vert->tex[0]; //rwwRMG - reverse coords from sof2mp
	tess.texCoords[tess.numVertexes][0][1] = vert->tex[1];

	vert->tessIndex = tess.numVertexes;
	vert->tessRegistration = tess.registration;

	tess.numVertexes++;
	return(vert->tessIndex);
}

void CTRPatch::RenderWater(void)
{
	RB_CheckOverflow(4, 6);

	// Get the neighbouring patches
	int TL = RenderWaterVert(0, 0);
	int TR = RenderWaterVert(owner->GetTerxels(), 0);
	int BL = RenderWaterVert(0, owner->GetTerxels());
	int BR = RenderWaterVert(owner->GetTerxels(), owner->GetTerxels());

	// TL
	tess.indexes[tess.numIndexes++] = BL;
	tess.indexes[tess.numIndexes++] = TR;
	tess.indexes[tess.numIndexes++] = TL;

	// BR
	tess.indexes[tess.numIndexes++] = TR;
	tess.indexes[tess.numIndexes++] = BL;
	tess.indexes[tess.numIndexes++] = BR;
}

const bool CTRPatch::HasWater(void) const
{
	owner->SetRealWaterHeight( owner->GetBaseWaterHeight() + r_terrainWaterOffset->integer );
	return(common->GetMins()[2] < owner->GetWaterHeight());
}

void CTRPatch::SetVisibility(bool visCheck)
{
	if(visCheck)
	{
		if(DistanceSquared(mCenter, backEnd.refdef.vieworg) > TerrainDistanceCull)
		{
			misVisible = false;
		}
		else
		{
			// Set the visibility of the patch
			misVisible = !ri.CM_CullWorldBox(backEnd.viewParms.frustum, GetBounds());
		}
	}
	else
	{
		misVisible = true;
	}
}

/*
void CTRPatch::CalcNormal(void)
{
	CTerVert	*vert1, *vert2, *vert3;
	ivec5_t		TL, TR, BL, BR;
	vec3_t		v1, v2;

	VectorSet5(TL, 0, 0, TEXTURE_ALPHA_TL, -1, 0);
	VectorSet5(TR, owner->GetTerxels(), 0, TEXTURE_ALPHA_TR, -1, 0);
	VectorSet5(BL, 0, owner->GetTerxels(), TEXTURE_ALPHA_BL, -1, 0);
	VectorSet5(BR, owner->GetTerxels(), owner->GetTerxels(), TEXTURE_ALPHA_BR, -1, 0);

	vert1 = mRenderMap + (BL[1] * owner->GetRealWidth()) + BL[0];
	vert2 = mRenderMap + (TR[1] * owner->GetRealWidth()) + TR[0];
	vert3 = mRenderMap + (TL[1] * owner->GetRealWidth()) + TL[0];
	VectorSubtract(vert2->coords, vert1->coords, v1);
	VectorSubtract(vert3->coords, vert1->coords, v2);
	CrossProduct(v1, v2, mNormal[0]);
	VectorNormalize(mNormal[0]);
	mDistance[0] = DotProduct (vert1->coords, mNormal[0]);

	vert1 = mRenderMap + (BL[1] * owner->GetRealWidth()) + BL[0];
	vert2 = mRenderMap + (TR[1] * owner->GetRealWidth()) + TR[0];
	vert3 = mRenderMap + (BR[1] * owner->GetRealWidth()) + BR[0];
	VectorSubtract(vert2->coords, vert1->coords, v1);
	VectorSubtract(vert3->coords, vert1->coords, v2);
	CrossProduct(v1, v2, mNormal[1]);
	VectorNormalize(mNormal[1]);
	mDistance[1] = DotProduct (vert1->coords, mNormal[1]);
}
*/
//
// Reset all patches, recompute variance if needed
//
void CTRLandScape::Reset(bool visCheck)
{
	int			x, y;
	CTRPatch	*patch;

	TerrainDistanceCull = tr.distanceCull + mPatchSize;
	TerrainDistanceCull *= TerrainDistanceCull;

	// Go through the patches performing resets, compute variances, and linking.
	for(y = mPatchMiny; y < mPatchMaxy; y++)
	{
		for(x = mPatchMinx; x < mPatchMaxx; x++, patch++)
		{
			patch = GetPatch(x, y);
			patch->SetVisibility(visCheck);
		}
	}
}


//
// Render each patch of the landscape & adjust the frame variance.
//

void CTRLandScape::Render(void)
{
	int			x, y;
	CTRPatch	*patch;
	TPatchInfo	*current;
	int			i;

	// Render all the visible patches
	current = mSortedPatches;
	for(i=0;i<mSortedCount;i++)
	{
		if (current->mPatch->isVisible())
		{
			if (tess.shader != current->mShader)
			{
				RB_EndSurface();
				RB_BeginSurface(current->mShader, TerrainFog);
			}
			current->mPatch->Render(current->mPart);
		}
		current++;
	}
	RB_EndSurface();

	// Render all the water for visible patches
	// Done as a separate iteration to reduce the number of tesses created
	if(mWaterShader && (mWaterShader != tr.defaultShader))
	{
		RB_BeginSurface( mWaterShader, tr.world->globalFog );

		for(y = mPatchMiny; y < mPatchMaxy; y++ )
		{
			for(x = mPatchMinx; x < mPatchMaxx; x++ )
			{
				patch = GetPatch(x, y);
				if(patch->isVisible() && patch->HasWater())
				{
					patch->RenderWater();
				}
			}
		}
		RB_EndSurface();
	}
}

void CTRLandScape::CalculateRegion(void)
{
	vec3_t	mins, maxs, size, offset;

#if	_DEBUG
	mCycleCount++;
#endif
	VectorCopy(GetPatchSize(), size);
	VectorCopy(GetMins(), offset);

	mins[0] = backEnd.refdef.vieworg[0] - tr.distanceCull - (size[0] * 2.0f) - offset[0];
	mins[1] = backEnd.refdef.vieworg[1] - tr.distanceCull - (size[1] * 2.0f) - offset[1];

	maxs[0] = backEnd.refdef.vieworg[0] + tr.distanceCull + (size[0] * 2.0f) - offset[0];
	maxs[1] = backEnd.refdef.vieworg[1] + tr.distanceCull + (size[1] * 2.0f) - offset[1];

	mPatchMinx = Com_Clampi(0, GetBlockWidth(), floorf(mins[0] / size[0]));
	mPatchMaxx = Com_Clampi(0, GetBlockWidth(), ceilf(maxs[0] / size[0]));

	mPatchMiny = Com_Clampi(0, GetBlockHeight(), floorf(mins[1] / size[1]));
	mPatchMaxy = Com_Clampi(0, GetBlockHeight(), ceilf(maxs[1] / size[1]));
}

void CTRLandScape::CalculateRealCoords(void)
{
	int			x, y;

	// Work out the real world coordinates of each heightmap entry
	for(y = 0; y < GetRealHeight(); y++)
	{
		for(x = 0; x < GetRealWidth(); x++)
		{
			ivec3_t		icoords;
			int			offset;

			offset = (y * GetRealWidth()) + x;

			VectorSetM(icoords, x, y, mRenderMap[offset].height);
			VectorScaleVectorAdd(GetMins(), icoords, GetTerxelSize(), mRenderMap[offset].coords);
		}
	}
}

void CTRLandScape::CalculateNormals(void)
{
	int		x, y, offset = 0;

	// Work out the normals for every face
	for(y = 0; y < GetHeight(); y++)
	{
		for(x = 0; x < GetWidth(); x++)
		{
			vec3_t		vcenter, vleft;

			offset = (y * GetRealWidth()) + x;

			VectorSubtract(mRenderMap[offset].coords, mRenderMap[offset + 1].coords, vcenter);
			VectorSubtract(mRenderMap[offset].coords, mRenderMap[offset + GetRealWidth()].coords, vleft);

			CrossProduct(vcenter, vleft, mRenderMap[offset].normal);
			VectorNormalize(mRenderMap[offset].normal);
		}
		// Duplicate right edge condition
		VectorCopy(mRenderMap[offset].normal, mRenderMap[offset + 1].normal);
	}
	// Duplicate bottom line
	offset = GetHeight() * GetRealWidth();
	for(x = 0; x < GetRealWidth(); x++)
	{
		VectorCopy(mRenderMap[offset - GetRealWidth() + x].normal, mRenderMap[offset + x].normal);
	}
}

void CTRLandScape::CalculateLighting(void)
{
	int		x, y, offset = 0;

	// Work out the vertex normal (average of every attached face normal) and apply to the direction of the light
	for(y = 0; y < GetHeight(); y++)
	{
		for(x = 0; x < GetWidth(); x++)
		{
			vec3_t		ambient;
			vec3_t		directed, direction;
			vec3_t		total, tint;
			float		dp;

			offset = (y * GetRealWidth()) + x;

			// Work out average normal
		   	VectorCopy(GetRenderMap(x, y)->normal, total);
		   	VectorAdd(total, GetRenderMap(x + 1, y)->normal, total);
		   	VectorAdd(total, GetRenderMap(x + 1, y + 1)->normal, total);
		   	VectorAdd(total, GetRenderMap(x, y + 1)->normal, total);
			VectorNormalize(total);

			if (!R_LightForPoint(mRenderMap[offset].coords, ambient, directed, direction))
			{
				mRenderMap[offset].tint[0] =
					mRenderMap[offset].tint[1] =
					mRenderMap[offset].tint[2] = 255 >> tr.overbrightBits;
				mRenderMap[offset].tint[3] = 255;
				continue;
			}

			if(mRenderMap[offset].coords[2] < common->GetBaseWaterHeight())
			{
				VectorScale(ambient, 0.75f, ambient);
			}

			// Both normalised, so -1.0 < dp < 1.0
			dp = Com_Clampi(0.0f, 1.0f, DotProduct(direction, total));
			dp = powf(dp, 3);
			VectorScale(ambient, (1.0 - dp) * 0.5, ambient);
			VectorMA(ambient, dp, directed, tint);

			mRenderMap[offset].tint[0] = (byte)Com_Clampi(0.0f, 255.0f, tint[0] ) >> tr.overbrightBits;
			mRenderMap[offset].tint[1] = (byte)Com_Clampi(0.0f, 255.0f, tint[1] ) >> tr.overbrightBits;
			mRenderMap[offset].tint[2] = (byte)Com_Clampi(0.0f, 255.0f, tint[2] ) >> tr.overbrightBits;
			mRenderMap[offset].tint[3] = 0xff;

			/*
			mRenderMap[offset].tint[0] += tr.identityLight * 32;
			mRenderMap[offset].tint[1] += tr.identityLight * 32;
			mRenderMap[offset].tint[2] += tr.identityLight * 32;
			*/
		}
		mRenderMap[offset + 1].tint[0] = mRenderMap[offset].tint[0];
		mRenderMap[offset + 1].tint[1] = mRenderMap[offset].tint[1];
		mRenderMap[offset + 1].tint[2] = mRenderMap[offset].tint[2];
		mRenderMap[offset + 1].tint[3] = 0xff;
	}
	// Duplicate bottom line
	offset = GetHeight() * GetRealWidth();
	for(x = 0; x < GetRealWidth(); x++)
	{
		mRenderMap[offset + x].tint[0] = mRenderMap[offset - GetRealWidth() + x].tint[0];
		mRenderMap[offset + x].tint[1] = mRenderMap[offset - GetRealWidth() + x].tint[1];
		mRenderMap[offset + x].tint[2] = mRenderMap[offset - GetRealWidth() + x].tint[2];
		mRenderMap[offset + x].tint[3] = 0xff;
	}
}

void CTRLandScape::CalculateTextureCoords(void)
{
	int		x, y;

	for(y = 0; y < GetRealHeight(); y++)
	{
		for(x = 0; x < GetRealWidth(); x++)
		{
			int offset = (y * GetRealWidth()) + x;

			mRenderMap[offset].tex[0] = x * mTextureScale * GetTerxelSize()[0];
			mRenderMap[offset].tex[1] = y * mTextureScale * GetTerxelSize()[1];
		}
	}
}

void CTRLandScape::SetShaders(const int height, const qhandle_t shader)
{
	int		i;

	for(i = height; shader && (i < HEIGHT_RESOLUTION); i++)
	{
		if(!mHeightDetails[i].GetShader())
		{
			mHeightDetails[i].SetShader(shader);
		}
	}
}

void CTRLandScape::LoadTerrainDef(const char *td)
{
	char			terrainDef[MAX_QPATH];
	CGenericParser2	parse;
	CGPGroup		*basegroup, *classes, *items;

	Com_sprintf(terrainDef, MAX_QPATH, "ext_data/RMG/%s.terrain", td);
	ri.Printf( PRINT_ALL, "R_Terrain: Loading and parsing terrainDef %s.....\n", td);

	mWaterShader = NULL;
	mFlatShader  = NULL_HANDLE;

	if(!Com_ParseTextFile(terrainDef, parse))
	{
		Com_sprintf(terrainDef, MAX_QPATH, "ext_data/arioche/%s.terrain", td);
		if(!Com_ParseTextFile(terrainDef, parse))
		{
			ri.Printf( PRINT_ALL, "Could not open %s\n", terrainDef);
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
			const char* type = items->GetName ( );

			if(!Q_stricmp( type, "altitudetexture"))
			{
				int			height;
				const char	*shaderName;
			  	qhandle_t	shader;

				// Height must exist - the rest are optional
				height = atol(items->FindPairValue("height", "0"));

				// Shader for this height
				shaderName = items->FindPairValue("shader", "");
				if(strlen(shaderName))
				{
					shader = RE_RegisterShader(shaderName);
					if(shader)
					{
						SetShaders(height, shader);
					}
				}
			}
			else if(!Q_stricmp(type, "water"))
			{
				mWaterShader = R_GetShaderByHandle(RE_RegisterShader(items->FindPairValue("shader", "")));
			}
			else if(!Q_stricmp(type, "flattexture"))
			{
				mFlatShader = RE_RegisterShader ( items->FindPairValue("shader", "") );
			}

			items = (CGPGroup *)items->GetNext();
		}
		classes = (CGPGroup *)classes->GetNext();
	}

	Com_ParseTextFileDestroy(parse);
}

qhandle_t CTRLandScape::GetBlendedShader(qhandle_t a, qhandle_t b, qhandle_t c, bool surfaceSprites)
{
	qhandle_t	blended;

	// Special case single pass shader
	if((a == b) && (a == c))
	{
		return(a);
	}

	blended = R_CreateBlendedShader(a, b, c, surfaceSprites );
	return(blended);
}

static int ComparePatchInfo(const TPatchInfo *arg1, const TPatchInfo *arg2)
{
	shader_t	*s1, *s2;

	if ((arg1->mPart & PI_TOP))
	{
		s1 = arg1->mPatch->GetTLShader();
	}
	else
	{
		s1 = arg1->mPatch->GetBRShader();
	}

	if ((arg2->mPart & PI_TOP))
	{
		s2 = arg2->mPatch->GetTLShader();
	}
	else
	{
		s2 = arg2->mPatch->GetBRShader();
	}

	if (s1 < s2)
	{
		return -1;
	}
	else if (s1 > s2)
	{
		return 1;
	}

	return 0;
}

void CTRLandScape::CalculateShaders(void)
{
	int						x, y;
	int						width, height;
	int						offset;
//	int						offsets[4];
	qhandle_t				handles[4];
	CTRPatch				*patch;
	qhandle_t				*shaders;
	TPatchInfo				*current = mSortedPatches;

	width  = GetWidth ( ) / common->GetTerxels ( );
	height = GetHeight ( ) / common->GetTerxels ( );

	shaders = new qhandle_t [ (width+1) * (height+1) ];

	// On the first pass determine all of the shaders for the entire
	// terrain assuming no flat ground
	offset = 0;
	for ( y = 0; y < height + 1; y ++ )
	{
		if ( y <= height )
		{
			offset = common->GetTerxels ( ) * y * GetRealWidth ( );
		}
		else
		{
			offset = common->GetTerxels ( ) * (y-1) * GetRealWidth ( );
			offset += GetRealWidth ( );
		}

		for ( x = 0; x < width + 1; x ++, offset += common->GetTerxels ( ) )
		{
			// Save the shader
			shaders[y * width + x] = GetHeightDetail(mRenderMap[offset].height)->GetShader ( );
		}
	}

	// On the second pass determine flat ground and replace the shader
	// at that point with the flat ground shader
	if ( mFlatShader )
	{
		for ( y = 1; y < height; y ++ )
		{
			for ( x = 1; x < width; x ++ )
			{
				int		offset;
				int		xx;
				int		yy;
				byte*	flattenMap = common->GetFlattenMap ( );
				bool	flat	   = false;

				offset  = (x) * common->GetTerxels ( );
				offset += (y) * common->GetTerxels ( ) * GetRealWidth();

				offset -= GetRealWidth();
				offset -= 1;

				for ( yy = 0; yy < 3 && !flat; yy++ )
				{
					for ( xx = 0; xx < 3 && !flat; xx++ )
					{
						if ( flattenMap [ offset + xx] & 0x80)
						{
							flat = true;
							break;
						}
					}

					offset += GetRealWidth();
				}

/*
				// Calculate the height map offset
				offset  = x * common->GetTerxels ( );
				offset += (y * common->GetTerxels ( ) * GetRealWidth());

				// Calculate the offsets around this particular shader location
				offsets[INDEX_TL] = offset - 1 - GetRealWidth();
				offsets[INDEX_TR] = offsets[INDEX_TL] + 1;
				offsets[INDEX_BL] = offsets[INDEX_TL] + GetRealWidth();
				offsets[INDEX_BR] = offsets[INDEX_BL] + 1;

				// If not equal to the top left one then skip
				if ( mRenderMap[offset].height != mRenderMap[offsets[INDEX_TL]].height )
				{
					continue;
				}

				// If not equal to the top right one then skip
				if ( mRenderMap[offset].height != mRenderMap[offsets[INDEX_TR]].height )
				{
					continue;
				}

				// If not equal to the bottom left one then skip
				if ( mRenderMap[offset].height != mRenderMap[offsets[INDEX_BL]].height )
				{
					continue;
				}

				// If not equal to the bottom right one then skip
				if ( mRenderMap[offset].height != mRenderMap[offsets[INDEX_BR]].height )
				{
					continue;
				}
	*/

				// This shader is now a flat shader
				if ( flat )
				{
					shaders[y * width + x] = mFlatShader;
				}

#ifdef _DEBUG
				Com_OPrintf("Flat Area:  %f %f\n",
									GetMins()[0] + (GetMaxs()[0]-GetMins()[0])/width * x,
									GetMins()[1] + (GetMaxs()[1]-GetMins()[1])/height * y );
#endif
			}
		}
	}

	// Now that the shaders have been determined, set them for each patch
	patch = mTRPatches;
	mSortedCount = 0;
	for ( y = 0; y < height; y ++ )
	{
		for ( x = 0; x < width; x ++, patch++ )
		{
			bool surfaceSprites = true;

			handles[INDEX_TL] = shaders[ x + y * width ];
			handles[INDEX_TR] = shaders[ x + 1 + y * width ];
			handles[INDEX_BL] = shaders[ x + (y + 1) * width ];
			handles[INDEX_BR] = shaders[ x + 1 + (y + 1) * width ];

			if ( handles[INDEX_TL] == mFlatShader ||
				 handles[INDEX_TR] == mFlatShader ||
				 handles[INDEX_BL] == mFlatShader ||
				 handles[INDEX_BR] == mFlatShader    )
			{
				surfaceSprites = false;
			}

			patch->SetTLShader(GetBlendedShader(handles[INDEX_TR], handles[INDEX_BL], handles[INDEX_TL], surfaceSprites));
			current->mPatch = patch;
			current->mShader = patch->GetTLShader();
			current->mPart = PI_TOP;

			patch->SetBRShader(GetBlendedShader(handles[INDEX_TR], handles[INDEX_BL], handles[INDEX_BR], surfaceSprites));
			if (patch->GetBRShader() == current->mShader)
			{
				current->mPart |= PI_BOTTOM;
			}
			else
			{
				mSortedCount++;
				current++;

				current->mPatch = patch;
				current->mShader = patch->GetBRShader();
				current->mPart = PI_BOTTOM;
			}
			mSortedCount++;
			current++;
		}
	}

	// Cleanup our temporary array
	delete[] shaders;

	qsort(mSortedPatches, mSortedCount, sizeof(*mSortedPatches), (int (QDECL *)(const void *,const void *))ComparePatchInfo);

}

void CTRPatch::SetRenderMap(const int x, const int y)
{
	mRenderMap = localowner->GetRenderMap(x, y);
}

void InitRendererPatches( CCMPatch *patch, void *userdata )
{
	int			  	tx, ty, bx, by;
	CTRPatch	  	*localpatch;
	CCMLandScape	*owner;
	CTRLandScape	*localowner;

	// Set owning landscape
	localowner = (CTRLandScape *)userdata;
	owner = (CCMLandScape *)localowner->GetCommon();

	// Get TRPatch pointer
	tx = patch->GetHeightMapX();
	ty = patch->GetHeightMapY();
	bx = tx / owner->GetTerxels();
	by = ty / owner->GetTerxels();

	localpatch = localowner->GetPatch(bx, by);
	localpatch->Clear();

	localpatch->SetCommon(patch);
	localpatch->SetOwner(owner);
	localpatch->SetLocalOwner(localowner);
	localpatch->SetRenderMap(tx, ty);
	localpatch->SetCenter();
//	localpatch->CalcNormal();
}

void CTRLandScape::CopyHeightMap(void)
{
	const CCMLandScape	*common = GetCommon();
	const byte			*heightMap = common->GetHeightMap();
	CTerVert			*renderMap = mRenderMap;
	int					i;

	for(i = 0; i < common->GetRealArea(); i++)
	{
		renderMap->height = *heightMap;
		renderMap++;
		heightMap++;
	}
}

CTRLandScape::~CTRLandScape(void)
{
	if(mTRPatches)
	{
		Z_Free(mTRPatches);
		mTRPatches = NULL;
	}
	if (mSortedPatches)
	{
		Z_Free(mSortedPatches);
		mSortedPatches = 0;
	}
	if(mRenderMap)
	{
		Z_Free(mRenderMap);
		mRenderMap = NULL;
	}
}

CTRLandScape::CTRLandScape(const char *configstring)
{
	int					shaderNum;
	const CCMLandScape	*common;

	memset(this, 0, sizeof(*this));

	// Sets up the common aspects of the terrain
	common = ri.CM_RegisterTerrain(configstring, false);
	SetCommon(common);

	tr.landScape.landscape = this;

	mTextureScale = (float)atof(Info_ValueForKey(configstring, "texturescale")) / common->GetTerxels();
	LoadTerrainDef(Info_ValueForKey(configstring, "terrainDef"));

	// To normalise the variance value to a reasonable number
	mScalarSize = VectorLengthSquared(common->GetSize());

	// Calculate and set variance depth
	mMaxNode = (Q_log2(common->GetTerxels()) << 1) - 1;

	// Allocate space for the renderer specific data
	mRenderMap = (CTerVert *)Z_Malloc(sizeof(CTerVert) * common->GetRealArea(), TAG_R_TERRAIN);

	// Copy byte heightmap to rendermap to speed up calcs
	CopyHeightMap();

	// Calculate the real world location for each heightmap entry
	CalculateRealCoords();

	// Calculate the normal of each terxel
	CalculateNormals();

	// Calculate modulation values for the heightmap
	CalculateLighting();

	// Calculate texture coords (not projected - real)
	CalculateTextureCoords();

	ri.Printf( PRINT_ALL, "R_Terrain: Creating renderer patches.....\n");
	// Initialise all terrain patches
	mTRPatches = (CTRPatch *)Z_Malloc(sizeof(CTRPatch) * common->GetBlockCount(), TAG_R_TERRAIN);

	mSortedCount = 2 * common->GetBlockCount();
	mSortedPatches = (TPatchInfo *)Z_Malloc(sizeof(TPatchInfo) * mSortedCount, TAG_R_TERRAIN);

	ri.CM_TerrainPatchIterate(common, InitRendererPatches, this);

	// Calculate shaders dependent on the .terrain file
	CalculateShaders();

	// Get the contents shader
	shaderNum = atol(Info_ValueForKey(configstring, "shader"));;
	mShader = R_GetShaderByHandle(R_GetShaderByNum(shaderNum, *tr.world));

	mPatchSize = VectorLength(common->GetPatchSize());

#if	_DEBUG
	mCycleCount = 0;
#endif
}

// ---------------------------------------------------------------------

void RB_SurfaceTerrain( surfaceInfo_t *surf )
{
	/*
	if(backEnd.refdef.rdflags & RDF_PROJECTION2D)
	{
		return;
	}
	*/
	srfTerrain_t *ls = (srfTerrain_t *)surf;
	CTRLandScape *landscape = ls->landscape;

	TerrainFog = tr.world->globalFog;

	landscape->CalculateRegion();
	landscape->Reset();
//	landscape->Tessellate();
	landscape->Render();
}

void R_CalcTerrainVisBounds(CTRLandScape *landscape)
{
	const CCMLandScape *common = landscape->GetCommon();

	// Set up the visbounds using terrain data
	if ( common->GetMins()[0] < tr.viewParms.visBounds[0][0] )
	{
		tr.viewParms.visBounds[0][0] = common->GetMins()[0];
	}
	if ( common->GetMins()[1] < tr.viewParms.visBounds[0][1] )
	{
		tr.viewParms.visBounds[0][1] = common->GetMins()[1];
	}
	if ( common->GetMins()[2] < tr.viewParms.visBounds[0][2] )
	{
		tr.viewParms.visBounds[0][2] = common->GetMins()[2];
	}

	if ( common->GetMaxs()[0] > tr.viewParms.visBounds[1][0] )
	{
		tr.viewParms.visBounds[1][0] = common->GetMaxs()[0];
	}
	if ( common->GetMaxs()[1] > tr.viewParms.visBounds[1][1] )
	{
		tr.viewParms.visBounds[1][1] = common->GetMaxs()[1];
	}
	if ( common->GetMaxs()[2] > tr.viewParms.visBounds[1][2] )
	{
		tr.viewParms.visBounds[1][2] = common->GetMaxs()[2];
	}
}

void R_AddTerrainSurfaces(void)
{
	CTRLandScape	*landscape;

	if (!r_drawTerrain->integer || (tr.refdef.rdflags & RDF_NOWORLDMODEL))
	{
		return;
	}

	landscape = tr.landScape.landscape;
	if(landscape)
	{
		R_AddDrawSurf( (surfaceType_t *)(&tr.landScape), landscape->GetShader(), 0, qfalse );
		R_CalcTerrainVisBounds(landscape);
	}
}

void RE_InitRendererTerrain( const char *info )
{
	//CTRLandScape	*ls;

	if ( !info || !info[0] )
	{
		ri.Printf( PRINT_ALL, "RE_RegisterTerrain: NULL name\n" );
		return;
	}

	ri.Printf( PRINT_ALL, "R_Terrain: Creating RENDERER data.....\n");

	// Create and register a new landscape structure
	/*ls = */new CTRLandScape(info);
}

void R_TerrainInit(void)
{
	tr.landScape.surfaceType = SF_TERRAIN;
	tr.landScape.landscape = NULL;

	r_terrainTessellate = ri.Cvar_Get("r_terrainTessellate", "3", CVAR_CHEAT);
	r_drawTerrain = ri.Cvar_Get("r_drawTerrain", "1", CVAR_CHEAT);
	r_showFrameVariance = ri.Cvar_Get("r_showFrameVariance", "0", 0);
	r_terrainWaterOffset = ri.Cvar_Get("r_terrainWaterOffset", "0", 0);

	tr.distanceCull = 6000;
	tr.distanceCullSquared = tr.distanceCull * tr.distanceCull;
}

void R_TerrainShutdown(void)
{
	CTRLandScape	*ls;

//	ri.Printf( PRINT_ALL, "R_Terrain: Shutting down RENDERER terrain.....\n");
	ls = tr.landScape.landscape;
	if(ls)
	{
		ri.CM_ShutdownTerrain(0);
		delete ls;
		tr.landScape.landscape = NULL;
	}
}

// end
