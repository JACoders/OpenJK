// Filename:-	R_GLM.cpp
//
// contains a whole wodge of code pasted from our other codebases in order to quickly get this format up and running...
//
#include "stdafx.h"
#include "includes.h"
#include "R_Common.h"
#include "R_Model.h"
#include "R_Image.h"
#include "matcomp.h"
#include "textures.h"
#include "skins.h"
#include "oldskins.h"
#include "modviewtreeview.h"	// for GetString()
#include "mc_compress2.h"
#include "special_defines.h"
//
#include "r_glm.h"


/*
==============
RB_SurfaceAnim
==============
*/
void RB_SurfaceGhoul( surfaceInfo_t *surf ) {
	int				 j, k;
	int				baseIndex, baseVertex;
	int				numVerts;
	mdxmVertex_t 	*v;
	mdxaBone_t		*bonePtr, *bone;
	int				*triangles;
	int				indexes;

	// grab the pointer to the surface info within the loaded mesh file
	mdxmSurface_t	*surface = (mdxmSurface_t *)surf->surfaceData;

	int *piBoneRefTable = (int*)((byte*)surface + surface->ofsBoneReferences);

	// stats...
	//
	giSurfaceVertsDrawn = surface->numVerts;
	giSurfaceTrisDrawn  = surface->numTriangles;
	giRenderedBoneWeights = 0;
	giOmittedBoneWeights = 0;
  
	//
	// deform the vertexes by the lerped bones
	//
   
	// first up, sanity check our numbers
	RB_CheckOverflow( surface->numVerts, surface->numTriangles );

	// now copy the right number of verts to the temporary area for verts for this shader
	triangles = (int *) ((byte *)surface + surface->ofsTriangles);
	indexes = surface->numTriangles * 3;
	baseIndex = tess.numIndexes;
	baseVertex = tess.numVertexes;
	for (j = 0 ; j < indexes ; j++) {
		tess.indexes[baseIndex + j] = baseVertex + triangles[j];
	}
	tess.numIndexes += indexes;

	// point us at the bone structure that should have been pre-computed
	bonePtr = (mdxaBone_t *)surf->boneList;

	// whip through and actually transform each vertex
	numVerts = surface->numVerts;
	
	v = (mdxmVertex_t *) ((byte *)surface + surface->ofsVerts);
	mdxmVertexTexCoord_t *pTexCoords = (mdxmVertexTexCoord_t *) &v[numVerts];
	for ( j = 0; j < numVerts; j++ ) 
	{		
		vec3_t			tempVert, tempNormal;

		VectorClear( tempVert );
		VectorClear( tempNormal );
//		w = v->weights;		

		const int iNumWeights = G2_GetVertWeights( v );
		
		int iWeightsUsed	= 0;
		int iWeightsOmitted	= 0;

		float fTotalWeight = 0.0f;
		for ( k = 0 ; k < iNumWeights ; k++ ) 
		{
			int		iBoneIndex	= G2_GetVertBoneIndex( v, k );
			float	fBoneWeight	= G2_GetVertBoneWeight( v, k, fTotalWeight, iNumWeights );

			if (!AppVars.bBoneWeightThreshholdingActive || 
				 fBoneWeight * 100 > AppVars.fBoneWeightThreshholdPercent
				 )
			{
				bone = &bonePtr[piBoneRefTable[iBoneIndex]];

				tempVert[0] += fBoneWeight * ( DotProduct( bone->matrix[0], v->vertCoords ) + bone->matrix[0][3] );
				tempVert[1] += fBoneWeight * ( DotProduct( bone->matrix[1], v->vertCoords ) + bone->matrix[1][3] );
				tempVert[2] += fBoneWeight * ( DotProduct( bone->matrix[2], v->vertCoords ) + bone->matrix[2][3] );

				tempNormal[0] += fBoneWeight * DotProduct( bone->matrix[0], v->normal );
				tempNormal[1] += fBoneWeight * DotProduct( bone->matrix[1], v->normal );
				tempNormal[2] += fBoneWeight * DotProduct( bone->matrix[2], v->normal );

				iWeightsUsed++;
			}
			else
			{
				iWeightsOmitted++;
			}
		}
				
		giRenderedBoneWeights				+= iWeightsUsed;
		tess.WeightsUsed	[baseVertex + j] = iWeightsUsed;

		giOmittedBoneWeights				+= iWeightsOmitted;
		tess.WeightsOmitted	[baseVertex + j] = iWeightsOmitted;

		
//		assert(fabs(tempVert[0]) < 1000.0f);
//		assert(fabs(tempVert[1]) < 1000.0f);
//		assert(fabs(tempVert[2]) < 1000.0f);

		tess.xyz[baseVertex + j][0] = tempVert[0];
		tess.xyz[baseVertex + j][1] = tempVert[1];
		tess.xyz[baseVertex + j][2] = tempVert[2];

		tess.normal[baseVertex + j][0] = tempNormal[0];
		tess.normal[baseVertex + j][1] = tempNormal[1];
		tess.normal[baseVertex + j][2] = tempNormal[2];

		tess.texCoords[baseVertex + j][0][0] = pTexCoords[j].texCoords[0];
		tess.texCoords[baseVertex + j][0][1] = pTexCoords[j].texCoords[1];
		
//		OutputDebugString(va("tex: %1.4f, %1.4f\n",v->texCoords[0],v->texCoords[1]));

//		v = (mdxmVertex_t *)&v->weights[/*v->numWeights*/surface->maxVertBoneWeights];
		v++;
	}

	tess.numVertexes += surface->numVerts;


	// modview stuff for surface highlighting....
	//
	{
		tess.iSurfaceNum	= surface->thisSurfaceIndex;
		//
		// sigh, need to work out if this surface is a tag or not...
		//
		mdxmHeader_t			*pMDXMHeader		= (mdxmHeader_t *) ((byte *)surface + surface->ofsHeader);
		mdxmHierarchyOffsets_t	*pHierarchyOffsets	= (mdxmHierarchyOffsets_t *) ((byte *) pMDXMHeader + sizeof(*pMDXMHeader));
		mdxmSurfHierarchy_t		*pSurfHierarchy		= (mdxmSurfHierarchy_t *) ((byte *) pHierarchyOffsets + pHierarchyOffsets->offsets[surface->thisSurfaceIndex]);

		tess.bSurfaceIsG2Tag = (pSurfHierarchy->flags & G2SURFACEFLAG_ISBOLT);
		
//		if (tess.bSurfaceIsG2Tag)
//		{
//			OutputDebugString(va("Surface %d is a tag\n",surface->thisSurfaceIndex));
//		}

	}
}

 
/*
=================
R_LoadMDXM - load a Ghoul 2 Mesh file
=================
*/
qboolean R_LoadMDXM( model_t *mod, void *buffer, const char *mod_name ) {
	int					i,l, j;
	mdxmHeader_t		*pinmodel, *mdxm;
	mdxmLOD_t			*lod;
	mdxmSurface_t		*surf;
	int					version;
	int					size;
//MODVIEWREM	shader_t			*sh;
	mdxmSurfHierarchy_t	*surfInfo;

#ifndef _M_IX86
	int					k;
	int					frameSize;
	mdxmTag_t			*tag;
	mdxmTriangle_t		*tri;
	mdxmVertex_t		*v;
 	mdxmFrame_t			*cframe;
	int					*boneRef;
#endif
	
//	bool bSkinsExist = Skins_FilesExist(mod_name) || OldSkins_FilesExist(mod_name);
	bool bOldSkinsExist = OldSkins_FilesExist(mod_name);
    
	pinmodel = (mdxmHeader_t *)buffer;

	version = LittleLong (pinmodel->version);
	if (version != MDXM_VERSION) {
		ri.Printf( PRINT_WARNING, "R_LoadMDXM: %s has wrong version (%i should be %i)\n",
				 mod_name, version, MDXM_VERSION);
		return qfalse;
	}

	mod->type = MOD_MDXM;
	size = LittleLong(pinmodel->ofsEnd);
	mod->dataSize += size;
	mdxm = mod->mdxm = (mdxmHeader_t*) ri.Hunk_Alloc( size );

	memcpy( mdxm, buffer, size );
	

    LL(mdxm->ident);
    LL(mdxm->version);
    LL(mdxm->numLODs);
    LL(mdxm->ofsLODs);
	LL(mdxm->numSurfaces);
	if (mdxm->numSurfaces > MAX_G2_SURFACES)
	{
		ri.Printf( PRINT_WARNING, "R_LoadMDXM: numSurfaces == %d, max is %d\n", mdxm->numSurfaces, MAX_G2_SURFACES);
		return qfalse;
	}
	LL(mdxm->ofsSurfHierarchy);
    LL(mdxm->ofsEnd);

	// first up, go load in the animation file we need that has the skeletal animation info for this model
		

#if 1	//	 kludge code for overriding skeletons	
	if (AppVars.bAllowGLAOverrides)
	{
		if (GetYesNo(va("Override anim file:\n\n\"%s\"  ?\n\n( Model: \"%s\" )",mdxm->animName,mdxm->name)))
		{
			LPCSTR psNewAnimName = GetString("Enter new anim name",mdxm->animName);
			if (psNewAnimName)
			{						
				strncpy(mdxm->animName,psNewAnimName,sizeof(mdxm->animName)-1);
				mdxm->animName[sizeof(mdxm->animName)-1]='\0';
			}
		}
	}
#endif

	mdxm->animIndex = RE_RegisterModel( va ("%s.gla",mdxm->animName));
	if (!mdxm->animIndex) 
	{
		ri.Printf( PRINT_WARNING, "R_LoadMDXM: missing animation file %s for mesh %s\n", mdxm->animName, mdxm->name);
		return qfalse;
	}

	// this shouldn't be needed unless someone has failed to recompile something...
	//
	mdxaHeader_t *pMDXAHeader = (mdxaHeader_t *) RE_GetModelData( mdxm->animIndex );
	if (pMDXAHeader->numBones != mdxm->numBones)
	{
		ri.Error (ERR_DROP, "R_LoadMDXM:   # bones mismatch!\n\n\"%s\" has %d bones\n\n\"%s\" has %d\n\nThis model probably needs recompiling",
								mod_name, mdxm->numBones, pMDXAHeader->name, pMDXAHeader->numBones);
	}



	surfInfo = (mdxmSurfHierarchy_t *)( (byte *)mdxm + mdxm->ofsSurfHierarchy);
 	for ( i = 0 ; i < mdxm->numSurfaces ; i++) 
	{
		LL(surfInfo->numChildren);
		LL(surfInfo->parentIndex);

		// do all the children indexs
		for (j=0; j<surfInfo->numChildren; j++)
		{
			LL(surfInfo->childIndexes[j]);
		}

/* MODVIEWREM
		// get the shader name
		sh = R_FindShader( surfInfo->shader, LIGHTMAP_NONE, qtrue );
		// insert it in the surface list
		
		if ( sh->defaultShader ) 
		{
			surfInfo->shaderIndex = 0;
		}
		else
		{
			surfInfo->shaderIndex = sh->index;
		}
*/

		// weird logic here, for speed.  If there's a slash in the name (which therefore precludes it from being a SOF2
		//	material name, and there's no old-type skin file (which precludes it being an overridden CHC surface shader),
		//	then load the file directly as a texture/shader,
		//	else leave it for later skin-binding.
		//
		if (!bOldSkinsExist && (strchr(surfInfo->shader,'/') || strchr(surfInfo->shader,'\\')))
		{
			surfInfo->shaderIndex = Texture_Load(surfInfo->shader);
		}
		else
		{
			surfInfo->shaderIndex = -1;
		}

		// find the next surface
		surfInfo = (mdxmSurfHierarchy_t *)( (byte *)surfInfo + (int)( &((mdxmSurfHierarchy_t *)0)->childIndexes[ surfInfo->numChildren ] ));
  	}


	mod->numLods = mdxm->numLODs -1 ;	//copy this up to the model for ease of use - it will get inced after this.

	// swap all the LOD's	(we need to do the middle part of this even for intel, because of shader reg and err-check)
	lod = (mdxmLOD_t *) ( (byte *)mdxm + mdxm->ofsLODs );
	for ( l = 0 ; l < mdxm->numLODs ; l++)
	{

		LL(lod->ofsEnd);
		// swap all the surfaces

		mdxmLODSurfOffset_t *pLODSurfOffset = (mdxmLODSurfOffset_t*) ( (byte *)lod + sizeof (mdxmLOD_t) );
		
		for ( i = 0 ; i < mdxm->numSurfaces ; i++) 
		{
			LL(pLODSurfOffset->offsets[i]);

			surf = (mdxmSurface_t *) ((byte*)pLODSurfOffset +  pLODSurfOffset->offsets[i]);

			LL(surf->numTriangles);
			LL(surf->ofsTriangles);
			LL(surf->numVerts);
			LL(surf->ofsVerts);
			LL(surf->ofsEnd);
			LL(surf->ofsHeader);
			LL(surf->numBoneReferences);
			LL(surf->ofsBoneReferences);
										
			if ( surf->numVerts > (bQ3RulesApply?SHADER_MAX_VERTEXES:ACTUAL_SHADER_MAX_VERTEXES) ) {
				ri.Error (ERR_DROP, "R_LoadMDXM: %s has more than %i verts on a surface (%i)",
					mod_name, (bQ3RulesApply?SHADER_MAX_VERTEXES:ACTUAL_SHADER_MAX_VERTEXES), surf->numVerts );
			}
			if ( surf->numTriangles*3 > (bQ3RulesApply?SHADER_MAX_INDEXES:ACTUAL_SHADER_MAX_INDEXES) ) {
				ri.Error (ERR_DROP, "R_LoadMDXM: %s has more than %i triangles on a surface (%i)",
					mod_name, (bQ3RulesApply?SHADER_MAX_INDEXES:ACTUAL_SHADER_MAX_INDEXES) / 3, surf->numTriangles );
			}
		
			// change to surface identifier
			surf->ident = SF_MDX;

			// set pointer to surface in the model surface pointer array
			assert(i != MAX_G2_SURFACES);
			mod->mdxmsurf[l][i] = surf;

			// register the shaders
#ifndef _M_IX86
//
// optimisation, we don't bother doing this for standard intel case since our data's already in that format...
//
			// FIXME - is this correct? 
			// do all the bone reference data
			boneRef = (int *) ( (byte *)surf + surf->ofsBoneReferences );
			for ( j = 0 ; j < surf->numBoneReferences ; j++ ) 
			{
					LL(boneRef[j]);
			}

			
			// swap all the triangles
			tri = (mdxmTriangle_t *) ( (byte *)surf + surf->ofsTriangles );
			for ( j = 0 ; j < surf->numTriangles ; j++, tri++ ) 
			{
				LL(tri->indexes[0]);
				LL(tri->indexes[1]);
				LL(tri->indexes[2]);
			}

			// swap all the vertexes
			v = (mdxmVertex_t *) ( (byte *)surf + surf->ofsVerts );
			for ( j = 0 ; j < surf->numVerts ; j++ ) 
			{
				v->normal[0] = LittleFloat( v->normal[0] );
				v->normal[1] = LittleFloat( v->normal[1] );
				v->normal[2] = LittleFloat( v->normal[2] );

				v->texCoords[0] = LittleFloat( v->texCoords[0] );
				v->texCoords[1] = LittleFloat( v->texCoords[1] );

				v->numWeights = LittleLong( v->numWeights );
  			    v->offset[0] = LittleFloat( v->offset[0] );
				v->offset[1] = LittleFloat( v->offset[1] );
				v->offset[2] = LittleFloat( v->offset[2] );

				for ( k = 0 ; k </*v->numWeights*/surf->maxVertBoneWeights ; k++ ) 
				{
					v->weights[k].boneIndex = LittleLong( v->weights[k].boneIndex );
					v->weights[k].boneWeight = LittleFloat( v->weights[k].boneWeight );
				}
				v = (mdxmVertex_t *)&v->weights[/*v->numWeights*/surf->maxVertBoneWeights];
			}
#endif

			// find the next surface
			surf = (mdxmSurface_t *)( (byte *)surf + surf->ofsEnd );
		}

		// find the next LOD
		lod = (mdxmLOD_t *)( (byte *)lod + lod->ofsEnd );
	}

#ifndef _M_IX86
//
// optimisation, we don't bother doing this for standard intel case since our data's already in that format...
//
	tag = (mdxmTag_t *) ( (byte *)mdxm + mdxm->ofsTags );
	for ( i = 0 ; i < md4->numTags ; i++) {
		LL(tag->boneIndex);
		tag++;
	}
#endif

	return qtrue;
}

/*
=================
R_LoadMDXA - load a Ghoul 2 animation file
=================
*/
qboolean R_LoadMDXA( model_t *mod, void *buffer, const char *mod_name ) {

	mdxaHeader_t		*pinmodel, *mdxa;
	int					version;
	int					size;

#ifndef _M_IX86
	int					j, k, i;
	int					frameSize;
	mdxaFrame_t			*cframe;
	mdxaSkel_t			*boneInfo;
#endif

 	pinmodel = (mdxaHeader_t *)buffer;

	version = LittleLong (pinmodel->version);
	if (version != MDXA_VERSION) {
		ri.Printf( PRINT_WARNING, "R_LoadMDXA: %s has wrong version (%i should be %i)\n",
				 mod_name, version, MDXA_VERSION);
		return qfalse;
	}

	mod->type = MOD_MDXA;
	size = LittleLong(pinmodel->ofsEnd);
	mod->dataSize += size;
	mdxa = mod->mdxa = (mdxaHeader_t*) ri.Hunk_Alloc( size );

	memcpy( mdxa, buffer, size );

    LL(mdxa->ident);
    LL(mdxa->version);
    LL(mdxa->numFrames);
    LL(mdxa->numBones);
    LL(mdxa->ofsFrames);
    LL(mdxa->ofsEnd);

 	if ( mdxa->numFrames < 1 ) {
		ri.Printf( PRINT_WARNING, "R_LoadMDXAa: %s has no frames\n", mod_name );
		return qfalse;
	}

	if ( mdxa->numBones > MAX_POSSIBLE_BONES ) {
		ri.Error (ERR_DROP, "R_LoadMDXA: %s has more than %i bones (%i)",
			mod_name, MAX_POSSIBLE_BONES, mdxa->numBones );
	}

	return qtrue;
}


/*
================
R_GetMDXTag for Ghoul II models...
================
*/

// itu?
void R_GetMDXATag( mdxmHeader_t *mod, int framenum, const char *tagName ,md3Tag_t * dest)
{
//	int				i;
//	int				frameSize;
	mdxaHeader_t	*mdxa;
//	mdxaFrame_t		*cframe;
//	mdxmTag_t		*tag;
//	md4Bone_t		tbone;

	mdxa = tr.models[mod->animIndex]->mdxa;

	if ( framenum >= mdxa->numFrames ) 
	{
		// it is possible to have a bad frame while changing models, so don't error
		framenum = mdxa->numFrames - 1;
	}
	AxisClear( dest->axis );
	VectorClear( dest->origin );
}

//=====================================================================================================================
// Surface List handling routines - so entities can determine what surfaces attached to a model are operational or not.

// set a named surface offFlags - if it doesn't find a surface with this name in the list then it will add one.
qboolean G2_SetSurfaceOnOff (qhandle_t model, surfaceInfo_t *slist, const char *surfaceName, const SurfaceOnOff_t offFlags, const int surface)
{
	int i;
	mdxmSurface_t		*surf;	
	// find the model we want
	model_t				*mod = R_GetModelByHandle(model);
	mdxmHierarchyOffsets_t *surfIndexes = (mdxmHierarchyOffsets_t *)((byte *)mod->mdxm + sizeof(mdxmHeader_t));
	mdxmSurfHierarchy_t	*surfInfo;

	// did we find a ghoul 2 model or not?
	if (!mod->mdxm)
	{
		assert(0);
		return qfalse;
	}
   
 	// first find if we already have this surface in the list
	for (i=0; i<MAX_G2_SURFACES; i++)
	{
		// are we at the end of the list?
		if (slist[i].surface == -1)
		{
			break;
		}

		// FIXME - is this the same as just using slist[i].surface instead of the surf->thisSurfaceIndex??
		surf = mod->mdxmsurf[0][slist[i].surface];
		// back track and get the surfinfo struct for this surface
		surfInfo = (mdxmSurfHierarchy_t *)((byte *)surfIndexes + surfIndexes->offsets[surf->thisSurfaceIndex]);

  		// same name as one already in?
		if (!stricmp (surfInfo->name, surfaceName))
		{
			assert(surface == i);
			// set descendants value
			slist[i].offFlags = offFlags;
			return qtrue;
		}
	}

	// run out of space?
	if (i == MAX_G2_SURFACES)
	{
		assert(0);
		return qfalse;
	}

//	OutputDebugString(va("Storing surface # %d  in slot %d ('%s')\n",surface,i,surfaceName));

	assert(surface == i);

	if (surface != i)
	{
		// fill this in later 
		/////ri.Error (ERR_DROP, "R_LoadMD3: %s has more than %i verts on a surface (%i)",
	}
		

	// insert here then
	slist[i].offFlags = offFlags;
	slist[i].surface = surface;
 	slist[i].ident = SF_MDX;
	slist[i].surfaceData = (void *)mod->mdxmsurf[0][surface];
 
	// if we can, set the next surface pointer to a -1 to make the walking of the lists faster
	if (i+1 < MAX_G2_SURFACES)
	{
		slist[i+1].surface = -1;
	}
	return qtrue;
}

// search through all the surfaces in the model looking for those with '_off_' in the name. This indicates this surface is due to be off to begin with
void G2_GetSurfaceList (qhandle_t model, surfaceInfo_t *slist)
{
	model_t				*mod;
	mdxmSurfHierarchy_t	*surf;
	int					i;

	// init the surface list
	memset(slist, 0, sizeof(slist) * MAX_G2_SURFACES);
	slist[0].surface = -1;
   
	// find the model we want
	mod = R_GetModelByHandle(model);
 
	// did we find a ghoul 2 model or not?
	if (!mod->mdxm)
	{
		return;
	}

	// set up pointers to surface info
	surf = (mdxmSurfHierarchy_t *) ( (byte *)mod->mdxm + mod->mdxm->ofsSurfHierarchy );
	mdxmLODSurfOffset_t * pLODSurfOffset = (mdxmLODSurfOffset_t *)((byte *)mod->mdxm + mod->mdxm->ofsLODs + sizeof(mdxmLOD_t));	

	for ( i = 0 ; i < mod->mdxm->numSurfaces ; i++) 
	{
		mdxmSurface_t *surface = (mdxmSurface_t *) ((byte*)pLODSurfOffset +  pLODSurfOffset->offsets[i]);
//		OutputDebugString(va("Master surface list %d/%d: '%s'\n",i,mod->mdxm->numSurfaces,surf->name));
		// if we have the word "_off_" in the name, then we want it off to begin with
	 	if (!stricmp("_off", &surf->name[strlen(surf->name)-4]))
	 	{
			G2_SetSurfaceOnOff(model, slist, surf->name, SURF_OFF, i);
		}
		else
		{
			G2_SetSurfaceOnOff(model, slist, surf->name, SURF_ON, i);
		}
		// find the next surface
  		surf = (mdxmSurfHierarchy_t *)( (byte *)surf + (int)( &((mdxmSurfHierarchy_t *)0)->childIndexes[ surf->numChildren ] ));
  		surface =(mdxmSurface_t *)( (byte *)surface + surface->ofsEnd );
	}
}

// return a named surfaces off flags - should tell you if this surface is on or off.
SurfaceOnOff_t MyFlags;	// globalised for one specific query, only valid if function itself returns SURF_INHERENTLYOFF
SurfaceOnOff_t G2_IsSurfaceOff (qhandle_t model, surfaceInfo_t *slist, const char *surfaceName)
{
	model_t				*mod = R_GetModelByHandle(model);
	int i;
	mdxmSurface_t	*surf;
	mdxmHierarchyOffsets_t *surfIndexes = (mdxmHierarchyOffsets_t *)((byte *)mod->mdxm + sizeof(mdxmHeader_t));
	mdxmSurfHierarchy_t	*surfInfo;

	// did we find a ghoul 2 model or not?
	if (!mod->mdxm)
	{
		return SURF_ERROR;
	}
   
	// first find if we already have this surface in the list
	for (i=0; i<MAX_G2_SURFACES; i++)
	{
				// are we at the end of the list?
		if (slist[i].surface == -1)
		{
			break;
		}

		// FIXME - is this the same as just using slist[i].surface instead of the surf->thisSurfaceIndex??
		surf = mod->mdxmsurf[0][slist[i].surface];
   		// back track and get the surfinfo struct for this surface
		surfInfo = (mdxmSurfHierarchy_t *)((byte *)surfIndexes + surfIndexes->offsets[surf->thisSurfaceIndex]);

		// same name as one already in?
		if (!stricmp (surfInfo->name, surfaceName))
		{
			// if this surface is root or OFF+NO DESCENDANTS, then just return it, else if it's OFF or ON, then for
			//	100% accuracy we should really check the ancestors to see if we're inherently off because of 
			//	a higher surface having no descendants
			//
			if (slist[i].offFlags == SURF_NO_DESCENDANTS || surfInfo->parentIndex == -1)	// ... or root surface
				return slist[i].offFlags;

			/*SurfaceOnOff_t */MyFlags = slist[i].offFlags;	// now made global

			// check the surfaces above this one then...
			//
			while (1)
			{
				if (slist[i].offFlags == SURF_NO_DESCENDANTS)
					return SURF_INHERENTLYOFF;	// someone above us has no descendants

				if (surfInfo->parentIndex == -1)
					return MyFlags;				// reached the root, so I'm ok

				// get parent surface...
				//				
				surfInfo = (mdxmSurfHierarchy_t *)((byte *)surfIndexes + surfIndexes->offsets[surfInfo->parentIndex]);
				surfaceName = surfInfo->name;

				for (i=0; i<MAX_G2_SURFACES; i++)
				{
					if (slist[2].surface == -1)	// EOL?
						return MyFlags;			// fuck it, couldn't find a parent, just return my flags

					surf = mod->mdxmsurf[0][slist[i].surface];
					surfInfo = (mdxmSurfHierarchy_t *)((byte *)surfIndexes + surfIndexes->offsets[surf->thisSurfaceIndex]);

					if (!stricmp (surfInfo->name, surfaceName))
						break;
				}
			}
			return MyFlags;	// will never reach here now
		}
	}
	assert(0);
	return SURF_ERROR;
}


//=====================================================================================================================
// Bone List handling routines - so entities can override bone info on a bone by bone level, and also interrogate this info

// Given a bone name, see if that bone is already in our bone list - note the model_t pointer that gets passed in here MUST point at the 
// gla file, not the glm file type.
int G2_Find_Bone(const model_t *mod, boneInfo_t *blist, const char *boneName)
{
	int					i;
	mdxaSkel_t			*skel;
	mdxaSkelOffsets_t	*offsets;
   	offsets = (mdxaSkelOffsets_t *)((byte *)mod->mdxa + sizeof(mdxaHeader_t));
	skel = (mdxaSkel_t *)((byte *)mod->mdxa + sizeof(mdxaHeader_t) + offsets->offsets[0]);

	// look through entire list
	for(i=0; i<MAX_BONE_OVERRIDES; i++)
	{
		// if this bone entry has no info in it, bounce over it
		if (blist->boneNumber == -1)
		{
			continue;
		}

		// figure out what skeletal info structure this bone entry is looking at
		skel = (mdxaSkel_t *)((byte *)mod->mdxa + sizeof(mdxaHeader_t) + offsets->offsets[blist[i].boneNumber]);

		// if name is the same, we found it
		if (!stricmp(skel->name, boneName))
		{
			return i;
		}
	}

	// didn't find it
	return -1;
}

// we need to add a bone to the list - find a free one and see if we can find a corresponding bone in the gla file
int G2_Add_Bone (const model_t *mod, boneInfo_t *blist, const char *boneName)
{
	int i, x;
	mdxaSkel_t			*skel;
	mdxaSkelOffsets_t	*offsets;
	 
   	offsets = (mdxaSkelOffsets_t *)((byte *)mod->mdxa + sizeof(mdxaHeader_t));
		 
	// look through entire list
	for(i=0; i<MAX_BONE_OVERRIDES; i++)
	{
		// if this bone entry has info in it, bounce over it
		if (blist[i].boneNumber != -1)
		{
			skel = (mdxaSkel_t *)((byte *)mod->mdxa + sizeof(mdxaHeader_t) + offsets->offsets[blist[i].boneNumber]);
			// if name is the same, we found it
			if (!stricmp(skel->name, boneName))
			{
				return i;
			}

			continue;
		}
	 
		// walk the entire list of bones in the gla file for this model and see if any match the name of the bone we want to find
		for (x=0; x< mod->mdxa->numBones; x++)
		{

			skel = (mdxaSkel_t *)((byte *)mod->mdxa + sizeof(mdxaHeader_t) + offsets->offsets[x]);

			// if name is the same, we found it
			if (!stricmp(skel->name, boneName))
			{
				blist[i].flags = 0;
				blist[i].boneNumber = x;
				return i;
			}
		}
		// we found a free one, but no bone to correspond with the bone name
		assert(0);
		return -1;
	}
	assert(0);
	return -1;
}


// Given a model handle, and a bone name, we want to remove this bone from the bone override list
qboolean G2_Remove_Bone_Index ( boneInfo_t *blist, int index)
{
	// did we find it?
	if (index != -1)
	{
		// check the flags first - if it's still being used Do NOT remove it
		if (!blist[index].flags)
		{
			// set this bone to not used
			blist[index].boneNumber = -1;
			return qtrue;
		}
	}

	assert(0);
	// no
	return qfalse;
}

// given a bone number, see if there is an override bone in the bone list
int	G2_Find_Bone_In_List(boneInfo_t *blist, const int boneNum)
{
	int i; 
	
	// look through entire list
	for(i=0; i<MAX_BONE_OVERRIDES; i++)
	{
		if (blist[i].boneNumber == boneNum)
		{
			return i;
		}
	}
	return -1;
}

// given a model, bonelist and bonename, lets stop an anim if it's playing.
qboolean G2_Stop_Bone_Anim_Index( boneInfo_t *blist, int index)
{
	// did we find it?
	if (index != -1)
	{
		blist[index].flags &= ~(BONE_ANIM_OVERRIDE_LOOP || BONE_ANIM_OVERRIDE);
		// try and remove this bone if we can
		return G2_Remove_Bone_Index(blist, index);
	}

	assert(0);

	return qfalse;
}


//=========================================================================================
//// Public Bone Routines


// Given a model handle, and a bone name, we want to remove this bone from the bone override list
qboolean G2_Remove_Bone (const qhandle_t model, boneInfo_t *blist, const char *boneName)
{
	model_t		*mod_m = R_GetModelByHandle(model); 
	model_t		*mod_a = R_GetModelByHandle(mod_m->mdxm->animIndex); 
	int			index = G2_Find_Bone(mod_a, blist, boneName);
 
	// did we find it?
	if (index != -1)
	{
		// set this bone to not used
		blist[index].boneNumber = -1;
		return qtrue;
	}
	assert(0);

	// no
	return qfalse;
}

// Given a model handle, and a bone name, we want to set angles specifically for overriding
qboolean G2_Set_Bone_Angles(const qhandle_t model, boneInfo_t *blist, const char *boneName, const float *angles, const int flags)
{
	model_t		*mod_m = R_GetModelByHandle(model); 
	model_t		*mod_a = R_GetModelByHandle(mod_m->mdxm->animIndex); 
	int			index = G2_Find_Bone(mod_a, blist, boneName);
 
	// did we find it?
	if (index != -1)
	{
		// yes, so set the angles and flags correctly
		blist[index].flags |= flags;
		VectorCopy(angles,blist[index].angles);
		return qtrue;
	}

	// no - lets try and add this bone in
	index = G2_Add_Bone(mod_a, blist, boneName);

	// did we find a free one?
	if (index != -1)
	{
		// yes, so set the angles and flags correctly
		blist[index].flags |= flags;
		VectorCopy(angles,blist[index].angles);
		return qtrue;
	}
	assert(0);

	// no
	return qfalse;
}

// Given a model handle, and a bone name, we want to get the override angles and return them
qboolean G2_Get_Bone_Angles(const qhandle_t model, boneInfo_t *blist, const char *boneName, float *angles)
{
  	model_t		*mod_m = R_GetModelByHandle(model); 
	model_t		*mod_a = R_GetModelByHandle(mod_m->mdxm->animIndex); 
	int			index = G2_Find_Bone(mod_a, blist, boneName);
 
	// did we find it?
	if (index != -1)
	{
		// yes, so copy the angles into the return set
		VectorCopy(blist[index].angles, angles);
		return qtrue;
	}
	assert(0);

	// no
	return qfalse;

}

// given a model, bone name, a bonelist, a start/end frame number, a anim speed and some anim flags, set up or modify an existing bone entry for a new set of anims
qboolean G2_Set_Bone_Anim(const qhandle_t model, boneInfo_t *blist, const char *boneName, const int startFrame, const int endFrame, const int flags, const float animSpeed)
{
	model_t		*mod_m = R_GetModelByHandle(model); 
	model_t		*mod_a = R_GetModelByHandle(mod_m->mdxm->animIndex); 
	int			index = G2_Find_Bone(mod_a, blist, boneName);
 
	// did we find it?
	if (index != -1)
	{
		// yes, so set the anim data and flags correctly
		blist[index].endFrame = endFrame;
		blist[index].startFrame = startFrame;
		blist[index].animSpeed = animSpeed;
		// start up the animation:)
		blist[index].newFrame = startFrame;
		// if we weren't previously animating, set the current frame to the same as the new frame so interpolation doesn't freak out
		if (!(blist[index].flags && (BONE_ANIM_OVERRIDE | BONE_ANIM_OVERRIDE_LOOP)))
		{
			blist[index].currentFrame = startFrame;
		}
		blist[index].flags |= flags;
		return qtrue;
	}

	// no - lets try and add this bone in
	index = G2_Add_Bone(mod_a, blist, boneName);

	// did we find a free one?
	if (index != -1)
	{
		// yes, so set the anim data and flags correctly
		blist[index].endFrame = endFrame;
		blist[index].startFrame = startFrame;
		blist[index].animSpeed = animSpeed;
		// start up the animation:)
		blist[index].newFrame = startFrame;

		// if we weren't previously animating, set the current frame to the same as the new frame so interpolation doesn't freak out
		if (!(blist[index].flags && (BONE_ANIM_OVERRIDE | BONE_ANIM_OVERRIDE_LOOP)))
		{
			blist[index].currentFrame = startFrame;
		}
		blist[index].flags |= flags;
		return qtrue;
	}

	assert(0);
	// no
	return qfalse;
}

// given a model, bonelist and bonename, return the current frame, startframe and endframe of the current animation
// NOTE if we aren't running an animation, then qfalse is returned
qboolean G2_Get_Bone_Anim(const qhandle_t model, boneInfo_t *blist, const char *boneName, float *currentFrame, int *startFrame, int *endFrame)
{
  	model_t		*mod_m = R_GetModelByHandle(model); 
	model_t		*mod_a = R_GetModelByHandle(mod_m->mdxm->animIndex); 
	int			index = G2_Find_Bone(mod_a, blist, boneName);
 
	// did we find it?
	if (index != -1)
	{ 
		// are we an animating bone?
		if (blist[index].flags && (BONE_ANIM_OVERRIDE_LOOP || BONE_ANIM_OVERRIDE))
		{
			*currentFrame = blist[index].currentFrame;
			*startFrame = blist[index].startFrame;
			*endFrame = blist[index].endFrame;
			return qtrue;
		}
	}
	assert(0);

	return qfalse;
}

// given a model, bonelist and bonename, lets stop an anim if it's playing.
qboolean G2_Stop_Bone_Anim(const qhandle_t model, boneInfo_t *blist, const char *boneName)
{
  	model_t		*mod_m = R_GetModelByHandle(model); 
	model_t		*mod_a = R_GetModelByHandle(mod_m->mdxm->animIndex); 
	int			index = G2_Find_Bone(mod_a, blist, boneName);
 
	// did we find it?
	if (index != -1)
	{
		blist[index].flags &= ~(BONE_ANIM_OVERRIDE_LOOP || BONE_ANIM_OVERRIDE);
		// try and remove this bone if we can
		return G2_Remove_Bone_Index(blist, index);
	}
	assert(0);

	return qfalse;
}

// given a model, bonelist and bonename, lets stop an anim if it's playing.
qboolean G2_Stop_Bone_Angles(const qhandle_t model, boneInfo_t *blist, const char *boneName)
{
  	model_t		*mod_m = R_GetModelByHandle(model); 
	model_t		*mod_a = R_GetModelByHandle(mod_m->mdxm->animIndex); 
	int			index = G2_Find_Bone(mod_a, blist, boneName);
 
	// did we find it?
	if (index != -1)
	{
		blist[index].flags &= ~(BONE_ANGLES_RELATIVE || BONE_ANGLES_ADDITIVE || BONE_ANGLES_ABSOLUTE);
		// try and remove this bone if we can
		return G2_Remove_Bone_Index(blist, index);
	}
	assert(0);

	return qfalse;
}


// actually walk the bone list and animate each and every bone if there is a need for it.
void G2_Animate_Bone_List(boneInfo_t *blist, float timeoffset)
{
	int i; 
	
	// look through entire list
	for(i=0; i<MAX_BONE_OVERRIDES; i++)
	{
		// we we a valid bone override?
		if (blist[i].boneNumber != -1)
		{
			// are we animating?
			if (blist[i].flags && (BONE_ANIM_OVERRIDE_LOOP || BONE_ANIM_OVERRIDE))
			{
				// set up old frame
				blist[i].currentFrame = blist[i].newFrame;

				// yes - add in animation speed to current frame
				blist[i].newFrame += (blist[i].animSpeed * timeoffset);

				// turn off new anim flag
				blist[i].flags &= ~BONE_ANIM_NEW_ANIM;

				// did we run off the end?
				if (((blist[i].animSpeed > 0.0f) && (blist[i].newFrame >= (float)blist[i].endFrame)) || 
					((blist[i].animSpeed < 0.0f) && (blist[i].newFrame <= (float)blist[i].startFrame)))
				{
					// yep - decide what to do
					if (blist[i].flags & BONE_ANIM_OVERRIDE_LOOP)
					{
						// allow us to play the anim backwards
						if ((blist[i].animSpeed) > 0.0f)
						{
							// loop it
							blist[i].newFrame = blist[i].startFrame;
						}
						else
						{
							// loop it
							blist[i].newFrame = blist[i].endFrame;
						}
						// we've changed anims, the bone interpolater needs to know this. Activate the flag
						blist[i].flags |= BONE_ANIM_NEW_ANIM;
					}
					else
					{
						// nope, just stop it. And remove the bone if possible
						G2_Stop_Bone_Anim_Index(blist, i);
					}
				}
			}
		}
	}
}

// set the bone list to all unused so the bone transformation routine ignores it.
void G2_Init_Bone_List(boneInfo_t *blist)
{
	int i; 
	
	// look through entire list
	for(i=0; i<MAX_BONE_OVERRIDES; i++)
	{
		blist[i].boneNumber = -1;
	}
}



/// assorted Ghoul 2 functions.







/*

All bones should be an identity orientation to display the mesh exactly
as it is specified.

For all other frames, the bones represent the transformation from the 
orientation of the bone in the base frame to the orientation in this
frame.

*/


//======================================================================
//
// Bone Manipulation code

// nasty little matrix multiply going on here..
void Multiply_4x4Matrix(mdxBone4_t *out, mdxBone4_t *in2, mdxBone4_t *in) 
{
	// first row of out
	out->m[0][0] = (in2->m[0][0] * in->m[0][0]) + (in2->m[0][1] * in->m[1][0]) + (in2->m[0][2] * in->m[2][0]) + (in2->m[0][3] * in->m[3][0]);
	out->m[0][1] = (in2->m[0][0] * in->m[0][1]) + (in2->m[0][1] * in->m[1][1]) + (in2->m[0][2] * in->m[2][1]) + (in2->m[0][3] * in->m[3][1]);
	out->m[0][2] = (in2->m[0][0] * in->m[0][2]) + (in2->m[0][1] * in->m[1][2]) + (in2->m[0][2] * in->m[2][2]) + (in2->m[0][3] * in->m[3][2]);
	// second row of out
	out->m[1][0] = (in2->m[1][0] * in->m[0][0]) + (in2->m[1][1] * in->m[1][0]) + (in2->m[1][2] * in->m[2][0]) + (in2->m[1][3] * in->m[3][0]);
	out->m[1][1] = (in2->m[1][0] * in->m[0][1]) + (in2->m[1][1] * in->m[1][1]) + (in2->m[1][2] * in->m[2][1]) + (in2->m[1][3] * in->m[3][1]);
	out->m[1][2] = (in2->m[1][0] * in->m[0][2]) + (in2->m[1][1] * in->m[1][2]) + (in2->m[1][2] * in->m[2][2]) + (in2->m[1][3] * in->m[3][2]);
	// third row of out
	out->m[2][0] = (in2->m[2][0] * in->m[0][0]) + (in2->m[2][1] * in->m[1][0]) + (in2->m[2][2] * in->m[2][0]) + (in2->m[2][3] * in->m[3][0]);
	out->m[2][1] = (in2->m[2][0] * in->m[0][1]) + (in2->m[2][1] * in->m[1][1]) + (in2->m[2][2] * in->m[2][1]) + (in2->m[2][3] * in->m[3][1]);
	out->m[2][2] = (in2->m[2][0] * in->m[0][2]) + (in2->m[2][1] * in->m[1][2]) + (in2->m[2][2] * in->m[2][2]) + (in2->m[2][3] * in->m[3][2]);
	// fourth row of out
	out->m[3][0] = (in2->m[3][0] * in->m[0][0]) + (in2->m[3][1] * in->m[1][0]) + (in2->m[3][2] * in->m[2][0]) + (in2->m[3][3] * in->m[3][0]);
	out->m[3][1] = (in2->m[3][0] * in->m[0][1]) + (in2->m[3][1] * in->m[1][1]) + (in2->m[3][2] * in->m[2][1]) + (in2->m[3][3] * in->m[3][1]);
	out->m[3][2] = (in2->m[3][0] * in->m[0][2]) + (in2->m[3][1] * in->m[1][2]) + (in2->m[3][2] * in->m[2][2]) + (in2->m[3][3] * in->m[3][2]);
}

// nasty little matrix multiply going on here..
void Multiply_3x3Matrix(mdxaBone_t *out, mdxaBone_t *in2, mdxaBone_t *in) 
{
	// first row of out                                                                                      
	out->matrix[0][0] = (in2->matrix[0][0] * in->matrix[0][0]) + (in2->matrix[0][1] * in->matrix[1][0]) + (in2->matrix[0][2] * in->matrix[2][0]);
	out->matrix[0][1] = (in2->matrix[0][0] * in->matrix[0][1]) + (in2->matrix[0][1] * in->matrix[1][1]) + (in2->matrix[0][2] * in->matrix[2][1]);
	out->matrix[0][2] = (in2->matrix[0][0] * in->matrix[0][2]) + (in2->matrix[0][1] * in->matrix[1][2]) + (in2->matrix[0][2] * in->matrix[2][2]);
	// second row of out                                                                                     
	out->matrix[1][0] = (in2->matrix[1][0] * in->matrix[0][0]) + (in2->matrix[1][1] * in->matrix[1][0]) + (in2->matrix[1][2] * in->matrix[2][0]);
	out->matrix[1][1] = (in2->matrix[1][0] * in->matrix[0][1]) + (in2->matrix[1][1] * in->matrix[1][1]) + (in2->matrix[1][2] * in->matrix[2][1]);
	out->matrix[1][2] = (in2->matrix[1][0] * in->matrix[0][2]) + (in2->matrix[1][1] * in->matrix[1][2]) + (in2->matrix[1][2] * in->matrix[2][2]);
	// third row of out                                                                                      
	out->matrix[2][0] = (in2->matrix[2][0] * in->matrix[0][0]) + (in2->matrix[2][1] * in->matrix[1][0]) + (in2->matrix[2][2] * in->matrix[2][0]);
	out->matrix[2][1] = (in2->matrix[2][0] * in->matrix[0][1]) + (in2->matrix[2][1] * in->matrix[1][1]) + (in2->matrix[2][2] * in->matrix[2][1]);
	out->matrix[2][2] = (in2->matrix[2][0] * in->matrix[0][2]) + (in2->matrix[2][1] * in->matrix[1][2]) + (in2->matrix[2][2] * in->matrix[2][2]);
}


// convert a 3x4 matrix into a 4x4 ready for multiplication
void From3x4(mdxaBone_t *mat, mdxBone4_t *out)
{
	for (int i=0; i<4; i++)
	{
		for (int j=0; j<3; j++)
		{
			out->m[i][j]=mat->matrix[j][i];
		}
	}

	// set right row to identity
	out->m[0][3] = 0.0f;
	out->m[1][3] = 0.0f;
	out->m[2][3] = 0.0f;
	out->m[3][3] = 1.0f;
}

// convert a 4x4 m back into a 3x4 m
void To3x4(mdxBone4_t *in, mdxaBone_t *mat)
{
	for (int i=0; i<4; i++)
	{
		for (int j=0; j<3; j++)
		{
			mat->matrix[j][i] = in->m[i][j];
		}
	}
}


// create a matrix thats at angle 0,0,0
void Create_Identity_Matrix(mdxaBone_t *matrix)
{
	// make the identity matrix - ie matrix of 0,0,0 degrees
	matrix->matrix[0][1] = matrix->matrix[0][2] = matrix->matrix[1][0] = matrix->matrix[1][2] = matrix->matrix[2][0] = matrix->matrix[2][1] = 0.0f;
	matrix->matrix[0][0] = matrix->matrix[1][1] = matrix->matrix[2][2] = 1.0f;
}

// create a matrix using a set of angles
void Create_Matrix(vec3_t angle, mdxaBone_t *matrix)
{
	mdxaBone_t	matrix_temp;
	float		cos_temp, sin_temp;

	Create_Identity_Matrix(matrix);

	// create matrix_x if we have a YAW angle
	if (angle[0])
	{
		cos_temp = (float)cos(-angle[PITCH]);
		sin_temp = (float)sin(-angle[PITCH]);
		// we can afford to stuff these in directly, since YAW is the first thing hit
		matrix->matrix[1][1] = cos_temp;
		matrix->matrix[1][2] = -sin_temp;
		matrix->matrix[2][1] = sin_temp;
		matrix->matrix[2][2] = cos_temp;
	}

	// create matrix_y if we have a PITCH angle
	if (angle[1])
	{
		Create_Identity_Matrix(&matrix_temp);
		cos_temp = (float)cos(-angle[YAW]);
		sin_temp = (float)sin(-angle[YAW]);
		matrix_temp.matrix[0][0] = cos_temp;
		matrix_temp.matrix[0][2] = sin_temp;
		matrix_temp.matrix[2][0] = -sin_temp;
		matrix_temp.matrix[2][2] = cos_temp;
		Multiply_3x3Matrix(matrix, &matrix_temp, matrix);
	}

	// create matrix_z if we have a ROLL angle
	if (angle[2])
	{
		Create_Identity_Matrix(&matrix_temp);
		cos_temp = (float)cos(angle[ROLL]);
		sin_temp = (float)sin(angle[ROLL]);
		matrix_temp.matrix[0][0] = cos_temp;
		matrix_temp.matrix[0][1] = -sin_temp;
		matrix_temp.matrix[1][0] = sin_temp;
		matrix_temp.matrix[1][1] = cos_temp;
		Multiply_3x3Matrix(matrix, &matrix_temp, matrix);
	}
}

static int G2_GetBonePoolIndex(	const mdxaHeader_t *pMDXAHeader, int iFrame, int iBone)
{
	const int iOffsetToIndex = (iFrame * pMDXAHeader->numBones * 3) + (iBone * 3);

	mdxaIndex_t *pIndex = (mdxaIndex_t *) ((byte*) pMDXAHeader + pMDXAHeader->ofsFrames + iOffsetToIndex);

	return pIndex->iIndex & 0x00FFFFFF;	// this will cause problems for big-endian machines... ;-)
}


/*static inline*/ void UnCompressBone(float mat[3][4], int iBoneIndex, const mdxaHeader_t *pMDXAHeader, int iFrame)
{
	mdxaCompQuatBone_t *pCompBonePool = (mdxaCompQuatBone_t *) ((byte *)pMDXAHeader + pMDXAHeader->ofsCompBonePool);
	int index = G2_GetBonePoolIndex( pMDXAHeader, iFrame, iBoneIndex );
	MC_UnCompressQuat(mat, pCompBonePool[ index ].Comp);
}




// transform each individual bone's information - making sure to use any override information provided, both for angles and for animations, as
// well as multiplying each bone's matrix by it's parents matrix 
int g_iTotalG2BonesXformed;
int iTempLastBoneBeforeSecondary;
int iLastBoneBeforeSecondary;
void G2_TransformBone (mdxaBone_t *pModViewFeedback_BoneList, bool *pModViewFeedback_Validity,	// dirty code: copy this back up to the appropriate container list for bone-pos viewing...
					   int newFrame, int currentFrame, int parent, int child, mdxaHeader_t *header, float framelerp, float backlerp, int *usedBoneList, int lastIndex,
					   bool bPrimary, int iBoneNum_SecondaryStart
					   )
{
	mdxaBone_t		*bonePtr;
	mdxaBone_t		tbone[3];
	mdxaSkel_t		*skel;
	mdxaSkelOffsets_t *offsets;
	boneInfo_t		*boneList;
	int				i, boneListIndex;
	mdxBone4_t		outMatrix, inMatrix, in2Matrix;
/*MODVIEWREM
	float			delta, actualFrame;
	mdxBone4_t		overrideMatrix;
*/
	int				angleOverride = 0;

	// decide here if we should go down this path? - is this bone used? -If not, return from this function. Due the hierarchial nature of the bones
	// any bone below this one in the tree shouldn't be used either.
	if (!usedBoneList[child])
	{
		return;
	}
	
	if (bPrimary && child == iBoneNum_SecondaryStart)
	{
		iLastBoneBeforeSecondary = iTempLastBoneBeforeSecondary;
		return;
	}

	iTempLastBoneBeforeSecondary = child;

	g_iTotalG2BonesXformed++;

	boneList = tr.currentEntity->e.blist;

	// should this bone be overridden by a bone in the bone list?
	boneListIndex = G2_Find_Bone_In_List(boneList, child);
	if (boneListIndex != -1)
	{
		/*MODVIEWREM*/
	}
	   
	// decide where the transformed bone is going
	bonePtr = tr.currentEntity->e.tempBoneList;

  	//
  	// lerp this bone - use the temp space on the ref entity to put the bone transforms into
  	//
//MODVIEWREM  	if (boneList[lastIndex].newFrame == boneList[lastIndex].currentFrame  )
	if (newFrame == currentFrame  )
  	{
  		if (child)
  		{
//			MC_UnCompress(tbone[2].matrix,pCompBonePool[aframe->boneIndexes[child]].Comp);
			UnCompressBone(tbone[2].matrix, child, header, newFrame);
  		}
  		else
  		{
//			MC_UnCompress(bonePtr[child].matrix,pCompBonePool[aframe->boneIndexes[child]].Comp);
			UnCompressBone(bonePtr[child].matrix, child, header, newFrame);
  		}
  	}
  	else
  	{
//  		MC_UnCompress(tbone[0].matrix,pCompBonePool[   aframe->boneIndexes[child]].Comp);
  //		MC_UnCompress(tbone[1].matrix,pCompBonePool[aoldFrame->boneIndexes[child]].Comp);
		UnCompressBone(tbone[0].matrix, child, header, newFrame);
		UnCompressBone(tbone[1].matrix, child, header, currentFrame);

  		int j;
		if (child)
  		{
   			for ( j = 0 ; j < 12 ; j++ ) 
  			{
  				((float *)&tbone[2])[j] = backlerp * ((float *)&tbone[0])[j]
  					+ (1.0 - backlerp) * ((float *)&tbone[1])[j];
  			}
  		}
  		else
  		{
  			for ( j = 0 ; j < 12 ; j++ ) 
  			{
   				((float *)&bonePtr[child])[j] = backlerp * ((float *)&tbone[0])[j]
   					+ (1.0 - backlerp) * ((float *)&tbone[1])[j];
  			}
  		}
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

	{
		// modview hack... :-)
		//
		pModViewFeedback_BoneList[child] = bonePtr[child];
		pModViewFeedback_Validity[child] = true;
	}



	// figure out where the bone hirearchy info is
	offsets = (mdxaSkelOffsets_t *)((byte *)header + sizeof(mdxaHeader_t));
	skel = (mdxaSkel_t *)((byte *)header + sizeof(mdxaHeader_t) + offsets->offsets[child]);

	// now work out what children we have to call this recursively for
	for (i=0; i< skel->numChildren; i++)
	{
		G2_TransformBone(pModViewFeedback_BoneList, pModViewFeedback_Validity,	// modview hack
							newFrame, currentFrame, child, skel->children[i], header, framelerp, backlerp, usedBoneList, lastIndex,
							bPrimary, iBoneNum_SecondaryStart
							);
	}
}

// start the recursive hierarchical bone transform and lerp process for this model
void G2_TransformGhoulBones( mdxaHeader_t *header, int *usedBoneList, refEntity_t e)
{
	// calculate the lerp factor - current client time minus the last time we got a snap shot, divided by 200 which is one 10hz frame
	// this should never get above 1.0f, and if it does, then we should have run the animation function on this models blist
	// to predict the next set of frames.	

	// now recursively call the bone transform routines using the bone hierarchy
	g_iTotalG2BonesXformed = 0;

	//updateme
	// open the file for writing
	//OutputDebugString("=====\n");
	G2_TransformBone(e.pXFormedG2Bones, e.pXFormedG2BonesValid,	// dirty code: copy this back up to the appropriate container list for bone-pos viewing...
						e.iFrame_Primary, e.iOldFrame_Primary, 0, 0, header, 1.0f - e.backlerp, e.backlerp, usedBoneList, 0, 
						true, e.iBoneNum_SecondaryStart);
	// need to transform secondary anim bones?...
	//
	if (e.iBoneNum_SecondaryStart != -1)
	{
		G2_TransformBone(e.pXFormedG2Bones, e.pXFormedG2BonesValid,	// dirty code: copy this back up to the appropriate container list for bone-pos viewing...
							e.iFrame_Secondary, e.iOldFrame_Secondary, iLastBoneBeforeSecondary, e.iBoneNum_SecondaryStart, header, 1.0f - e.backlerp, e.backlerp, usedBoneList, 0,
							false, e.iBoneNum_SecondaryStart);
	}

//	OutputDebugString(va("%d G2 bones xformed\n",g_iTotalG2BonesXformed));
	if ( e.piXformedG2Bones)
	{
		*e.piXformedG2Bones = g_iTotalG2BonesXformed;
	}
/*
	//
	if (  !mdview.interpolate || pModel->iNumFrames == 0 || iNextFrame == pModel->iNumFrames
//		backEnd.currentEntity->e.oldframe == backEnd.currentEntity->e.frame 
		) 
	{
		backlerp	= 0;	// if backlerp is 0, lerping is off and frontlerp is never used
		frontlerp	= 1;
	} 
	else  
	{
		backlerp	= mdview.frameFrac;
		frontlerp	= 1.0f - backlerp;
	}
	header = (md4Header_t *)((byte *)surface + surface->ofsHeader);
*/
}

//======================================================================
//
// Surface Manipulation code				  
void R_RenderSurfaces(surfaceInfo_t *slist, trRefEntity_t *ent, int iLOD)//MODVIEWREM, shader_t *cust_shader, int fogNum, qboolean	personalModel)
{
	int			i;
	shader_t	*shader = 0;
	GLuint gluiTextureBind = 0;
	
	// back track and get the surfinfo struct for this surface
	mdxmSurface_t			*surface = tr.currentModel->mdxmsurf[iLOD][slist->surface];
	mdxmHierarchyOffsets_t	*surfIndexes = (mdxmHierarchyOffsets_t *)((byte *)tr.currentModel->mdxm + sizeof(mdxmHeader_t));
	mdxmSurfHierarchy_t		*surfInfo = (mdxmSurfHierarchy_t *)((byte *)surfIndexes + surfIndexes->offsets[surface->thisSurfaceIndex]);
	
	// if this surface is not off, add it to the shader render list
	//
	// Update, previously I checked to see if we should actually render tag surfaces, but because of the
	//	way I do surface bolt-ons I now always pass them on to the renderer to do the xform calcs, 
	//
	if (AppVars.iSurfaceNumToHighlight == surface->thisSurfaceIndex ||
		slist->offFlags == SURF_ON 
/*
		&&
			(
				// special check for displaying tag surfaces or not...
				//
				AppVars.bShowTagSurfaces						// show tag surfaces as well (in which case anything's fine)
				|| !(surfInfo->flags & G2SURFACEFLAG_ISBOLT)	// ... or this isn't a tag surface so don't worry
				//	or it is a tag surface, but we're highlighting either all tag surfaces, or just this one explicitly
				|| (AppVars.bSurfaceHighlight && (AppVars.iSurfaceNumToHighlight == iITEMHIGHLIGHT_ALL_TAGSURFACES || AppVars.iSurfaceNumToHighlight == surface->thisSurfaceIndex))
				//  or it is a tag surface, but something's bolted to it so we need to process it to fill in matrix info for bolted object
				|| (Model_CountItemsBoltedHere(ent->e.hModel, surface->thisSurfaceIndex, false))	// aaarggh!!! This is horrible!!!
			)
*/
		)
	{
		// set the surface info to point at the where the transformed bone list is going to be for when the surface gets rendered out
		slist->boneList = ent->e.tempBoneList;
		slist->surfaceData = (void *)surface;
/*MODVIEWREM
 		if ( ent->e.customShader ) 
		{
			shader = cust_shader;
		} 
		else if ( ent->e.customSkin > 0 && ent->e.customSkin < tr.numSkins ) 
		{
			skin_t *skin;
			int		j;
			
			skin = R_GetSkinByHandle( ent->e.customSkin );
			
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
*/
			//shader = R_GetShaderByHandle( surfInfo->shaderIndex );

			if (AppVars.bForceWhite)
			{
				gluiTextureBind = 0;
			}
			else
			{
				if (surfInfo->shaderIndex == -1)
				{
					gluiTextureBind = AnySkin_GetGLBind( ent->e.hModel, surfInfo->shader, surfInfo->name );					
				}
				else
				{
					gluiTextureBind = Texture_GetGLBind( surfInfo->shaderIndex );
				}
			}
/*
		}
		// we will add shadows even if the main object isn't visible in the view

		// stencil shadows can't do personal models unless I polyhedron clip
		if ( !personalModel
			&& r_shadows->integer == 2 
			&& fogNum == 0
			&& !(ent->e.renderfx & ( RF_NOSHADOW | RF_DEPTHHACK ) ) 
			&& shader->sort == SS_OPAQUE ) 
		{
			R_AddDrawSurf( (surfaceType_t *)slist, tr.shadowShader, 0, qfalse );
		}

		// projection shadows work fine with personal models
		if ( r_shadows->integer == 3
			&& fogNum == 0
			&& (ent->e.renderfx & RF_SHADOW_PLANE )
			&& shader->sort == SS_OPAQUE ) 
		{
			R_AddDrawSurf( (surfaceType_t *)slist, tr.projectionShadowShader, 0, qfalse );
		}

		// don't add third_person objects if not viewing through a portal
		if ( !personalModel ) 
		{
			R_AddDrawSurf( (surfaceType_t *)slist, shader, fogNum, qfalse );
		}
		*/
		R_AddDrawSurf( (surfaceType_t *)slist, gluiTextureBind);
	}
	else
	// if we are turning off all descendants, then stop this recursion now
	if (slist->offFlags == SURF_NO_DESCENDANTS)
	{
		return;
	}

	// now recursively call for the children
	for (i=0; i< surfInfo->numChildren; i++)
	{
		R_RenderSurfaces(&ent->e.slist[surfInfo->childIndexes[i]], ent, iLOD);//MODVIEWREM, cust_shader, fogNum, personalModel);
	}
}


// build the used bone list so when doing bone transforms we can determine if we need to do it or not
void G2_ConstructUsedBoneList(surfaceInfo_t *slist, int *boneUsedList, trRefEntity_t *ent)
{
	int i,j;

	// back track and get the surfinfo struct for this surface
	mdxmSurface_t			*surface = tr.currentModel->mdxmsurf[0][slist->surface];	// always use LOD 0
	mdxmHierarchyOffsets_t	*surfIndexes = (mdxmHierarchyOffsets_t *)((byte *)tr.currentModel->mdxm + sizeof(mdxmHeader_t));
	mdxmSurfHierarchy_t		*surfInfo = (mdxmSurfHierarchy_t *)((byte *)surfIndexes + surfIndexes->offsets[surface->thisSurfaceIndex]);
	model_t					*mod_a = R_GetModelByHandle(tr.currentModel->mdxm->animIndex);
	mdxaSkelOffsets_t		*offsets = (mdxaSkelOffsets_t *)((byte *)mod_a->mdxa + sizeof(mdxaHeader_t));
	mdxaSkel_t				*skel, *childSkel;
/*
	OutputDebugString(va("G2_ConstructUsedBoneList(): Surface %s\n",surfInfo->name));

	if (stricmp(surfInfo->name,"stupidtriangle_off")==0)
	{
		int z=1;
	}
	if (stricmp(surfInfo->name,"head_side")==0)
	{
		int z=1;
	}
*/
	
	// if this surface is not off, add it to the shader render list
	if (AppVars.iSurfaceNumToHighlight == surface->thisSurfaceIndex ||
		slist->offFlags == SURF_ON || slist->offFlags == SURF_OFF)	// stetemp
	{
		int	*bonesReferenced = (int *)((byte*)surface + surface->ofsBoneReferences);
		// now whip through the bones this surface uses 
		for (i=0; i<surface->numBoneReferences;i++)
		{
			const int iBoneIndex = bonesReferenced[i];
			boneUsedList[iBoneIndex] = 1;
//			OutputDebugString(va("boneUsedList[%d]=1\n",bonesReferenced[i]));

			// now go and check all the descendant bones attached to this bone and see if any have the always flag on them. If so, activate them
 			skel = (mdxaSkel_t *)((byte *)mod_a->mdxa + sizeof(mdxaHeader_t) + offsets->offsets[iBoneIndex]);

			// for every child bone...
			for (j=0; j< skel->numChildren; j++)
			{
				// get the skel data struct for each child bone of the referenced bone
 				childSkel = (mdxaSkel_t *)((byte *)mod_a->mdxa + sizeof(mdxaHeader_t) + offsets->offsets[skel->children[j]]);

				// does it have the always on flag on?
				if (childSkel->flags & G2BONEFLAG_ALWAYSXFORM)
				{
					// yes, make sure it's in the list of bones to be transformed.
					boneUsedList[skel->children[j]] = 1;
//					OutputDebugString(va("boneUsedList[%d]=1  (child)\n",skel->children[j]));
				}
			}

			// now we need to ensure that the parents of this bone are actually active...
			//			
			int iParentBone = skel->parent;
			while (iParentBone != -1)
			{	
				if (boneUsedList[iParentBone])	// no need to go higher
					break;
				boneUsedList[iParentBone] = 1;
				skel = (mdxaSkel_t *)((byte *)mod_a->mdxa + sizeof(mdxaHeader_t) + offsets->offsets[iParentBone]);				
				iParentBone = skel->parent;
			}
		}
	}
 	else
	// if we are turning off all descendants, then stop this recursion now
	if (slist->offFlags == SURF_NO_DESCENDANTS)
	{
		return;
	}

	// now recursively call for the children
//	OutputDebugString(va("Scanning %d children\n",surfInfo->numChildren));
	for (i=0; i< surfInfo->numChildren; i++)
	{
		/*
		if (surfInfo->childIndexes[i] == 0)
		{
			int z=1;
		}*/
		G2_ConstructUsedBoneList(&ent->e.slist[surfInfo->childIndexes[i]], boneUsedList, ent);
	}

}

void R_AddGhoulSurfaces( trRefEntity_t *ent ) {
	mdxaHeader_t	*aHeader;
	mdxmHeader_t	*mHeader;
	mdxmLOD_t		*lod;
	shader_t		*shader = 0;
	shader_t		*cust_shader = 0;
	int				fogNum = 0;
/*MODVIEWREM
	qboolean		personalModel;
	int				cull;
*/
	int				i, whichLod;
	int				boneUsedList[MAX_POSSIBLE_BONES];

   	// don't add third_person objects if not in a portal
//MODVIEWREM	personalModel = (qboolean)((ent->e.renderfx & RF_THIRD_PERSON) && !tr.viewParms.isPortal);

   	aHeader = tr.models[tr.currentModel->mdxm->animIndex]->mdxa;
	mHeader = tr.currentModel->mdxm;

	if ( ent->e.renderfx & RF_CAP_FRAMES) 
	{
		// this stuff is probably not needed now, because we're in ModView instead of Q3, but...
		//
		if (ent->e.iFrame_Primary > aHeader->numFrames-1)
		{
			ent->e.iFrame_Primary = aHeader->numFrames-1;
		}
		if (ent->e.iOldFrame_Primary > aHeader->numFrames-1)
		{
			ent->e.iOldFrame_Primary = aHeader->numFrames-1;
		}
		//
		// and new stuff for compatibility...
		//
		if (ent->e.iFrame_Secondary > aHeader->numFrames-1)
		{
			ent->e.iFrame_Secondary = aHeader->numFrames-1;
		}
		if (ent->e.iOldFrame_Secondary > aHeader->numFrames-1)
		{
			ent->e.iOldFrame_Secondary = aHeader->numFrames-1;
		}
	}
	else if ( ent->e.renderfx & RF_WRAP_FRAMES ) 
	{
		ent->e.iFrame_Primary %= aHeader->numFrames;
		ent->e.iOldFrame_Primary %= aHeader->numFrames;
		//
		ent->e.iFrame_Secondary %= aHeader->numFrames;
		ent->e.iOldFrame_Secondary %= aHeader->numFrames;
	}

	//
	// Validate the frames so there is no chance of a crash.
	// This will write directly into the entity structure, so
	// when the surfaces are rendered, they don't need to be
	// range checked again.
	//
	// ModView: ITU? Oh, well...
	if (   (ent->e.iFrame_Primary >= aHeader->numFrames) 
		|| (ent->e.iFrame_Primary < 0)
		|| (ent->e.iOldFrame_Primary >= aHeader->numFrames)
		|| (ent->e.iOldFrame_Primary < 0) 
		) 
	{
#ifdef _DEBUG
			ri.Printf (PRINT_ALL, "R_AddGhoulSurfaces: no such frame %d to %d for '%s'\n",
#else
			ri.Printf (PRINT_DEVELOPER, "R_AddGhoulSurfaces: no such frame %d to %d for '%s'\n",				
#endif
			ent->e.iOldFrame_Primary, ent->e.iFrame_Primary,
			tr.currentModel->name );
			ent->e.iFrame_Primary = 0;
			ent->e.iOldFrame_Primary = 0;
	}

	// new stuff. still probably not needed, but...
	if (   (ent->e.iFrame_Secondary >= aHeader->numFrames) 
		|| (ent->e.iFrame_Secondary < 0)
		|| (ent->e.iOldFrame_Secondary >= aHeader->numFrames)
		|| (ent->e.iOldFrame_Secondary < 0) 
		) 
	{
#ifdef _DEBUG
			ri.Printf (PRINT_ALL, "R_AddGhoulSurfaces: no such frame %d to %d for '%s'\n",
#else
			ri.Printf (PRINT_DEVELOPER, "R_AddGhoulSurfaces: no such frame %d to %d for '%s'\n",				
#endif
			ent->e.iOldFrame_Secondary, ent->e.iFrame_Secondary,
			tr.currentModel->name );
			ent->e.iFrame_Secondary = 0;
			ent->e.iOldFrame_Secondary = 0;
	}


/*MODVIEWREM
	//
	// cull the entire model if merged bounding box of both frames
	// is outside the view frustum.
	//
	cull = R_GCullModel ( aHeader, ent );
	if ( cull == CULL_OUT ) 
	{
		return;
	}
*/
	//
	// compute LOD
	//
	lod = (mdxmLOD_t *)( (byte *)mHeader + mHeader->ofsLODs );
	whichLod = R_ComputeLOD( ent );
	if (whichLod >= mHeader->numLODs)
	{
		whichLod  = mHeader->numLODs -1;
	}
	for ( i = 0; i < whichLod; i++)
	{
		lod = (mdxmLOD_t*)( (byte *)lod + lod->ofsEnd );
	}
/*MODVIEWREM
	//
	// set up lighting now that we know we aren't culled
	//
	if ( !personalModel || r_shadows->integer > 1 ) 
	{
		// FIXME!! Is there something here we should be looking at?
		R_SetupEntityLighting( &tr.refdef, ent );
	}

	//
	// see if we are in a fog volume
	//
	fogNum = R_GComputeFogNum( aHeader, ent );


	//
	// draw all surfaces
	//
	cust_shader = R_GetShaderByHandle( ent->e.customShader );
*/
	// construct a list of all bones used by this model - this makes the bone transform go a bit faster since it will dump out bones
	// that aren't being used. - NOTE this will fuck up any models that have surfaces turned off where the lower surfaces aren't.


	memset(boneUsedList, 0, sizeof(boneUsedList));	// findmeste
	//
	// if highlighting, ensure that the bone we're over is transformed, even if no surfaces actually reference it
	//	( this is only needed when force-keeping the motion bone during compile)
	//
	extern ModelContainer_t* gpContainerBeingRendered;
	if (gpContainerBeingRendered	// arrrghhh!!!!
		&&
		AppVars.bBoneHighlight
		)
	{
		if (gpContainerBeingRendered->iBoneHighlightNumber >= 0)
		{
			boneUsedList[gpContainerBeingRendered->iBoneHighlightNumber] = 1;
		}
	}

//	OutputDebugString("### Start\n");

	int iStartSurface = ent->e.iSurfaceNum_RootOverride;
	if (iStartSurface == -1)
	{
		iStartSurface = 0;
	}

	G2_ConstructUsedBoneList(&ent->e.slist[iStartSurface],boneUsedList,ent);
	// make sure the root bone is marked as being referenced
	boneUsedList[0] =1;

	// pre-transform all the bones of this model
	G2_TransformGhoulBones( aHeader, boneUsedList, ent->e );

	// start the walk of the surface hierarchy	
 	R_RenderSurfaces(&ent->e.slist[iStartSurface], ent, whichLod);//	MODVIEWREM, cust_shader, fogNum, personalModel);
}


/*
void R_LoadQuaternionIndex(const char* filename)
{
	int		len;

	FILE*	in;

	in = fopen(filename, "rb");

	if(in)
	{
		fseek(in, 0, SEEK_END);
		len = ftell(in);
		rewind(in);

		quaternionIndex = (cqpoint_t*)malloc(len);

		fread(quaternionIndex,1,len,in);

		fclose(in);
	}

}
*/


//////////////////// eof /////////////////

