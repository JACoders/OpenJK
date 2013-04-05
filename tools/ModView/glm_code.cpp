// Filename:-	glm_code.cpp
//

#include "stdafx.h"
#include "includes.h"
#include "special_defines.h"
//
#include "anims.h"
#include "R_Model.h"
#include "mdx_format.h"
#include "sequence.h"
#include "parser.h"
#include "shader.h"
#include "skins.h"
#include "textures.h"	// for some stats stuff
//
#include "glm_code.h"



void *gpvDefaultGLA = NULL;
// this code is called before the rest of the app has even started, so do NOT try any other function calls!!!!...
//
static void GLMModel_BuildDefaultGLA()
{
	if (!gpvDefaultGLA)
	{
		gpvDefaultGLA = malloc(1024*1024);	// well OTT, 1MB is over 360 bytes, but WTF, it gets resized later
		byte *at = (byte*) gpvDefaultGLA;	// 'at' so I can paste code from another app

		// now create GLA file...
		//
		mdxaHeader_t *pMDXAHeader = (mdxaHeader_t *) at;
		at += sizeof(mdxaHeader_t);
		{// for brace-skipping...			

					pMDXAHeader->ident			=	MDXA_IDENT;
					pMDXAHeader->version		=	MDXA_VERSION;
			strncpy(pMDXAHeader->name,"*default",sizeof(pMDXAHeader->name));
					pMDXAHeader->name[sizeof(pMDXAHeader->name)-1]='\0';
					pMDXAHeader->fScale = 1.0f;
					pMDXAHeader->numFrames		=	1;	// inherently, when doing MD3 to G2 files
	//				pMDXAHeader->ofsFrames		=	not known yet
					pMDXAHeader->numBones		=	1;	// inherently, when doing MD3 to G2 files
	//				pMDXAHeader->ofsCompBonePool=	not known yet
	//				pMDXAHeader->ofsSkel		=	not known yet
	//				pMDXAHeader->ofsEnd			=	not known yet

			// write out bone hierarchy...
			//
			mdxaSkelOffsets_t * pSkelOffsets = (mdxaSkelOffsets_t *) at;
			at += (int)( &((mdxaSkelOffsets_t *)0)->offsets[ pMDXAHeader->numBones ] );
			
			pMDXAHeader->ofsSkel = at - (byte *) pMDXAHeader; 
			for (int iSkelIndex = 0; iSkelIndex < pMDXAHeader->numBones; iSkelIndex++)
			{
				mdxaSkel_t *pSkel = (mdxaSkel_t *) at;

				pSkelOffsets->offsets[iSkelIndex] = (byte *) pSkel - (byte *) pSkelOffsets;

				// setup flags...
				//
						pSkel->flags = 0;
				strcpy(	pSkel->name, "ModView internal default");	// doesn't matter what this is called
						pSkel->parent= -1;	// index of bone that is parent to this one, -1 = NULL/root

				static const mdxaBone_t IdentityBone = 
				{				
					1.0f, 0.0f, 0.0f, 0.0f,
					0.0f, 1.0f, 0.0f, 0.0f,
					0.0f, 0.0f, 1.0f, 0.0f
				};
				
				// base and inverse of an identity matrix are the same, so...
				//
				memcpy(pSkel->BasePoseMat.matrix,	IdentityBone.matrix, sizeof(pSkel->BasePoseMat.matrix));
				memcpy(pSkel->BasePoseMatInv.matrix,IdentityBone.matrix, sizeof(pSkel->BasePoseMatInv.matrix) );

						pSkel->numChildren	=	0;	// inherently, when doing MD3 to G2 files

				int iThisSkelSize = (int)( &((mdxaSkel_t *)0)->children[ pSkel->numChildren ] );

				at += iThisSkelSize;
			}

			// write out frames...
			//			
			pMDXAHeader->ofsFrames = at - (byte *) pMDXAHeader;			
//			int iFrameSize = (int)( &((mdxaFrame_t *)0)->boneIndexes[ pMDXAHeader->numBones ] );
			byte *pIndexes = (byte*) at;

			int iTotalBonePoolIndexBytes = pMDXAHeader->numFrames * pMDXAHeader->numBones * 3;

			while (iTotalBonePoolIndexBytes & 3) iTotalBonePoolIndexBytes++;	// align-padding
			at += iTotalBonePoolIndexBytes;

			for (int i=0; i<pMDXAHeader->numFrames; i++)
			{
				mdxaIndex_t *pIndex = (mdxaIndex_t *) &pIndexes[ (i*pMDXAHeader->numBones*3) + (0*3) ];
				pIndex->iIndex = 0;	// inherently, when doing MD3 to G2 files
			}

			// now write out compressed bone pool...
			//
			pMDXAHeader->ofsCompBonePool = at - (byte *) pMDXAHeader;
			for (int iCompBoneIndex = 0; iCompBoneIndex < 1/*CompressedBones.size()*/; iCompBoneIndex++)
			{
				mdxaCompQuatBone_t *pCompBone = (mdxaCompQuatBone_t *) at;
				at += sizeof(mdxaCompQuatBone_t);

				// a binary dump of a compressed matrix <g>...
				//
				static const byte Comp[sizeof(mdxaCompQuatBone_t)]=
				{
					0xFD, 0xBF, 0xFE, 0x7F,
					0xFE, 0x7F, 0xFE, 0x7F,
					0x00, 0x80, 0x00, 0x80,
					0x00, 0x80,
				};

				memcpy(pCompBone->Comp,Comp,sizeof(Comp));
			}
			
			// done...
			//
			pMDXAHeader->ofsEnd = at - (byte *) pMDXAHeader;		
		}

		realloc(gpvDefaultGLA, pMDXAHeader->ofsEnd);

/*		FILE *fhHandle = fopen("c:\\default.gla","wb");
		if (fhHandle)
		{
			fwrite(gpvDefaultGLA, 1, pMDXAHeader->ofsEnd, fhHandle);
			fclose(fhHandle);
		}
*/
	}
}

static void GLMModel_DestroyDefaultGLA()
{
	SAFEFREE(gpvDefaultGLA);
}

// this is just evil for the sake of it... :-)
//
struct tacky_s
{
	tacky_s()
	{
		GLMModel_BuildDefaultGLA();
	}
	~tacky_s()
	{
		GLMModel_DestroyDefaultGLA();
	}
};
tacky_s tackyness;

void* GLMModel_GetDefaultGLA(void)
{
	assert(gpvDefaultGLA);
	return gpvDefaultGLA;
}


// extern-called by main Model_Delete() code...
//
void GLMModel_DeleteExtra(void)
{
//	SAFEFREE(pvLoadedGLA);
//	pMDXMHeader = NULL;	// do NOT free this, dup ptr only
}

LPCSTR GLMModel_GetSurfaceName( ModelHandle_t hModel, int iSurfaceIndex )
{
	mdxmHeader_t	*pMDXMHeader	= (mdxmHeader_t	*) RE_GetModelData(hModel);

	mdxmHierarchyOffsets_t	*pHierarchyOffsets	= (mdxmHierarchyOffsets_t *) ((byte *) pMDXMHeader + sizeof(*pMDXMHeader));

	assert( iSurfaceIndex < pMDXMHeader->numSurfaces );
	if (	iSurfaceIndex < pMDXMHeader->numSurfaces )
	{
		mdxmSurfHierarchy_t *pSurfHierarchy	= (mdxmSurfHierarchy_t *) ((byte *) pHierarchyOffsets + pHierarchyOffsets->offsets[iSurfaceIndex]);
		return pSurfHierarchy->name;
	}

	return "GLMModel_GetSurfaceName(): Bad surface index";
}


LPCSTR GLMModel_GetSurfaceShaderName( ModelHandle_t hModel, int iSurfaceIndex )
{
	mdxmHeader_t	*pMDXMHeader	= (mdxmHeader_t	*) RE_GetModelData(hModel);

	mdxmHierarchyOffsets_t	*pHierarchyOffsets	= (mdxmHierarchyOffsets_t *) ((byte *) pMDXMHeader + sizeof(*pMDXMHeader));

	assert( iSurfaceIndex < pMDXMHeader->numSurfaces );
	if (	iSurfaceIndex < pMDXMHeader->numSurfaces )
	{
		mdxmSurfHierarchy_t *pSurfHierarchy	= (mdxmSurfHierarchy_t *) ((byte *) pHierarchyOffsets + pHierarchyOffsets->offsets[iSurfaceIndex]);
		return pSurfHierarchy->shader;
	}

	assert(0);
	return "GLMModel_GetSurfaceShaderName(): Bad surface index";
}


// interesting use of static here, this function IS called externally, but only through a ptr. 
//	This is to stop people accessing it directly.
//
// return basic info on the supplied model arg...
//
static LPCSTR GLMModel_GetBoneName( ModelHandle_t hModel, int iBoneIndex )
{
	mdxmHeader_t	*pMDXMHeader	= (mdxmHeader_t	*) RE_GetModelData(hModel);
	mdxaHeader_t	*pMDXAHeader	= (mdxaHeader_t	*) RE_GetModelData(pMDXMHeader->animIndex);

	assert( iBoneIndex < pMDXAHeader->numBones);
	if (	iBoneIndex < pMDXAHeader->numBones)
	{
		mdxaSkelOffsets_t *pSkelOffsets = (mdxaSkelOffsets_t *) ((byte *)pMDXAHeader + sizeof(*pMDXAHeader));
		mdxaSkel_t *pSkelEntry = (mdxaSkel_t *) ((byte *) pSkelOffsets + pSkelOffsets->offsets[ iBoneIndex ] );

		return pSkelEntry->name;
	}

	return "GLMModel_GetBoneName(): Bad bone index";
}

LPCSTR GLMModel_BoneInfo( ModelHandle_t hModel, int iBoneIndex )
{
	mdxmHeader_t	*pMDXMHeader	= (mdxmHeader_t	*) RE_GetModelData(hModel);
	mdxaHeader_t	*pMDXAHeader	= (mdxaHeader_t	*) RE_GetModelData(pMDXMHeader->animIndex);

	assert( iBoneIndex < pMDXAHeader->numBones);
	if (	iBoneIndex < pMDXAHeader->numBones)
	{
		mdxaSkelOffsets_t *pSkelOffsets = (mdxaSkelOffsets_t *) ((byte *)pMDXAHeader + sizeof(*pMDXAHeader));
		mdxaSkel_t		  *pSkelEntry	= (mdxaSkel_t *) ((byte *) pSkelOffsets + pSkelOffsets->offsets[ iBoneIndex ] );

		static string str;

		str = va("Bone %d/%d:  %s\n\n", iBoneIndex, pMDXAHeader->numBones, pSkelEntry->name );
		str+= va("    ->parentIndex:\t%d\n",pSkelEntry->parent );
		str+= va("    ->numChildren:\t%d  ",pSkelEntry->numChildren );
		//
		// list children...
		//
		if (pSkelEntry->numChildren)
		{
			str += "( ";
			for (int i=0; i<pSkelEntry->numChildren; i++)
			{
				str += va("%s%d",!i?"":", ",pSkelEntry->children[i]);
			}
			str += " )";
		}
		str += "\n";

		// flags...
		//
		unsigned int iFlags = pSkelEntry->flags;
		str += va("    ->flags:\t0x%08X%s\n",iFlags,!iFlags?"":va("\t( Breakdown follows: )"));
		if (iFlags)
		{
			str += va("\t\t--------------------------------------------------\n");
			#define BONEFLAG(bit)										\
					if (iFlags & bit)									\
					{													\
						str += va("\t\t0x%08X:\t%s\n",bit,#bit);		\
						iFlags ^= bit;									\
					}

			BONEFLAG(G2BONEFLAG_ALWAYSXFORM);

			if (iFlags)
			{
				str += va("\t\t0x%08X:\tUNKNOWN FLAG(S)!!\n",iFlags);
			}
		}

		// list surfaces that use this bone...
		//
		str += "\nSurfaces using this bone:\n";
		// 2 passes, one to count, second to build string. count logic affects string display for infobox size reasons...
		//
		int iUsingCount=0;
		string strSurfacesUsing;
		for (int iPass=0; iPass<2; iPass++)	
		{
			strSurfacesUsing="";
			for (int iLOD = 0; iLOD < pMDXMHeader->numLODs; iLOD++)
			{
				string strSurfacesUsingItThisLOD;
				
				for (int iSurface = 0; iSurface < pMDXMHeader->numSurfaces; iSurface++)
				{
					if (GLMModel_SurfaceContainsBoneReference( hModel, iLOD, iSurface, iBoneIndex))
					{
						if (!iPass)
						{
							iUsingCount++;
						}
						else
						{
							LPCSTR psSurfaceName = GLMModel_GetSurfaceName( hModel, iSurface );
	
							if (iUsingCount>40)	// arb
							{
								strSurfacesUsingItThisLOD += va("%s,    ",psSurfaceName);
							}
							else
							{
								strSurfacesUsingItThisLOD += va("    %s\n",psSurfaceName);
							}
						}
					}
				}

				if (!strSurfacesUsingItThisLOD.empty())
				{
					strSurfacesUsing += va("LOD %d:\n",iLOD);
					strSurfacesUsing += strSurfacesUsingItThisLOD;
					strSurfacesUsing += "\n";
				}
			}
		}
		if (!strSurfacesUsing.empty())
		{
			str += strSurfacesUsing;
		}
		else
		{
			str += "<none>";
		}

		// anything else...
		//
		
		return str.c_str();
	}

	return "GLMModel_BoneInfo(): Bad bone index";
}


bool GLMModel_SurfaceContainsBoneReference(ModelHandle_t hModel, int iLODNumber, int iSurfaceNumber, int iBoneNumber)
{
	mdxmHeader_t			*pMDXMHeader		= (mdxmHeader_t	*) RE_GetModelData(hModel);
	mdxmHierarchyOffsets_t	*pHierarchyOffsets	= (mdxmHierarchyOffsets_t *) ((byte *) pMDXMHeader + sizeof(*pMDXMHeader));
//	mdxmSurfHierarchy_t		*pSurfHierarchy		= (mdxmSurfHierarchy_t *) ((byte *) pHierarchyOffsets + pHierarchyOffsets->offsets[iSurfaceNumber]);

//	if (iLODNumber >= pMDXMHeader->numLODs)
//		return false;	// can't reference it if we don't have a LOD of this level of course

	mdxmLOD_t *pLOD = (mdxmLOD_t *)((byte *)pMDXMHeader + pMDXMHeader->ofsLODs);
	for (int iLOD = 0; iLOD < iLODNumber && iLOD < pMDXMHeader->numLODs-1; iLOD++)
	{
		pLOD = (mdxmLOD_t *)((byte *)pLOD + pLOD->ofsEnd);
	}	

	mdxmLODSurfOffset_t *pLODSurfOffset = (mdxmLODSurfOffset_t *) &pLOD[1];
	mdxmSurface_t *pSurface = (mdxmSurface_t *) ((byte *) pLODSurfOffset +  pLODSurfOffset->offsets[iSurfaceNumber]);

	int *pBoneRefs = (int *) ( (byte *)pSurface + pSurface->ofsBoneReferences );
	for (int iBone = 0; iBone < pSurface->numBoneReferences; iBone++)
	{
		if (pBoneRefs[iBone] == iBoneNumber)
			return true;
	}

	return false;
}

bool GLMModel_SurfaceIsTag(ModelHandle_t hModel, int iSurfaceIndex )
{
	mdxmHeader_t			*pMDXMHeader		= (mdxmHeader_t	*) RE_GetModelData(hModel);
	mdxmHierarchyOffsets_t	*pHierarchyOffsets	= (mdxmHierarchyOffsets_t *) ((byte *) pMDXMHeader + sizeof(*pMDXMHeader));

	assert( iSurfaceIndex < pMDXMHeader->numSurfaces );
	if (	iSurfaceIndex < pMDXMHeader->numSurfaces )
	{
		mdxmSurfHierarchy_t *pSurfHierarchy	= (mdxmSurfHierarchy_t *) ((byte *) pHierarchyOffsets + pHierarchyOffsets->offsets[iSurfaceIndex]);
		return (pSurfHierarchy->flags & G2SURFACEFLAG_ISBOLT);
	}
	
	return false;
}


bool GLMModel_SurfaceIsON(ModelHandle_t hModel, int iSurfaceIndex )
{
	LPCSTR psSurfaceName = GLMModel_GetSurfaceName(hModel, iSurfaceIndex);

	ModelContainer_t *pContainer = ModelContainer_FindFromModelHandle( hModel );
	if (!pContainer)
	{
		ErrorBox(va("GLMModel_SurfaceIsON(): Illegal surface index %d!",iSurfaceIndex));
		return false;
	}

	SurfaceOnOff_t sOnOff = trap_G2_IsSurfaceOff (hModel, pContainer->slist, psSurfaceName);

	return (sOnOff == SURF_ON);
}


LPCSTR vtos(vec3_t v3)
{
	return va("%.3f %.3f %.3f",v3[0],v3[1],v3[2]);
}

static LPCSTR v2tos(vec2_t v2)
{
	return va("%.3f %.3f",v2[0],v2[1]);
}


// generate info suitable for sending to Notepad (can be a BIG string)...
//
LPCSTR GLMModel_SurfaceVertInfo( ModelHandle_t hModel, int iSurfaceIndex )
{
	static CString	str;
					str = "";

	mdxmHeader_t			*pMDXMHeader		= (mdxmHeader_t	*) RE_GetModelData(hModel);
	mdxmHierarchyOffsets_t	*pHierarchyOffsets	= (mdxmHierarchyOffsets_t *) ((byte *) pMDXMHeader + sizeof(*pMDXMHeader));
	mdxmSurfHierarchy_t		*pSurfHierarchy		= (mdxmSurfHierarchy_t *) ((byte *) pHierarchyOffsets + pHierarchyOffsets->offsets[iSurfaceIndex]);
	mdxmLOD_t				*pLOD				= (mdxmLOD_t *)		((byte *)pMDXMHeader + pMDXMHeader->ofsLODs);

	str += va("Detailed vert info for surface %d/%d (\"%s\") of model \"%s\":\n\n",
											  iSurfaceIndex,
												pMDXMHeader->numSurfaces,
														pSurfHierarchy->name,
																		pMDXMHeader->name);
	int iVert = 0;
													

	for (int iLOD = 0; iLOD < pMDXMHeader->numLODs; iLOD++)
	{
		mdxmLODSurfOffset_t *pLODSurfOffset = (mdxmLODSurfOffset_t *) &pLOD[1];
		mdxmSurface_t		*pSurface		= (mdxmSurface_t *) ((byte*)pLODSurfOffset +  pLODSurfOffset->offsets[iSurfaceIndex]);
		int *piBoneRefTable = (int*)((byte*)pSurface + pSurface->ofsBoneReferences);

		str+= va("LOD %d/%d:\n",iLOD,	pMDXMHeader->numLODs );
		str+= va("#Verts:\t%d\n",		pSurface->numVerts );

		mdxmVertex_t			*pVert			= (mdxmVertex_t *) ((byte *)pSurface + pSurface->ofsVerts);
		mdxmVertexTexCoord_t	*pVertTexCoords	= (mdxmVertexTexCoord_t	*) &pVert[pSurface->numVerts];
		for (iVert=0; iVert < pSurface->numVerts; iVert++)
		{
			StatusMessage(va("LOD %d/%d, Dumping Vert %d/%d",iLOD,pMDXMHeader->numLODs,iVert,pSurface->numVerts));

			const int iNumWeights = G2_GetVertWeights( pVert );

			str += va("    Vert %d/%d:\n",iVert,pSurface->numVerts);
			str += va("    ->normal:        %s\n",vtos(pVert->normal));
			str += va("    ->vertCoords:    %s\n",vtos(pVert->vertCoords));
			str += va("    ->texCoords:     %s\n",v2tos(pVertTexCoords[iVert].texCoords));
			str += va("    ->numWeights:    %d\n",iNumWeights);
				
			float fTotalWeight = 0.0f;
			for (int iWeight=0; iWeight<iNumWeights; iWeight++)
			{
				int		iBoneIndex	= G2_GetVertBoneIndex( pVert, iWeight );
				float	fBoneWeight	= G2_GetVertBoneWeight( pVert, iWeight, fTotalWeight, iNumWeights );

				iBoneIndex = piBoneRefTable[iBoneIndex];	//!!!!!!!!

				str += va("                     Bone: %d ( \"%s\" ), Weight:%3f\n",iBoneIndex, GLMModel_GetBoneName( hModel, iBoneIndex ),fBoneWeight);
			}
			
			str += "\n";

			pVert++;// = (mdxmVertex_t *)&pVert->weights[/*pVert->numWeights*/pSurface->maxVertBoneWeights];
		}

		// new bit, check every vert against every other in this surface for co-existing points with differing texture coords...
		//
		StatusMessage(va("LOD %d/%d, Analysing duplicate verts ( temp / tinkering code )",iLOD,pMDXMHeader->numLODs));
		pVert = (mdxmVertex_t *) ((byte *)pSurface + pSurface->ofsVerts);
		for (iVert=0; iVert<pSurface->numVerts; iVert++)
		{				
			mdxmVertex_t *pOtherVert = (mdxmVertex_t *) ((byte *)pSurface + pSurface->ofsVerts);
			for (int iOtherVert=0; iOtherVert<iVert; iOtherVert++)
			{
				if (iOtherVert != iVert)
				{
#define VectorEqual(a,b)		(fabs((a)[0]-(b)[0])<0.001f && fabs((a)[1]-(b)[1])<0.001f && fabs((a)[2]-(b)[2])<0.001f)

					if (VectorEqual(pVert->vertCoords,pOtherVert->vertCoords)
						&&
						VectorEqual(pVert->normal,pOtherVert->normal)
						&&
						!VectorEqual(pVertTexCoords[iVert].texCoords, pVertTexCoords[iOtherVert].texCoords)
						)
					{
						str += va("Vert %d and %d have same coords\n",iVert,iOtherVert);
						str += "... and the same normal\n";
						str += "... but different texCoords\n";
					}
				}

				pOtherVert++;// = (mdxmVertex_t *)&pOtherVert->weights[/*pOtherVert->numWeights*/pSurface->maxVertBoneWeights];
			}

			pVert++;// = (mdxmVertex_t *)&pVert->weights[/*pVert->numWeights*/pSurface->maxVertBoneWeights];
		}

		// next LOD...
		//
		pLOD = (mdxmLOD_t *)((byte *)pLOD + pLOD->ofsEnd);
	}
	
	StatusMessage(NULL);
	return (LPCSTR) str;//str.c_str();
}



LPCSTR GLMModel_SurfaceInfo( ModelHandle_t hModel, int iSurfaceIndex, bool bShortVersionForTag )
{	
	mdxmHeader_t	*pMDXMHeader	= (mdxmHeader_t	*) RE_GetModelData(hModel);
//	mdxaHeader_t	*pMDXAHeader	= (mdxaHeader_t	*) RE_GetModelData(pMDXMHeader->animIndex);

	mdxmHierarchyOffsets_t	*pHierarchyOffsets	= (mdxmHierarchyOffsets_t *) ((byte *) pMDXMHeader + sizeof(*pMDXMHeader));
	mdxmSurfHierarchy_t		*pSurfHierarchy		= (mdxmSurfHierarchy_t *) ((byte *) pHierarchyOffsets + pHierarchyOffsets->offsets[iSurfaceIndex]);

	assert ( iSurfaceIndex < pMDXMHeader->numSurfaces );

	static string str;

	str = va("%sSurface %d/%d:  '%s'\n\n", (pSurfHierarchy->flags & G2SURFACEFLAG_ISBOLT)?"Tag-":"",iSurfaceIndex, pMDXMHeader->numSurfaces, pSurfHierarchy->name );

if (bShortVersionForTag)
{
}
else
{
	str+= va("    ->shader:\t%s\n",		pSurfHierarchy->shader );
}


	// flags...
	//
	unsigned int iFlags = pSurfHierarchy->flags;
	str += va("    ->flags:\t0x%08X%s\n",iFlags,!iFlags?"":va("\t( Breakdown follows: )"));
	if (iFlags)
	{
		str += va("\t\t--------------------------------------------------\n");
		#define SURFFLAG(bit)										\
				if (iFlags & bit)									\
				{													\
					str += va("\t\t0x%08X:\t%s\n",bit,#bit);		\
					iFlags ^= bit;									\
				}

		SURFFLAG(G2SURFACEFLAG_ISBOLT);
		SURFFLAG(G2SURFACEFLAG_OFF);

		if (iFlags)
		{
			str += va("\t\t0x%08X:\tUNKNOWN FLAG(S)!!\n",iFlags);
		}
	}




	str+= va("    ->parentIndex:\t%d\n",pSurfHierarchy->parentIndex );
	str+= va("    ->numChildren:\t%d  ",pSurfHierarchy->numChildren );
	//
	// list children...
	//
	if (pSurfHierarchy->numChildren)
	{
		str += "( ";
		for (int i=0; i<pSurfHierarchy->numChildren; i++)
		{
			str += va("%s%d",!i?"":", ",pSurfHierarchy->childIndexes[i]);
		}
		str += " )";
	}
	str += "\n";

	// anything else...
	//

	str+= "\n";
	mdxmLOD_t *pLOD = (mdxmLOD_t *)((byte *)pMDXMHeader + pMDXMHeader->ofsLODs);
	for (int iLOD = 0; iLOD < pMDXMHeader->numLODs; iLOD++)
	{
		mdxmLODSurfOffset_t *pLODSurfOffset = (mdxmLODSurfOffset_t *) &pLOD[1];
		mdxmSurface_t		*pSurface		= (mdxmSurface_t *) ((byte*)pLODSurfOffset +  pLODSurfOffset->offsets[iSurfaceIndex]);

		str+= va("    LOD %d/%d:\n",iLOD, pMDXMHeader->numLODs );
	if (bShortVersionForTag)	// may actually be useful to know these? oh well, for now... inhibit
	{
	}
	else
	{
		str+= va("        # Verts:\t%d\n",		pSurface->numVerts );
		str+= va("        # Tris:\t%d\n",		pSurface->numTriangles );
	}
		str+= va("        # BoneRefs:\t%d\n",	pSurface->numBoneReferences );
		//
		// list bone refs...
		//
		if (pSurface->numBoneReferences)
		{
			int *pBoneRefs = (int *) ( (byte *)pSurface + pSurface->ofsBoneReferences );

//			str += "( ";
			for (int i=0; i<pSurface->numBoneReferences; i++)
			{
//				str += va("%s%d",!i?"":", ",pBoneRefs[i]);
				str += va("                   \t( %3d )  \"%s\"\n",pBoneRefs[i],GLMModel_GetBoneName(hModel, pBoneRefs[i] ));
			}
//			str += " )";
		}
//		str+= "\n";

	if (bShortVersionForTag)	// may actually be useful to know these? oh well, for now... inhibit
	{
	}
	else
	{
//		str+= va("        # MaxVertBoneWeights:\t %d\n", pSurface->maxVertBoneWeights);
	}


		str+= "\n";

		pLOD = (mdxmLOD_t *)((byte *)pLOD + pLOD->ofsEnd);
	}

	return str.c_str();
}


// provides common functionality to save duping logic...
//
static LPCSTR GLMModel_CreateSurfaceName( LPCSTR psSurfaceName, bool bOnOff)
{
	static CString string;
	
	string = psSurfaceName;	// do NOT use in constructor form since this is a static (that got me first time... :-)

	if (!bOnOff)
	{
		string.Insert(0, "//////// ");
	}

	return (LPCSTR) string;
}


static bool R_GLM_AddSurfaceToTree( ModelHandle_t hModel, HTREEITEM htiParent, int iThisSurfaceIndex, mdxmHierarchyOffsets_t *pHierarchyOffsets, bool bTagsOnly)
{
	bool bReturn = true;

	mdxmSurfHierarchy_t *pSurfHierarchy_This = (mdxmSurfHierarchy_t *) ((byte *) pHierarchyOffsets + pHierarchyOffsets->offsets[iThisSurfaceIndex]);


	// insert me...
	//
	TreeItemData_t	TreeItemData={0};
					TreeItemData.iItemType		= bTagsOnly ? TREEITEMTYPE_GLM_TAGSURFACE : TREEITEMTYPE_GLM_SURFACE;
					TreeItemData.iModelHandle	= hModel;
					TreeItemData.iItemNumber	= iThisSurfaceIndex;					

	HTREEITEM htiThis = NULL;
	if (!bTagsOnly || (pSurfHierarchy_This->flags & G2SURFACEFLAG_ISBOLT) )
	{
		htiThis = ModelTree_InsertItem(	GLMModel_CreateSurfaceName(pSurfHierarchy_This->name, true),	// LPCTSTR psName, 
										htiParent,			// HTREEITEM hParent
										TreeItemData.uiData,// TREEITEMTYPE_GLM_SURFACE | iThisSurfaceIndex	// UINT32 uiUserData
										bTagsOnly?TVI_SORT:TVI_LAST
										);
	}

	// insert my children...
	//
	for (int iChild = 0; iChild < pSurfHierarchy_This->numChildren; iChild++)
	{
		R_GLM_AddSurfaceToTree(	hModel, bTagsOnly?htiParent:htiThis, pSurfHierarchy_This->childIndexes[iChild], pHierarchyOffsets, bTagsOnly );
	}


	return bReturn;
}



static bool R_GLM_AddBoneToTree( ModelHandle_t hModel, HTREEITEM htiParent, int iThisBoneIndex, mdxaSkelOffsets_t* pSkelOffsets)
{
	bool bReturn = true;

	mdxaSkel_t *pSkeletonEntry_This = (mdxaSkel_t *) ((byte *) pSkelOffsets + pSkelOffsets->offsets[ iThisBoneIndex ] );

	
	// insert me...
	//
	TreeItemData_t	TreeItemData={0};
					TreeItemData.iItemType		= TREEITEMTYPE_GLM_BONE;
					TreeItemData.iModelHandle	= hModel;
					TreeItemData.iItemNumber	= iThisBoneIndex;

	HTREEITEM htiThis = ModelTree_InsertItem(	pSkeletonEntry_This->name,	// LPCTSTR psName, 
												htiParent,					// HTREEITEM hParent
												TreeItemData.uiData	//	TREEITEMTYPE_GLM_BONE | iThisBoneIndex	// UINT32 uiUserData
												);

	// insert my children...
	//
	for (int iChild = 0; iChild < pSkeletonEntry_This->numChildren; iChild++)
	{
		R_GLM_AddBoneToTree( hModel, htiThis, pSkeletonEntry_This->children[iChild], pSkelOffsets);
	}

	return bReturn;
}


// Note, this function is only really supposed to be called once, to setup the Container that owns this model
//
static int GLMModel_GetNumFrames( ModelHandle_t hModel )
{
	// I should really try-catch these, but for now...
	//
	mdxmHeader_t	*pMDXMHeader	= (mdxmHeader_t	*) RE_GetModelData(hModel);
	mdxaHeader_t	*pMDXAHeader	= (mdxaHeader_t	*) RE_GetModelData(pMDXMHeader->animIndex);

	return pMDXAHeader->numFrames;
}

// Note, this function is only really supposed to be called once, to setup the Container that owns this model
//
static int GLMModel_GetNumBones( ModelHandle_t hModel )
{
	// I should really try-catch these, but for now...
	//
	mdxmHeader_t	*pMDXMHeader	= (mdxmHeader_t	*) RE_GetModelData(hModel);
	mdxaHeader_t	*pMDXAHeader	= (mdxaHeader_t	*) RE_GetModelData(pMDXMHeader->animIndex);

	return pMDXAHeader->numBones;
}


// Note, this function is only really supposed to be called once, to setup the Container that owns this model
//
static int GLMModel_GetNumSurfaces( ModelHandle_t hModel )
{
	// I should really try-catch these, but for now...
	//
	mdxmHeader_t	*pMDXMHeader	= (mdxmHeader_t	*) RE_GetModelData(hModel);	

	return pMDXMHeader->numSurfaces;
}



// Note, this function is only really supposed to be called once, to setup the Container that owns this model
//
static int GLMModel_GetNumLODs( ModelHandle_t hModel )
{
	// I should really try-catch this, but for now...
	//
	mdxmHeader_t *pMDXMHeader = (mdxmHeader_t *) RE_GetModelData(hModel);
	return pMDXMHeader->numLODs;
}

// these next 2 functions are closely related, the GetCount function fills in public data which the other reads on query
//
set <string> stringSet;
static int GLMModel_GetUniqueShaderCount( ModelHandle_t hModel )
{	
	mdxmHeader_t	*pMDXMHeader	= (mdxmHeader_t	*) RE_GetModelData(hModel);
//	mdxaHeader_t	*pMDXAHeader	= (mdxaHeader_t	*) RE_GetModelData(pMDXMHeader->animIndex);

	mdxmHierarchyOffsets_t	*pHierarchyOffsets	= (mdxmHierarchyOffsets_t *) ((byte *) pMDXMHeader + sizeof(*pMDXMHeader));

	stringSet.clear();
	
	for (int iSurfaceIndex = 0; iSurfaceIndex < pMDXMHeader->numSurfaces; iSurfaceIndex++)
	{
		mdxmSurfHierarchy_t	*pSurfHierarchy = (mdxmSurfHierarchy_t *) ((byte *) pHierarchyOffsets + pHierarchyOffsets->offsets[iSurfaceIndex]);

		string strShader(pSurfHierarchy->shader);

		stringSet.insert(stringSet.end(),strShader);
	}

	return stringSet.size();
}
static LPCSTR GLMModel_GetUniqueShader(int iShader)
{
	assert(iShader < stringSet.size());
	
	for (set <string>::iterator it = stringSet.begin(); it != stringSet.end(); ++it)
	{
		if (!iShader--)
			return (*it).c_str();
	}	

	return "(Error)";	// should never get here
}

// interesting use of static here, this function IS called externally, but only through a ptr. 
//	This is to stop people accessing it directly.
//
// return basic info on the supplied model arg...
//
static LPCSTR GLMModel_Info( ModelHandle_t hModel )
{
	// I should really try-catch these, but for now...
	//
	mdxmHeader_t	*pMDXMHeader	= (mdxmHeader_t	*) RE_GetModelData(hModel);
	mdxaHeader_t	*pMDXAHeader	= (mdxaHeader_t	*) RE_GetModelData(pMDXMHeader->animIndex);

	static string str;

	ModelContainer_t *pContainer = ModelContainer_FindFromModelHandle( hModel );

	str = va("Model: %s%s\n\n", Model_GetFilename(hModel), !pContainer?"":!pContainer->pBoneBolt_ParentContainer?(!pContainer->pSurfaceBolt_ParentContainer?"":va("   ( Bolted to parent model via surface bolt '%s' )",Model_GetBoltName(pContainer->pSurfaceBolt_ParentContainer,pContainer->iSurfaceBolt_ParentBoltIndex,false))):va("   ( Bolted to parent model via bone bolt '%s' )",Model_GetBoltName(pContainer->pBoneBolt_ParentContainer,pContainer->iBoneBolt_ParentBoltIndex,true)));
	str+= va("GLM Info:\t\t( FileSize: %d bytes )\n\n",pMDXMHeader->ofsEnd);
	str+= va("    ->ident :\t%X\n",		pMDXMHeader->ident );	// extra space before ':' here to push tab to next column
	str+= va("    ->version:\t%d\n",	pMDXMHeader->version );
	str+= va("    ->name:\t%s\n",		pMDXMHeader->name );
	str+= va("    ->animName:\t%s\n",	pMDXMHeader->animName );
	str+= va("    ->numBones:\t%d\n",	pMDXMHeader->numBones );
	str+= va("    ->numLODs:\t%d\n",	pMDXMHeader->numLODs );

	// work out what types of surfaces we have for extra info...
	//
	int iNumTagSurfaces=0;
	int iNumOFFSurfaces=0;
	for (int i=0; i<pMDXMHeader->numSurfaces; i++)
	{
		if (GLMModel_SurfaceIsTag(hModel, i))
			iNumTagSurfaces++;

		LPCSTR psSurfaceName = GLMModel_GetSurfaceName( hModel, i);
		if (!stricmp("_off",&psSurfaceName[strlen(psSurfaceName)-4]))
			iNumOFFSurfaces++;
	}

	str+= va("    ->numSurfaces:\t%d",pMDXMHeader->numSurfaces );

	if (iNumTagSurfaces || iNumOFFSurfaces)
	{
		str+= va("  ( = %d default", (pMDXMHeader->numSurfaces - iNumTagSurfaces) - iNumOFFSurfaces);

		if (iNumTagSurfaces)
		{
			str+= va(" + %d TAG",iNumTagSurfaces);
		}
		if (iNumOFFSurfaces)
		{
			str+= va(" + %d OFF",iNumOFFSurfaces);
		}
		str+= " )";
	}
	str+= "\n";


	// show shader usage...
	//
	if (pContainer->SkinSets.size())
	{
		// model uses skin files...
		//
		str += va("\n\nSkin Shader Info:\n\nSkin File:\t\t%s\nEthnic ver:\t%s\n",
														pContainer->strCurrentSkinFile.c_str(),
																		pContainer->strCurrentSkinEthnic.c_str()
					);																		
	}
	else
	if (pContainer->OldSkinSets.size())
	{
		// model is using old EF1/ID type skins...
		//
		str += va("\n\nSkin Info:\n\nSkin File:\t\t%s\n", pContainer->strCurrentSkinFile.c_str());
	}
	else
	{
		// standard shaders...
		//
		int iUniqueShaderCount = GLMModel_GetUniqueShaderCount( hModel );
		str += va("\n\nShader Info:\t( %d unique shaders )\n\n", iUniqueShaderCount);
		for (int iUniqueShader = 0; iUniqueShader < iUniqueShaderCount; iUniqueShader++)
		{
			bool bFound = false;

			LPCSTR	psShaderName		= GLMModel_GetUniqueShader( iUniqueShader );
			LPCSTR	psLocalTexturePath	= R_FindShader( psShaderName );
		
			if (psLocalTexturePath && strlen(psLocalTexturePath))
			{
				TextureHandle_t hTexture		= TextureHandle_ForName( psLocalTexturePath );
				GLuint			uiBind			= (hTexture == -1)?0:Texture_GetGLBind( hTexture );
				bFound = (uiBind||!stricmp(psShaderName,"[NoMaterial]"));			
			}
			str += va("     %s%s\n",String_EnsureMinLength(psShaderName[0]?psShaderName:"<blank>",16/*arb*/),(!bFound)?"\t(Not Found)":"");
		}
	}

	// show anim file details...
	//
		
	str+= va("\n\nGLA Info:\t\t( FileSize: %d bytes )\n\n",pMDXAHeader->ofsEnd);

//	mdxaHeader_t *pMDXAHeader = (mdxaHeader_t *) pvLoadedGLA;

	str+= va("    ->ident :\t%X\n",		pMDXAHeader->ident );	// extra space before ':' here to push tab to next column
	str+= va("    ->version:\t%d\n",	pMDXAHeader->version );
	str+= va("    ->name:\t%s\n",		pMDXAHeader->name );
	str+= va("    ->scale:\t%g\n",		(pMDXAHeader->fScale == 0.0f)?1.0f:pMDXAHeader->fScale);
	str+= va("    ->numFrames:\t%d\n",	pMDXAHeader->numFrames );
	str+= va("    ->ofsFrames:\t%d\n",	pMDXAHeader->ofsFrames );
	str+= va("    ->numBones:\t%d\n",	pMDXAHeader->numBones );
	str+= va("    ->ofsCompBonePool:\t%d\n",	pMDXAHeader->ofsCompBonePool );
	str+= va("    ->ofsSkel:\t%d\n",	pMDXAHeader->ofsSkel );

	return str.c_str();
}



// call this to re-evaluate any part of the tree that has surfaces owned by this model, and set their text ok...
//
// hTreeItem = tree item to start from, pass NULL to start from root
//
bool R_GLMModel_Tree_ReEvalSurfaceText(ModelHandle_t hModel, HTREEITEM hTreeItem /* = NULL */, bool bDeadFromHereOn /* = false */)
{
	bool bReturn = false;

	if (!hTreeItem)
		 hTreeItem = ModelTree_GetRootItem();

	if (hTreeItem)
	{
		// process this tree item...
		//
		TreeItemData_t	TreeItemData;
						TreeItemData.uiData = ModelTree_GetItemData(hTreeItem);

		if (TreeItemData.iModelHandle == hModel)
		{
			// ok, tree item belongs to this model, so what is it?...
			//
			bool bKillMyChildren = false;

			ModelContainer_t *pContainer = ModelContainer_FindFromModelHandle(hModel);
			if (pContainer)
			{
				if (TreeItemData.iItemType == TREEITEMTYPE_GLM_SURFACE)
				{
					// it's a surface, so re-eval its text...
					//
					LPCSTR psSurfaceName = GLMModel_GetSurfaceName( hModel, TreeItemData.iItemNumber );

					// a little harmless optimisation here...
					//
					SurfaceOnOff_t sOnOff = bDeadFromHereOn ? SURF_OFF : trap_G2_IsSurfaceOff (hModel, pContainer->slist, psSurfaceName);

					ModelTree_SetItemText( hTreeItem, GLMModel_CreateSurfaceName( psSurfaceName, bDeadFromHereOn?false:(sOnOff == SURF_ON)));
					
					if (sOnOff == SURF_NO_DESCENDANTS)
					{
						bKillMyChildren = true;
					}
				}
			}

			// process child...
			//
			HTREEITEM hTreeItem_Child = ModelTree_GetChildItem(hTreeItem);
			if (hTreeItem_Child)
				R_GLMModel_Tree_ReEvalSurfaceText(hModel, hTreeItem_Child, (bDeadFromHereOn || bKillMyChildren) );


			// process siblings...
			//
			HTREEITEM hTreeItem_Sibling = ModelTree_GetNextSiblingItem(hTreeItem);
			if (hTreeItem_Sibling)
				R_GLMModel_Tree_ReEvalSurfaceText(hModel, hTreeItem_Sibling, bDeadFromHereOn);
		}
	}

	return true;
}


// read an optional set of skin files (SOF2-style), and if present, add them into the model tree...
//
// return is success/fail (but it's an optional file, so return bool is just FYI really)
//    (note that partial failures still count as successes, as long as at least one file succeeds)
//
static bool GLMModel_ReadSkinFiles(HTREEITEM hParent, ModelContainer_t *pContainer, LPCSTR psLocalFilename)
{
	// check for optional .g2skin files... (SOF2-type)
	//
	if (Skins_Read(psLocalFilename, pContainer))	
	{
		return Skins_ApplyToTree(hParent, pContainer);
	}

	// check for optional .skin files... (CHC-type)
	//
	if (OldSkins_Read(psLocalFilename, pContainer))
	{
		return OldSkins_ApplyToTree(hParent, pContainer);
	}

	return false;
}
	  

// read an optional bone alias file, and if present then add into the model tree...
//  (this may become a generic model function, not just a GLMModel one, but for now...
//
// return is success/fail (but it's an optional file, so return bool is just FYI really)
//    (note that partial failures still count as successes, as long as at least one alias succeeds)
//
static bool GLMModel_ReadBoneAliasFile(HTREEITEM hParent, HTREEITEM hInsertAfter, ModelContainer_t *pContainer, LPCSTR psLocalFilename)
{	
	TreeItemData_t	TreeItemData = {0};
					TreeItemData.iModelHandle = pContainer->hModel;

	HTREEITEM hTreeItem_BoneAliases	= NULL;
	bool bReturn = false;


	// check for optional alias file...
	//
	CString strALIASFilename(va("%s%s.alias",gamedir,psLocalFilename));
	CString strErrors;

	if (FileExists(strALIASFilename))
	{
		TreeItemData.iItemType	= TREEITEMTYPE_BONEALIASHEADER;
		hTreeItem_BoneAliases	= ModelTree_InsertItem("Bone Aliases", hParent, TreeItemData.uiData, hInsertAfter);
	
		if (hTreeItem_BoneAliases)
		{	
			if (Parser_Load(strALIASFilename, pContainer->Aliases))
			{
				for (MappedString_t::iterator it = pContainer->Aliases.begin(); it != pContainer->Aliases.end(); ++it)
				{
					CString strBoneName_Real (((*it).first).c_str());
					CString strBoneName_Alias(((*it).second).c_str());

					int iRealBoneIndex = ModelContainer_BoneIndexFromName(pContainer, strBoneName_Real);

					if (iRealBoneIndex != -1)
					{
						TreeItemData.iItemType	= TREEITEMTYPE_GLM_BONEALIAS;
						TreeItemData.iItemNumber= iRealBoneIndex;

						ModelTree_InsertItem(strBoneName_Alias, hTreeItem_BoneAliases, TreeItemData.uiData );

						bReturn = true;
					}
					else
					{
						strErrors += va("Bone: \"%s\" (Alias: \"%s\")",(LPCSTR) strBoneName_Real, (LPCSTR) strBoneName_Alias);
					}
				}				
			}
			else
			{
				TreeItemData.iItemType = TREEITEMTYPE_NULL;
				ModelTree_InsertItem("Error during parse!",	hTreeItem_BoneAliases, TreeItemData.uiData);
			}
		}	
	}

	if (!strErrors.IsEmpty())
	{
		strErrors.Insert(0,va("The following bone names in the alias file: \"%s\"\n\n...had no corresponding bones in the anim file for the model: \"%s\"\n\n\n",(LPCSTR) strALIASFilename, pContainer->sLocalPathName));
		ErrorBox(strErrors);
	}

	bReturn = true;

	return bReturn;
}


// return = true if some sequences created (because of having found a valid animation file, 
//	either "<modelname>.frames" (SOF2) or "animation.cfg" (CHC)...
//
static bool GLMModel_ReadSequenceInfo(HTREEITEM hTreeItem_Root, ModelContainer_t *pContainer, LPCSTR psLocalFilename_GLA)
{
	assert(hTreeItem_Root);

	// try a CHC-style "animation.cfg" file...
	//
	bool bFromANIMATIONCFG = Anims_ReadFile_ANIMATION_CFG(pContainer, psLocalFilename_GLA);
	//
	// if no joy with that, try a SOF2 "<name>.frames" file...
	//
	if (!bFromANIMATIONCFG)
	{
		Anims_ReadFile_FRAMES(pContainer, psLocalFilename_GLA);
	}

	// now add to tree if we found something...
	//
	if (pContainer->SequenceList.size())
	{
		TreeItemData_t	TreeItemData={0};
						TreeItemData.iItemType		= TREEITEMTYPE_SEQUENCEHEADER;
						TreeItemData.iModelHandle	= pContainer->hModel;

						HTREEITEM hTreeItem_Sequences = ModelTree_InsertItem(va("Sequences %s",bFromANIMATIONCFG?"( From animation.cfg )":"( From .frames file )"), hTreeItem_Root, TreeItemData.uiData);

						ModelTree_InsertSequences(pContainer, hTreeItem_Sequences);		
	}

	return !!(pContainer->SequenceList.size());
}


// if we get this far now then we no longer need to check the model data because RE_RegisterModel has already 
//	validated it (and loaded the GLA file!). Oh well...
//
// this MUST be called after Jake's code has finished, since I read from his tables...
//
bool GLMModel_Parse(struct ModelContainer *pContainer, LPCSTR psLocalFilename, HTREEITEM hTreeItem_Parent /* = NULL */)
{
	bool bReturn = false;

	ModelHandle_t hModel = pContainer->hModel;

	mdxmHeader_t	*pMDXMHeader	= (mdxmHeader_t	*) RE_GetModelData(hModel);
	mdxaHeader_t	*pMDXAHeader	= (mdxaHeader_t	*) RE_GetModelData(pMDXMHeader->animIndex);
	
	HTREEITEM hTreeItem_Bones		= NULL;

	if (pMDXMHeader->ident == MDXM_IDENT)
	{
		if (pMDXMHeader->version == MDXM_VERSION)
		{
//			// now see if we can find the corresponding GLA file...
//			//
//			CString strGLAFilename(va("%s%s.gla",gamedir,pMDXMHeader->animName));
//
//			if (FileExists(strGLAFilename))
			{
//				assert(pvLoadedGLA == NULL);

//				int iFilelenGLA = LoadFile( strGLAFilename, &pvLoadedGLA, true );	// bReportErrors

//				if (iFilelenGLA != -1)
				{
					mdxaHeader_t *pMDXAHeader = (mdxaHeader_t *) RE_GetModelData( pMDXMHeader->animIndex );

					if (pMDXAHeader->ident == MDXA_IDENT)
					{
						if (pMDXAHeader->version == MDXA_VERSION)
						{
							// phew, all systems go...
							//
							bReturn = true;
			
							TreeItemData_t	TreeItemData={0};
											TreeItemData.iModelHandle = hModel;
									
							TreeItemData.iItemType	= TREEITEMTYPE_MODELNAME;
							pContainer->hTreeItem_ModelName = ModelTree_InsertItem(va("==>  %s  <==",Filename_WithoutPath(/*Filename_WithoutExt*/(psLocalFilename))), hTreeItem_Parent, TreeItemData.uiData);

							TreeItemData.iItemType	= TREEITEMTYPE_SURFACEHEADER;
				  HTREEITEM hTreeItem_Surfaces		= ModelTree_InsertItem("Surfaces",	pContainer->hTreeItem_ModelName, TreeItemData.uiData);

							TreeItemData.iItemType	= TREEITEMTYPE_TAGSURFACEHEADER;
				  HTREEITEM hTreeItem_TagSurfaces	= ModelTree_InsertItem("Tag Surfaces",	pContainer->hTreeItem_ModelName, TreeItemData.uiData);

							TreeItemData.iItemType	= TREEITEMTYPE_BONEHEADER;
							hTreeItem_Bones			= ModelTree_InsertItem("Bones",		pContainer->hTreeItem_ModelName, TreeItemData.uiData);

							// send surface heirarchy to tree...
							//
							mdxmHierarchyOffsets_t *pHierarchyOffsets = (mdxmHierarchyOffsets_t *) ((byte *) pMDXMHeader + sizeof(*pMDXMHeader));

							R_GLM_AddSurfaceToTree( hModel, hTreeItem_Surfaces, 0, pHierarchyOffsets, false);
							R_GLM_AddSurfaceToTree( hModel, hTreeItem_TagSurfaces, 0, pHierarchyOffsets, true);

							// special error check for badly-hierarchied surfaces... (bad test data inadvertently supplied by Rob Gee :-)
							//
							int iNumSurfacesInTree = ModelTree_GetChildCount(hTreeItem_Surfaces);
							if (iNumSurfacesInTree != pMDXMHeader->numSurfaces)
							{
								ErrorBox(va("Model has %d surfaces, but only %d of them are connected up through the heirarchy, the rest will never be recursed into.\n\nThis model needs rebuilding, guys...",pMDXMHeader->numSurfaces,iNumSurfacesInTree));
								bReturn = false;
							}

							if (!ModelTree_ItemHasChildren( hTreeItem_TagSurfaces ))
							{
								ModelTree_DeleteItem( hTreeItem_TagSurfaces );
							}

							// send bone heirarchy to tree...
							//
							mdxaSkelOffsets_t *pSkelOffsets = (mdxaSkelOffsets_t *) ((byte *)pMDXAHeader + sizeof(*pMDXAHeader));

							R_GLM_AddBoneToTree( hModel, hTreeItem_Bones, 0, pSkelOffsets);
						}
						else
						{
							ErrorBox(va("Wrong GLA format version number: %d (expecting %d)\n\n( file: \"%s\" )",pMDXAHeader->version, MDXA_VERSION, pMDXAHeader->name ));
						}
					}
					else
					{
						ErrorBox(va("Wrong GLA file ident: %X (expecting %X)\n\n( file: \"%s\" )",pMDXAHeader->ident, MDXA_IDENT, pMDXAHeader->name ));
					}
				}
//				else
//				{
//					ErrorBox(va("Unable to find corresponding animation file: \"%s\"!",(LPCSTR)strGLAFilename));
//				}
			}
//			else
//			{
//				ErrorBox(va("Unable to find corresponding GLA file \"%s\"!",(LPCSTR)strGLAFilename));
//			}
		}
		else
		{
			ErrorBox(va("Wrong model format version number: %d (expecting %d)",pMDXMHeader->version, MDXM_VERSION));
		}
	}
	else
	{
		ErrorBox(va("Wrong model Ident: %X (expecting %X)",pMDXMHeader->ident, MDXM_IDENT));
	}
			

	if (bReturn)
	{
		bReturn = R_GLMModel_Tree_ReEvalSurfaceText(hModel);

		if (bReturn)
		{
			// let's try looking for "<modelname>.frames" in the same dir for simple sequence info...
			//
			{
				// now fill in the fields we need in the container to avoid GLM-specific queries...
				//
				pContainer->pModelInfoFunction				= GLMModel_Info;
				pContainer->pModelGetBoneNameFunction		= GLMModel_GetBoneName;
				pContainer->pModelGetBoneBoltNameFunction	= GLMModel_GetBoneName;	// same thing in this format
				pContainer->pModelGetSurfaceNameFunction	= GLMModel_GetSurfaceName;
				pContainer->pModelGetSurfaceBoltNameFunction= GLMModel_GetSurfaceName;	// same thing in this format
				pContainer->iNumFrames		= GLMModel_GetNumFrames	( hModel );
				pContainer->iNumLODs		= GLMModel_GetNumLODs	( hModel );
				pContainer->iNumBones		= GLMModel_GetNumBones	( hModel );
				pContainer->iNumSurfaces	= GLMModel_GetNumSurfaces(hModel );

				pContainer->iBoneBolt_MaxBoltPoints		= pContainer->iNumBones;	// ... since these are pretty much the same in this format
				pContainer->iSurfaceBolt_MaxBoltPoints	= pContainer->iNumSurfaces;	// ... since these are pretty much the same in this format

				GLMModel_ReadSkinFiles	  (pContainer->hTreeItem_ModelName, pContainer, psLocalFilename);
				GLMModel_ReadSequenceInfo (pContainer->hTreeItem_ModelName, pContainer, pMDXMHeader->animName);
				GLMModel_ReadBoneAliasFile(pContainer->hTreeItem_ModelName, hTreeItem_Bones, pContainer, pMDXMHeader->animName);
			}
		}
	}

	return bReturn;
}





SurfaceOnOff_t GLMModel_Surface_GetStatus( ModelHandle_t hModel, int iSurfaceIndex )
{
	ModelContainer_t *pContainer = ModelContainer_FindFromModelHandle(hModel);
	
	if (pContainer)
	{
		LPCSTR psSurfaceName = GLMModel_GetSurfaceName( hModel, iSurfaceIndex );
		
		return trap_G2_IsSurfaceOff(hModel, pContainer->slist, psSurfaceName);
	}
	return SURF_ERROR;
}

bool GLMModel_Surface_SetStatus( ModelHandle_t hModel, int iSurfaceIndex, SurfaceOnOff_t eStatus )
{
	bool bReturn = false;

	ModelContainer_t *pContainer = ModelContainer_FindFromModelHandle(hModel);

	if (pContainer)
	{
		LPCSTR psSurfaceName = GLMModel_GetSurfaceName( hModel, iSurfaceIndex );

		if (trap_G2_SetSurfaceOnOff (hModel, pContainer->slist, psSurfaceName, eStatus, iSurfaceIndex))
		{
			R_GLMModel_Tree_ReEvalSurfaceText(hModel);
			bReturn = true;
		}
		else
		{
			ErrorBox(va("G2_SetSurfaceOnOff(): Error, probably too many surfaces turned off? (max = %d)",MAX_G2_SURFACES));
		}
	}

	if (bReturn)
		ModelList_ForceRedraw();

	return bReturn;
}

// only call this once the model is fully up and running, not as part of the load code (because of container usage)...
//
void GLMModel_Surfaces_DefaultAll(ModelHandle_t hModel)
{
	ModelContainer_t *pContainer = ModelContainer_FindFromModelHandle(hModel);

	if (pContainer)
	{
		trap_G2_SurfaceOffList(hModel, &pContainer->slist);
		R_GLMModel_Tree_ReEvalSurfaceText(hModel);
	}
}

bool GLMModel_Surface_Off( ModelHandle_t hModel, int iSurfaceIndex )
{
	return GLMModel_Surface_SetStatus( hModel, iSurfaceIndex, SURF_OFF );
}

bool GLMModel_Surface_On( ModelHandle_t hModel, int iSurfaceIndex )
{
	return GLMModel_Surface_SetStatus( hModel, iSurfaceIndex, SURF_ON );
}

bool GLMModel_Surface_NoDescendants(ModelHandle_t hModel, int iSurfaceIndex )
{
	return GLMModel_Surface_SetStatus( hModel, iSurfaceIndex, SURF_NO_DESCENDANTS );
}

mdxaBone_t *GLMModel_GetBasePoseMatrix(ModelHandle_t hModel, int iBoneIndex)
{
	mdxmHeader_t	*pMDXMHeader	= (mdxmHeader_t	*) RE_GetModelData(hModel);
	mdxaHeader_t	*pMDXAHeader	= (mdxaHeader_t	*) RE_GetModelData(pMDXMHeader->animIndex);

	assert( iBoneIndex < pMDXAHeader->numBones);
	
	mdxaSkelOffsets_t *pSkelOffsets = (mdxaSkelOffsets_t *) ((byte *)pMDXAHeader + sizeof(*pMDXAHeader));
	mdxaSkel_t *pSkelEntry = (mdxaSkel_t *) ((byte *) pSkelOffsets + pSkelOffsets->offsets[ iBoneIndex ] );

	return &pSkelEntry->BasePoseMat;
}



// This code was put in for Keith to auto-measure models, it works fine, but I've REM'd it for now since it
//	doesn't really serve any useful purpose in ModView other than to look interesting...   :-)
//

void ClearBounds (vec3_t mins, vec3_t maxs)
{
	mins[0] = mins[1] = mins[2] = 99999;
	maxs[0] = maxs[1] = maxs[2] = -99999;
}


void AddPointToBounds (const vec3_t v, vec3_t mins, vec3_t maxs)
{
	int		i;
	vec_t	val;

	for (i=0 ; i<3 ; i++)
	{
		val = v[i];
		if (val < mins[i])
			mins[i] = val;
		if (val > maxs[i])
			maxs[i] = val;
	}
}

#include "MatComp.h"

extern void Multiply_4x4Matrix(mdxBone4_t *out, mdxBone4_t *in2, mdxBone4_t *in);
extern void From3x4(mdxaBone_t *mat, mdxBone4_t *out);
extern void To3x4(mdxBone4_t *in, mdxaBone_t *mat);

extern void UnCompressBone(float mat[3][4], int iBoneIndex, const mdxaHeader_t *pMDXAHeader, int iFrame);

// transform each individual bone's information - making sure to use any override information provided, both for angles and for animations, as
// well as multiplying each bone's matrix by it's parents matrix 
static void CUTDOWN_G2_TransformBone ( int newFrame, int parent, int child, mdxaHeader_t *header, mdxaBone_t *bonePtr)
{
	mdxaBone_t		tbone[3];
	mdxaSkel_t		*skel;
	mdxaSkelOffsets_t *offsets;
	int				i;//, boneListIndex;
	mdxBone4_t		outMatrix, inMatrix, in2Matrix;
	int				angleOverride = 0;

	// decide where the transformed bone is going

  	//
  	// lerp this bone - use the temp space on the ref entity to put the bone transforms into
  	//
  	if (child)
  	{
//		MC_UnCompress(tbone[2].matrix,pCompBonePool[aframe->boneIndexes[child]].Comp);
		UnCompressBone(tbone[2].matrix, child, header, newFrame);		
  	}
  	else
  	{
//		MC_UnCompress(bonePtr[child].matrix,pCompBonePool[aframe->boneIndexes[child]].Comp);
		UnCompressBone(bonePtr[child].matrix, child, header, newFrame);		
  	}

	// now transform the matrix by it's parent, asumming we have a parent, and we aren't overriding the angles absolutely
	if (child)
	{
		// convert from 3x4 matrix to a 4x4 matrix
		From3x4(&bonePtr[parent], &inMatrix);
		From3x4(&tbone[2], &in2Matrix);

		// do the multiplication
		Multiply_4x4Matrix(&outMatrix, &in2Matrix, &inMatrix);

		// convert result back into a 3x4 matrix for use later
		To3x4(&outMatrix, &bonePtr[child]);
	}


	// figure out where the bone hirearchy info is
	offsets = (mdxaSkelOffsets_t *)((byte *)header + sizeof(mdxaHeader_t));
	skel = (mdxaSkel_t *)((byte *)header + sizeof(mdxaHeader_t) + offsets->offsets[child]);

	// now work out what children we have to call this recursively for
	for (i=0; i< skel->numChildren; i++)
	{
		CUTDOWN_G2_TransformBone( newFrame, child, skel->children[i], header, bonePtr );
	}
}


// start the recursive hierarchical bone transform and lerp process for this model...
//
static void CUTDOWN_G2_TransformGhoulBones( int iFrame, mdxaHeader_t *header, mdxaBone_t *bonePtr)
{
	CUTDOWN_G2_TransformBone( iFrame, 0, 0, header, bonePtr);
}

// horrible hacky mess
static void CUTDOWN_G2_RecurseSurfaces( ModelHandle_t hModel, ModelContainer_t *pContainer, int iSurface, mdxmLODSurfOffset_t *pLODSurfOffset, mdxaBone_t *bonePtr, vec3_t &v3Mins, vec3_t &v3Maxs, mdxmHeader_t *pMDXMHeader)
{
	mdxmHierarchyOffsets_t	*surfIndexes = (mdxmHierarchyOffsets_t *)((byte *)pMDXMHeader + sizeof(mdxmHeader_t));
	mdxmSurfHierarchy_t		*surfInfo = (mdxmSurfHierarchy_t *)((byte *)surfIndexes + surfIndexes->offsets[iSurface]);

	mdxmSurface_t *pSurface = (mdxmSurface_t *) ((byte *) pLODSurfOffset +  pLODSurfOffset->offsets[iSurface]);
	int *piBoneRefTable = (int*)((byte*)pSurface + pSurface->ofsBoneReferences);

	//////////////////////////////
	//
	// remove if this code is cut/paste outside of ModView...
	//
	if (AppVars.iSurfaceNumToHighlight == iSurface ||
		trap_G2_IsSurfaceOff (hModel, pContainer->slist, GLMModel_GetSurfaceName( hModel, iSurface)) == SURF_ON)
	//
	//////////////////////////////
	{
		if (AppVars.bShowTagSurfaces || !GLMModel_SurfaceIsTag(hModel, iSurface))
		{
			// whip through and actually transform each vertex
			//		
			mdxmVertex_t *v = (mdxmVertex_t *) ((byte *)pSurface + pSurface->ofsVerts);
			mdxmVertexTexCoord_t *pTexCoords = (mdxmVertexTexCoord_t *) &v[pSurface->numVerts];
			for ( int j=0; j<pSurface->numVerts; j++ )
			{
				vec3_t			tempVert;
//				mdxmWeight_t	*w;

				VectorClear( tempVert );
//				w = v->weights;		

				const int iNumWeights = G2_GetVertWeights( v );
			
				float fTotalWeight = 0.0f;
				for ( int k=0 ; k<iNumWeights ; k++ ) 
				{
					int		iBoneIndex	= G2_GetVertBoneIndex( v, k );
					float	fBoneWeight	= G2_GetVertBoneWeight( v, k, fTotalWeight, iNumWeights );

					mdxaBone_t *bone = &bonePtr[piBoneRefTable[iBoneIndex]];

					tempVert[0] += fBoneWeight * ( DotProduct( bone->matrix[0], v->vertCoords ) + bone->matrix[0][3] );
					tempVert[1] += fBoneWeight * ( DotProduct( bone->matrix[1], v->vertCoords ) + bone->matrix[1][3] );
					tempVert[2] += fBoneWeight * ( DotProduct( bone->matrix[2], v->vertCoords ) + bone->matrix[2][3] );
				}

				AddPointToBounds(tempVert, v3Mins, v3Maxs);

//					if (v3Mins[1] < -33)
//					{
//						int z=1;
//					}

				v++;// = (mdxmVertex_t *)&v->weights[/*v->numWeights*/pSurface->maxVertBoneWeights];
			}
		}
	}

	// recurse this surface's children...
	//
	for (int iChildSurface = 0; iChildSurface < surfInfo->numChildren; iChildSurface++)
	{
		CUTDOWN_G2_RecurseSurfaces( hModel, pContainer, surfInfo->childIndexes[iChildSurface], pLODSurfOffset, bonePtr, v3Mins, v3Maxs, pMDXMHeader);
	}
}


bool GLMModel_GetBounds(ModelHandle_t hModel, int iLODNumber, int iFrameNumber, vec3_t &v3Mins, vec3_t &v3Maxs)
{	
	mdxmHeader_t	*pMDXMHeader	= (mdxmHeader_t	*) RE_GetModelData(hModel);
	mdxaHeader_t	*pMDXAHeader	= (mdxaHeader_t	*) RE_GetModelData(pMDXMHeader->animIndex);

	ClearBounds (v3Mins, v3Maxs);

	if (iLODNumber >= pMDXMHeader->numLODs)
		return false;	// can't reference it if we don't have a LOD of this level of course

	if (iFrameNumber >= pMDXAHeader->numFrames)
		return false;

	mdxmLOD_t *pLOD = (mdxmLOD_t *)((byte *)pMDXMHeader + pMDXMHeader->ofsLODs);
	for (int iLOD = 0; iLOD < iLODNumber && iLOD < pMDXMHeader->numLODs-1; iLOD++)
	{
		pLOD = (mdxmLOD_t *)((byte *)pLOD + pLOD->ofsEnd);
	}	
	mdxmLODSurfOffset_t *pLODSurfOffset = (mdxmLODSurfOffset_t *) &pLOD[1];


	mdxaBone_t *bonePtr = (mdxaBone_t *) malloc( sizeof(mdxaBone_t) * pMDXAHeader->numBones);
	if (!bonePtr)	
		return false;

	//////////////////////////////
	//
	// remove if this code is cut/paste outside of ModView...
	//
	ModelContainer_t *pContainer = ModelContainer_FindFromModelHandle(hModel);	
	if (!pContainer)
		return false;
	//
	//////////////////////////////


	CUTDOWN_G2_TransformGhoulBones( iFrameNumber, pMDXAHeader, bonePtr);

	int iStartSurface = pContainer->iSurfaceNum_RootOverride;
	if (iStartSurface == -1)
	{
		iStartSurface = 0;
	}

	CUTDOWN_G2_RecurseSurfaces( hModel, pContainer, iStartSurface, pLODSurfOffset, bonePtr, v3Mins, v3Maxs, pMDXMHeader);

	free(bonePtr);
	return true;
}

// work out an edge-or-not bool for every vert in the specified LOD of the model...
//
// return value is the corrected (if over the model limit) LOD level
//
int GLMModel_EnsureGenerated_VertEdgeInfo(ModelHandle_t hModel, int iLOD, SurfaceEdgeInfoPerLOD_t &SurfaceEdgeInfoPerLOD)
{
	mdxmHeader_t *pMDXMHeader = (mdxmHeader_t *) RE_GetModelData(hModel);
	if (iLOD >= pMDXMHeader->numLODs)
		iLOD  = pMDXMHeader->numLODs-1;	// correct it before looking up...

	SurfaceEdgeInfoPerLOD_t::iterator itSurfaceEdgeInfoPerLOD = SurfaceEdgeInfoPerLOD.find(iLOD);
	if (itSurfaceEdgeInfoPerLOD == SurfaceEdgeInfoPerLOD.end())
	{
		// info not present for this LOD, so generate it...
		//
		// First, get various model pointers ready...
		//		
		mdxmHierarchyOffsets_t	*pHierarchyOffsets	= (mdxmHierarchyOffsets_t *) ((byte *) pMDXMHeader + sizeof(*pMDXMHeader));
	//	mdxmSurfHierarchy_t		*pSurfHierarchy		= (mdxmSurfHierarchy_t *) ((byte *) pHierarchyOffsets + pHierarchyOffsets->offsets[iSurfaceNumber]);

		mdxmLOD_t *pLOD = (mdxmLOD_t *)((byte *)pMDXMHeader + pMDXMHeader->ofsLODs);
		for (int _iLOD = 0; _iLOD < iLOD && _iLOD < pMDXMHeader->numLODs-1; _iLOD++)
		{
			pLOD = (mdxmLOD_t *)((byte *)pLOD + pLOD->ofsEnd);
		}	

		mdxmLODSurfOffset_t *pLODSurfOffset = (mdxmLODSurfOffset_t *) &pLOD[1];

		// now go through the model and flag each vert as edge or not...
		//		
		for (int iSurface = 0; iSurface<pMDXMHeader->numSurfaces; iSurface++)
		{
			if (iSurface == 59)
			{
				int z=1;
			}
			mdxmSurface_t	*pSurface	= (mdxmSurface_t  *) ((byte *) pLODSurfOffset +  pLODSurfOffset->offsets[iSurface]);
			mdxmTriangle_t	*pTriangles	= (mdxmTriangle_t *) ((byte *) pSurface + pSurface->ofsTriangles);

			VertIsEdge_t	vVertIsEdge;
							vVertIsEdge.resize(pSurface->numVerts);

			// To work out if a vert is an edge or not:
			//
			// for every vert, find the triangles using it, and their verts,
			//	then for every one of those other verts if they're used by two triangles
			//	then the original vert is an internal (non-edge) one...
			//
			mdxmVertex_t *pThisVert = (mdxmVertex_t *) ((byte *)pSurface + pSurface->ofsVerts);
			for (int iThisVert = 0; iThisVert<pSurface->numVerts; iThisVert++)
			{
				// build up tris-using-this-vert list...
				//
				vector <mdxmTriangle_t *> vTrisUsing;
				set	   <int>			  stOtherVertsUsed;

				for (int iTrisUsing=0; iTrisUsing<pSurface->numTriangles; iTrisUsing++)
				{
					mdxmTriangle_t *pTriUsing = &pTriangles[iTrisUsing];

					if (pTriUsing->indexes[0] == iThisVert || 
						pTriUsing->indexes[1] == iThisVert || 
						pTriUsing->indexes[2] == iThisVert
						)
					{
						// store this tri...
						//
						vTrisUsing.push_back( pTriUsing );
						//
						// store it's other verts...
						//
						stOtherVertsUsed.insert(stOtherVertsUsed.end(), pTriUsing->indexes[0]);
						stOtherVertsUsed.insert(stOtherVertsUsed.end(), pTriUsing->indexes[1]);
						stOtherVertsUsed.insert(stOtherVertsUsed.end(), pTriUsing->indexes[2]);
					}
				}

				// (algorithm next:) 
				//
				// now scan through every vert in the list (except the original) and see if they're used by
				//	two triangles only...
				//
				bool bThisVertIsInternal = true;
				for (set <int>::iterator it = stOtherVertsUsed.begin(); bThisVertIsInternal && it != stOtherVertsUsed.end(); ++it)
				{
					int iScanVert = (*it);

					if (iScanVert != iThisVert)
					{
						int iTrisUsingThisScanVert = 0;
						for (int iScanTri = 0; iScanTri < vTrisUsing.size(); iScanTri++)
						{
							mdxmTriangle_t *pScanTri = vTrisUsing[iScanTri];

							if (pScanTri->indexes[0] == iScanVert || 
								pScanTri->indexes[1] == iScanVert || 
								pScanTri->indexes[2] == iScanVert
								)
							{
								iTrisUsingThisScanVert++;
							}
						}
						if (iTrisUsingThisScanVert != 2)
						{
							bThisVertIsInternal = false;
						}
					}
				}

				// finally, record the result...
				//				
				vVertIsEdge[ iThisVert ] = !bThisVertIsInternal;

				// next vert...
				//
				pThisVert++;// = (mdxmVertex_t *)&pThisVert->weights[/*pThisVert->numWeights*/pSurface->maxVertBoneWeights];
			}

			// record this set of surface vert-edge bools...
			//
			SurfaceEdgeVertBools_t &SurfaceEdgeVertBools = SurfaceEdgeInfoPerLOD[iLOD];
									SurfaceEdgeVertBools[ iSurface ] = vVertIsEdge;
		}
	}

	return iLOD;
}

//////////////// eof //////////////



