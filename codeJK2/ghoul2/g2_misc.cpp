// leave this as first line for PCH reasons...
//
#include "../server/exe_headers.h"



#ifndef __Q_SHARED_H
	#include "../game/q_shared.h"
#endif

#if !defined(TR_LOCAL_H)
	#include "../renderer/tr_local.h"
#endif

#include "../renderer/MatComp.h"

#if !defined(G2_H_INC)
	#include "G2.h"
#endif

#if !defined (MINIHEAP_H_INC)
	#include "../qcommon/miniheap.h"
#endif

#define G2_MODEL_OK(g) ((g)&&(g)->mValid&&(g)->aHeader&&(g)->currentModel&&(g)->animModel)

#include "../server/server.h"

extern mdxaBone_t		worldMatrix;
extern mdxaBone_t		worldMatrixInv;

const mdxaBone_t &EvalBoneCache(int index,CBoneCache *boneCache);

#pragma warning(disable : 4512)		//assignment op could not be genereated
class CTraceSurface
{
public:
	int					surfaceNum;
	surfaceInfo_v		&rootSList;
	const model_t		*currentModel;
	const int			lod;
	vec3_t		rayStart;
	vec3_t		rayEnd;
	CCollisionRecord	*collRecMap;
	const int			entNum;
	const int			modelIndex;
	const skin_t		*skin;
	const shader_t		*cust_shader;
	int					*TransformedVertsArray;
	const EG2_Collision	eG2TraceType;
	bool				hitOne;
	float				m_fRadius;
	
	CTraceSurface(
		int					initsurfaceNum,
		surfaceInfo_v		&initrootSList,
		const model_t		*initcurrentModel,
		int					initlod,
		vec3_t				initrayStart,
		vec3_t				initrayEnd,
		CCollisionRecord	*initcollRecMap,
		int					initentNum,
		int					initmodelIndex,
		const skin_t		*initskin,
		const shader_t		*initcust_shader,
		int					*initTransformedVertsArray,
		const EG2_Collision	einitG2TraceType,
		float				fRadius
		):
	
		surfaceNum(initsurfaceNum),
		rootSList(initrootSList),   
		currentModel(initcurrentModel),
		lod(initlod),          
		collRecMap(initcollRecMap),  
		entNum(initentNum),       
		modelIndex(initmodelIndex),
		skin(initskin),
		cust_shader(initcust_shader),
		eG2TraceType(einitG2TraceType),
		hitOne(false),
		TransformedVertsArray(initTransformedVertsArray),
		m_fRadius(fRadius)
	{
		VectorCopy(initrayStart, rayStart);
		VectorCopy(initrayEnd, rayEnd);
	}         
};

// assorted Ghoul 2 functions.
// list all surfaces associated with a model
void G2_List_Model_Surfaces(const char *fileName)
{
	int			i, x;
  	model_t		*mod_m = R_GetModelByHandle(RE_RegisterModel(fileName));
	mdxmSurfHierarchy_t	*surf;

	surf = (mdxmSurfHierarchy_t *) ( (byte *)mod_m->mdxm + mod_m->mdxm->ofsSurfHierarchy );
	mdxmSurface_t *surface = (mdxmSurface_t *)((byte *)mod_m->mdxm + mod_m->mdxm->ofsLODs + sizeof(mdxmLOD_t));

	for ( x = 0 ; x < mod_m->mdxm->numSurfaces ; x++) 
	{
		Com_Printf("Surface %i Name %s\n", x, surf->name);
		if (r_verbose->value)
		{
			Com_Printf("Num Descendants %i\n",  surf->numChildren);
			for (i=0; i<surf->numChildren; i++)
			{
				Com_Printf("Descendant %i\n", surf->childIndexes[i]);
			}
		}
		// find the next surface
  		surf = (mdxmSurfHierarchy_t *)( (byte *)surf + (int)( &((mdxmSurfHierarchy_t *)0)->childIndexes[ surf->numChildren ] ));
  		surface =(mdxmSurface_t *)( (byte *)surface + surface->ofsEnd );
	}

}

// list all bones associated with a model
void G2_List_Model_Bones(const char *fileName, int frame)
{
	int				x, i;
	mdxaSkel_t		*skel;
	mdxaSkelOffsets_t	*offsets;
  	model_t			*mod_m = R_GetModelByHandle(RE_RegisterModel(fileName)); 
	model_t			*mod_a = R_GetModelByHandle(mod_m->mdxm->animIndex);
// 	mdxaFrame_t		*aframe=0;
//	int				frameSize;
	mdxaHeader_t	*header = mod_a->mdxa;

	// figure out where the offset list is
	offsets = (mdxaSkelOffsets_t *)((byte *)header + sizeof(mdxaHeader_t));

//    frameSize = (int)( &((mdxaFrame_t *)0)->boneIndexes[ header->numBones ] );   

//	aframe = (mdxaFrame_t *)((byte *)header + header->ofsFrames + (frame * frameSize));
	// walk each bone and list it's name
	for (x=0; x< mod_a->mdxa->numBones; x++)
	{
		skel = (mdxaSkel_t *)((byte *)header + sizeof(mdxaHeader_t) + offsets->offsets[x]);
		Com_Printf("Bone %i Name %s\n", x, skel->name);

		Com_Printf("X pos %f, Y pos %f, Z pos %f\n", skel->BasePoseMat.matrix[0][3], skel->BasePoseMat.matrix[1][3], skel->BasePoseMat.matrix[2][3]);

		// if we are in verbose mode give us more details
		if (r_verbose->value)
		{
			Com_Printf("Num Descendants %i\n",  skel->numChildren);
			for (i=0; i<skel->numChildren; i++)
			{
				Com_Printf("Num Descendants %i\n",  skel->numChildren);
			}
		}
	}
}


/************************************************************************************************
 * G2_GetAnimFileName
 *    obtain the .gla filename for a model
 *
 * Input
 *    filename of model 
 *
 * Output
 *    true if we successfully obtained a filename, false otherwise
 *
 ************************************************************************************************/
qboolean G2_GetAnimFileName(const char *fileName, char **filename)
{
	// find the model we want
	model_t				*mod = R_GetModelByHandle(RE_RegisterModel(fileName));

	if (mod && mod->mdxm && (mod->mdxm->animName[0] != 0))
	{
		*filename = mod->mdxm->animName;
		return qtrue;
	}
	return qfalse;
}


/////////////////////////////////////////////////////////////////////
// 
//	Code for collision detection for models gameside
//
/////////////////////////////////////////////////////////////////////

int G2_DecideTraceLod(CGhoul2Info &ghoul2, int useLod)
{
	int returnLod = useLod;

   	// if we are overriding the LOD at top level, then we can afford to only check this level of model
   	if (ghoul2.mLodBias > returnLod)
   	{
   		returnLod =  ghoul2.mLodBias;
   	}
	assert(G2_MODEL_OK(&ghoul2));
	
	assert(ghoul2.currentModel);
	assert(ghoul2.currentModel->mdxm);
	//what about r_lodBias?

	// now ensure that we haven't selected a lod that doesn't exist for this model
	if ( returnLod >= ghoul2.currentModel->mdxm->numLODs )
 	{
 		returnLod = ghoul2.currentModel->mdxm->numLODs - 1;
 	}

	return returnLod;
}

void R_TransformEachSurface( const mdxmSurface_t *surface, vec3_t scale, CMiniHeap *G2VertSpace, int *TransformedVertsArray,CBoneCache *boneCache) 
{
	int				 j, k;
	mdxmVertex_t 	*v;
	float			*TransformedVerts;

	//
	// deform the vertexes by the lerped bones
	//
	int *piBoneReferences = (int*) ((byte*)surface + surface->ofsBoneReferences);
   
	// alloc some space for the transformed verts to get put in
	TransformedVerts = (float *)G2VertSpace->MiniHeapAlloc(surface->numVerts * 5 * 4);
	TransformedVertsArray[surface->thisSurfaceIndex] = (int)TransformedVerts;
	if (!TransformedVerts)
	{
		Com_Error(ERR_DROP, "Ran out of transform space for Ghoul2 Models. Adjust MiniHeapSize in SV_SpawnServer.\n");
	}

	// whip through and actually transform each vertex
	const int numVerts = surface->numVerts;
	v = (mdxmVertex_t *) ((byte *)surface + surface->ofsVerts);
	mdxmVertexTexCoord_t *pTexCoords = (mdxmVertexTexCoord_t *) &v[numVerts];

	// optimisation issue
	if ((scale[0] != 1.0) || (scale[1] != 1.0) || (scale[2] != 1.0))
	{
		for ( j = 0; j < numVerts; j++ ) 
		{
			vec3_t			tempVert, tempNormal;
//			mdxmWeight_t	*w;

			VectorClear( tempVert );
			VectorClear( tempNormal );
//			w = v->weights;

			const int iNumWeights = G2_GetVertWeights( v );

			float fTotalWeight = 0.0f;
			for ( k = 0 ; k < iNumWeights ; k++ ) 
			{
				int		iBoneIndex	= G2_GetVertBoneIndex( v, k );
				float	fBoneWeight	= G2_GetVertBoneWeight( v, k, fTotalWeight, iNumWeights );

				const mdxaBone_t &bone=EvalBoneCache(piBoneReferences[iBoneIndex],boneCache);

				tempVert[0] += fBoneWeight * ( DotProduct( bone.matrix[0], v->vertCoords ) + bone.matrix[0][3] );
				tempVert[1] += fBoneWeight * ( DotProduct( bone.matrix[1], v->vertCoords ) + bone.matrix[1][3] );
				tempVert[2] += fBoneWeight * ( DotProduct( bone.matrix[2], v->vertCoords ) + bone.matrix[2][3] );

				tempNormal[0] += fBoneWeight * DotProduct( bone.matrix[0], v->normal );
				tempNormal[1] += fBoneWeight * DotProduct( bone.matrix[1], v->normal );
				tempNormal[2] += fBoneWeight * DotProduct( bone.matrix[2], v->normal );
			}
			int pos = j * 5;

			// copy tranformed verts into temp space
			TransformedVerts[pos++] = tempVert[0] * scale[0];
			TransformedVerts[pos++] = tempVert[1] * scale[1];
			TransformedVerts[pos++] = tempVert[2] * scale[2];
			// we will need the S & T coors too for hitlocation and hitmaterial stuff
			TransformedVerts[pos++] = pTexCoords[j].texCoords[0];
			TransformedVerts[pos] = pTexCoords[j].texCoords[1];

			v++;// = (mdxmVertex_t *)&v->weights[/*v->numWeights*/surface->maxVertBoneWeights];
		}
	}
	else
	{
		int pos = 0;
	  	for ( j = 0; j < numVerts; j++ ) 
		{
			vec3_t			tempVert, tempNormal;
//			const mdxmWeight_t	*w;

			VectorClear( tempVert );
			VectorClear( tempNormal );
//			w = v->weights;

			const int iNumWeights = G2_GetVertWeights( v );

			float fTotalWeight = 0.0f;
			for ( k = 0 ; k < iNumWeights ; k++ ) 
			{
				int		iBoneIndex	= G2_GetVertBoneIndex( v, k );
				float	fBoneWeight	= G2_GetVertBoneWeight( v, k, fTotalWeight, iNumWeights );

				const mdxaBone_t &bone=EvalBoneCache(piBoneReferences[iBoneIndex],boneCache);

				tempVert[0] += fBoneWeight * ( DotProduct( bone.matrix[0], v->vertCoords ) + bone.matrix[0][3] );
				tempVert[1] += fBoneWeight * ( DotProduct( bone.matrix[1], v->vertCoords ) + bone.matrix[1][3] );
				tempVert[2] += fBoneWeight * ( DotProduct( bone.matrix[2], v->vertCoords ) + bone.matrix[2][3] );

				tempNormal[0] += fBoneWeight * DotProduct( bone.matrix[0], v->normal );
				tempNormal[1] += fBoneWeight * DotProduct( bone.matrix[1], v->normal );
				tempNormal[2] += fBoneWeight * DotProduct( bone.matrix[2], v->normal );
			}

			// copy tranformed verts into temp space
			TransformedVerts[pos++] = tempVert[0];
			TransformedVerts[pos++] = tempVert[1];
			TransformedVerts[pos++] = tempVert[2];
			// we will need the S & T coors too for hitlocation and hitmaterial stuff
			TransformedVerts[pos++] = pTexCoords[j].texCoords[0];
			TransformedVerts[pos++] = pTexCoords[j].texCoords[1];

			v++;// = (mdxmVertex_t *)&v->weights[/*v->numWeights*/surface->maxVertBoneWeights];
		}
	}
}

void G2_TransformSurfaces(int surfaceNum, surfaceInfo_v &rootSList, 
					CBoneCache *boneCache, const model_t *currentModel, int lod, vec3_t scale, CMiniHeap *G2VertSpace, int *TransformedVertArray, bool secondTimeAround)
{
	int	i;
	assert(currentModel);
	assert(currentModel->mdxm);
	// back track and get the surfinfo struct for this surface
	const mdxmSurface_t			*surface = (mdxmSurface_t *)G2_FindSurface(currentModel, surfaceNum, lod);
	const mdxmHierarchyOffsets_t	*surfIndexes = (mdxmHierarchyOffsets_t *)((byte *)currentModel->mdxm + sizeof(mdxmHeader_t));
	const mdxmSurfHierarchy_t		*surfInfo = (mdxmSurfHierarchy_t *)((byte *)surfIndexes + surfIndexes->offsets[surface->thisSurfaceIndex]);
	
	// see if we have an override surface in the surface list
	const surfaceInfo_t	*surfOverride = G2_FindOverrideSurface(surfaceNum, rootSList);

	// really, we should use the default flags for this surface unless it's been overriden
	int offFlags = surfInfo->flags;

  	if (surfOverride)
	{
		offFlags = surfOverride->offFlags;
	}
	// if this surface is not off, add it to the shader render list
	if (!offFlags)
	{

		R_TransformEachSurface(surface, scale, G2VertSpace, TransformedVertArray, boneCache);
	}

	// if we are turning off all descendants, then stop this recursion now
	if (offFlags & G2SURFACEFLAG_NODESCENDANTS)
	{
		return;
	}

	// now recursively call for the children
	for (i=0; i< surfInfo->numChildren; i++)
	{
		G2_TransformSurfaces(surfInfo->childIndexes[i], rootSList, boneCache, currentModel, lod, scale, G2VertSpace, TransformedVertArray, secondTimeAround);
	}
}

// main calling point for the model transform for collision detection. At this point all of the skeleton has been transformed.
void G2_TransformModel(CGhoul2Info_v &ghoul2, const int frameNum, vec3_t scale, CMiniHeap *G2VertSpace, int useLod)
{
	int				i, lod;
	vec3_t			correctScale;


	VectorCopy(scale, correctScale);
	// check for scales of 0 - that's the default I believe
	if (!scale[0])
	{
		correctScale[0] = 1.0;
	}
	if (!scale[1])
	{
		correctScale[1] = 1.0;
	}
	if (!scale[2])
	{
		correctScale[2] = 1.0;
	}

	// walk each possible model for this entity and try rendering it out
	for (i=0; i<ghoul2.size(); i++)
	{
		CGhoul2Info &g=ghoul2[i];
		// don't bother with models that we don't care about.
		if (!g.mValid)
		{
			continue;
		}
		assert(g.mBoneCache);
		assert(G2_MODEL_OK(&g));
		// stop us building this model more than once per frame
		g.mMeshFrameNum = frameNum;

		// decide the LOD
		lod = G2_DecideTraceLod(g, useLod);

		// give us space for the transformed vertex array to be put in
		ghoul2[i].mTransformedVertsArray = (int*)G2VertSpace->MiniHeapAlloc(g.currentModel->mdxm->numSurfaces * 4);
		if (!g.mTransformedVertsArray)
		{
			Com_Error(ERR_DROP, "Ran out of transform space for Ghoul2 Models. Adjust MiniHeapSize in SV_SpawnServer.\n");
		}

		memset(g.mTransformedVertsArray, 0,(g.currentModel->mdxm->numSurfaces * 4)); 

		G2_FindOverrideSurface(-1,g.mSlist); //reset the quick surface override lookup;
		// recursively call the model surface transform
		G2_TransformSurfaces(g.mSurfaceRoot, g.mSlist, g.mBoneCache,  g.currentModel, lod, correctScale, G2VertSpace, g.mTransformedVertsArray, false);
	}
}


// work out how much space a triangle takes
static float	G2_AreaOfTri(const vec3_t A, const vec3_t B, const vec3_t C)
{
	vec3_t	cross, ab, cb;
	VectorSubtract(A, B, ab);
	VectorSubtract(C, B, cb);

	CrossProduct(ab, cb, cross);

	return VectorLength(cross);
}

// actually determine the S and T of the coordinate we hit in a given poly
static void G2_BuildHitPointST( const vec3_t A, const float SA, const float TA,
						 const vec3_t B, const float SB, const float TB,
						 const vec3_t C, const float SC, const float TC,
						 const vec3_t P, float *s, float *t,float &bary_i,float &bary_j)
{
	float	areaABC = G2_AreaOfTri(A, B, C);

	float i = G2_AreaOfTri(P, B, C) / areaABC;
	bary_i=i;
	float j = G2_AreaOfTri(A, P, C) / areaABC;
	bary_j=j;
	float k = G2_AreaOfTri(A, B, P) / areaABC;

	*s = SA * i + SB * j + SC * k;
	*t = TA * i + TB * j + TC * k;

	*s=fmod(*s, 1);
	if (*s< 0)
	{
		*s+= 1.0;
	}

	*t=fmod(*t, 1);
	if (*t< 0)
	{
		*t+= 1.0;
	}

}


// routine that works out given a ray whether or not it hits a poly
qboolean G2_SegmentTriangleTest( const vec3_t start, const vec3_t end,
	const vec3_t A, const vec3_t B, const vec3_t C,
	qboolean backFaces,qboolean frontFaces,vec3_t returnedPoint,vec3_t returnedNormal, float *denom)
{
	static const float tiny=1E-10f;
	vec3_t returnedNormalT;
	vec3_t edgeAC;

	VectorSubtract(C, A, edgeAC);
	VectorSubtract(B, A, returnedNormalT); 

	CrossProduct(returnedNormalT, edgeAC, returnedNormal);
	
	vec3_t ray;
	VectorSubtract(end, start, ray);

	*denom=DotProduct(ray, returnedNormal);
	
	if (fabs(*denom)<tiny||        // triangle parallel to ray
		(!backFaces && *denom>0)||		// not accepting back faces
		(!frontFaces && *denom<0))		//not accepting front faces
	{
		return qfalse;
	}

	vec3_t toPlane;
	VectorSubtract(A, start, toPlane);
	
	float t=DotProduct(toPlane, returnedNormal)/ *denom;
	
	if (t<0.0f||t>1.0f)
	{
		return qfalse; // off segment
	}
	
	VectorScale(ray, t, ray);
	
	VectorAdd(ray, start, returnedPoint);

	vec3_t edgePA;
	VectorSubtract(A, returnedPoint, edgePA);

	vec3_t edgePB;
	VectorSubtract(B, returnedPoint, edgePB);

	vec3_t edgePC;
	VectorSubtract(C, returnedPoint, edgePC);
	
	vec3_t temp;
	
	CrossProduct(edgePA, edgePB, temp);
	if (DotProduct(temp, returnedNormal)<0.0f)
	{
		return qfalse; // off triangle
	}

	CrossProduct(edgePC, edgePA, temp);
	if (DotProduct(temp,returnedNormal)<0.0f)
	{
		return qfalse; // off triangle
	}
	
	CrossProduct(edgePB, edgePC, temp);
	if (DotProduct(temp, returnedNormal)<0.0f)
	{
		return qfalse; // off triangle
	}	
	return qtrue;
}

// now we're at poly level, check each model space transformed poly against the model world transfomed ray
static bool G2_TracePolys(const mdxmSurface_t *surface, const mdxmSurfHierarchy_t *surfInfo, CTraceSurface &TS)
{
	int				j, numTris;
	
	// whip through and actually transform each vertex
	const mdxmTriangle_t *tris = (mdxmTriangle_t *) ((byte *)surface + surface->ofsTriangles);
	const float *verts = (float *)TS.TransformedVertsArray[surface->thisSurfaceIndex];
	numTris = surface->numTriangles;
	for ( j = 0; j < numTris; j++ ) 
	{
		float			face;
		vec3_t	hitPoint, normal;
		// determine actual coords for this triangle
		const float *point1 = &verts[(tris[j].indexes[0] * 5)];
		const float *point2 = &verts[(tris[j].indexes[1] * 5)];
		const float *point3 = &verts[(tris[j].indexes[2] * 5)];
		// did we hit it?
		if (G2_SegmentTriangleTest(TS.rayStart, TS.rayEnd, point1, point2, point3, qtrue, qtrue, hitPoint, normal, &face))
		{	// find space in the collision records for this record
			for (int i=0; i<MAX_G2_COLLISIONS;i++)
			{
				if (TS.collRecMap[i].mEntityNum == -1)
				{
					CCollisionRecord  	&newCol = TS.collRecMap[i];
					vec3_t			  	distVect;
					float				x_pos = 0, y_pos = 0;
					
					newCol.mPolyIndex = j;
					newCol.mEntityNum = TS.entNum;
					newCol.mSurfaceIndex = surface->thisSurfaceIndex;
					newCol.mModelIndex = TS.modelIndex;
					if (face>0)
					{
						newCol.mFlags = G2_FRONTFACE;
					}
					else
					{
						newCol.mFlags = G2_BACKFACE;
					}

					VectorSubtract(hitPoint, TS.rayStart, distVect);
					newCol.mDistance = VectorLength(distVect);

					// put the hit point back into world space
					TransformAndTranslatePoint(hitPoint, newCol.mCollisionPosition, &worldMatrix);

					// transform normal (but don't translate) into world angles
					TransformPoint(normal, newCol.mCollisionNormal, &worldMatrix);
					VectorNormalize(newCol.mCollisionNormal);

					newCol.mMaterial = newCol.mLocation = 0;

					// Determine our location within the texture, and barycentric coordinates
					G2_BuildHitPointST(point1, point1[3], point1[4],
									   point2, point2[3], point2[4],
									   point3, point3[3], point3[4],
									   hitPoint, &x_pos, &y_pos,newCol.mBarycentricI,newCol.mBarycentricJ); 
									
/*
					const shader_t		*shader = 0;
					// now, we know what surface this hit belongs to, we need to go get the shader handle so we can get the correct hit location and hit material info
					if ( cust_shader ) 
					{
						shader = cust_shader;
					} 
					else if ( skin ) 
					{
						int		j;
							
						// match the surface name to something in the skin file
						shader = tr.defaultShader;
						for ( j = 0 ; j < skin->numSurfaces ; j++ )
						{
							// the names have both been lowercased
							if ( !strcmp( skin->surfaces[j]->name, surfInfo->name ) ) 
							{
								shader = skin->surfaces[j]->shader;
								break;
							}
						}
					} 
					else 
					{
						shader = R_GetShaderByHandle( surfInfo->shaderIndex );
					}

					// do we even care to decide what the hit or location area's are? If we don't have them in the shader there is little point
					if ((shader->hitLocation) || (shader->hitMaterial))
					{
 						// ok, we have a floating point position. - determine location in data we need to look at
						if (shader->hitLocation)
						{
							newCol.mLocation = *(hitMatReg[shader->hitLocation].loc +
												((int)(y_pos * hitMatReg[shader->hitLocation].height) * hitMatReg[shader->hitLocation].width) +
												((int)(x_pos * hitMatReg[shader->hitLocation].width)));
							Com_Printf("G2_TracePolys hit location: %d\n", newCol.mLocation); 
						}

						if (shader->hitMaterial)
						{
							newCol.mMaterial = *(hitMatReg[shader->hitMaterial].loc +
												((int)(y_pos * hitMatReg[shader->hitMaterial].height) * hitMatReg[shader->hitMaterial].width) +
												((int)(x_pos * hitMatReg[shader->hitMaterial].width)));
						}
					}
*/
					// exit now if we should
					if (TS.eG2TraceType == G2_RETURNONHIT)
					{
						TS.hitOne = true;
						return true;
					}

					break;
				}
			}
			if (i==MAX_G2_COLLISIONS)
			{
				assert(i!=MAX_G2_COLLISIONS);		// run out of collision record space - will probalbly never happen
				TS.hitOne = true;	//force stop recursion
				return true;	// return true to avoid wasting further time, but no hit will result without a record
			}
		}
	}
	return false;
}


struct SVertexTemp
{
	int flags;
//	int touch;
//	int newindex;
//	float tex[2];
	SVertexTemp()
	{
//		touch=0;
	}
};

#define MAX_GORE_VERTS (3000)
static SVertexTemp GoreVerts[MAX_GORE_VERTS];

// now we're at poly level, check each model space transformed poly against the model world transfomed ray
static bool G2_RadiusTracePolys(
								const mdxmSurface_t *surface, 
								CTraceSurface &TS
								)
{
	int		j;
	vec3_t basis1;
	vec3_t basis2;
	vec3_t taxis;
	vec3_t saxis;

	basis2[0]=0.0f;
	basis2[1]=0.0f;
	basis2[2]=1.0f;

	vec3_t v3RayDir;
	VectorSubtract(TS.rayEnd, TS.rayStart, v3RayDir);

	CrossProduct(v3RayDir,basis2,basis1);

	if (DotProduct(basis1,basis1)<.1f)
	{
		basis2[0]=0.0f;
		basis2[1]=1.0f;
		basis2[2]=0.0f;
		CrossProduct(v3RayDir,basis2,basis1);
	}

	CrossProduct(v3RayDir,basis1,basis2);
	// Give me a shot direction not a bunch of zeros :) -Gil
//	assert(DotProduct(basis1,basis1)>.0001f);
//	assert(DotProduct(basis2,basis2)>.0001f);

	VectorNormalize(basis1);
	VectorNormalize(basis2);

	const float c=cos(0);//theta
	const float s=sin(0);//theta

	VectorScale(basis1, 0.5f * c / TS.m_fRadius,taxis);
	VectorMA(taxis,     0.5f * s / TS.m_fRadius,basis2,taxis);

	VectorScale(basis1,-0.5f * s /TS.m_fRadius,saxis);
	VectorMA(    saxis, 0.5f * c /TS.m_fRadius,basis2,saxis);

	const float * const verts = (float *)TS.TransformedVertsArray[surface->thisSurfaceIndex];
	const int numVerts = surface->numVerts;
	
	int flags=63;
	//rayDir/=lengthSquared(raydir);
	const float f = VectorLengthSquared(v3RayDir); 
	v3RayDir[0]/=f;
	v3RayDir[1]/=f;
	v3RayDir[2]/=f;

	for ( j = 0; j < numVerts; j++ ) 
	{
		const int pos=j*5;
		vec3_t delta;
		delta[0]=verts[pos+0]-TS.rayStart[0];
		delta[1]=verts[pos+1]-TS.rayStart[1];
		delta[2]=verts[pos+2]-TS.rayStart[2];
		const float s=DotProduct(delta,saxis)+0.5f;
		const float t=DotProduct(delta,taxis)+0.5f;
		const float u=DotProduct(delta,v3RayDir);
		int vflags=0;

		if (s>0)
		{
			vflags|=1;
		}
		if (s<1)
		{
			vflags|=2;
		}
		if (t>0)
		{
			vflags|=4;
		}
		if (t<1)
		{
			vflags|=8;
		}
		if (u>0)
		{
			vflags|=16;
		}
		if (u<1)
		{
			vflags|=32;
		}

		vflags=(~vflags);
		flags&=vflags;
		GoreVerts[j].flags=vflags;
	}

	if (flags)
	{
		return false; // completely off the gore splotch  (so presumably hit nothing? -Ste)
	}
	const int numTris = surface->numTriangles;
	const mdxmTriangle_t * const tris = (mdxmTriangle_t *) ((byte *)surface + surface->ofsTriangles);

	for ( j = 0; j < numTris; j++ ) 
	{
		assert(tris[j].indexes[0]>=0&&tris[j].indexes[0]<numVerts);
		assert(tris[j].indexes[1]>=0&&tris[j].indexes[1]<numVerts);
		assert(tris[j].indexes[2]>=0&&tris[j].indexes[2]<numVerts);
		flags=63&
			GoreVerts[tris[j].indexes[0]].flags&
			GoreVerts[tris[j].indexes[1]].flags&
			GoreVerts[tris[j].indexes[2]].flags;
		if (flags)
		{
			continue;
		}
		else
		{
			// we hit a triangle, so init a collision record...
			//
			for (int i=0; i<MAX_G2_COLLISIONS;i++)
			{
				if (TS.collRecMap[i].mEntityNum == -1)
				{
					CCollisionRecord  	&newCol = TS.collRecMap[i];
					
					newCol.mPolyIndex = j;
					newCol.mEntityNum = TS.entNum;
					newCol.mSurfaceIndex = surface->thisSurfaceIndex;
					newCol.mModelIndex = TS.modelIndex;
//					if (face>0)
//					{
						newCol.mFlags = G2_FRONTFACE;
//					}
//					else
//					{
//						newCol.mFlags = G2_BACKFACE;
//					}

					//get normal from triangle				
					const float *A = &verts[(tris[j].indexes[0] * 5)];
					const float *B = &verts[(tris[j].indexes[1] * 5)];
					const float *C = &verts[(tris[j].indexes[2] * 5)];
					vec3_t normal;
					vec3_t edgeAC, edgeBA;

					VectorSubtract(C, A, edgeAC);
					VectorSubtract(B, A, edgeBA);
					CrossProduct(edgeBA, edgeAC, normal);

					// transform normal (but don't translate) into world angles
					TransformPoint(normal, newCol.mCollisionNormal, &worldMatrix);
					VectorNormalize(newCol.mCollisionNormal);

					newCol.mMaterial = newCol.mLocation = 0;
					// exit now if we should
					if (TS.eG2TraceType == G2_RETURNONHIT)
					{
						TS.hitOne = true;
						return true;
					}

					//i don't know the hitPoint, but let's just assume it's the first vert for now...
					const float *hitPoint = A;
					vec3_t			  distVect;

					VectorSubtract(hitPoint, TS.rayStart, distVect);
					newCol.mDistance = VectorLength(distVect);

					// put the hit point back into world space
					TransformAndTranslatePoint(hitPoint, newCol.mCollisionPosition, &worldMatrix);
					newCol.mBarycentricI = newCol.mBarycentricJ = 0.0f;

					break;
				}
			}
			if (i==MAX_G2_COLLISIONS)
			{
				//assert(i!=MAX_G2_COLLISIONS);		// run out of collision record space - happens OFTEN
				TS.hitOne = true;	//force stop recursion
				return true;	// return true to avoid wasting further time, but no hit will result without a record
			}
		}
	}

	return false;
}


// look at a surface and then do the trace on each poly
static void G2_TraceSurfaces(CTraceSurface &TS)
{
	int	i;
	// back track and get the surfinfo struct for this surface
	assert(TS.currentModel);
	assert(TS.currentModel->mdxm);
	const mdxmSurface_t		*surface = (mdxmSurface_t *)G2_FindSurface(TS.currentModel, TS.surfaceNum, TS.lod);
	const mdxmHierarchyOffsets_t	*surfIndexes = (mdxmHierarchyOffsets_t *)((byte *)TS.currentModel->mdxm + sizeof(mdxmHeader_t));
	const mdxmSurfHierarchy_t		*surfInfo = (mdxmSurfHierarchy_t *)((byte *)surfIndexes + surfIndexes->offsets[surface->thisSurfaceIndex]);
	
	// see if we have an override surface in the surface list
	const surfaceInfo_t	*surfOverride = G2_FindOverrideSurface(TS.surfaceNum, TS.rootSList);

	// don't allow recursion if we've already hit a polygon
	if (TS.hitOne)
	{
		return;
	}

	// really, we should use the default flags for this surface unless it's been overriden
	int offFlags = surfInfo->flags;

	// set the off flags if we have some
	if (surfOverride)
	{
		offFlags = surfOverride->offFlags;
	}

	// if this surface is not off, try to hit it
	if (!offFlags)
	{
		if (!(fabs(TS.m_fRadius) < 0.1))	// if not a point-trace
		{
			// .. then use radius check
			//
			if (G2_RadiusTracePolys(surface,		// const mdxmSurface_t *surface, 
									TS
									)
				&& (TS.eG2TraceType == G2_RETURNONHIT)
				)
			{
				TS.hitOne = true;
				return;
			}
		}
		else
		{
			// go away and trace the polys in this surface
			if (G2_TracePolys(surface, surfInfo, TS)
				&& (TS.eG2TraceType == G2_RETURNONHIT)
				)
			{
				// ok, we hit one, *and* we want to return instantly because the returnOnHit is set
				// so indicate we've hit one, so other surfaces don't get hit and return
				TS.hitOne = true;
				return;
			}
		}
	}

	// if we are turning off all descendants, then stop this recursion now
	if (offFlags & G2SURFACEFLAG_NODESCENDANTS)
	{
		return;
	}

	// now recursively call for the children
	for (i=0; i< surfInfo->numChildren && !TS.hitOne; i++)
	{
		TS.surfaceNum = surfInfo->childIndexes[i];
		G2_TraceSurfaces(TS);
	}
}

void G2_TraceModels(CGhoul2Info_v &ghoul2, vec3_t rayStart, vec3_t rayEnd, CCollisionRecord *collRecMap, int entNum, EG2_Collision eG2TraceType, int useLod, float fRadius)
{
	int				i, lod;
	skin_t			*skin;
	shader_t		*cust_shader;

	// walk each possible model for this entity and try tracing against it
	for (i=0; i<ghoul2.size(); i++)
	{
		// don't bother with models that we don't care about.
		if (!ghoul2[i].mValid)
		{
			continue;
		}
		assert(G2_MODEL_OK(&ghoul2[i]));
		// do we really want to collide with this object?
		if (ghoul2[i].mFlags & GHOUL2_NOCOLLIDE) 
		{
			continue;
		}
		
		if (ghoul2[i].mCustomShader)
		{
			cust_shader = R_GetShaderByHandle(ghoul2[i].mCustomShader );
		}
		else
		{
			cust_shader = NULL;
		}

		// figure out the custom skin thing
		if ( ghoul2[i].mSkin > 0 && ghoul2[i].mSkin < tr.numSkins ) 
		{
			skin = R_GetSkinByHandle( ghoul2[i].mSkin );
		}
		else
		{
			skin = NULL;
		}

		lod = G2_DecideTraceLod(ghoul2[i],useLod);

		//reset the quick surface override lookup
		G2_FindOverrideSurface(-1, ghoul2[i].mSlist); 

		CTraceSurface TS(ghoul2[i].mSurfaceRoot, ghoul2[i].mSlist,  ghoul2[i].currentModel, lod, rayStart, rayEnd, collRecMap, entNum, i, skin, cust_shader, ghoul2[i].mTransformedVertsArray, eG2TraceType, fRadius);
		// start the surface recursion loop
		G2_TraceSurfaces(TS);

		// if we've hit one surface on one model, don't bother doing the rest
		if (TS.hitOne)
		{
			break;
		}
	}
}

void TransformPoint (const vec3_t in, vec3_t out, mdxaBone_t *mat) {
	for (int i=0;i<3;i++)
	{
		out[i]= in[0]*mat->matrix[i][0] + in[1]*mat->matrix[i][1] + in[2]*mat->matrix[i][2];
	}
}

void TransformAndTranslatePoint (const vec3_t in, vec3_t out, mdxaBone_t *mat) {

	for (int i=0;i<3;i++)
	{
		out[i]= in[0]*mat->matrix[i][0] + in[1]*mat->matrix[i][1] + in[2]*mat->matrix[i][2] + mat->matrix[i][3];
	}
}


// create a matrix using a set of angles
void Create_Matrix(const float *angle, mdxaBone_t *matrix)
{
	vec3_t		axis[3];

	// convert angles to axis
	AnglesToAxis( angle, axis );
	matrix->matrix[0][0] = axis[0][0];
	matrix->matrix[1][0] = axis[0][1];
	matrix->matrix[2][0] = axis[0][2];

	matrix->matrix[0][1] = axis[1][0];
	matrix->matrix[1][1] = axis[1][1];
	matrix->matrix[2][1] = axis[1][2];

	matrix->matrix[0][2] = axis[2][0];
	matrix->matrix[1][2] = axis[2][1];
	matrix->matrix[2][2] = axis[2][2];

	matrix->matrix[0][3] = 0;
	matrix->matrix[1][3] = 0;
	matrix->matrix[2][3] = 0;


}

// given a matrix, generate the inverse of that matrix
void Inverse_Matrix(mdxaBone_t *src, mdxaBone_t *dest)
{
	int i, j;

    for (i = 0; i < 3; i++)
	{
        for (j = 0; j < 3; j++)
		{
            dest->matrix[i][j]=src->matrix[j][i];
		}
	}
    for (i = 0; i < 3; i++)
	{
        dest->matrix[i][3]=0;
        for (j = 0; j < 3; j++)
		{
            dest->matrix[i][3]-=dest->matrix[i][j]*src->matrix[j][3];
		}
	}
}

// generate the world matrix for a given set of angles and origin - called from lots of places
void G2_GenerateWorldMatrix(const vec3_t angles, const vec3_t origin)
{
	Create_Matrix(angles, &worldMatrix);
	worldMatrix.matrix[0][3] = origin[0];
	worldMatrix.matrix[1][3] = origin[1];
	worldMatrix.matrix[2][3] = origin[2];

	Inverse_Matrix(&worldMatrix, &worldMatrixInv);
}

// go away and determine what the pointer for a specific surface definition within the model definition is
void *G2_FindSurface(const model_s *mod, int index, int lod)
{
	assert(mod);
	assert(mod->mdxm);

	// point at first lod list
	byte	*current = (byte*)((int)mod->mdxm + (int)mod->mdxm->ofsLODs);
	int i;

	//walk the lods
	assert(lod>=0&&lod<mod->mdxm->numLODs);
	for (i=0; i<lod; i++)
	{
		mdxmLOD_t *lodData = (mdxmLOD_t *)current;
		current += lodData->ofsEnd;
	}

	// avoid the lod pointer data structure
	current += sizeof(mdxmLOD_t);

	mdxmLODSurfOffset_t *indexes = (mdxmLODSurfOffset_t *)current;
	// we are now looking at the offset array
	assert(index>=0&&index<mod->mdxm->numSurfaces);
	current += indexes->offsets[index];

	return (void *)current;
}

#define SURFACE_SAVE_BLOCK_SIZE	sizeof(surfaceInfo_t)
#define BOLT_SAVE_BLOCK_SIZE sizeof(boltInfo_t)
#define BONE_SAVE_BLOCK_SIZE sizeof(boneInfo_t)

qboolean G2_SaveGhoul2Models(CGhoul2Info_v &ghoul2, char **buffer, int *size)
{

	// is there anything to save?
	if (!ghoul2.IsValid()||!ghoul2.size())
	{
		*buffer = (char *)Z_Malloc(4, TAG_GHOUL2, qfalse);
		int *tempBuffer = (int *)*buffer;
		*tempBuffer = 0;
		*size = 4;
		return qtrue;
	}

	// yeah, lets get busy
	*size = 0;

	// this one isn't a define since I couldn't work out how to figure it out at compile time
	int ghoul2BlockSize = (int)&ghoul2[0].mTransformedVertsArray - (int)&ghoul2[0].mModelindex;

	// add in count for number of ghoul2 models
	*size += 4;	
	// start out working out the total size of the buffer we need to allocate
	for (int i=0; i<ghoul2.size();i++)
	{
		*size += ghoul2BlockSize;
		// add in count for number of surfaces
		*size += 4;	
		*size += (ghoul2[i].mSlist.size() * SURFACE_SAVE_BLOCK_SIZE);
		// add in count for number of bones
		*size += 4;	
		*size += (ghoul2[i].mBlist.size() * BONE_SAVE_BLOCK_SIZE);
		// add in count for number of bolts
		*size += 4;	
		*size += (ghoul2[i].mBltlist.size() * BOLT_SAVE_BLOCK_SIZE);
	}

	// ok, we should know how much space we need now
	*buffer = (char*)Z_Malloc(*size, TAG_GHOUL2, qfalse);

	// now lets start putting the data we care about into the buffer
	char *tempBuffer = *buffer;

	// save out how many ghoul2 models we have
	*(int *)tempBuffer = ghoul2.size();
	tempBuffer +=4;

	for (i=0; i<ghoul2.size();i++)
	{
		// first save out the ghoul2 details themselves
//		OutputDebugString(va("G2_SaveGhoul2Models(): ghoul2[%d].mModelindex = %d\n",i,ghoul2[i].mModelindex));
		memcpy(tempBuffer, &ghoul2[i].mModelindex, ghoul2BlockSize);
		tempBuffer += ghoul2BlockSize;

		// save out how many surfaces we have
		*(int*)tempBuffer = ghoul2[i].mSlist.size();
		tempBuffer +=4;

		// now save the all the surface list info
		for (int x=0; x<ghoul2[i].mSlist.size(); x++)
		{
			memcpy(tempBuffer, &ghoul2[i].mSlist[x], SURFACE_SAVE_BLOCK_SIZE);
			tempBuffer += SURFACE_SAVE_BLOCK_SIZE;
		}
		
		// save out how many bones we have
		*(int*)tempBuffer = ghoul2[i].mBlist.size();
		tempBuffer +=4;

		// now save the all the bone list info
		for (x=0; x<ghoul2[i].mBlist.size(); x++)
		{
			memcpy(tempBuffer, &ghoul2[i].mBlist[x], BONE_SAVE_BLOCK_SIZE);
			tempBuffer += BONE_SAVE_BLOCK_SIZE;
		}

		// save out how many bolts we have
		*(int*)tempBuffer = ghoul2[i].mBltlist.size();
		tempBuffer +=4;

		// lastly save the all the bolt list info
		for (x=0; x<ghoul2[i].mBltlist.size(); x++)
		{
			memcpy(tempBuffer, &ghoul2[i].mBltlist[x], BOLT_SAVE_BLOCK_SIZE);
			tempBuffer += BOLT_SAVE_BLOCK_SIZE;
		}
	}

	return qtrue;
}

// have to free space malloced in the save system here because the game DLL can't.
void G2_FreeSaveBuffer(char *buffer)
{
	Z_Free(buffer);
}

int G2_FindConfigStringSpace(char *name, int start, int max)
{
	char	s[MAX_STRING_CHARS];

	for (int  i=1 ; i<max ; i++ ) 
	{
		SV_GetConfigstring( start + i, s, sizeof( s ) );
		if ( !s[0] ) 
		{
			break;
		}
		if ( !stricmp( s, name ) ) 
		{
			return i;
		}
	}

	SV_SetConfigstring(start + i, name);
	return i;
}

void G2_LoadGhoul2Model(CGhoul2Info_v &ghoul2, char *buffer)
{
	// first thing, lets see how many ghoul2 models we have, and resize our buffers accordingly
	int newSize = *(int*)buffer;
	ghoul2.resize(newSize);
	buffer += 4;

	// did we actually resize to a value?
	if (!newSize)
	{
		// no, ok, well, done then.
		return;
	}

	// this one isn't a define since I couldn't work out how to figure it out at compile time
	int ghoul2BlockSize = (int)&ghoul2[0].mTransformedVertsArray - (int)&ghoul2[0].mModelindex;

	// now we have enough instances, lets go through each one and load up the relevant details
	for (int i=0; i<ghoul2.size(); i++)
	{
		ghoul2[i].mSkelFrameNum = 0;
		ghoul2[i].mModelindex=-1;
		ghoul2[i].mFileName[0]=0;
		ghoul2[i].mValid=false;
		// load the ghoul2 info from the buffer
		memcpy(&ghoul2[i].mModelindex, buffer, ghoul2BlockSize);
		buffer +=ghoul2BlockSize;

		if (ghoul2[i].mModelindex!=-1&&ghoul2[i].mFileName[0])
		{
			ghoul2[i].mModelindex = i;
			G2_SetupModelPointers(&ghoul2[i]);
		}

		// give us enough surfaces to load up the data
		ghoul2[i].mSlist.resize(*(int*)buffer);
		buffer +=4;

		// now load all the surfaces
		for (int x=0; x<ghoul2[i].mSlist.size(); x++)
		{
			memcpy(&ghoul2[i].mSlist[x], buffer, SURFACE_SAVE_BLOCK_SIZE);
			buffer += SURFACE_SAVE_BLOCK_SIZE;
		}

		// give us enough bones to load up the data
		ghoul2[i].mBlist.resize(*(int*)buffer);
		buffer +=4;

		// now load all the bones
		for (x=0; x<ghoul2[i].mBlist.size(); x++)
		{
			memcpy(&ghoul2[i].mBlist[x], buffer, BONE_SAVE_BLOCK_SIZE);
			buffer += BONE_SAVE_BLOCK_SIZE;
		}

		// give us enough bolts to load up the data
		ghoul2[i].mBltlist.resize(*(int*)buffer);
		buffer +=4;

		// now load all the bolts
		for (x=0; x<ghoul2[i].mBltlist.size(); x++)
		{
			memcpy(&ghoul2[i].mBltlist[x], buffer, BOLT_SAVE_BLOCK_SIZE);
			buffer += BOLT_SAVE_BLOCK_SIZE;
		}
	}
}