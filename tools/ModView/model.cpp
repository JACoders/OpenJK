// Filename:-	model.cpp
//
// non-format specific model routines entry point, calls format-specific code from within here
//
// ( This is the nice clean gateway module into all the evil crap I have to call from other codebases )
//

#include "stdafx.h"
#include "includes.h"
#include "ModViewTreeView.h"
#include "glm_code.h"
#include "R_Model.h"
#include "R_Surface.h"
#include "textures.h"
#include "TEXT.H"
#include "sequence.h"
#include "script.h"
#include "shader.h"
#include "skins.h"
//
#include "model.h"



static int	Model_MultiSeq_GetSeqHint(ModelContainer_t *pContainer, bool bPrimary);
static void Model_MultiSeq_SetSeqHint(ModelContainer_t *pContainer, bool bPrimary, int iHint);
static bool Model_MultiSeq_EnsureSeqHintLegal(ModelContainer_t *pContainer, int iFrame, bool bPrimary);


#define sERROR_MODEL_NOT_LOADED		"Error: Model not loaded, you shouldn't get here! -Ste"
#define sERROR_CONTAINER_NOT_FOUND	"Error: Could not resolve model handle to container ptr, you shouldn't get here! -Ste"
#define sSECONDARY_ANIM_STATS_STRING "(Secondary anim)"	// try and keep this fairly short, since it occupies roughly the same space as "bolt: <boltname>"

#define POINT_SCALE 64.0f
#define POINT_ST_SCALE	16384.0f

ModViewAppVars_t AppVars;

bool gbRenderInhibit = false;	// MUST stay in this state except when loading a model

// some protos...
//
static void ModelDraw_InfoText_Totals(void);
static void ModelDraw_InfoText_Header(void);
static void R_ModelContainer_CallBack_InfoText(ModelContainer_t *pContainer, void *pvData);
typedef struct	// simple struct for passing text data to callback functions during info printing (ZEROMEM'd)
{
	int iTextY;
	int iTextX;
	int iPrevX;
	int iTextXForVertStats;

	char sString[1024];
	int iTot_RenderedVerts;
	int iTot_RenderedTris;
	int iTot_RenderedSurfs;
	int iTot_XformedG2Bones;
	int iTot_RenderedBoneWeights;
	int iTot_OmittedBoneWeights;

	// auto-measure stuff, finding longest string for neater padding...
	//
	int iFrameDigitsNeeded;
	int iAttachedViaCharsNeeded;
	int iSequenceNameCharsNeeded;
	int iModelNameCharsNeeded;
	int iModelVertInfoCharsNeeded;
	//
	int iMostMultiLockedSequences;
	bool bAnyMultiLockedSecondarySequences;
	int iMultiLockedTextX;

} TextData_t;
TextData_t TextData;



double getDoubleTime (void)
{
	return (double)clock() / (double)CLOCKS_PER_SEC;
}


// returns NULL if not attached to anything, else name of tag-surface or boltpoint
//
static LPCSTR Stats_GetParentAttachmentPointString(ModelContainer_t *pContainer)
{
	LPCSTR psAttachedVia =	(!pContainer->pBoneBolt_ParentContainer)? 
							((!pContainer->pSurfaceBolt_ParentContainer)?
								NULL:
								pContainer->pSurfaceBolt_ParentContainer->tSurfaceBolt_BoltPoints[pContainer->iSurfaceBolt_ParentBoltIndex].sAttachName.c_str())
								:
								pContainer->pBoneBolt_ParentContainer->tBoneBolt_BoltPoints[pContainer->iBoneBolt_ParentBoltIndex].sAttachName.c_str();

	return psAttachedVia;
}

// returned string will be valid for printing, even if only blank...
//
static LPCSTR Stats_GetAttachmentString(ModelContainer_t *pContainer)
{
	LPCSTR psAttachedVia		= Stats_GetParentAttachmentPointString(pContainer);
	LPCSTR psAttachmentString	= va("%s", psAttachedVia?va("(bolt: \"%s\")",psAttachedVia):"");

	return psAttachmentString;
}

static void R_ModelContainer_Apply_Actual(ModelContainer_t* pContainer, void (*pFunction) ( ModelContainer_t* pContainer, void *pvData), void *pvData, bool bFromBottomUp )
{
	int iBoltPoint = 0;

	if (!bFromBottomUp )
	{
		// process this...
		//
		pFunction(pContainer, pvData);
	}

	// process this container's bone bolts...
	//	
	for (iBoltPoint=0; iBoltPoint < pContainer->iBoneBolt_MaxBoltPoints; iBoltPoint++)
	{
		BoltPoint_t	*pBoltPoint = &pContainer->tBoneBolt_BoltPoints[ iBoltPoint ];

		for (int iBoltOn = 0; iBoltOn < pBoltPoint->vBoltedContainers.size(); iBoltOn++)
		{				
			R_ModelContainer_Apply(&pBoltPoint->vBoltedContainers[ iBoltOn ], pFunction, pvData);
		}
	}

	// process this container's surface bolts...
	//
	for (iBoltPoint=0; iBoltPoint<pContainer->iSurfaceBolt_MaxBoltPoints; iBoltPoint++)
	{
		BoltPoint_t	*pBoltPoint = &pContainer->tSurfaceBolt_BoltPoints[ iBoltPoint ];

		for (int iBoltOn = 0; iBoltOn < pBoltPoint->vBoltedContainers.size(); iBoltOn++)
		{
			R_ModelContainer_Apply(&pBoltPoint->vBoltedContainers[ iBoltOn ], pFunction, pvData);
		}
	}

	if (bFromBottomUp )
	{
		// process this...	(which has no children by now)
		//
		pFunction(pContainer, pvData);
	}
}

void R_ModelContainer_Apply(ModelContainer_t* pContainer, void (*pFunction) ( ModelContainer_t* pContainer, void *pvData), void *pvData)
{
	R_ModelContainer_Apply_Actual(pContainer, pFunction, pvData, false);
}

// same as above, but calls from bottom of recursion tree to top, so can destroy ptrs to lower elements during freeup...
//
static void R_ModelContainer_ApplyFromBottomUp(ModelContainer_t* pContainer, void (*pFunction) ( ModelContainer_t* pContainer, void *pvData), void *pvData = NULL);
static void R_ModelContainer_ApplyFromBottomUp(ModelContainer_t* pContainer, void (*pFunction) ( ModelContainer_t* pContainer, void *pvData), void *pvData )
{
	R_ModelContainer_Apply_Actual(pContainer, pFunction, pvData, true);
}



// set the supplied container to be empty...  (note that because of stl, 99% of this works for either init or dealloc)
//
static void ModelContainer_Clear(ModelContainer_t* pContainer, void *pvData = NULL);// last field provided for R_ModelContainer_Apply() only
static void ModelContainer_Clear(ModelContainer_t* pContainer, void *pvData)
{
	pContainer->hModel		=	0;
	pContainer->eModType	= MOD_BAD;		

	ZEROMEM(pContainer->sLocalPathName);
	ZEROMEM(pContainer->slist);
	ZEROMEM(pContainer->blist);
	pContainer->iBoneNum_SecondaryStart = -1; // default, meaning "ignore", else bone num to stop primary animation on, and begin secondary
	pContainer->iSurfaceNum_RootOverride = -1;

	pContainer->iCurrentFrame_Primary	=	0;
	pContainer->iOldFrame_Primary		=	0;
	pContainer->iCurrentFrame_Secondary =	0;
	pContainer->iOldFrame_Secondary		=	0;
	pContainer->iSequenceLockNumber_Primary	  = -1;
	pContainer->iSequenceLockNumber_Secondary = -1;

	pContainer->iNumFrames		=	0;	
	pContainer->iNumLODs		=	0;
	pContainer->iNumBones		=	0;
	pContainer->iNumSurfaces	=	0;

	// stats only...
	pContainer->iRenderedTris	=	0;
	pContainer->iRenderedVerts	=	0;
	pContainer->iRenderedSurfs	=	0;
	pContainer->iXformedG2Bones	=	0;	
	pContainer->iRenderedBoneWeights = 0;
	pContainer->iOmittedBoneWeights = 0;

	pContainer->SequenceList.clear();
	pContainer->bSeqMultiLock_Primary_Active = false;
	pContainer->SeqMultiLock_Primary.clear();
	pContainer->bSeqMultiLock_Secondary_Active = false;
	pContainer->SeqMultiLock_Secondary.clear();
	pContainer->iSeqMultiLock_Primary_SeqHint   =0;	// not really important what number is picked
	pContainer->iSeqMultiLock_Secondary_SeqHint =0;	// ""

	pContainer->SkinSets.clear();
	pContainer->SkinSetsSurfacePrefs.clear();
	pContainer->OldSkinSets.clear();
	pContainer->strCurrentSkinFile	= "";
	pContainer->strCurrentSkinEthnic= "";
	pContainer->MaterialBinds.clear();
	pContainer->MaterialShaders.clear();

	pContainer->SurfaceEdgeInfoPerLOD.clear();

	pContainer->iBoneHighlightNumber	= iITEMHIGHLIGHT_NONE;
	pContainer->iSurfaceHighlightNumber	= iITEMHIGHLIGHT_NONE;
//	pContainer->iRenderedBoneWeightsThisSurface = 0;
	pContainer->Aliases.clear();

	pContainer->pModelInfoFunction				= NULL;
	pContainer->pModelGetBoneNameFunction		= NULL;
	pContainer->pModelGetBoneBoltNameFunction	= NULL;
	pContainer->pModelGetSurfaceNameFunction	= NULL;
	pContainer->pModelGetSurfaceBoltNameFunction= NULL;

	// some freaky stuff...
	//
	ZEROMEM(pContainer->XFormedG2Bones);
	ZEROMEM(pContainer->XFormedG2BonesValid);
	ZEROMEM(pContainer->XFormedG2TagSurfs);
	ZEROMEM(pContainer->XFormedG2TagSurfsValid);

	// special linkage stuff (this can all safely be cleared since during freeup we're called recursively backwards)...
	//
	if (pContainer->tBoneBolt_BoltPoints.size() || pContainer->tSurfaceBolt_BoltPoints.size())
	{
		AppVars.iTotalContainers--;	// we're calling this to free stuff, not to init it
	}
	//
	// now do specific bolt-free code...
	//
	{// bone bolts...
	 
		pContainer->pBoneBolt_ParentContainer = NULL;
		pContainer->iBoneBolt_ParentBoltIndex = -1;	// .. if we're the root model, seems a reasonable default (as is NULL on line above)
		pContainer->iBoneBolt_MaxBoltPoints = -1;	// similar to modtype_bad, should never exist when init code called after this
		pContainer->tBoneBolt_BoltPoints.clear();
	}
	{// surface bolts...

		pContainer->pSurfaceBolt_ParentContainer = NULL;
		pContainer->iSurfaceBolt_ParentBoltIndex = -1;	// .. if we're the root model, seems a reasonable default (as is NULL on line above)
		pContainer->iSurfaceBolt_MaxBoltPoints = -1;	// similar to modtype_bad, should never exist when init code called after this
		pContainer->tSurfaceBolt_BoltPoints.clear();
	}

	pContainer->hTreeItem_ModelName = NULL;
	pContainer->hTreeItem_BoltOns	= NULL;
}


// this is stuff that gets cleared during both AppVars_OnceOnlyInit() and Model_Delete()...
//
static void AppVars_Delete(void)
{
//	int iNumBolts = 0;
	R_ModelContainer_ApplyFromBottomUp(&AppVars.Container, ModelContainer_Clear);//, &iNumBolts);

	AppVars.iSurfaceNumToHighlight = iITEMHIGHLIGHT_NONE;
	AppVars.hModelToHighLight		= 0;

	// do this stuff so new models don't load and immediately animate...
	//
	AppVars.bAnimate = false;
	AppVars.bForceWrapWhenAnimating = false;
	AppVars.iLOD = 0;
	
	AppVars.iTotalContainers	= 0;
	AppVars.strLoadedModelPath	= "";
	AppVars.hModelLastLoaded	= NULL;
}

void AppVars_ResetViewParams(void)
{
#if 0
	// FOV 90 params...
	//
	AppVars.xPos		= 0.0f;
	AppVars.yPos		= 0.0f;
	AppVars.zPos		= -2.0f;

    AppVars.rotAngleX	= 0.0f;
	AppVars.rotAngleY	= 0.0f;
	AppVars.rotAngleZ	= -90.0f;

	AppVars.dFOV		= 90.0f;
#else
	// FOV 10 params... (and slightly rotated to pleasing angle)
	//
	AppVars.xPos		= 0.0f;
	AppVars.yPos		= 0.0f;
	AppVars.zPos		= -30.0f;

    AppVars.rotAngleX	= 15.5f;
	AppVars.rotAngleY	= 44.0f;
	AppVars.rotAngleZ	= -90.0f;

	AppVars.dFOV		= 10.0f;
#endif

	AppVars.xPos_SCROLL		= 0.0f;
	AppVars.yPos_SCROLL		= 0.0f;
	AppVars.zPos_SCROLL		= 0.0f;

    AppVars.rotAngleX_SCROLL	= 0.0f;
	AppVars.rotAngleY_SCROLL	= 0.0f;
	AppVars.rotAngleZ_SCROLL	= 0.0f;
}

void AppVars_OnceOnlyInit(void)
{
	AppVars.bFinished			=	false;
	AppVars.bBilinear			=	true;	
	AppVars.bInterpolate		=	true;
	AppVars.bUseAlpha			=	false;
	AppVars.bWireFrame			=	false;
	AppVars.bOriginLines		=	false;
	AppVars.bBBox				=	false;
	AppVars.bFloor				=	false;
	AppVars.fFloorZ				=	-50;
	AppVars.bRuler				=	false;
	AppVars.bBoneHighlight		=	true;
	AppVars.bBoneWeightThreshholdingActive = false;
	AppVars.fBoneWeightThreshholdPercent = 5.0f;	// 
	AppVars.bSurfaceHighlight	=	true;
	AppVars.bSurfaceHighlightShowsBoneWeighting = false;
	AppVars.bTriIndexes			=	false;
	AppVars.bVertIndexes		=	false;
	AppVars.bVertWeighting		=	false;
	AppVars.bAtleast1VertWeightDisplayed =	false;
	AppVars.bVertexNormals		=	false;
	AppVars.bShowOriginsAsRGB	=	true;
	AppVars.bForceWhite			=	false;
	AppVars.bCleanScreenShots	=	true;
	AppVars.bFullPathsInSequenceTreeitems = false;
	AppVars.bCrackHighlight		=	false;
	AppVars.bShowUnshadowableSurfaces = false;
	AppVars.bAllowGLAOverrides	=	false;
	AppVars.bShowPolysAsDoubleSided = true;
	
	// crap...
	//
	AppVars.iSurfaceNumToHighlight = iITEMHIGHLIGHT_NONE;
	AppVars.hModelToHighLight	= NULL;
	AppVars.hModelLastLoaded	= NULL;
	AppVars.bAlwaysOnTop		= false;
	AppVars.bSortSequencesByAlpha = false;

	AppVars.iLOD				=	0;
	
	AppVars_ResetViewParams();

	AppVars._R = AppVars._G = AppVars._B = 256/5;	// dark grey

	AppVars.dAnimSpeed			= 0.05;	// so 1/this = 20 = 20FPS
	AppVars.dTimeStamp1			= getDoubleTime();
	AppVars.fFramefrac			= 0.0f;
	AppVars.bAnimate			= false;	
	AppVars.bForceWrapWhenAnimating = false;

	AppVars_Delete();
}


void AppVars_WriteIdeal(void)
{
	if (!AppVars.strLoadedModelPath.IsEmpty())
	{
		CString strOut;

#define OUTBYTE(blah)	strOut += va("%s:%d\n",#blah,AppVars.blah);
#define OUTDOUBLE(blah) strOut += va("%s:%f\n",#blah,AppVars.blah);
		
		OUTBYTE(bBilinear);
		OUTBYTE(bOriginLines);
		OUTBYTE(bBBox);
		OUTBYTE(bUseAlpha);
		OUTBYTE(bWireFrame);
		OUTBYTE(_R);
		OUTBYTE(_G);
		OUTBYTE(_B);

		OUTDOUBLE(dFOV);
		OUTDOUBLE(dAnimSpeed);
		OUTDOUBLE(xPos);
		OUTDOUBLE(yPos);
		OUTDOUBLE(zPos);
		OUTDOUBLE(rotAngleX);
		OUTDOUBLE(rotAngleY);
		OUTDOUBLE(rotAngleZ);

		LPCSTR psIdealName = va("%s.ideal",Filename_WithoutExt(AppVars.strLoadedModelPath));

		FILE *fHandle = fopen(psIdealName,"wt");
		if (fHandle)
		{
			fprintf(fHandle,(LPCSTR)strOut);
			fclose(fHandle);
		}
		else
		{
			ErrorBox(va("Unable to write \"%s\"!, write protected?",psIdealName));
		}
	}
	else
	{
		ErrorBox("Cannot write out a .ideal file if no model loaded");
	}
}

void AppVars_ReadIdeal(void)
{
	if (!AppVars.strLoadedModelPath.IsEmpty())
	{
		LPCSTR psIdealName = va("%s.ideal",Filename_WithoutExt(AppVars.strLoadedModelPath));

		FILE *fHandle = fopen(psIdealName,"rt");
		if (fHandle)
		{
			CString str;
			char sLine[1024];

			while (fgets(sLine,sizeof(sLine),fHandle)!=NULL)
			{
				// deal with CR stuff manually, in case file was edited by hand and last line doesn't have one...
				//
				if (strchr(sLine,'\n'))
					*strchr(sLine,'\n') = '\0';

				str += sLine;
				str += "\n";
			}
						
			fclose(fHandle);

			extern bool Gallery_Active(void);
			if (Gallery_Active())
			{
				// this won't actually put up a box if the gallery is active, but it will add it to the
				//	overall report file so they'll know about it...
				// 
				// (I may offer the ability to use these, but can't be bothered hassling them with extra
				//	Yes/No queries at the moment)
				//
				WarningBox(va("Ignoring settings file \"%s\" during gallery snapshots",psIdealName));
				return;
			}

			// now check for certain values...
			//
			while (1)
			{
				int iLoc = str.Find('\n');
				if (iLoc == -1)
					break;
				
				CString strThis = str.Left(iLoc);				
				if (strThis.IsEmpty())
					break;

				str = str.Mid(iLoc+1);			

				iLoc = strThis.Find(':');
				if (iLoc == -1)
					break;

				CString strValue = strThis.Mid(iLoc+1);
				strThis = strThis.Left(iLoc);

				// now look for one of the named/saved fields...
				//

#define CHECKBOOL(blah)									\
				if (strThis.CompareNoCase(#blah) == 0)	\
				{										\
					AppVars.blah = !!atoi(strValue);	\
					continue;							\
				}
				
#define CHECKBYTE(blah)									\
				if (strThis.CompareNoCase(#blah) == 0)	\
				{										\
					AppVars.blah = atoi(strValue);		\
					continue;							\
				}

#define CHECKDOUBLE(blah)								\
				if (strThis.CompareNoCase(#blah) == 0)	\
				{										\
					AppVars.blah = atof(strValue);		\
					continue;							\
				}

				CHECKBOOL(bBilinear);
				CHECKBOOL(bOriginLines);
				CHECKBOOL(bBBox);
				CHECKBOOL(bUseAlpha);
				CHECKBOOL(bWireFrame);
				CHECKBYTE(_R);
				CHECKBYTE(_G);
				CHECKBYTE(_B);

				CHECKDOUBLE(dFOV);
				CHECKDOUBLE(dAnimSpeed);
				CHECKDOUBLE(xPos);
				CHECKDOUBLE(yPos);
				CHECKDOUBLE(zPos);
				CHECKDOUBLE(rotAngleX);
				CHECKDOUBLE(rotAngleY);
				CHECKDOUBLE(rotAngleZ);				
			}

			TextureList_SetFilter();	// in case filtering was changed
			ModelList_ForceRedraw();
		}
		// DT EDIT
		/*
		else
		{
			ErrorBox( va("Couldn't open file: %s\n", psIdealName));
			return;
		}
		*/
	}
}


// the global stuff for any loaded model, regardless of format...
//

// this deletes the primary model, all bolted models, and the low-level model cache
//
void Model_Delete(void)
{
	// delete common stuff...
	//
//	SAFEFREE(pvLoadedModel);
	
	ModelTree_DeleteAllItems();

	// delete any format-specific stuff that this code doesn't know about...
	//
	GLMModel_DeleteExtra();		// delete anything specific to this format	
	RE_DeleteModels();

	// delete other app vars...
	//	
	AppVars_Delete();
	extern bool g_bReportImageLoadErrors;
	g_bReportImageLoadErrors = false;	// uninhibit any inhibited errors
}



LPCSTR Model_GetSupportedTypesFilter(bool bScriptsEtcAlsoAllowed /* = false */)
{
	static char sFilterString[1024];

	strcpy(sFilterString,"Model files (*.glm)|*.glm|");

	if (bScriptsEtcAlsoAllowed)
	{
		strcat(sFilterString, Script_GetFilter(false));
	}

	strcat(sFilterString,"All Files(*.*)|*.*||");

	return sFilterString;
}


// findme: All code that uses cut/paste from other projects should go through here, since it allows the whole
//			code-exit mechanism that they use to be trapped properly...
//
// call this before calling any cut/paste other-format model code
ModelHandle_t Model_Register( CString strLocalFilename )
{
	ModelHandle_t hModel = NULL;

	try
	{
		StatusMessage(va("Registering model: \"%s\"\n",(LPCSTR)strLocalFilename));

		hModel = RE_RegisterModel( strLocalFilename );
	}

	catch(LPCSTR psMessage)
	{
		Model_Delete();
		ErrorBox(psMessage);
		hModel = NULL;
	}

	StatusMessage(NULL);

	return hModel;
}


ModelContainer_t* pMatchingContainer;

static void ModelContainer_CallBack_HandleCheck(ModelContainer_t* pContainer, void *pvData )
{
	if (pContainer->hModel == *((ModelHandle_t*) pvData))
	{
		pMatchingContainer = pContainer;
	}
}

ModelContainer_t* ModelContainer_FindFromModelHandle(ModelHandle_t hModel)
{
	pMatchingContainer = NULL;
	R_ModelContainer_Apply(&AppVars.Container, ModelContainer_CallBack_HandleCheck, &hModel);

	if (pMatchingContainer)
		return pMatchingContainer;

	return NULL;
}



// read in a model using main engine code, then parse any extra stuff that this modview apps wants to know about...
//
// note that this doesn't know or care whether it's the parent container or a bolt on, and neither should it.
//	Any error will delete all loaded models (as per usual)...
//
static ModelHandle_t ModelContainer_RegisterModel(LPCSTR psLocalFilename, ModelContainer_t *pContainer, HTREEITEM hTreeItem_Parent = NULL);
static ModelHandle_t ModelContainer_RegisterModel(LPCSTR psLocalFilename, ModelContainer_t *pContainer, HTREEITEM hTreeItem_Parent)
{
	CWaitCursor wait;

	ModelContainer_Clear(pContainer);	//	ZEROMEM(*pContainer);

	ModelHandle_t hModel = Model_Register( psLocalFilename );
	strncpy(pContainer->sLocalPathName,psLocalFilename,sizeof(pContainer->sLocalPathName));
	pContainer->sLocalPathName[sizeof(pContainer->sLocalPathName)-1] = '\0';

	int iBoltPoint = 0;

	if (hModel)
	{
		pContainer->hModel = hModel;

		modtype_t modtype = MOD_BAD;	// reasonable default

		// do any game-type post-process code...  (hence the try-catch block)
		//
		try
		{
			if ( (modtype = RE_GetModelType( hModel )) == MOD_MDXM)
			{
				trap_G2_SurfaceOffList(hModel, &pContainer->slist);
				trap_G2_Init_Bone_List(&pContainer->blist);
				//trap_G2_Set_Bone_Anim(ent->ghoulmodel, ent->s.blist, "model_root", 0, 9, BONE_ANIM_OVERRIDE_LOOP, 0.1f);
			}
		}

		catch(LPCSTR psMessage)
		{
			Model_Delete();
			ErrorBox(psMessage);
			hModel = NULL;
		}


		// now do any of my post-process stuff... (which doesn't need try-catch because it's well written :-)
		//
		if (hModel)
		{
			pContainer->eModType = modtype;

			bool bModelOk = true;
			switch (modtype)
			{
				case MOD_MDXM:
					
					bModelOk = GLMModel_Parse( pContainer, psLocalFilename, hTreeItem_Parent);

					if (bModelOk)
					{
						// specific to this format...
						//
						assert(pContainer->pModelGetBoneNameFunction);
						assert(pContainer->pModelGetBoneBoltNameFunction);
						assert(pContainer->pModelGetSurfaceBoltNameFunction);
					}
					break;

				default:
					//assert(0);
					bModelOk = false;
					ErrorBox(va("The model \"%s\" is valid, but ModView doesn't fully support this type at present",psLocalFilename));
					break;
			}

			if (bModelOk)
			{
				// the above switch-case should have filled in these per-format...
				//
				assert(pContainer->iBoneBolt_MaxBoltPoints != -1);	// check that deliberate illegal default is overwritten
				assert(pContainer->iSurfaceBolt_MaxBoltPoints != -1);	// check that deliberate illegal default is overwritten
				assert(pContainer->iNumLODs);
				assert(pContainer->iNumFrames);
				assert(pContainer->pModelInfoFunction);				
				assert(pContainer->pModelGetSurfaceNameFunction);

				// if failed to read any sequence files then make a default one...
				//
				if (!pContainer->SequenceList.size())
				{
					pContainer->SequenceList.push_back( *Sequence_CreateDefault(pContainer->iNumFrames) );
				} 
				
				// default bolton stuff (ensure that matrix mem initialised, and bolton array resized correctly...)
				//
				// bone bolts...
				//
				pContainer->tBoneBolt_BoltPoints.resize(pContainer->iBoneBolt_MaxBoltPoints);
				for (iBoltPoint = 0; iBoltPoint < pContainer->iBoneBolt_MaxBoltPoints; iBoltPoint++)
				{
					BoltPoint_t *pBoltPoint = &pContainer->tBoneBolt_BoltPoints[ iBoltPoint ];
					
					pBoltPoint->vMatricesPerFrame.resize( pContainer->iNumFrames );
					pBoltPoint->sAttachName = pContainer->pModelGetBoneBoltNameFunction(pContainer->hModel, iBoltPoint);
					pBoltPoint->vBoltedContainers.clear();	// probably not nec., but wtf?
				}

				//
				// surface bolts...
				//
				pContainer->tSurfaceBolt_BoltPoints.resize(pContainer->iSurfaceBolt_MaxBoltPoints);
				for (iBoltPoint=0; iBoltPoint<pContainer->iSurfaceBolt_MaxBoltPoints; iBoltPoint++)
				{
					BoltPoint_t *pBoltPoint = &pContainer->tSurfaceBolt_BoltPoints[iBoltPoint];

					pBoltPoint->vMatricesPerFrame.resize( pContainer->iNumFrames );
					pBoltPoint->sAttachName = pContainer->pModelGetSurfaceBoltNameFunction(pContainer->hModel, iBoltPoint);
					pBoltPoint->vBoltedContainers.clear();				
				}

				// finally, we can do stuff like skin file code that can popup GetYesNo boxes, which cause grief
				//	if they happen before the bolt stuff above has occured...
				//
				switch (modtype)
				{
					case MOD_MDXM:
						
						// only one of these will be valid at once, so no need to check...
						//
						Skins_ApplyDefault(pContainer);
						OldSkins_ApplyDefault(pContainer);
						break;

					default:
						assert(0);
						break;
				}
			}
			
			if (!bModelOk)
			{
				Model_Delete();				
				hModel = NULL;
			}
		}
	}

	return hModel;
}


void ModelTree_DeleteAllItems(void)
{
	if (gModViewTreeViewhandle)	// will be valid unless this is called from app exit
	{
		gModViewTreeViewhandle->DeleteAllItems();
	}
}

DWORD ModelTree_GetItemData(HTREEITEM hTreeItem)
{
	if (gModViewTreeViewhandle)	// will be valid unless this is called from app exit
	{
		return gModViewTreeViewhandle->GetTreeCtrl().GetItemData(hTreeItem);
	}

	assert(0);
	return NULL;
}

bool ModelTree_SetItemText(HTREEITEM hTreeItem, LPCSTR psText)
{
	if (gModViewTreeViewhandle)	// will be valid unless this is called from app exit
	{
		return !!gModViewTreeViewhandle->GetTreeCtrl().SetItemText(hTreeItem, psText);
	}

	assert(0);
	return NULL;
}



// param 'bPure' should be TRUE if you want to strip stuff like "////" from "///////// surfacename",
//	and return the original un-decorated text by querying the model directly if possible...
//
// this function was written so Keith could remote-query from ConfuseEd...
//
LPCSTR ModelTree_GetItemText(HTREEITEM hTreeItem, bool bPure /* = false */)
{
	if (gModViewTreeViewhandle)	// will be valid unless this is called from app exit
	{
		if (bPure)
		{
			// let's see if this is a treeitem type that can return pure text...
			//
			TreeItemData_t	TreeItemData;
							TreeItemData.uiData = ModelTree_GetItemData(hTreeItem);

			if (TreeItemData.iItemType == TREEITEMTYPE_GLM_SURFACE
				||
				TreeItemData.iItemType == TREEITEMTYPE_GLM_TAGSURFACE
				)
			{
				return GLMModel_GetSurfaceName( TreeItemData.iModelHandle, TreeItemData.iItemNumber );
			}
		}

		// whatever it is, just return its itemtext in full...
		//


		// do NOT use the CString(input) constructor here!!
		//
		static CString	string;
						string = gModViewTreeViewhandle->GetTreeCtrl().GetItemText(hTreeItem);
		return (LPCSTR) string;
	}

	assert(0);
	return NULL;
}

// search tree for an item whose userdata matches what's been passed...
//
// hTreeItem = tree item to start from, pass NULL to start from root
//
static HTREEITEM R_ModelTree_FindItemWithThisData(HTREEITEM hTreeItem, UINT32 uiData2Match, int *piItemsScanned = NULL);
static HTREEITEM R_ModelTree_FindItemWithThisData(HTREEITEM hTreeItem, UINT32 uiData2Match, int *piItemsScanned/*=NULL*/)
{
	if (!hTreeItem)
		 hTreeItem = ModelTree_GetRootItem();

	if (hTreeItem)
	{
		if (piItemsScanned)
		{
			*piItemsScanned +=1;
		}
//		LPCSTR psText = ModelTree_GetItemText(hTreeItem);
//		OutputDebugString(va("Scanning item %X (%s)\n",hTreeItem,psText));

		// check this tree item...
		//
		TreeItemData_t	TreeItemData;
						TreeItemData.uiData = ModelTree_GetItemData(hTreeItem);

		// match?...
		//
		if (TreeItemData.uiData == uiData2Match)
			return hTreeItem;

		// check child...
		//
		HTREEITEM hTreeItem_Child = ModelTree_GetChildItem(hTreeItem);
		if (hTreeItem_Child)
		{
			HTREEITEM hTreeItemFound = R_ModelTree_FindItemWithThisData(hTreeItem_Child, uiData2Match, piItemsScanned);
			if (hTreeItemFound)
				return hTreeItemFound;
		}
		
		// process siblings...
		//
		HTREEITEM hTreeItem_Sibling = ModelTree_GetNextSiblingItem(hTreeItem);
		if (hTreeItem_Sibling)
		{
			HTREEITEM hTreeItemFound = R_ModelTree_FindItemWithThisData(hTreeItem_Sibling, uiData2Match, piItemsScanned);
			if (hTreeItemFound)
				return hTreeItemFound;
		}

		// this treeitem isnt a match, and neither are its siblings or children, so...
		//
		return NULL;
	}

	// we must have called this when the treeview was uninitialised... (duh!)
	//
	ASSERT(0);
	return NULL;
}


int ModelTree_GetChildCount(HTREEITEM hTreeItem)
{
	int iChildCount = 0;

	if (gModViewTreeViewhandle)	// will be valid unless this is called from app exit	 
	{		
		if (gModViewTreeViewhandle->GetTreeCtrl().ItemHasChildren(hTreeItem))
		{
			hTreeItem = gModViewTreeViewhandle->GetTreeCtrl().GetChildItem(hTreeItem);			
			R_ModelTree_FindItemWithThisData(hTreeItem, 0xDEADDEAD, &iChildCount);	// massive-function abuse here! :-)
		}
	}

	return iChildCount;
}

bool ModelTree_ItemHasChildren(HTREEITEM hTreeItem)
{
	if (gModViewTreeViewhandle)	// will be valid unless this is called from app exit
	{
		return !!gModViewTreeViewhandle->GetTreeCtrl().ItemHasChildren(hTreeItem);
	}

	assert(0);
	return NULL;
}

HTREEITEM ModelTree_GetChildItem(HTREEITEM hTreeItem)
{
	if (gModViewTreeViewhandle)	// will be valid unless this is called from app exit
	{
		return gModViewTreeViewhandle->GetTreeCtrl().GetChildItem(hTreeItem);
	}

	assert(0);
	return NULL;
}

HTREEITEM ModelTree_GetNextSiblingItem(HTREEITEM hTreeItem)
{
	if (gModViewTreeViewhandle)	// will be valid unless this is called from app exit
	{
		return gModViewTreeViewhandle->GetTreeCtrl().GetNextSiblingItem(hTreeItem);
	}

	assert(0);
	return NULL;
}


HTREEITEM ModelTree_GetRootItem(void)
{
	if (gModViewTreeViewhandle)	// will be valid unless this is called from app exit
	{
		return gModViewTreeViewhandle->GetRootItem();
	}

	ASSERT(0);
	return NULL;
}


bool ModelTree_DeleteItem(HTREEITEM hTreeItem)
{
	if (gModViewTreeViewhandle)	// will be valid unless this is called from app exit
	{
		return !!gModViewTreeViewhandle->GetTreeCtrl().DeleteItem(hTreeItem);
	}

	ASSERT(0);
	return NULL;
}



// this should only be called when you know it's a GLM model for the moment...  (put in for Keith's remote access)
//
HTREEITEM ModelTree_GetRootSurface(ModelHandle_t hModel)
{
	if (gModViewTreeViewhandle && Model_Loaded(hModel))	// will be valid unless this is called from app exit
	{
		TreeItemData_t	TreeItemData = {0};
						TreeItemData.iItemType		= TREEITEMTYPE_SURFACEHEADER;
						TreeItemData.iModelHandle	= hModel;

		HTREEITEM hTreeItem_SurfaceHeader = R_ModelTree_FindItemWithThisData(NULL, TreeItemData.uiData);
		if (hTreeItem_SurfaceHeader)
		{
			return ModelTree_GetChildItem(hTreeItem_SurfaceHeader);
		}		
	}

	ASSERT(0);
	return NULL;
}


// this should only be called when you know it's a GLM model for the moment...  (put in for Keith's remote access)
//
HTREEITEM ModelTree_GetRootBone(ModelHandle_t hModel)
{
	if (gModViewTreeViewhandle && Model_Loaded(hModel))	// will be valid unless this is called from app exit
	{
		TreeItemData_t	TreeItemData = {0};
						TreeItemData.iItemType		= TREEITEMTYPE_BONEHEADER;
						TreeItemData.iModelHandle	= hModel;

		HTREEITEM hTreeItem_BoneHeader = R_ModelTree_FindItemWithThisData(NULL, TreeItemData.uiData);
		if (hTreeItem_BoneHeader)
		{
			return ModelTree_GetChildItem(hTreeItem_BoneHeader);
		}		
	}

	ASSERT(0);
	return NULL;
}




static int SortSequenceBy_Alpha(const void *elem1, const void *elem2)
{
	Sequence_t *pSeq1 = *(Sequence_t**)elem1;
	Sequence_t *pSeq2 = *(Sequence_t**)elem2;
	return stricmp(pSeq1->sName, pSeq2->sName);
}

static int SortSequenceBy_FrameNum(const void *elem1, const void *elem2)
{
	Sequence_t *pSeq1 = *(Sequence_t**)elem1;
	Sequence_t *pSeq2 = *(Sequence_t**)elem2;

	return pSeq1->iStartFrame - pSeq2->iStartFrame;
}
	
void ModelTree_InsertSequences(ModelHandle_t hModel, HTREEITEM hTreeItem_Sequences)
{
	ModelContainer_t *pContainer = ModelContainer_FindFromModelHandle(hModel);
	
	if (pContainer)
	{
		ModelTree_InsertSequences(pContainer, hTreeItem_Sequences);
		return;
	}

	assert(0);
	ErrorBox(sERROR_CONTAINER_NOT_FOUND);
}

void ModelTree_InsertSequences(ModelContainer_t *pContainer, HTREEITEM hTreeItem_Sequences)
{
	CWaitCursor wait;
	int iSequenceIndex = 0;

	// delete any child items already present...
	//
	HTREEITEM hTreeItemChild = gModViewTreeViewhandle->GetTreeCtrl().GetChildItem(hTreeItem_Sequences);
	if (hTreeItemChild)
	{
		while (hTreeItemChild)
		{
			HTREEITEM hNext = gModViewTreeViewhandle->GetTreeCtrl().GetNextSiblingItem(hTreeItemChild);

			gModViewTreeViewhandle->GetTreeCtrl().DeleteItem(hTreeItemChild);

			hTreeItemChild = hNext;
		}
	}

	// make new list...
	//
	vector <Sequence_t*> Sequences;

	// add sequence ptrs to list...
	//
	for (iSequenceIndex = 0; iSequenceIndex<pContainer->SequenceList.size(); iSequenceIndex++)
	{
		Sequence_t *pSequence = &pContainer->SequenceList[iSequenceIndex];

		Sequences.push_back(pSequence);
	}

	// sort 'em...
	//
	qsort(&Sequences[0], Sequences.size(), sizeof(Sequence_t*), AppVars.bSortSequencesByAlpha ? SortSequenceBy_Alpha : SortSequenceBy_FrameNum);

	// add to tree...
	//
	for (int i=0; i<Sequences.size(); i++)
	{
		Sequence_t *pSequence	= Sequences[i];
		iSequenceIndex			= Sequence_GetIndex( pSequence, pContainer );

		if (iSequenceIndex != -1)
		{
			TreeItemData_t	TreeItemData				= {0};
							TreeItemData.iItemType		= TREEITEMTYPE_SEQUENCE;
							TreeItemData.iItemNumber	= iSequenceIndex;
							TreeItemData.iModelHandle	= pContainer->hModel;

			HTREEITEM htiThis = ModelTree_InsertItem(	Sequence_CreateTreeName(pSequence),	// LPCTSTR psName,
														hTreeItem_Sequences,				// HTREEITEM hParent
														TreeItemData.uiData					//	TREEITEMTYPE_GLM_SURFACE | iThisSurfaceIndex	// UINT32 uiUserData
													);
		}
		else
		{
			assert(0 && "ModelTree_InsertSequences(): Unknown sequence ptr!");
		}
	}
}



// hParent can be NULL...
//
HTREEITEM ModelTree_InsertItem(LPCTSTR psName, HTREEITEM hParent, UINT32 uiUserData /* = NULL */ , HTREEITEM hInsertAfter /* = TVI_LAST */)
{
	if (gModViewTreeViewhandle)	// will be valid unless this is called from app exit
	{
		return gModViewTreeViewhandle->InsertItem(psName, hParent, uiUserData, hInsertAfter);
	}

	ASSERT(0);
	return NULL;
}


bool Model_Loaded(ModelHandle_t hModel /* = NULL */)
{
	if (hModel)
	{
		return !!ModelContainer_FindFromModelHandle(hModel);
//		return !!(AppVars.mcPrimary.hModel == hModel);
	}

	return !!AppVars.Container.hModel;	// query primary container
}


static ModelContainer_t *ModelContainer_AllocNew(void)
{
	ModelContainer_t *pNewContainer = new ModelContainer_t;

	ModelContainer_Clear(pNewContainer);
	return pNewContainer;
}

// ensure that there's a "BoltOns" header in a container, and if not, add one and add it to the tree also.
//
static bool ModelContainer_EnsureBoltOnHeader(ModelContainer_t *pContainer)
{
	bool bReturn = false;

	if (pContainer->hTreeItem_BoltOns)
		return true;	// already got one

	TreeItemData_t	TreeItemData={0};
					TreeItemData.iItemType		= TREEITEMTYPE_BOLTONSHEADER;
					TreeItemData.iModelHandle	= pContainer->hModel;
	
	pContainer->hTreeItem_BoltOns = ModelTree_InsertItem("BoltOns", pContainer->hTreeItem_ModelName, TreeItemData.uiData);

	return !!pContainer->hTreeItem_BoltOns;
}


static bool _Actual_Model_LoadPrimary(LPCSTR psFullPathedFilename)
{
	bool bReturn = false;

	SetQdirFromPath( psFullPathedFilename );

	// I'll check for file existance first, to avoid deleting current model until a valid replacement is ready...
	//
	if (FileExists( psFullPathedFilename ))
	{
		Model_Delete();

		CString strLocalFilename( psFullPathedFilename );
		Filename_RemoveQUAKEBASE(strLocalFilename);

//		AppVars.Container = *ModelContainer_AllocNew();
		ModelContainer_Clear(&AppVars.Container);
		ModelHandle_t hModel = ModelContainer_RegisterModel(strLocalFilename, &AppVars.Container);//ModelContainer_AllocNew());

		if (hModel)
		{
			AppVars.strLoadedModelPath	= psFullPathedFilename;
			AppVars.iTotalContainers	= 1;
			AppVars.hModelLastLoaded	= hModel;
			bReturn = true;

			AppVars_ReadIdeal();	// check for optional "modelname.ideal"...
		}
	}
	else
	{
		ErrorBox(va("File not found: \"%s\"!",psFullPathedFilename));
	}


	if (!bReturn)
	{
		Model_Delete();
	}

	ModelList_ForceRedraw();

	return bReturn;
}

bool Model_LoadPrimary(LPCSTR psFullPathedFilename)
{
	gbRenderInhibit = true;
	bool b = _Actual_Model_LoadPrimary(psFullPathedFilename);
	gbRenderInhibit = false;

	ModelList_ForceRedraw();

	return b;
}



void Model_ApplyOldSkin( ModelHandle_t hModel, LPCSTR psSkin )
{
	ModelContainer_t *pContainer = ModelContainer_FindFromModelHandle(hModel);
	
	if (pContainer)
	{			
		OldSkins_Apply(pContainer, psSkin);
	}
	else
	{
		assert(0);
		ErrorBox(sERROR_CONTAINER_NOT_FOUND);
	}

	ModelList_ForceRedraw();
}


bool Model_SkinHasSurfacePrefs( ModelHandle_t hModel, LPCSTR psSkin )
{
	ModelContainer_t *pContainer = ModelContainer_FindFromModelHandle(hModel);
	
	if (pContainer)
	{	
		return Skins_FileHasSurfacePrefs(pContainer, psSkin);
	}
	else
	{
		assert(0);
		ErrorBox(sERROR_CONTAINER_NOT_FOUND);
	}

	return false;
}

void Model_ApplyEthnicSkin(ModelHandle_t hModel, LPCSTR psSkin, LPCSTR psEthnic, bool bApplySurfacePrefs, bool bDefaultSurfaces )
{
	ModelContainer_t *pContainer = ModelContainer_FindFromModelHandle(hModel);
	
	if (pContainer)
	{			
		Skins_ApplyEthnic(pContainer, psSkin, psEthnic, bApplySurfacePrefs, bDefaultSurfaces );
	}
	else
	{
		assert(0);
		ErrorBox(sERROR_CONTAINER_NOT_FOUND);
	}

	ModelList_ForceRedraw();
}

void Model_ApplySkinShaderVariant( ModelHandle_t hModel, LPCSTR psSkin, LPCSTR psEthnic, LPCSTR psMaterial, int iVariant)
{
	ModelContainer_t *pContainer = ModelContainer_FindFromModelHandle(hModel);
	
	if (pContainer)
	{			
		Skins_ApplySkinShaderVariant(pContainer, psSkin, psEthnic, psMaterial, iVariant );
	}
	else
	{
		assert(0);
		ErrorBox(sERROR_CONTAINER_NOT_FOUND);
	}

	ModelList_ForceRedraw();
}

void Model_ValidateSkin(ModelHandle_t hModel, int iSkinNumber)
{
	ModelContainer_t *pContainer = ModelContainer_FindFromModelHandle(hModel);
	
	if (pContainer)
	{
		if (pContainer->SkinSets.size())
		{
			Skins_Validate( pContainer, iSkinNumber );
		}
		else
		if (pContainer->OldSkinSets.size())
		{
			OldSkins_Validate(pContainer, iSkinNumber);
		}
	}
	else
	{
		assert(0);
		ErrorBox(sERROR_CONTAINER_NOT_FOUND);
	}

	ModelList_ForceRedraw();
}



// deletes whatever's bolted to the supplied bolt number of the suppplied model...
//
bool Model_DeleteBoltOn(ModelHandle_t hModel, int iBoltPointToDelete, bool bBoltIsBone, int iBoltOnAtBoltPoint)
{
	bool bReturn = false;

	ModelContainer_t *pContainer = ModelContainer_FindFromModelHandle(hModel);
	
	if (pContainer)
	{
		return Model_DeleteBoltOn(pContainer, iBoltPointToDelete, bBoltIsBone, iBoltOnAtBoltPoint);
	}
	else
	{
		assert(0);
		ErrorBox(sERROR_CONTAINER_NOT_FOUND);
	}

	return bReturn;
}

// deletes <iBoltOnAtBoltPoint / all(-1)> from <iBoltPointToDelete> on <pContainer>
//
bool Model_DeleteBoltOn(ModelContainer_t *pContainer, int iBoltPointToDelete, bool bBoltIsBone, int iBoltOnAtBoltPoint)
{		
	bool bReturn = false;

	vector <BoltPoint_t> &vBoltPoints	= bBoltIsBone ? pContainer->tBoneBolt_BoltPoints : pContainer->tSurfaceBolt_BoltPoints;
	int					 &iMaxBoltPoints= bBoltIsBone ? pContainer->iBoneBolt_MaxBoltPoints : pContainer->iSurfaceBolt_MaxBoltPoints;	

	if (iBoltPointToDelete < (bBoltIsBone ? pContainer->iBoneBolt_MaxBoltPoints : pContainer->iSurfaceBolt_MaxBoltPoints))
	{
		if (iBoltPointToDelete < (bBoltIsBone ? pContainer->tBoneBolt_BoltPoints.size() : pContainer->tSurfaceBolt_BoltPoints.size()))
		{
			BoltPoint_t *pBoltOn = bBoltIsBone ?	&pContainer->tBoneBolt_BoltPoints	[iBoltPointToDelete] :
													&pContainer->tSurfaceBolt_BoltPoints[iBoltPointToDelete];
			// stl-type limits, ie [Begin, End)...
			//
			int iBoltOnBegin = (iBoltOnAtBoltPoint == -1) ? 0 : iBoltOnAtBoltPoint;
			int iBoltOnEnd   = (iBoltOnAtBoltPoint == -1) ? pBoltOn->vBoltedContainers.size() : iBoltOnAtBoltPoint+1;

			for (int iBoltOn = iBoltOnBegin; iBoltOn != iBoltOnEnd; iBoltOn++)
			{
				ModelContainer_t *pBoltedContainer = &pBoltOn->vBoltedContainers[ iBoltOn ];
			
				// delete all tree items from the bolt downwards... (including models bolted to the bolt)
				//	
				ModelTree_DeleteItem(pBoltedContainer->hTreeItem_ModelName);

				// delete underlying containers... (including models bolted to the bolt)
				//
				R_ModelContainer_ApplyFromBottomUp(pBoltedContainer, ModelContainer_Clear);

/*				// delete the bolt itself and mark it as empty...
				//
				if (bBoltIsBone)
				{
					delete (pContainer->tBoneBolt_BoltPoints[iBoltPointToDelete].pBoltedContainer);
							pContainer->tBoneBolt_BoltPoints[iBoltPointToDelete].pBoltedContainer = NULL;
				}
				else
				{
					delete (pContainer->tSurfaceBolt_BoltPoints[iBoltPointToDelete].pBoltedContainer);
							pContainer->tSurfaceBolt_BoltPoints[iBoltPointToDelete].pBoltedContainer = NULL;
				}
*/
			}
			int iSize = pBoltOn->vBoltedContainers.size();
			pBoltOn->vBoltedContainers.erase(pBoltOn->vBoltedContainers.begin() + iBoltOnBegin, pBoltOn->vBoltedContainers.begin() + iBoltOnEnd);
			int iSize2 = pBoltOn->vBoltedContainers.size();

			// finally, for neatness, check if our BoltOns treeitem is now empty (no children), and delete it if so.
			//	(a new one will be created if a bolt is added again later)
			//
			if (!ModelTree_ItemHasChildren(pContainer->hTreeItem_BoltOns))
			{
				ModelTree_DeleteItem(pContainer->hTreeItem_BoltOns);
				pContainer->hTreeItem_BoltOns = NULL;
			}

			bReturn = true;
		}
		else
		{
			ErrorBox(va("Model_DeleteBoltOn(): %s bolt index %d is legal (max %d), but container only has %d entries!!! (Tell me! -Ste)",
												bBoltIsBone?"bone":"surface",
															iBoltPointToDelete, 
																			iMaxBoltPoints,
																										vBoltPoints.size()
						)
					);
		}
	}
	else
	{
		ErrorBox(va("Model_DeleteBoltOn(): Illegal %s bolt index %d (max = %d)!",bBoltIsBone?"bone":"surface",iBoltPointToDelete, bBoltIsBone?pContainer->iBoneBolt_MaxBoltPoints:pContainer->iSurfaceBolt_MaxBoltPoints));
	}

	if (bReturn)
	{
		ModelList_ForceRedraw();
	}

	return bReturn;
}

// returns -1 for unknown or error...
//
static int Model_GetBoltOnNumber(ModelContainer_t *pContainer, ModelContainer_t *pParentContainer, int iParentBoltIndex, bool bBoltIsBone)
{
	vector <BoltPoint_t> &vParentBoltPoints = bBoltIsBone ? pParentContainer->tBoneBolt_BoltPoints : pParentContainer->tSurfaceBolt_BoltPoints;

	if (iParentBoltIndex < vParentBoltPoints.size())
	{
		BoltPoint_t *pBoltOn = &vParentBoltPoints[ iParentBoltIndex ];

		for (int iBoltOn = 0; iBoltOn < pBoltOn->vBoltedContainers.size(); iBoltOn++)
		{
			if ( pContainer == &pBoltOn->vBoltedContainers[ iBoltOn ] )
				return iBoltOn;
		}
	}
	else
	{
		ErrorBox(va("Model_GetBoltOnNumber(): Error, illegal %s bolt index %d (max = %d)!  (Tell me! - Ste)\n",(bBoltIsBone?"bone":"surface"), iParentBoltIndex, vParentBoltPoints.size()));
	}

	return -1;
}


// deletes the supplied container of a bolted model from whatever its bolted to...
//
// (this works for either bone or surface boltons)
//
bool Model_DeleteBoltOn(ModelContainer_t *pContainer)
{
	bool bReturn = false;

	if (pContainer->pBoneBolt_ParentContainer)
	{
		int iBoltOnNumber = Model_GetBoltOnNumber(pContainer, pContainer->pBoneBolt_ParentContainer, pContainer->iBoneBolt_ParentBoltIndex, true);
		return Model_DeleteBoltOn(pContainer->pBoneBolt_ParentContainer, pContainer->iBoneBolt_ParentBoltIndex, true, iBoltOnNumber);
	}
	if (pContainer->pSurfaceBolt_ParentContainer)
	{
		int iBoltOnNumber = Model_GetBoltOnNumber(pContainer, pContainer->pSurfaceBolt_ParentContainer, pContainer->iSurfaceBolt_ParentBoltIndex, false);
		return Model_DeleteBoltOn(pContainer->pSurfaceBolt_ParentContainer, pContainer->iSurfaceBolt_ParentBoltIndex, false, iBoltOnNumber);
	}

	return bReturn;
}

// deletes the supplied boltOn from whatever it's bolted to...
//
// (this works for either bone or surface boltons)
//
bool Model_DeleteBoltOn(ModelHandle_t hModelBoltOn)
{
	bool bReturn = false;

	ModelContainer_t *pContainer = ModelContainer_FindFromModelHandle(hModelBoltOn);
	
	if (pContainer)
	{
		return Model_DeleteBoltOn(pContainer);
	}
	else
	{
		assert(0);
		ErrorBox(sERROR_CONTAINER_NOT_FOUND);
	}

	return bReturn;
}


// used for UI update check...
//
bool Model_HasParent(ModelHandle_t hModel)
{
	bool bReturn = false;

	ModelContainer_t *pContainer = ModelContainer_FindFromModelHandle(hModel);
	
	if (pContainer)
	{
		return (pContainer->pBoneBolt_ParentContainer || pContainer->pSurfaceBolt_ParentContainer);
	}
	else
	{
		assert(0);
		ErrorBox(sERROR_CONTAINER_NOT_FOUND);
	}

	return bReturn;
}

// used for UI update check...  (also used in renderer now, to work out whether or not to xform a g2 tag surface
//
int Model_CountItemsBoltedHere(ModelHandle_t hModel, int iBoltIndex, bool bBoltIsBone)
{
	ModelContainer_t *pContainer = ModelContainer_FindFromModelHandle(hModel);
	
	if (pContainer)
	{
		if (bBoltIsBone)
		{
			if (iBoltIndex < pContainer->iBoneBolt_MaxBoltPoints)
			{
				return pContainer->tBoneBolt_BoltPoints[ iBoltIndex ].vBoltedContainers.size();
			}
		}
		else
		{
			if (iBoltIndex < pContainer->iSurfaceBolt_MaxBoltPoints)
			{
				return pContainer->tSurfaceBolt_BoltPoints[ iBoltIndex ].vBoltedContainers.size();
			}
		}
	}
	else
	{
		assert(0);
		ErrorBox(sERROR_CONTAINER_NOT_FOUND);
	}

	return 0;
}


bool Model_LoadBoltOn(LPCSTR psFullPathedFilename, ModelHandle_t hModel, LPCSTR psBoltName, bool bBoltIsBone, bool bBoltReplacesAllExisting)
{
	int iBoltIndex = Model_GetBoltIndex(hModel, psBoltName, bBoltIsBone);
	
	if (iBoltIndex == -1)
		return false;

	return Model_LoadBoltOn(psFullPathedFilename, hModel, iBoltIndex, bBoltIsBone, bBoltReplacesAllExisting);
}

static bool _Actual_Model_LoadBoltOn(LPCSTR psFullPathedFilename, ModelHandle_t hModel, int iBoltIndex, bool bBoltIsBone, bool bBoltReplacesAllExisting)
{
	if (!hModel)
	{
		ErrorBox("Attempting to bolt to NULL model!");
		return false;
	}

	// soft-error here for this, that doesn't zap anything important...
	//
	if (!FileExists( psFullPathedFilename ))
	{
		ErrorBox(va("File not found: \"%s\"!",psFullPathedFilename));
		return false;
	}

	// update: zap anything already there!...
	//
	if (bBoltReplacesAllExisting)
	{
		Model_DeleteBoltOn(hModel, iBoltIndex, bBoltIsBone, -1);
	}

	// ok, any errors from here on in will delete the primary model if they occur...
	//
	bool bReturn = false;

	ModelContainer_t *pContainer = ModelContainer_FindFromModelHandle( hModel );
	if (pContainer)
	{	
		ModelContainer_EnsureBoltOnHeader(pContainer);

		// some rather unfortunate return-logic here for soft-errors, but...
		//
		if (iBoltIndex < (bBoltIsBone?pContainer->iBoneBolt_MaxBoltPoints:pContainer->iSurfaceBolt_MaxBoltPoints) && iBoltIndex >= 0)
		{
			CString strLocalFilename( psFullPathedFilename );
			Filename_RemoveQUAKEBASE(strLocalFilename);

			ModelContainer_t *pContainer_BoltOn = ModelContainer_AllocNew();
			
			// do this further down now...
			// ModelHandle_t hModel = ModelContainer_RegisterModel(strLocalFilename, pContainer_BoltOn, pContainer->hTreeItem_BoltOns);

			// note that we do this code to link everything in whether or not we succeeded in registering the model
			//	so that the Model_Delete code catches everything, ie we don't have any orphaned pointers
			if (bBoltIsBone)
			{
				pContainer->tBoneBolt_BoltPoints[iBoltIndex].vBoltedContainers.push_back(*pContainer_BoltOn);
			}
			else
			{
				pContainer->tSurfaceBolt_BoltPoints[iBoltIndex].vBoltedContainers.push_back(*pContainer_BoltOn);
			}

			vector <ModelContainer_t> &vBoltedContainers =	bBoltIsBone ?
															pContainer->tBoneBolt_BoltPoints	[ iBoltIndex ].vBoltedContainers
															:
															pContainer->tSurfaceBolt_BoltPoints	[ iBoltIndex ].vBoltedContainers;

			ModelContainer_t *pNewContainerLocation =	&vBoltedContainers[ vBoltedContainers.size() -1 ];
			ModelHandle_t hModel = ModelContainer_RegisterModel(strLocalFilename, pNewContainerLocation, pContainer->hTreeItem_BoltOns);

			if (bBoltIsBone)
			{
				pNewContainerLocation->pBoneBolt_ParentContainer = pContainer;
				pNewContainerLocation->iBoneBolt_ParentBoltIndex = iBoltIndex;				
			}
			else
			{
				pNewContainerLocation->pSurfaceBolt_ParentContainer	= pContainer;
				pNewContainerLocation->iSurfaceBolt_ParentBoltIndex	= iBoltIndex;
			}
			
			delete (pContainer_BoltOn);

			if (hModel)
			{
				AppVars.iTotalContainers++;
				AppVars.hModelLastLoaded = hModel;
				bReturn = true;
			}
		}
		else
		{
			ErrorBox(va("Illegal %s bolt index %d specified, max = %d",(bBoltIsBone?"bone":"surface"),iBoltIndex, bBoltIsBone?pContainer->iBoneBolt_MaxBoltPoints:pContainer->iSurfaceBolt_MaxBoltPoints));
		}
	}
	else
	{
		ErrorBox(sERROR_CONTAINER_NOT_FOUND);
	}

	// problems?...
	//
	if (!bReturn)
	{
		Model_Delete();
	}

	ModelList_ForceRedraw();	// update window, needed for when called by external client progs

	return bReturn;
}

bool Model_LoadBoltOn(LPCSTR psFullPathedFilename, ModelHandle_t hModel, int iBoltIndex, bool bBoltIsBone, bool bBoltReplacesAllExisting)
{
	gbRenderInhibit = true;
	bool b = _Actual_Model_LoadBoltOn(psFullPathedFilename, hModel, iBoltIndex, bBoltIsBone, bBoltReplacesAllExisting);
	gbRenderInhibit = false;

	ModelList_ForceRedraw();

	return b;
}

bool Model_SurfaceIsTag( ModelContainer_t *pContainer, int iSurfaceIndex)
{
	if (Model_Loaded(pContainer->hModel))
	{
		return GLMModel_SurfaceIsTag( pContainer->hModel, iSurfaceIndex );
	}

	ErrorBox( "Model_SurfaceIsTag(): No model loaded" );
	return false;
}

bool Model_SurfaceIsTag( ModelHandle_t hModel, int iSurfaceIndex)
{
	ModelContainer_t *pContainer = ModelContainer_FindFromModelHandle(hModel);
	
	if (pContainer)
	{
		return Model_SurfaceIsTag( pContainer, iSurfaceIndex );
	}

	return false;
}

LPCSTR Model_GLMSurfaceInfo( ModelHandle_t hModel, int iSurfaceIndex, bool bShortVersionForTag )
{
	if (Model_Loaded(hModel))
	{
		return GLMModel_SurfaceInfo( hModel, iSurfaceIndex, bShortVersionForTag );
	}

	return sERROR_MODEL_NOT_LOADED;
}

// generate info suitable for sending to Notepad (can be a BIG string)...
//
LPCSTR Model_GLMSurfaceVertInfo( ModelHandle_t hModel, int iSurfaceIndex )
{
	if (Model_Loaded(hModel))
	{
		return GLMModel_SurfaceVertInfo( hModel, iSurfaceIndex );
	}

	return sERROR_MODEL_NOT_LOADED;
}

bool Model_SurfaceContainsBoneReference(ModelHandle_t hModel, int iLODNumber, int iSurfaceNumber, int iBoneNumber)
{
	// fixme: at some point this'll need to be non-GLM specific, but for now...
	//
	if (Model_Loaded(hModel))
	{
		return GLMModel_SurfaceContainsBoneReference(hModel, iLODNumber, iSurfaceNumber, iBoneNumber);
	}

	ErrorBox( "Model_SurfaceContainsBoneReference(): No model loaded" );
	return false;
}

LPCSTR	Model_GLMBoneInfo( ModelHandle_t hModel, int iBoneIndex )
{
	if (Model_Loaded(hModel))
	{
		return GLMModel_BoneInfo( hModel, iBoneIndex );
	}

	return sERROR_MODEL_NOT_LOADED;
}


int Model_GetNumFrames( ModelHandle_t hModel )
{
	ModelContainer_t *pContainer = ModelContainer_FindFromModelHandle( hModel );
	if (pContainer)
	{	
		return pContainer->iNumFrames;
	}

	assert(0);
	ErrorBox(sERROR_CONTAINER_NOT_FOUND);
	return 0;
}


LPCSTR Model_Info( ModelHandle_t hModel )
{
	ModelContainer_t *pContainer = ModelContainer_FindFromModelHandle( hModel );
	if (pContainer)
	{	
		if (pContainer->pModelInfoFunction)
		{
			return pContainer->pModelInfoFunction(hModel);
		}

		assert(0);
		return "Model_Info(): No function defined!";
	}

	return sERROR_CONTAINER_NOT_FOUND;
}

LPCSTR Model_GetBoneName( ModelHandle_t hModel, int iBoneIndex )
{
	ModelContainer_t *pContainer = ModelContainer_FindFromModelHandle( hModel );
	if (pContainer)
	{	
		if (pContainer->pModelGetBoneNameFunction)
		{
			return pContainer->pModelGetBoneNameFunction(hModel, iBoneIndex);
		}

		assert(0);
		return "Model_GetBoneName(): No function defined!";
	}

	return sERROR_CONTAINER_NOT_FOUND;
}

LPCSTR Model_GetSurfaceName( ModelContainer_t *pContainer, int iSurfaceIndex )
{
	if (pContainer->pModelGetSurfaceNameFunction)
	{
		return pContainer->pModelGetSurfaceNameFunction(pContainer->hModel, iSurfaceIndex);
	}

	ErrorBox("Model_GetSurfaceName(): No function defined!");
	exit(1);
	return NULL;
}

LPCSTR Model_GetSurfaceName( ModelHandle_t hModel, int iSurfaceIndex )
{
	ModelContainer_t *pContainer = ModelContainer_FindFromModelHandle( hModel );
	if (pContainer)
	{	
		return Model_GetSurfaceName( pContainer, iSurfaceIndex);
	}

	return sERROR_CONTAINER_NOT_FOUND;
}


// this function isn't terribly fast, so try not to call it somewhere speed dependant...
//
// so far only used for test code, but may be useful to keep around...
//
// returns -1 for error, so check it!!
//
int Model_GetBoltIndex( ModelHandle_t hModel, LPCSTR psBoltName, bool bBoltIsBone)
{
	ModelContainer_t *pContainer = ModelContainer_FindFromModelHandle( hModel );
	if (pContainer)
	{	
		return Model_GetBoltIndex( pContainer, psBoltName, bBoltIsBone );
	}

	ErrorBox( "Model_GetBoltIndex(): " sERROR_CONTAINER_NOT_FOUND );
	return -1;
}

// this function isn't terribly fast, so try not to call it somewhere speed dependant...
//
// so far only used for test code, but may be useful to keep around...
//
// returns -1 for error, so check it!!
//
int Model_GetBoltIndex( ModelContainer_t *pContainer, LPCSTR psBoltName, bool bBoltIsBone )	// semi-recursive now
{
	if (pContainer->pModelGetBoneBoltNameFunction && pContainer->pModelGetSurfaceBoltNameFunction)
	{
		// check against bolt names...
		//
		for (int i=0; i<(bBoltIsBone?pContainer->iBoneBolt_MaxBoltPoints:pContainer->iSurfaceBolt_MaxBoltPoints); i++)
		{
			if (!stricmp(Model_GetBoltName( pContainer, i, bBoltIsBone), psBoltName ))
				return i;
		}
	
		if (bBoltIsBone)
		{
			// check aliases (uses recursion)
			//
			bool bAlreadyHere = false;
			if (!bAlreadyHere)
			{
				for (MappedString_t::iterator it = pContainer->Aliases.begin(); it != pContainer->Aliases.end(); ++it)
				{
					string strAliasName = (*it).second;

					if (!stricmp(strAliasName.c_str(), psBoltName))
					{
						string strRealName = (*it).first;
						bAlreadyHere = true;
						int iAnswer = Model_GetBoltIndex( pContainer, strRealName.c_str(), bBoltIsBone );
						bAlreadyHere = false;
						return iAnswer;
					}
				}
			}
		}

		ErrorBox(va("Model_GetBoltIndex(): Unable to find bolt called \"%s\"!",psBoltName));
		return -1;
	}

	ErrorBox( "Model_GetBoltIndex(): need either a ->pModelGetBoneBoltNameFunction or ->pModelGetSurfaceBoltNameFunction!");
	return -1;
}


LPCSTR Model_GetBoltName( ModelHandle_t hModel, int iBoltIndex, bool bBoltIsBone )
{
	ModelContainer_t *pContainer = ModelContainer_FindFromModelHandle( hModel );
	if (pContainer)
	{	
		return Model_GetBoltName( pContainer, iBoltIndex, bBoltIsBone );
	}

	return sERROR_CONTAINER_NOT_FOUND;
}


LPCSTR Model_GetBoltName( ModelContainer_t *pContainer, int iBoltIndex, bool bBoltIsBone )
{
	if (bBoltIsBone)
	{
		if (pContainer->pModelGetBoneBoltNameFunction)
		{
			return pContainer->pModelGetBoneBoltNameFunction(pContainer->hModel, iBoltIndex);
		}
	}
	else
	{
		if (pContainer->pModelGetSurfaceBoltNameFunction)
		{
			return pContainer->pModelGetSurfaceBoltNameFunction(pContainer->hModel, iBoltIndex);
		}
	}

	assert(0);
	return "Model_GetBoltName(): No <bolttype>GetName() function defined!";
}


LPCSTR Model_GetFilename( ModelHandle_t hModel )
{
	ModelContainer_t *pContainer = ModelContainer_FindFromModelHandle( hModel );
	if (pContainer)
	{	
		return pContainer->sLocalPathName;
	}

	return sERROR_CONTAINER_NOT_FOUND;
}


// this returns the full disk path of the primary model... (mainly only used for new-file browsing)
//
LPCSTR Model_GetFullPrimaryFilename( void )
{
	if (Model_Loaded())
	{
		return AppVars.strLoadedModelPath;
	}
	
	// only usually gets here on first File-Open if not launched via file-association...
	//
	return "";
}


bool Model_GLMSurface_Off( ModelHandle_t hModel, int iSurfaceIndex )
{
	if (Model_Loaded(hModel))
	{			
		return GLMModel_Surface_Off(hModel, iSurfaceIndex );
	}

	assert(0);
	return false;
}

bool Model_GLMSurface_SetStatus( ModelHandle_t hModel, int iSurfaceIndex, SurfaceOnOff_t eStatus )
{
	if (Model_Loaded(hModel))
	{			
		return GLMModel_Surface_SetStatus(hModel, iSurfaceIndex, eStatus );
	}

	assert(0);
	return false;
}

bool Model_GLMSurface_SetStatus( ModelHandle_t hModel, LPCSTR psSurfaceName, SurfaceOnOff_t eStatus )
{
	if (Model_Loaded(hModel))
	{
		ModelContainer_t *pContainer = ModelContainer_FindFromModelHandle( hModel );
		if (pContainer)
		{	
			// convert surface name to index...
			//
			for (int iSurface=0; iSurface<pContainer->iNumSurfaces; iSurface++)
			{
				if (!stricmp(psSurfaceName, pContainer->pModelGetSurfaceNameFunction( pContainer->hModel, iSurface )))
				{
					return Model_GLMSurface_SetStatus( pContainer->hModel, iSurface, eStatus);
				}
			}
		}
		WarningBox(va("Model_GLMSurface_SetStatus():\nSurface \"%s\" not found!\n\n ( Model: \"%s\" )",psSurfaceName,pContainer->sLocalPathName));
		return false;
	}

	assert(0);
	return false;
}

void Model_GLMSurfaces_DefaultAll(ModelHandle_t hModel)
{
	if (Model_Loaded(hModel))
	{
		GLMModel_Surfaces_DefaultAll(hModel);
	}
}


bool Model_GLMSurface_On( ModelHandle_t hModel, int iSurfaceIndex )
{
	if (Model_Loaded(hModel))
	{
		return GLMModel_Surface_On(hModel, iSurfaceIndex );
	}

	assert(0);
	return false;
}

bool Model_GLMSurface_NoDescendants( ModelHandle_t hModel, int iSurfaceIndex )
{
	if (Model_Loaded(hModel))
	{
		return GLMModel_Surface_NoDescendants(hModel, iSurfaceIndex );
	}

	assert(0);
	return false;
}

SurfaceOnOff_t Model_GLMSurface_GetStatus( ModelHandle_t hModel, int iSurfaceIndex )		 
{
	if (Model_Loaded(hModel))
	{
		return GLMModel_Surface_GetStatus( hModel, iSurfaceIndex );
	}

	assert(0);
	return SURF_ERROR;
}


// subroutinised so there's common logic for the 2 functions below this (important!)...
//
static bool Model_SurfaceIsDifferentFromDefault(ModelContainer_t *pContainer, SurfaceOnOff_t eStatusToCheck, int iSurface)
{
	SurfaceOnOff_t eThisStatus = Model_GLMSurface_GetStatus( pContainer->hModel, iSurface );

	if (eThisStatus == SURF_INHERENTLYOFF)
	{
		extern SurfaceOnOff_t MyFlags;
		eThisStatus = MyFlags;	// get the real state, not the inherent one
	}		

	if (eThisStatus == eStatusToCheck)
	{
		// this surface's status matches the query, now is it naturally this way?
		//
		LPCSTR psSurfaceName = Model_GetSurfaceName( pContainer, iSurface );
			
		if (psSurfaceName)	// problems if we don't have this of course, but errormessaged already
		{
			bool bSurfaceNameIncludesOFF = !stricmp("_off", &psSurfaceName[strlen(psSurfaceName)-4]);

			switch (eThisStatus)
			{
				case SURF_ON:

					if (bSurfaceNameIncludesOFF)
						return true;
					break;

				case SURF_OFF:

					if (!bSurfaceNameIncludesOFF)
						return true;
					break;

				case SURF_NO_DESCENDANTS:

					// no surface will ever have this set initially, the loader can only set ON or OFF...
					//
					return true;					
			}
		}
	}

	return false;
}


// script-query function, asks how many surfaces are set to 'eStatus' by the user, in other words how many
//	are different from their default state...
//
int Model_GetNumSurfacesDifferentFromDefault(ModelContainer_t *pContainer, SurfaceOnOff_t eStatus)
{
	int iTotalMatchingSurfaces = 0;

	for (int iSurface=0; iSurface<pContainer->iNumSurfaces; iSurface++)
	{	
		if (Model_SurfaceIsDifferentFromDefault(pContainer, eStatus, iSurface))
			iTotalMatchingSurfaces++;		
	}

	return iTotalMatchingSurfaces;
}


LPCSTR Model_GetSurfaceDifferentFromDefault(ModelContainer_t *pContainer, SurfaceOnOff_t eStatus, int iSurfaceIndex)
{
	for (int iSurface=0; iSurface<pContainer->iNumSurfaces; iSurface++)
	{	
		if (Model_SurfaceIsDifferentFromDefault(pContainer, eStatus, iSurface))
		{
			if (!iSurfaceIndex--)
				return Model_GetSurfaceName( pContainer, iSurface );
		}
	}
	
	assert(0);
	return NULL;
}


void App_OnceOnly(void)
{
	AppVars_OnceOnlyInit();
	OnceOnlyCrap();	// init some rubbish that cut/paste code wants for other formats

	TextureList_OnceOnlyInit();	
}

ModelHandle_t Model_GetPrimaryHandle(void)
{
	return AppVars.Container.hModel;
}


static void ModelContainer_GenerateBBox(ModelContainer_t *pContainer, int iFrame, vec3_t &v3Mins, vec3_t &v3Maxs)
{
	v3Mins[0] = -50;
	v3Mins[1] = -50;
	v3Mins[2] = -50;

	v3Maxs[0] = 50;
	v3Maxs[1] = 50;
	v3Maxs[2] = 50;

	GLMModel_GetBounds(pContainer->hModel, 0, iFrame, v3Mins, v3Maxs);
}

// called from menu for a set-floor-height function...
//
float Model_GetLowestPointOnPrimaryModel(void)
{
	ModelContainer_t *pContainer = &AppVars.Container;	// primary container

	if (pContainer->hModel)	// model loaded?
	{			
		vec3_t v3Mins, v3Maxs;
		ModelContainer_GenerateBBox(pContainer, pContainer->iCurrentFrame_Primary, v3Mins, v3Maxs);

		return v3Mins[2];
	}

	assert(0);
	return 0.0f;
}

static void ModelDraw_Floor( ModelContainer_t *pContainer, bool bDrawingForOriginalContainer )
{
	if (bDrawingForOriginalContainer && !gbTextInhibit)
	{	
		if (AppVars.bFloor)
		{
			#define FLOOR_TILE_DIM		15.0f	// arb
			#define FLOOR_TILES_ACROSS	10		// ""

			glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_POLYGON_BIT);
			{
				glDisable(GL_TEXTURE_2D);
				glDisable(GL_LIGHTING);
				glDisable(GL_BLEND);
				glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

				for (int y=0; y<FLOOR_TILES_ACROSS; y++)
				{
					for (int x=0; x<FLOOR_TILES_ACROSS; x++)
					{
						float fX = (FLOOR_TILE_DIM*(float)x) - (((float)FLOOR_TILES_ACROSS/2.0f)*FLOOR_TILE_DIM);
						float fY = (FLOOR_TILE_DIM*(float)y) - (((float)FLOOR_TILES_ACROSS/2.0f)*FLOOR_TILE_DIM);
						float fZ = AppVars.fFloorZ;

						if ((y&1)^(x&1))
						{
							glColor3ub(21,81,32);	// a sort of dirty yellow/green check
						}							
						else
						{
							glColor3ub(125,131,18);	// ""
						}

						glBegin(GL_QUADS);
						{
							// frontface...
							//
							glVertex3f(fX,					fY,					fZ);
							glVertex3f(fX				,	fY+FLOOR_TILE_DIM,	fZ);
							glVertex3f(fX+FLOOR_TILE_DIM,	fY+FLOOR_TILE_DIM,	fZ);
							glVertex3f(fX+FLOOR_TILE_DIM,	fY,					fZ);							
							//
							// backface...
							//
							glVertex3f(fX,					fY,					fZ);
							glVertex3f(fX+FLOOR_TILE_DIM,	fY,					fZ);							
							glVertex3f(fX+FLOOR_TILE_DIM,	fY+FLOOR_TILE_DIM,	fZ);
							glVertex3f(fX				,	fY+FLOOR_TILE_DIM,	fZ);
						}
						glEnd();
					}
				}
			}
			glPopAttrib();
		}
	}
}

// this is kinda slow, but you only use it when you need to...
//
static void ModelDraw_BoundingBox( ModelContainer_t *pContainer, bool bDrawingForOriginalContainer )
{
	if (AppVars.bBBox && !gbTextInhibit)
	{
		glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
		{
			glDisable(GL_TEXTURE_2D);
			glDisable(GL_LIGHTING);
			glDisable(GL_BLEND);

			vec3_t v3Mins,v3Maxs;
			ModelContainer_GenerateBBox(pContainer,pContainer->iCurrentFrame_Primary,v3Mins,v3Maxs);

			vec3_t v3Corners[8];
					
			VectorSet(v3Corners[0],v3Mins[0],v3Mins[1],v3Mins[2]);
			VectorSet(v3Corners[1],v3Maxs[0],v3Mins[1],v3Mins[2]);
			VectorSet(v3Corners[2],v3Maxs[0],v3Mins[1],v3Maxs[2]);
			VectorSet(v3Corners[3],v3Mins[0],v3Mins[1],v3Maxs[2]);
			VectorSet(v3Corners[4],v3Mins[0],v3Maxs[1],v3Maxs[2]);
			VectorSet(v3Corners[5],v3Maxs[0],v3Maxs[1],v3Maxs[2]);
			VectorSet(v3Corners[6],v3Maxs[0],v3Maxs[1],v3Mins[2]);
			VectorSet(v3Corners[7],v3Mins[0],v3Maxs[1],v3Mins[2]);

			glColor3f(0.8,0.8,0.8);//1,1,1);
	/*				glBegin(GL_LINE_LOOP);
			{
				glVertex3fv(v3Corners[0]);
				glVertex3fv(v3Corners[1]);
				glVertex3fv(v3Corners[2]);
				glVertex3fv(v3Corners[3]);

				glVertex3fv(v3Corners[4]);
				glVertex3fv(v3Corners[5]);
				glVertex3fv(v3Corners[6]);
				glVertex3fv(v3Corners[7]);
			}
			glEnd();
	*/
			// new version, draw the above shape, but without 3 of the lines (and therefore without GL_LINE_LOOP)
			//	because otherwise the white lines fight with the coloured ones below...  (aesthetics)
			//
			glBegin(GL_LINES);
			{
				glVertex3fv(v3Corners[2]);
				glVertex3fv(v3Corners[3]);

				glVertex3fv(v3Corners[3]);
				glVertex3fv(v3Corners[4]);

				glVertex3fv(v3Corners[4]);
				glVertex3fv(v3Corners[5]);

				glVertex3fv(v3Corners[5]);
				glVertex3fv(v3Corners[6]);

				glVertex3fv(v3Corners[6]);
				glVertex3fv(v3Corners[7]);
			}
			glEnd();

			// draw the thicker lines for the 3 dimension axis...
			//
			glLineWidth(3);
			{
				// X...
				//
				glColor3f(1,0,0);
				glBegin(GL_LINES);
				{
					glVertex3fv(v3Corners[0]);
					glVertex3fv(v3Corners[1]);
				}
				glEnd();

				// Y...
				//
				glColor3f(0,1,0);
				glBegin(GL_LINES);
				{
					glVertex3fv(v3Corners[1]);
					glVertex3fv(v3Corners[2]);
				}
				glEnd();

				// Z...
				//
				glColor3f(0,0,1);
				glBegin(GL_LINES);
				{
					glVertex3fv(v3Corners[7]);
					glVertex3fv(v3Corners[0]);
				}
				glEnd();
			}
			glLineWidth(1);

			// now draw the coords...
			//
			for (int i=0; i<sizeof(v3Corners)/sizeof(v3Corners[0]); i++)
			{
				extern LPCSTR vtos(vec3_t v3);
				LPCSTR psCoordString = vtos(v3Corners[i]);
					
				Text_Display(psCoordString,v3Corners[i],200,70,0);//228/2,107/2,35/2);
			}

			// now draw the 3 dimensions (sizes)...
			//
			float fXDim = v3Maxs[0] - v3Mins[0];
			float fYDim = v3Maxs[1] - v3Mins[1];
			float fZDim = v3Maxs[2] - v3Mins[2];

			vec3_t v3Pos;		

			// X...
			//
			VectorAdd	(v3Corners[0],v3Corners[1],v3Pos);
			VectorScale	(v3Pos,0.5,v3Pos);
			Text_Display(va(" X = %.3f",fXDim),v3Pos,255,128,128);	// ruler-pink
			//
			// Y...
			//
			VectorAdd	(v3Corners[7],v3Corners[0],v3Pos);
			VectorScale	(v3Pos,0.5,v3Pos);
			Text_Display(va(" Y = %.3f",fYDim),v3Pos,255,128,128);
			//
			// Z...
			//
			VectorAdd	(v3Corners[1],v3Corners[2],v3Pos);
			VectorScale	(v3Pos,0.5,v3Pos);
			Text_Display(va(" Z = %.3f",fZDim),v3Pos,255,128,128);
		}
		glPopAttrib();
	}
}

static void ModelDraw_OriginLines(bool bDrawingForOriginalContainer)
{
	if (!gbTextInhibit)
	{
		if (AppVars.bOriginLines)
		{
			// draw origin lines...
			//
		#define ORIGIN_LINE_RADIUS 20
			static vec3_t v3X		= { ORIGIN_LINE_RADIUS,0,0};
			static vec3_t v3XNeg	= {-ORIGIN_LINE_RADIUS,0,0};

			static vec3_t v3Y		= {0, ORIGIN_LINE_RADIUS,0};
			static vec3_t v3YNeg	= {0,-ORIGIN_LINE_RADIUS,0};
			
			static vec3_t v3Z		= {0,0, ORIGIN_LINE_RADIUS};
			static vec3_t v3ZNeg	= {0,0,-ORIGIN_LINE_RADIUS};

			glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
			{
				glColor3f(0,0,1);
				glDisable(GL_TEXTURE_2D);
				glDisable(GL_LIGHTING);
				glDisable(GL_BLEND);
				glBegin(GL_LINES);
				{
					glVertex3fv(v3X);
					glVertex3fv(v3XNeg);

					glVertex3fv(v3Y);
					glVertex3fv(v3YNeg);

					glVertex3fv(v3Z);
					glVertex3fv(v3ZNeg);
				}
				glEnd();

		#define ORIGIN_TEXT_COLOUR 255,0,255
				Text_Display( "X",	v3X,	ORIGIN_TEXT_COLOUR);
				Text_Display("-X",	v3XNeg,	ORIGIN_TEXT_COLOUR);
				Text_Display( "Y",	v3Y,	ORIGIN_TEXT_COLOUR);
				Text_Display("-Y",	v3YNeg,	ORIGIN_TEXT_COLOUR);
				Text_Display( "Z",	v3Z,	ORIGIN_TEXT_COLOUR);
				Text_Display("-Z",	v3ZNeg,	ORIGIN_TEXT_COLOUR);
			}
			glPopAttrib();
		}
		

		if (AppVars.bRuler && bDrawingForOriginalContainer)
		{
			// draw a height graph thing...
			//
			glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
			{
				glColor3f(1,0.5,0.5);
				glDisable(GL_TEXTURE_2D);
				glDisable(GL_LIGHTING);
				glDisable(GL_BLEND);
				glBegin(GL_LINES);
				{
		#define iRULER_HEIGHT (ORIGIN_LINE_RADIUS*8)
					glVertex3f(-ORIGIN_LINE_RADIUS, 0,  iRULER_HEIGHT/2);
					glVertex3f(-ORIGIN_LINE_RADIUS, 0, -iRULER_HEIGHT/2);

					for (int i=0; i<iRULER_HEIGHT; i++)
					{
						glVertex3f(-ORIGIN_LINE_RADIUS,		0,  i-(iRULER_HEIGHT/2));
						glVertex3f( 0,						0,  i-(iRULER_HEIGHT/2));
					}
				}
				glEnd();

				for (int i=0;  i<iRULER_HEIGHT; i++)
				{
					vec3_t v3 = {-(ORIGIN_LINE_RADIUS+(ORIGIN_LINE_RADIUS/6)), 0,  i-(iRULER_HEIGHT/2)};
					Text_Display( va("%d __",i-(iRULER_HEIGHT/2)),	v3, 255,128,128);//ORIGIN_TEXT_COLOUR);										
				}
			}
			glPopAttrib();
		}
	}
}



static void BoneMatrix2GLMatrix(const mdxaBone_t *pBone, float *pGLMatrix)
{
	pGLMatrix[0] = pBone->matrix[0][0]; pGLMatrix[4] = pBone->matrix[0][1]; pGLMatrix[8] = pBone->matrix[0][2]; pGLMatrix[12] = pBone->matrix[0][3];
	pGLMatrix[1] = pBone->matrix[1][0]; pGLMatrix[5] = pBone->matrix[1][1]; pGLMatrix[9] = pBone->matrix[1][2]; pGLMatrix[13] = pBone->matrix[1][3];
	pGLMatrix[2] = pBone->matrix[2][0]; pGLMatrix[6] = pBone->matrix[2][1]; pGLMatrix[10]= pBone->matrix[2][2]; pGLMatrix[14] = pBone->matrix[2][3];
	pGLMatrix[3] = 0;					pGLMatrix[7] = 0;					pGLMatrix[11]= 0;					pGLMatrix[15] = 1;
}

typedef struct
{
	float matrix[16];
} GLMatrix_t;

typedef vector <GLMatrix_t>	PreRenderedMatrixPtrs_t;
							PreRenderedMatrixPtrs_t PreRenderedMatrixPtrs;

void PreRenderedMatrixPtrs_glMultiply(void)
{
	// reverse-iterate matrix chain to do all transforms...
	//
	for (int iMatrix = PreRenderedMatrixPtrs.size()-1; iMatrix>=0; iMatrix--)
	{
		GLMatrix_t *pMatrix = &PreRenderedMatrixPtrs[iMatrix];

		glMultMatrixf(pMatrix->matrix);
	}
}

// specialised use during render process only (assumes current valid GL modelview matrix)
//
static bool ModelContainer_ApplyRenderedMatrixToGL(ModelContainer_t *pContainer, int iBoltIndex, bool bBoltIsBone)
{
	bool bProceed = false;
	
	switch (pContainer->eModType)
	{
		case MOD_MDXM:

			if (bBoltIsBone)
			{
				bProceed = (pContainer->XFormedG2BonesValid[iBoltIndex] == true);

				if (bProceed)
				{
					// get lerped and xformed matrix from last render...
					//
					GLMatrix_t m;
					BoneMatrix2GLMatrix(&pContainer->XFormedG2Bones[iBoltIndex], &m.matrix[0]);

					// multiply by base pose matrix... (G2 specific)
					//
					GLMatrix_t m2;
					BoneMatrix2GLMatrix(GLMModel_GetBasePoseMatrix(pContainer->hModel, iBoltIndex), &m2.matrix[0]);

					//glMultMatrixf(m);
					//glMultMatrixf(m2);
					PreRenderedMatrixPtrs.push_back(m2);	// note reverse order because of push_back (reverse iteration later)
					PreRenderedMatrixPtrs.push_back(m);
				}
			}
			else
			{
				bProceed = (pContainer->XFormedG2TagSurfsValid[iBoltIndex] == true);
				//OutputDebugString(va("Checking tag surf %d: %s\n",iBoltIndex, bProceed?"VALID":"BAD"));

				if (bProceed)
				{
					// get lerped and xformed matrix from last render...
					//
					GLMatrix_t m;
					BoneMatrix2GLMatrix(&pContainer->XFormedG2TagSurfs[iBoltIndex], &m.matrix[0]);

					PreRenderedMatrixPtrs.push_back(m);					
				}				
			}
			break;

		default:
			assert(0);
			break;
	}

	return bProceed;
}

// this goes recursively up the bolting hierarchy and makes a list of matrices to multiply against, 
//	which we then iterate forwards through to do all the translations... (simpler code, but more cpu work)
//
static bool R_ModelContainer_ApplyGLParentMatrices(ModelContainer_t *pContainer)
{
	bool bProceed = true;

	if (pContainer->pBoneBolt_ParentContainer)
	{		
		bProceed = ModelContainer_ApplyRenderedMatrixToGL(pContainer->pBoneBolt_ParentContainer, pContainer->iBoneBolt_ParentBoltIndex, true);	// bBoltIsBone
		if (bProceed)
		{
			bProceed = R_ModelContainer_ApplyGLParentMatrices(pContainer->pBoneBolt_ParentContainer);
		}
	}
	else
	if (pContainer->pSurfaceBolt_ParentContainer)
	{
		bProceed = ModelContainer_ApplyRenderedMatrixToGL(pContainer->pSurfaceBolt_ParentContainer, pContainer->iSurfaceBolt_ParentBoltIndex, false);	// bBoltIsBone
		if (bProceed)
		{
			bProceed = R_ModelContainer_ApplyGLParentMatrices(pContainer->pSurfaceBolt_ParentContainer);
		}
	}

	return bProceed;
}

// sometimes I really hate this stuff, so I bury it away quietly...
//
static bool ModelContainer_HandleAllEvilMatrixCode(ModelContainer_t *pContainer)
{
	PreRenderedMatrixPtrs.clear();

	bool bProceed = R_ModelContainer_ApplyGLParentMatrices(pContainer);

	if (bProceed)
	{
		PreRenderedMatrixPtrs_glMultiply();
	}

	return bProceed;
}


ModelContainer_t *gpContainerBeingRendered = NULL;
static void ModelContainer_CallBack_AddToDrawList(ModelContainer_t* pContainer, void *pvData)
{
	glPushMatrix();
	{
		bool bProceed = ModelContainer_HandleAllEvilMatrixCode(pContainer);

		if (bProceed)
		{
			R_ModView_BeginEntityAdd();	// ##################

			// updateme?
			R_ModView_AddEntity(pContainer->hModel,		// ModelHandle_t hModel,
								pContainer->iCurrentFrame_Primary,// int iFrame,
								AppVars.bInterpolate?pContainer->iOldFrame_Primary:pContainer->iCurrentFrame_Primary,	// int iOldFrame,
								pContainer->iBoneNum_SecondaryStart,
								pContainer->iCurrentFrame_Secondary,// int iFrame,
								AppVars.bInterpolate?pContainer->iOldFrame_Secondary:pContainer->iCurrentFrame_Secondary,	// int iOldFrame,
								pContainer->iSurfaceNum_RootOverride,
								AppVars.fFramefrac,		// float fLerp
								pContainer->slist,
								pContainer->blist,
								pContainer->XFormedG2Bones,
								pContainer->XFormedG2BonesValid,
								pContainer->XFormedG2TagSurfs,
								pContainer->XFormedG2TagSurfsValid,
								//
								&pContainer->iRenderedTris,
								&pContainer->iRenderedVerts,
								&pContainer->iRenderedSurfs,
								&pContainer->iXformedG2Bones,
//								&pContainer->iRenderedBoneWeightsThisSurface,
								&pContainer->iRenderedBoneWeights,
								&pContainer->iOmittedBoneWeights
								);

			// render process will fill these in...
			//
			pContainer->iRenderedTris	=	0;
			pContainer->iRenderedVerts	=	0;
			pContainer->iRenderedSurfs	=	0;
			pContainer->iXformedG2Bones	=	0;
//			pContainer->iRenderedBoneWeightsThisSurface = 0;
			pContainer->iRenderedBoneWeights = 0;
			pContainer->iOmittedBoneWeights = 0;

			//
			// and will overwrite some/most of these, but to default them...
			//
			ZEROMEM(pContainer->XFormedG2Bones);
			ZEROMEM(pContainer->XFormedG2BonesValid);
			ZEROMEM(pContainer->XFormedG2TagSurfs);
			ZEROMEM(pContainer->XFormedG2TagSurfsValid);

			//############
			glColor3f(1,1,1);

			gpContainerBeingRendered = pContainer;	// aaaaarrrgghhh!!! hack city!!!!
			{
				RE_GenerateDrawSurfs();
				RE_RenderDrawSurfs();
			}
			gpContainerBeingRendered = NULL;				
			
			ModelDraw_BoundingBox( pContainer, !!(pContainer == &AppVars.Container) );
			ModelDraw_Floor		 ( pContainer, !!(pContainer == &AppVars.Container) );
			ModelDraw_OriginLines( !!(pContainer == &AppVars.Container) );
			
			//#####R_ModelContainer_Apply(&AppVars.Container, R_ModelContainer_CallBack_InfoText, &TextData);
			R_ModelContainer_CallBack_InfoText(pContainer, &TextData);
		}
	}
	glPopMatrix();
}

static void ModelList_AddModelsToDrawList(void)
{
	ModelDraw_InfoText_Header();

//######### R_ModView_BeginEntityAdd();

	// add all models to draw list... (now draws them as well, and prints stats)
	//
	R_ModelContainer_Apply(&AppVars.Container, ModelContainer_CallBack_AddToDrawList);

	ModelDraw_InfoText_Totals();
}



// used by both bones and tag surfaces.
//
// bHilitAsPure is true if this is a direct highlight, else false when called as a referenced bone during surface 
//	highlighting. This just affects outputs size and brightness, to make things easier to see.

//
static void DrawTagOrigin(bool bHilitAsPure, LPCSTR psTagText /* can be NULL */)
{
 	// draw origin lines...
	//						
	#define TAGHIGHLIGHT_LINE_LEN		(bHilitAsPure ? 20 : 10)	// spot the retro fit... ;-)
	#define TAGHIGHLIGHT_TEXT_COLOUR	 bHilitAsPure?255:150, 0,bHilitAsPure?255:150	// ""
	#define TAGHIGHLIGHT_TEXT_DISTANCE (bHilitAsPure ? 20 : 10)	// spot the retro fit... ;-)

	/*static */vec3_t v3X		= { TAGHIGHLIGHT_LINE_LEN,0,0};
	/*static */vec3_t v3XNeg	= {-TAGHIGHLIGHT_LINE_LEN,0,0};

	/*static */vec3_t v3Y		= {0, TAGHIGHLIGHT_LINE_LEN,0};
	/*static */vec3_t v3YNeg	= {0,-TAGHIGHLIGHT_LINE_LEN,0};
	
	/*static */vec3_t v3Z		= {0,0, TAGHIGHLIGHT_LINE_LEN};
	/*static */vec3_t v3ZNeg	= {0,0,-TAGHIGHLIGHT_LINE_LEN};

	glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
	{
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_LIGHTING);

		if (AppVars.bShowOriginsAsRGB)
		{
			glLineWidth(bHilitAsPure?4:2);
			{
				// X (red)...
				//
				glColor3f( (bHilitAsPure?1.0f:0.8f), 0, 0);
				glBegin(GL_LINES);
				{
					glVertex3fv(v3X);
					glVertex3f (0,0,0);
				}
				glEnd();

				// Y (green)...
				//
				glColor3f( 0, (bHilitAsPure?1.0f:0.8f), 0);
				glBegin(GL_LINES);
				{
					glVertex3fv(v3Y);
					glVertex3f (0,0,0);
				}
				glEnd();

				// Z (blue)...
				//
				glColor3f( 0, 0, (bHilitAsPure?1.0f:0.8f));
				glBegin(GL_LINES);
				{
					glVertex3fv(v3Z);
					glVertex3f (0,0,0);
				}
				glEnd();
			}
			glLineWidth(1);
		}
		else
		{
			glColor3f(0,bHilitAsPure?1:0.8,0);
			glBegin(GL_LINES);
			{
				glVertex3fv(v3X);
				glVertex3fv(v3XNeg);

				glVertex3fv(v3Y);
				glVertex3fv(v3YNeg);

				glVertex3fv(v3Z);
				glVertex3fv(v3ZNeg);
			}
			glEnd();
	
			Text_Display( "X",	v3X,	TAGHIGHLIGHT_TEXT_COLOUR);
			Text_Display("-X",	v3XNeg,	TAGHIGHLIGHT_TEXT_COLOUR);
			Text_Display( "Y",	v3Y,	TAGHIGHLIGHT_TEXT_COLOUR);
			Text_Display("-Y",	v3YNeg,	TAGHIGHLIGHT_TEXT_COLOUR);
			Text_Display( "Z",	v3Z,	TAGHIGHLIGHT_TEXT_COLOUR);
			Text_Display("-Z",	v3ZNeg,	TAGHIGHLIGHT_TEXT_COLOUR);
		}

		if (psTagText)
		{
			glColor3f(0.5,0.5,0.5);
			
			/*static */vec3_t v3NamePos = {TAGHIGHLIGHT_TEXT_DISTANCE,TAGHIGHLIGHT_TEXT_DISTANCE,TAGHIGHLIGHT_TEXT_DISTANCE};

			glLineStipple( 8, 0xAAAA);
			glEnable(GL_LINE_STIPPLE);

			glBegin(GL_LINES);
			{
				glVertex3f(0,0,0);
				glVertex3fv(v3NamePos);
			}
			glEnd();

			Text_Display( psTagText, v3NamePos, TAGHIGHLIGHT_TEXT_COLOUR );
		}
	}
	glPopAttrib();
}



static void ModelContainer_DrawTagSurfaceHighlights(ModelContainer_t *pContainer)
{	
	if (	!gbTextInhibit && 
			(AppVars.bSurfaceHighlight && pContainer->iSurfaceHighlightNumber != iITEMHIGHLIGHT_NONE)
		)
	{
		for (int iSurfaceIndex=0; iSurfaceIndex < pContainer->iNumSurfaces; iSurfaceIndex++)
		{
			bool bHighLit = (	pContainer->iSurfaceHighlightNumber == iSurfaceIndex		||
								pContainer->iSurfaceHighlightNumber == iITEMHIGHLIGHT_ALL_TAGSURFACES
								);

			if (bHighLit && Model_SurfaceIsTag( pContainer, iSurfaceIndex))
			{
				// this may get called twice, so pre-eval it here for speed...
				//
				LPCSTR psSurfaceName = Model_GetSurfaceName( pContainer->hModel, iSurfaceIndex );

				glPushMatrix();
				{
					PreRenderedMatrixPtrs.clear();

					bool bProceed = ModelContainer_ApplyRenderedMatrixToGL(pContainer, iSurfaceIndex, false);	// bBoneIsBolt
					
					if (bProceed)
					{
						// bone wants to be highlighted, and isn't disabled by virtue of disabled parent surface...
						//
						PreRenderedMatrixPtrs_glMultiply();

						// note special logic for first bool, in other words, if explicitly highlighting this surface, then draw 
						//	as normal, else if highlighting all tags, then just do as smaller/dimmer...						
						//
						DrawTagOrigin( !(pContainer->iSurfaceHighlightNumber == iITEMHIGHLIGHT_ALL_TAGSURFACES), psSurfaceName);
					}
				}
				glPopMatrix();
			}
		}
	}
}


// draw origin lines and name by any bone that's highlighted...
//
static void ModelContainer_DrawBoneHighlights(ModelContainer_t *pContainer)
{
	bool bBoneAliasHighlightingActive = (AppVars.bSurfaceHighlight && AppVars.bSurfaceHighlightShowsBoneWeighting && pContainer->iSurfaceHighlightNumber >=0 );	// -ve numbers are NONE, ALL or ALIASED, none of which we want

	if (	!gbTextInhibit && 

			(
				(AppVars.bBoneHighlight && pContainer->iBoneHighlightNumber != iITEMHIGHLIGHT_NONE)
				||
				bBoneAliasHighlightingActive				
			)
		)
	{
		for (int iBoneIndex=0; iBoneIndex < pContainer->iNumBones; iBoneIndex++)
		{
			// this may get called twice, so pre-eval it here for speed...
			//
			LPCSTR psBoneName = Model_GetBoneName( pContainer->hModel, iBoneIndex );

			bool bHilitAsPureBone = (	pContainer->iBoneHighlightNumber == iITEMHIGHLIGHT_ALL	
										||
										pContainer->iBoneHighlightNumber == iBoneIndex			
										||
										(pContainer->iBoneHighlightNumber == iITEMHIGHLIGHT_ALIASED && pContainer->Aliases.find(psBoneName)!=pContainer->Aliases.end())
									);

			bool bHilitAsBoneWeight = (bBoneAliasHighlightingActive && 
										Model_SurfaceContainsBoneReference(pContainer->hModel, AppVars.iLOD, pContainer->iSurfaceHighlightNumber, iBoneIndex)
										);

			if ( bHilitAsPureBone || bHilitAsBoneWeight )
			{
				glPushMatrix();
				{
					PreRenderedMatrixPtrs.clear();

					bool bProceed = ModelContainer_ApplyRenderedMatrixToGL(pContainer, iBoneIndex, true);	// bBoneIsBolt
					
					if (bProceed)
					{
						// bone wants to be highlighted, and isn't disabled by virtue of disabled parent surface...
						//
						PreRenderedMatrixPtrs_glMultiply();

						// note special logic for first bool, in other words, if explicitly highlighting this bone, then draw as normal, 
						//	else if highlighting as either part of a boneref, or one of ALL then draw smaller/dimmer...
						//
						DrawTagOrigin( (pContainer->iBoneHighlightNumber < 0 && pContainer->iBoneHighlightNumber != iITEMHIGHLIGHT_NONE)?false:bHilitAsPureBone,
														va("%s%s%s",
															psBoneName,
															(!bHilitAsPureBone || pContainer->iBoneHighlightNumber != iITEMHIGHLIGHT_ALIASED)?"":va("  (Alias: %s)",pContainer->Aliases[psBoneName].c_str()),
															bHilitAsBoneWeight?va(" ( %d )",iBoneIndex):""
															)
										);
					}
				}
				glPopMatrix();
			}
		}
	}
}




static Sequence_t *Stats_GetSequenceDisplayInfo(ModelContainer_t *pContainer, bool bPrimary, byte *pR = NULL, byte *pG = NULL, byte *pB = NULL, bool *pLocked = NULL);
static Sequence_t *Stats_GetSequenceDisplayInfo(ModelContainer_t *pContainer, bool bPrimary, byte *pR, byte *pG, byte *pB, bool *pLocked)
{
	// return either the locked local frame number in red (if anim locking is on), or just the anim sequence name
	//	if there's one corresponding to this...
	//	
	Sequence_t* pLockedSequence = NULL;

	if (Model_MultiSeq_IsActive(pContainer, bPrimary))
	{
		int iSequenceNumber = Model_MultiSeq_SeqIndexFromFrame(pContainer, bPrimary ? pContainer->iCurrentFrame_Primary : pContainer->iCurrentFrame_Secondary, bPrimary, false );
		pLockedSequence		= (iSequenceNumber == -1) ? NULL : &pContainer->SequenceList[iSequenceNumber];
	}
	else
	{
		int iSequenceNumber = bPrimary ? pContainer->iSequenceLockNumber_Primary : pContainer->iSequenceLockNumber_Secondary;
		pLockedSequence		= (iSequenceNumber == -1) ? NULL : &pContainer->SequenceList[iSequenceNumber];
	}

	if (pLockedSequence)
	{
		if ( pLocked)
			*pLocked = true;
		if ( pR)
			*pR = 255;
		if ( pG)
			*pG = 0;
		if ( pB)
			*pB = 0;
	}
	else
	{
		// no locked sequence, so print as normal and try and derive the one we just happen to be within...
		//
		if ( pLocked)
			*pLocked = false;
		if ( pR)
			*pR = 0;	// dim(ish) cyan
		if ( pG)
			*pG = 200;	// ...
		if ( pB)
			*pB = 200;	// ...

		pLockedSequence = Sequence_DeriveFromFrame( bPrimary ? pContainer->iCurrentFrame_Primary : pContainer->iCurrentFrame_Secondary, pContainer );
	}

	return pLockedSequence;
}



//#define ARB_VERTINFO_PADDING	30	//15	// 20
#define ARB_LOD_PADDING			9


static LPCSTR Stats_GetVertInfo(int iVerts, int iTris, int iSurfs, int iXFormedG2Bones, int iRenderedBoneWeights, int iOmittedBoneWeights)
{
	return va("(V:%4d T:%4d S:%2d%s)",
					iVerts,
						iTris, 
							iSurfs, 
							     (iXFormedG2Bones!=0)
									?
									va(" XF:%3d WT:%4d%s",iXFormedG2Bones,iRenderedBoneWeights,AppVars.bBoneWeightThreshholdingActive?va("+%d",iOmittedBoneWeights):"")
									:
									""
				);
}

static LPCSTR Stats_GetVertInfo(ModelContainer_t *pContainer)
{
	return Stats_GetVertInfo(	pContainer->iRenderedVerts,	// int iVerts, 
								pContainer->iRenderedTris,	// int iTris,
								pContainer->iRenderedSurfs, // int iSurfs,
								(pContainer->eModType == MOD_MDXM)?pContainer->iXformedG2Bones:0, // int iXFormedG2Bones
								(pContainer->eModType == MOD_MDXM)?pContainer->iRenderedBoneWeights:0, //int iRenderedBoneWeights,
								(pContainer->eModType == MOD_MDXM)?pContainer->iOmittedBoneWeights:0 //int iOmittedBoneWeights,
								);
}

static LPCSTR Stats_GetName(ModelContainer_t *pContainer)
{
	return va("%s",Filename_WithoutExt(Filename_WithoutPath(pContainer->sLocalPathName)));
}

static void R_ModelContainer_CallBack_MeasureDigits(ModelContainer_t *pContainer, void *pvData)
{
	TextData_t *pTextData = (TextData_t *) pvData;

	// heh... can you believe this?...
	//
	char sTest[1024];
	
	// frames readout...
	//
	sprintf(sTest,"%d",pContainer->iNumFrames);
	if (pTextData->iFrameDigitsNeeded < strlen(sTest))
		pTextData->iFrameDigitsNeeded = strlen(sTest);

	
	// attachment string...
	//
	sprintf(sTest,Stats_GetAttachmentString(pContainer));
	if (pTextData->iAttachedViaCharsNeeded < strlen(sTest))
		pTextData->iAttachedViaCharsNeeded = strlen(sTest);

	// "(Secondary Anim)" goes in same place as attachment string, but on line below (if present),
	//	so measure and store against same field...
	//
	if (AppVars.iTotalContainers>1)	// because I don't bother printing the header if there's only one model
	{
		sprintf(sTest,sSECONDARY_ANIM_STATS_STRING);
		if (pTextData->iAttachedViaCharsNeeded < strlen(sTest))
			pTextData->iAttachedViaCharsNeeded = strlen(sTest);
	}

	// sequence name string...
	//
	if (AppVars.iTotalContainers>1 || Model_SecondaryAnimLockingActive(pContainer))
	{
		// measure primary...
		//
		Sequence_t *pSequence = Stats_GetSequenceDisplayInfo( pContainer, true );
		int iLongestSeqString = pSequence?strlen(Sequence_GetName(pSequence,true)):0;
		if (pTextData->iSequenceNameCharsNeeded < iLongestSeqString)
			pTextData->iSequenceNameCharsNeeded = iLongestSeqString;	

		// measure secondary...
		//
		if (Model_SecondaryAnimLockingActive(pContainer))
		{
			pSequence = Stats_GetSequenceDisplayInfo( pContainer, false );
			iLongestSeqString = pSequence?strlen(Sequence_GetName(pSequence,true)):0;
			if (pTextData->iSequenceNameCharsNeeded < iLongestSeqString)
				pTextData->iSequenceNameCharsNeeded = iLongestSeqString;	
		}
	}

	int iLongestModelNameString = strlen(Stats_GetName(pContainer));
	if (pTextData->iModelNameCharsNeeded < iLongestModelNameString)
		pTextData->iModelNameCharsNeeded = iLongestModelNameString;

	int iLongestVertInfoString = strlen(Stats_GetVertInfo(pContainer));
	if (pTextData->iModelVertInfoCharsNeeded < iLongestVertInfoString)
		pTextData->iModelVertInfoCharsNeeded = iLongestVertInfoString;

	// measure number of lines needed for multi-lock sequences...
	//
	int iMultiLockSeqs_Primary	=	//Model_MultiSeq_IsActive(pContainer,true )?
									Model_MultiSeq_GetNumEntries(pContainer, true)
									//:0
									;
	int iMultiLockSeqs_Secondary=	//Model_MultiSeq_IsActive(pContainer,false)?
									Model_MultiSeq_GetNumEntries(pContainer, false)
									//:0
									;
	if (Model_SecondaryAnimLockingActive(pContainer))//iMultiLockSeqs_Secondary)
		pTextData->bAnyMultiLockedSecondarySequences = true;

	pTextData->iMostMultiLockedSequences = max(pTextData->iMostMultiLockedSequences,iMultiLockSeqs_Primary);
	pTextData->iMostMultiLockedSequences = max(pTextData->iMostMultiLockedSequences,iMultiLockSeqs_Secondary);
}

static void R_ModelContainer_CallBack_InfoText(ModelContainer_t *pContainer, void *pvData)
{
	TextData_t *pTextData = (TextData_t *) pvData;

	pTextData->iTextX = pTextData->iPrevX;

	// model name and basic frame info...
	//
	{
		// name...
		//
		pTextData->iTextX = Text_DisplayFlat(	String_EnsureMinLength( Stats_GetName(pContainer), pTextData->iModelNameCharsNeeded),
												pTextData->iTextX, pTextData->iTextY, 
												255,255,0		// RGB (yellow)
												);

		// frame info...
		//
		pTextData->iTextX = Text_DisplayFlat(	va("Frame: %*d/%*d", 												
													pTextData->iFrameDigitsNeeded,	pContainer->iCurrentFrame_Primary, 
													pTextData->iFrameDigitsNeeded,	pContainer->iNumFrames
													),
											pTextData->iTextX + (2*TEXT_WIDTH), pTextData->iTextY, 
											0, 255,0		// RGB (green)
											);
	}

	// LOD info...
	//
	{
		int iWhichLOD = (AppVars.iLOD < pContainer->iNumLODs) ? AppVars.iLOD : pContainer->iNumLODs-1;
		pTextData->iTextX = Text_DisplayFlat(	String_EnsureMinLength(va("(LOD %d/%d)",iWhichLOD,pContainer->iNumLODs),ARB_LOD_PADDING),
												pTextData->iTextX+(2*TEXT_WIDTH), pTextData->iTextY, 
												255/2,255,255/2,		// RGB
												false
											);
	}

	// vert / tri / surfaces info...
	//
	{
		pTextData->iTextXForVertStats = pTextData->iTextX+(2*TEXT_WIDTH);	// record for "totals" later
		pTextData->iTextX = 
		Text_DisplayFlat(	String_EnsureMinLength( Stats_GetVertInfo(pContainer), pTextData->iModelVertInfoCharsNeeded ),
							pTextData->iTextXForVertStats, pTextData->iTextY, 
							255, 255/2, 255/2,		// RGB (pink)
							false
						);

		pTextData->iTot_RenderedVerts	+= pContainer->iRenderedVerts;
		pTextData->iTot_RenderedTris	+= pContainer->iRenderedTris;
		pTextData->iTot_RenderedSurfs	+= pContainer->iRenderedSurfs;

		if (pContainer->eModType == MOD_MDXM)
		{
			pTextData->iTot_XformedG2Bones		+= pContainer->iXformedG2Bones;
			pTextData->iTot_RenderedBoneWeights += pContainer->iRenderedBoneWeights;
			pTextData->iTot_OmittedBoneWeights  += pContainer->iOmittedBoneWeights;
		}
	}


	// attached-via info... (bone bolt or surface bolt)
	//
	int iAttachedVia_X = pTextData->iTextX+(2*TEXT_WIDTH);	// also used for "secondary anim" printf later
	{
		pTextData->iTextX =
		Text_DisplayFlat(	String_EnsureMinLength( Stats_GetAttachmentString(pContainer), pTextData->iAttachedViaCharsNeeded ),	
							iAttachedVia_X, pTextData->iTextY, 
							0, 255/2,0,		// RGB
							false
						);
	}

	// sequence / anim info...
	//
	{
		bool bLocked = true;
		byte _R = 255;
		byte _G = 0;
		byte _B = 0;

		// primary...
		//
		Sequence_t *pSequence = Stats_GetSequenceDisplayInfo(pContainer, true, &_R, &_G, &_B, &bLocked);

		int iPrimarySeqTextX = pTextData->iTextX + (2*TEXT_WIDTH);		
		if (pSequence && !pSequence->bIsDefault)	// no point printing default sequence info
		{				
			// no space after "Seq: now because Sequence_GetName() prepends with either ' ' or '-'...
			//
			pTextData->iTextX = Text_DisplayFlat(  va("Seq:%s",String_EnsureMinLength(Sequence_GetName(pSequence,true),pTextData->iSequenceNameCharsNeeded)), iPrimarySeqTextX, pTextData->iTextY, _R,_G,_B);
			pTextData->iTextX = Text_DisplayFlat(	va("(%d/%d)",pContainer->iCurrentFrame_Primary - pSequence->iStartFrame, pSequence->iFrameCount),
										pTextData->iTextX+(1*TEXT_WIDTH), pTextData->iTextY,
										_R,
										_G,
										_B
										);
		}

		// secondary...
		//
		if (Model_SecondaryAnimLockingActive(pContainer))
		{
			pTextData->iTextY += TEXT_DEPTH;

			if (AppVars.iTotalContainers>1)	// looks neater if i don't bother printing this when only one model
			{
				// "(Secondary anim)"...
				//			
				Text_DisplayFlat(	String_EnsureMinLength( sSECONDARY_ANIM_STATS_STRING, pTextData->iAttachedViaCharsNeeded ),	
									iAttachedVia_X, pTextData->iTextY, 
									128,128,128,		// RGB
									false
								);
			}

			// sequence name...
			//
			pSequence = Stats_GetSequenceDisplayInfo(pContainer, false, &_R, &_G, &_B, &bLocked);

			if (pSequence && !pSequence->bIsDefault)	// no point printing default sequence info
			{
				// no space after "Seq: now because Sequence_GetName() prepends with either ' ' or '-'...
				//
				pTextData->iTextX = Text_DisplayFlat(  va("Seq:%s",String_EnsureMinLength(Sequence_GetName(pSequence,true),pTextData->iSequenceNameCharsNeeded)), iPrimarySeqTextX, pTextData->iTextY, _R,_G,_B);
				pTextData->iTextX = Text_DisplayFlat(	va("(%d/%d)",pContainer->iCurrentFrame_Secondary - pSequence->iStartFrame, pSequence->iFrameCount),
														pTextData->iTextX+(1*TEXT_WIDTH), pTextData->iTextY,
														_R,
														_G,
														_B
														);
			}
		}
	}

	// multi-lock sequence info...
	//
	{
		// (AppVars.iTotalContainers>1)	// looks neater if i don't bother printing this when only one model
		for (int iLockPass = 0; iLockPass<2; iLockPass++)	// 2 passes, primary and secondary lock
		{			
			if (//Model_MultiSeq_IsActive		( pContainer, !iLockPass )
				//&& 
				Model_MultiSeq_GetNumEntries( pContainer, !iLockPass )
				)
			{
				// note leading spaces in all text items to match leading-spaces on sequence names (to column-match
				//	for some that have leading '-' on them...
				//
				byte r,g,b;

//				OutputDebugString(va("most locked = %d\n",pTextData->iMostMultiLockedSequences));
				int iMultiLockedTextY = g_iScreenHeight - ((pTextData->iMostMultiLockedSequences
															+1													// <name>
															+(pTextData->bAnyMultiLockedSecondarySequences?1:0)	// <(primary)>
															+1													// (spacing between header and sequences)
															+3	// to move it up above other text already onscreen
															)*TEXT_DEPTH);
//				OutputDebugString(va("iMultiLockedTextY = %d\n",iMultiLockedTextY));
				int iFurthestX = 0;

				// <name>..
				r=255,g=255,b=0;		// RGB (yellow)
				if (!Model_MultiSeq_IsActive( pContainer, !iLockPass ))
				{
					r=128,g=128,b=0;	// dim yellow if inactive
				}
				int 
				iTempX = Text_DisplayFlat(	va(" %s",Filename_WithoutExt(Filename_WithoutPath(pContainer->sLocalPathName))),
													pTextData->iMultiLockedTextX, iMultiLockedTextY, 
													r,g,b
													);

				iFurthestX = max(iFurthestX,iTempX);
				iMultiLockedTextY += TEXT_DEPTH;
				
				// (primary) or (secondary)...
				//
				if (pTextData->bAnyMultiLockedSecondarySequences
					&&
					Model_SecondaryAnimLockingActive(pContainer)	// this probably makes the above check spurious, but wtf?
					)
				{
					iTempX = Text_DisplayFlat(	!iLockPass?" ( Primary )":" ( Secondary )",
												pTextData->iMultiLockedTextX, iMultiLockedTextY, 
												128,128,128		// grey
												);
					iFurthestX = max(iFurthestX,iTempX);
					iMultiLockedTextY += TEXT_DEPTH;
				}

				// (space)...
				//
				iMultiLockedTextY += TEXT_DEPTH;

				// now list the actual multilock sequences...
				//
				int iMultiLockedSequenceWereWithin = Model_MultiSeq_SeqIndexFromFrame(pContainer, !iLockPass?pContainer->iCurrentFrame_Primary:pContainer->iCurrentFrame_Secondary, !iLockPass, false );
				for (int iSeqIndex=0; iSeqIndex<Model_MultiSeq_GetNumEntries(pContainer, !iLockPass); iSeqIndex++)
				{
					int		iSeqEntry	= Model_MultiSeq_GetEntry(pContainer, iSeqIndex, !iLockPass);
					LPCSTR	psSeqName	= Model_Sequence_GetName (pContainer, iSeqEntry, true );
															
					r=0,g=200,b=200;	// default cyan

					if (!Model_MultiSeq_IsActive( pContainer, !iLockPass ))
					{
						r=100,g=100,b=100;	// greyed out
					}
					else
					{
						// locked?
						if (iSeqEntry == iMultiLockedSequenceWereWithin)
							r=255,g=0,b=0;	// red
					}

					iTempX = Text_DisplayFlat(	psSeqName,
												pTextData->iMultiLockedTextX, iMultiLockedTextY, 
												r,g,b
												);
					iFurthestX = max(iFurthestX,iTempX);
					iMultiLockedTextY += TEXT_DEPTH;
				}

				// move to next column for next model with multilocks...
				//
				pTextData->iMultiLockedTextX = iFurthestX + (2*TEXT_WIDTH);
			}
		}
	}

	pTextData->iTextY += TEXT_DEPTH;

	ModelContainer_DrawBoneHighlights(pContainer);
	ModelContainer_DrawTagSurfaceHighlights(pContainer);
}


static void ModelDraw_InfoText_Header(void)
{
	ZEROMEM(TextData);

	// display current picmip state...
	//
	sprintf(TextData.sString,"( PICMIP: %d )",TextureList_GetMip());
	Text_DisplayFlat(TextData.sString,	
						1*TEXT_WIDTH,
						1*TEXT_DEPTH,
						100,100,100,
						false
					);

	// display main model's skin file data if applicable (SOF2 models only, currently)...
	//
	ModelContainer_t *pContainer = &AppVars.Container;	// primary model only
	strcpy(TextData.sString,"");
	if (pContainer->SkinSets.size())
	{
		sprintf(TextData.sString,"Skin File: '%s'       Ethnic version: '%s'",
											pContainer->strCurrentSkinFile.c_str(),
																	pContainer->strCurrentSkinEthnic.c_str()
				);
	}
	if (pContainer->OldSkinSets.size())
	{
		sprintf(TextData.sString,"Skin File: '%s'",pContainer->strCurrentSkinFile.c_str());
	}
	if (strlen(TextData.sString))
	{
		Text_DisplayFlat(TextData.sString,
						1*TEXT_WIDTH,
						g_iScreenHeight - (2*TEXT_DEPTH),
						100,100,100,
						false
					);
	}


	// display current FPS and interp state...
	//
	sprintf(TextData.sString,"FPS: %2.2f %s",1/(AppVars.dAnimSpeed),AppVars.bAnimate?"(Playing)":"(Stopped)");

	int iFPS_Xpos = (g_iScreenWidth/2)-( (strlen(TextData.sString)/2)*TEXT_WIDTH);
		
	if (AppVars.bBoneWeightThreshholdingActive)
	{
		LPCSTR psThreshholdString = va("BoneWeightThresh: %g%%",AppVars.fBoneWeightThreshholdPercent);
		Text_DisplayFlat(psThreshholdString, iFPS_Xpos - ((strlen(psThreshholdString)+2)*TEXT_WIDTH),1*TEXT_DEPTH,
									180,180,180,false);
	}

	TextData.iTextX = 
	Text_DisplayFlat(TextData.sString, iFPS_Xpos, 1*TEXT_DEPTH,
						255,255,255,
						false
					);

	if (AppVars.bInterpolate)
	{
		TextData.iTextX = Text_DisplayFlat("(Interpolated)", TextData.iTextX+(2*TEXT_WIDTH),1*TEXT_DEPTH, 255/2,255/2,255/2,false);
	}

	TextData.iTextX = Text_DisplayFlat(va("(LOD: %d)",AppVars.iLOD+1), TextData.iTextX+(2*TEXT_WIDTH), 1*TEXT_DEPTH, 255/2,255,255/2,false);
/*		Text_DisplayFlat(sString,	g_iScreenWidth-((strlen(sString)+2)*TEXT_WIDTH),
																 2 *TEXT_DEPTH,
								255,255,255,
								false
					);*/

	TextData.iTextX = Text_DisplayFlat(va("( FOV: %g )",AppVars.dFOV), TextData.iTextX+(2*TEXT_WIDTH),1*TEXT_DEPTH, 255, 255, 255, false);

	if (AppVars.bUseAlpha)
	{
		Text_DisplayFlat("( Alpha )", TextData.iTextX+(2*TEXT_WIDTH),1*TEXT_DEPTH, 128, 128, 128, false);
	}


	// display verts/tris/surfaces info for all rendered models...
	//
	TextData.iTextY = 3*TEXT_DEPTH;
	TextData.iPrevX = 2*TEXT_WIDTH;
	TextData.iTot_RenderedVerts	= 0;
	TextData.iTot_RenderedTris	= 0;
	TextData.iTot_RenderedSurfs	= 0;
	TextData.iTot_XformedG2Bones= 0;
	TextData.iTot_RenderedBoneWeights = 0;
	TextData.iTot_OmittedBoneWeights = 0;

	// a tacky bit of code that looks nicer on output, basically I want to know how many digits I need for
	//	the frame printing to make all the loaded models line up nicely... :-)
	//
	TextData.iFrameDigitsNeeded			= 0;	
	TextData.iAttachedViaCharsNeeded	= 0;
	TextData.iSequenceNameCharsNeeded	= 0;
	TextData.iModelNameCharsNeeded		= 0;
	TextData.iModelVertInfoCharsNeeded	= 0;
	TextData.iMostMultiLockedSequences	= 0;
	TextData.iMultiLockedTextX			= 1*TEXT_WIDTH;	// was 2 until I added leading spaces to seq-name for +/- distinction

	R_ModelContainer_Apply(&AppVars.Container, R_ModelContainer_CallBack_MeasureDigits, &TextData);	
}

static void ModelDraw_InfoText_Totals(void)
{
	// print totals... (but only if > 1 model loaded, else no point)
	//
	if (AppVars.iTotalContainers>1)
	{
		TextData.iTextX = TextData.iPrevX;
		TextData.iTextY += TEXT_DEPTH;
		strcpy(TextData.sString,String_EnsureMinLength("( totals )",TextData.iModelNameCharsNeeded));
		TextData.iTextX = Text_DisplayFlat(TextData.sString, TextData.iTextX, TextData.iTextY, 255,255,0); // yellow		

		TextData.iTextX = 
		Text_DisplayFlat(	String_EnsureMinLength(
													Stats_GetVertInfo(	TextData.iTot_RenderedVerts,	// int iVerts
																		TextData.iTot_RenderedTris,		// int iTris
																		TextData.iTot_RenderedSurfs,	// int iSurfs
																		TextData.iTot_XformedG2Bones,	// int iXFormedG2Bones
																		TextData.iTot_RenderedBoneWeights,// int iRenderedBoneWeights
																		TextData.iTot_OmittedBoneWeights // int iOmittedBoneWeights
																	 ),
													TextData.iModelVertInfoCharsNeeded
													),
							TextData.iTextXForVertStats/*TextData.iTextX+(2*TEXT_WIDTH)*/, TextData.iTextY, 
							255, 255/2, 255/2,		// RGB (pink)
							false
						);

		TextData.iTextY += TEXT_DEPTH;
	}

	// kinda nice to know about this...
	//
	if (AppVars.bForceWhite)
	{
		TextData.iTextY += TEXT_DEPTH*2;
		TextData.iTextX  = TEXT_WIDTH*1;

		Text_DisplayFlat(	"( FORCEWHITE )",
							TextData.iTextX, TextData.iTextY, 
							255, 255, 255,		// RGB white
							false
						);
	}

	if (AppVars.bVertWeighting && AppVars.bAtleast1VertWeightDisplayed)
	{
//		TextData.iTextY += TEXT_DEPTH;
//		TextData.iTextY += TEXT_DEPTH;

		TextData.iTextY = g_iScreenHeight/2;
		TextData.iTextX = TextData.iPrevX;

		Text_DisplayFlat(	"# BoneWeights per vert:",
							TextData.iTextX, TextData.iTextY, 
							255, 255/2, 255/2,		// RGB (pink)
							false
						);

		TextData.iTextY += TEXT_DEPTH;

		byte r,g,b;

		for (int i= (AppVars.iSurfaceNumToHighlight == iITEMHIGHLIGHT_ALL)?3:1; i<6; i++)
		{				
			GetWeightColour(i,r,g,b);
			Text_DisplayFlat(	va("%d%s",i,i>=5?"+":""),
								TextData.iTextX, TextData.iTextY, 
								r,g,b,
								false
							);
			TextData.iTextY += TEXT_DEPTH;
		}

		AppVars.bAtleast1VertWeightDisplayed = false;
	}


/*
	// temp general test...
	//
	Text_DisplayFlat("top left",0,0,255,255,255);
	Text_DisplayFlat("top right",g_iScreenWidth - (TEXT_WIDTH*strlen("top right")) ,0,255,255,255);
	Text_DisplayFlat("bottom left",0,g_iScreenHeight - TEXT_DEPTH,255,255,255);
	Text_DisplayFlat("bottom right",g_iScreenWidth - (TEXT_WIDTH*strlen("bottom right")),g_iScreenHeight - TEXT_DEPTH,255,255,255);
*/
}

int giRenderCount=0;	// reset/checked during gallery loop
static void ModelList_Render_Actual(int iWindowWidth, int iWindowHeight)
{
	if (!gbRenderInhibit)
	{			
		giRenderCount++;
		bool bCatchError = false;

	//	giTotVertsDrawn = 0;	// stats
	//	giTotTrisDrawn  = 0;	//
	//	giTotSurfsDrawn = 0;

		GL_Enter3D( AppVars.dFOV, iWindowWidth, iWindowHeight, AppVars.bWireFrame, false);

		{// CLS, & setup GL stuff...
			glClearColor((float)1/((float)256/(float)AppVars._R), (float)1/((float)256/(float)AppVars._G), (float)1/((float)256/(float)AppVars._B), 0.0f);
			glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
			glLoadIdentity	(); 	
			glTranslatef	(	   AppVars.xPos, AppVars.yPos , AppVars.zPos );
			glScalef	    (     (float)0.05 , (float)0.05, (float)0.05 );
			glRotatef		( AppVars.rotAngleX, 1.0 ,  0.0 , 0.0 );
 			glRotatef		( AppVars.rotAngleY, 0.0 ,  1.0 , 0.0 );	
			glRotatef		( AppVars.rotAngleZ, 1.0 ,  0.0 , 0.0 );	

			extern bool gbScrollLockActive;
			if (gbScrollLockActive)
			{					
				glTranslatef	(	   AppVars.xPos_SCROLL, AppVars.yPos_SCROLL , AppVars.zPos_SCROLL );
//				glRotatef		( AppVars.rotAngleX_SCROLL, 1.0 ,  0.0 , 0.0 );
 //				glRotatef		( AppVars.rotAngleY_SCROLL, 0.0 ,  1.0 , 0.0 );	
//				glRotatef		( AppVars.rotAngleZ_SCROLL, 1.0 ,  0.0 , 0.0 );
			}


			if (AppVars.bUseAlpha && !AppVars.bWireFrame)
			{
				glEnable	(GL_BLEND);
				glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			}
			else
			{
				glDisable(GL_BLEND);
			}		
		}

		if (!Model_Loaded())
			return;

		
		try
		{
			ModelList_AddModelsToDrawList();
			// call the huge pile of cut-paste renderer code from elsewhere... (fingers crossed)
			//		
	//////		####RE_GenerateDrawSurfs();
	//////		####RE_RenderDrawSurfs();
	//		RB_GetDrawStats(&giTotVertsDrawn, &giTotTrisDrawn, &giTotSurfsDrawn);		
		}

		catch(LPCSTR psMessage)
		{
			Model_Delete();
			ErrorBox(psMessage);
			bCatchError = true;
		}

		// if all went well, draw view axis etc...
		//
		if (!bCatchError)
		{ 
			glDisable(GL_BLEND);

			// ( functions inhibit-checked internally )
			//
	//#####		ModelDraw_OriginLines();
	//#####		ModelDraw_InfoText();
		}

		//if (bFinalReturn)
		{
	//		static int z=0;
	//		z++;
	//		OutputDebugString(va("ModelList_Render: %d\n",z));
		}
	}
}

bool gbInRenderer = false;
void ModelList_Render(int iWindowWidth, int iWindowHeight)
{
	gbInRenderer = true;
		ModelList_Render_Actual(iWindowWidth, iWindowHeight);
	gbInRenderer = false;
}


/*
int Sys_Milliseconds (void)
{
	static bool bInitialised = false;
	static int sys_timeBase;

	int sys_curtime;

	if (!bInitialised)
	{
		sys_timeBase = timeGetTime();
		bInitialised = true;
	}
	sys_curtime = timeGetTime() - sys_timeBase;

	return sys_curtime;
}

float GetFloatTime(void)
{
	float fTime  = (float)Sys_Milliseconds() / 1000.0f;	// reduce to game-type seconds

	return fTime;
}
*/



// returns appropriate sequence index else -1 for not frame-not-found-in-sequences...
//
int Model_MultiSeq_SeqIndexFromFrame(ModelContainer_t *pContainer, int iFrame, bool bPrimary, bool bIsOldFrame )
{		
	if (bIsOldFrame)
	{
		for (int i=0; i<Model_MultiSeq_GetNumEntries(pContainer, bPrimary); i++)
		{
			int iSeqIndex = Model_MultiSeq_GetEntry(pContainer, i, bPrimary);
			Sequence_t *pSequence = &pContainer->SequenceList[iSeqIndex];
			if (Sequence_FrameIsWithin(pSequence, iFrame))
			{
				return iSeqIndex;	// this sequence is within our multiseq list, so ok to return it
			}
		}
	}
	else
	{
		if (Model_MultiSeq_EnsureSeqHintLegal(pContainer, iFrame, bPrimary))
		{
			return Model_MultiSeq_GetSeqHint(pContainer, bPrimary);
		}
	}
	
	// seq index isn't within our multiseq array, so...
	//
	return -1;
}

// similar to above, but returns the entrynum [ 0..<num entries-1> ] that container the seq index
int Model_MultiSeq_EntryIndexFromFrame(ModelContainer_t *pContainer, int iFrame, bool bPrimary )
{
	// Can't use this method, because sequences can have overlapping ranges unfortunately, 
	//	so Sequence_DeriveFromFrame() can return wrong indexes if there's more than one covering the same range.
/*	
	// find the real sequence index, then verify that it's within our multiseq array...
	//
	int iSeqIndex = Sequence_DeriveFromFrame( pContainer, iFrame );

	if (iSeqIndex != -1)
	{
		for (int i=0; i<Model_MultiSeq_GetNumEntries(pContainer, bPrimary); i++)
		{
			if ( iSeqIndex == Model_MultiSeq_GetEntry(pContainer, i, bPrimary))
			{
				return i;	// this sequence is within our multiseq list, so return list index
			}
		}
	}
*/
	int iSeqIndex = Model_MultiSeq_GetSeqHint(pContainer, bPrimary);

	for (int i=0; i<Model_MultiSeq_GetNumEntries(pContainer, bPrimary); i++)
	{
		int iThisSeqIndex = Model_MultiSeq_GetEntry(pContainer, i, bPrimary);
		Sequence_t *pSequence = &pContainer->SequenceList[iThisSeqIndex];
		if (iSeqIndex == iThisSeqIndex && Sequence_FrameIsWithin(pSequence, iFrame))
		{
			return i;	// this sequence is within our multiseq list, so ok to return its index within this list
		}
	}

	// seq index isn't within our multiseq array, so...
	//
	return -1;
}


static int Model_MultiSeq_GetSeqHint(ModelContainer_t *pContainer, bool bPrimary)
{		
	if (bPrimary)
	{
		return pContainer->iSeqMultiLock_Primary_SeqHint;
	}
	else
	{
		return pContainer->iSeqMultiLock_Secondary_SeqHint;
	}
}

static void Model_MultiSeq_SetSeqHint(ModelContainer_t *pContainer, bool bPrimary, int iHint)
{		
	int iPrev = Model_MultiSeq_GetSeqHint(pContainer, bPrimary);
	if (iPrev != iHint)
	{
		int z=1;
	}
	if (bPrimary)
	{
		pContainer->iSeqMultiLock_Primary_SeqHint = iHint;
	}
	else
	{
		pContainer->iSeqMultiLock_Secondary_SeqHint = iHint;
	}
}


// this is really naff, but i needed to add it once I found that workout out the sequence number from a frame
//	wasn't working properly because of frames appearing in multiple sequences...
//
// Note that this should only be called when Model_MultiSeq_IsActive(), and probably also when 
//	Model_MultiSeq_GetNumEntries() != 0
//
// returns false if did not fall within legal range...
//
static bool Model_MultiSeq_EnsureSeqHintLegal(ModelContainer_t *pContainer, int iFrame, bool bPrimary)
{
	bool bReturn = true;

	int iLegalSeqForThisFrame = -1;
	int iCurrentHint = Model_MultiSeq_GetSeqHint(pContainer, bPrimary);
	
	for (int i=0; i<Model_MultiSeq_GetNumEntries(pContainer, bPrimary); i++)
	{
		int iSeqIndex = Model_MultiSeq_GetEntry(pContainer, i, bPrimary);
		Sequence_t *pSequence = &pContainer->SequenceList[iSeqIndex];
		if (Sequence_FrameIsWithin(pSequence, iFrame))
		{
			if (iSeqIndex == iCurrentHint)
			{
				return true;	// all is well, our hint is pointing at a sequence that contains our current frame
			}

			// this isn't our hint, but it's legal for this frame, so preserve in case we need any old legal one later
			//
			iLegalSeqForThisFrame = iSeqIndex;
		}
	}

	// apparently our seq hint wasn't legal, so make it so...
	//	
	if (iLegalSeqForThisFrame == -1)
	{
		// whoooaaaaa!  Ok, this isn't good, our current frame isn't even within any of the multi-lock sequences,
		//	so presumably the frame legaliser hasn't kicked in yet, so for now, I guess we should just grab the
		//	first multi-locked sequence?...
		//		
		if (Model_MultiSeq_GetNumEntries(pContainer, bPrimary))
		{
			iLegalSeqForThisFrame = Model_MultiSeq_GetEntry(pContainer, 0, bPrimary);
		}
		else
		{
			// should never get here if this is called properly, but jic...
			//
			iLegalSeqForThisFrame = 0;	// should be at least globally legal			
		}
		bReturn = false;
	}

	Model_MultiSeq_SetSeqHint(pContainer, bPrimary, iLegalSeqForThisFrame);
	return bReturn;
}

// gets the next proposed frame in a multilocked sequence, where iStepVal can be +ve or -ve, (though usually just -1/0/1),
//	note that this will return a good nextframe number if everything's already legal, else it'll just return a non-locked
//	number and let the frame legaliser sort things out, but be right from then on. Basically think of this as a nextframe-num hinter
//
// (this isn't staggeringly fast at certain points, but 99% of the time it executes quickly)
//
static int Model_MultiSeq_GetNextFrame(ModelContainer_t *pContainer, int iFrame, int iStepVal, bool bPrimary)
{
	if (Model_MultiSeq_IsActive( pContainer, bPrimary))
	{
		if (Model_MultiSeq_GetNumEntries(pContainer, bPrimary))
		{
			if (Model_MultiSeq_EnsureSeqHintLegal(pContainer, iFrame, bPrimary))
			{
				// process the 'StepVal' param in a loop, 1 iteration per loop...
				//
				int iNewFrame = iFrame;
				int iStepDir = (iStepVal<0)?-1:1;

				for (int iStepValRemaining = iStepVal; iStepValRemaining!=0; iStepValRemaining += -iStepDir)
				{
					int iThisFrame = iNewFrame;					

					int iMultiLockEntryNum = Model_MultiSeq_EntryIndexFromFrame(pContainer, iThisFrame, bPrimary );												
					if (iMultiLockEntryNum != -1)
					{
						Sequence_t *pSeq = &pContainer->SequenceList[Model_MultiSeq_GetEntry(pContainer,iMultiLockEntryNum,bPrimary)];

						assert(pSeq);	//
						if (pSeq)		// itu?
						{
							int iNextFrame = iNewFrame + ((pSeq->iFPS<0)?-iStepDir:iStepDir);

							// this is the sequence we're currently in, does the proposed next frame also sit within this one?...
							//
							int iSeqFrameFirst = pSeq->iStartFrame;
							int iSeqFrameLast  = (iSeqFrameFirst + pSeq->iFrameCount)-1;

							if (iNextFrame >= iSeqFrameFirst && iNextFrame <= iSeqFrameLast)
							{
								// yes, so adopt it...
								//
								iNewFrame = iNextFrame;
							}
							else
							{
								// new frame is outside of current group, so get the index of the sequence after (or before) this one,
								//	and adopt the first or last frame of it...
								//
								iMultiLockEntryNum += iStepDir;

								if (iMultiLockEntryNum >= Model_MultiSeq_GetNumEntries(pContainer, bPrimary))
									iMultiLockEntryNum = 0;
								if (iMultiLockEntryNum < 0)
									iMultiLockEntryNum =  Model_MultiSeq_GetNumEntries(pContainer, bPrimary)-1;

								int iNextSeqIndex = Model_MultiSeq_GetEntry(pContainer, iMultiLockEntryNum, bPrimary);
								pSeq = &pContainer->SequenceList[iNextSeqIndex];

								assert(pSeq);	//
								if (pSeq)		// itu?
								{
									Model_MultiSeq_SetSeqHint(pContainer, bPrimary, iNextSeqIndex);
									iSeqFrameFirst = pSeq->iStartFrame;
									iSeqFrameLast  = (iSeqFrameFirst + pSeq->iFrameCount)-1;

									if (pSeq->iFPS < 0)			// stepping into a backwards sequence?
									{
										iNewFrame = (iStepDir>0)?iSeqFrameLast:iSeqFrameFirst;
									}
									else
									{
										iNewFrame = (iStepDir>0)?iSeqFrameFirst:iSeqFrameLast;
									}
								}
								else
								{
									return iFrame+iStepVal;	 // ... let the overall legaliser adjust this frame sometime after returning
								}
							}
						}
						else
						{
							// I don't think we'll ever get here, but wtf...
							//					
							return iFrame+iStepVal; // ... let the overall legaliser adjust this frame sometime after returning
						}
					}
					else
					{
						// err...
						return iFrame+iStepVal;	// ... let the overall legaliser adjust this frame sometime after returning
					}
				}

				return iNewFrame;
			}
			else
			{
				return iFrame+iStepVal; // ... let the overall legaliser adjust this frame sometime after returning
			}
		}
		else
		{
			// multi seq locking on, but no entries...
			//
			return iFrame+iStepVal;
		}
	}
	else
	{
		// single-seq locking on? if so, check for -ve dir sequences...
		//
		const int iSequenceLockNumber = bPrimary ? pContainer->iSequenceLockNumber_Primary : pContainer->iSequenceLockNumber_Secondary;
		const Sequence_t *pSequence	= (iSequenceLockNumber == -1) ? NULL : &pContainer->SequenceList[iSequenceLockNumber];
		if (pSequence)
		{
			if (pSequence->iFPS < 0)
			{
				return iFrame + -iStepVal;
			}
		}		
	}

	return iFrame+iStepVal;
}

// does NOT affect original container frame number, just legalises and returns it...
//
// new bit, passing in -1 as frame number is used during GoToEndFrame logic for legalising to end-of-range, instead of start-of-range
//
int ModelContainer_EnsureFrameLegal(ModelContainer_t *pContainer, int iFrame, bool bPrimary, bool bIsOldFrame)
{	
	bool bLegaliseToStart = true;	
	if (iFrame == -1)
	{
		bLegaliseToStart = false;
	}

	// multi-locking active? (highest priority)...
	//
	if (Model_MultiSeq_IsActive(pContainer, bPrimary))
	{
		if (Model_MultiSeq_GetNumEntries(pContainer, bPrimary))
		{
			// is the current frame within one of the multi-lock seqs?...
			//
			if (Model_MultiSeq_SeqIndexFromFrame(pContainer, iFrame, bPrimary, bIsOldFrame ) == -1)
			{
				// no, so try and fix it so it is...
				//

				// if we got here then we're outside all current multi-seqs, unfortunately because the specified sequences 
				//	can be at any position within the master list order I can't just work out which 2 sequences I'm between
				//	and jump to the start of the higher one, so working on the principle that 99% of the time this code is
				//	used when lerping between frame & frame+1 I'll see if I can find iFrame-1 anywhere...
				//
				iFrame--;

				// try and find the seq we may have just come from...
				//
				// find the real sequence index, then verify that it's within our multiseq array...
				//
				int iSeqIndex = Sequence_DeriveFromFrame( pContainer, iFrame );

				if (iSeqIndex != -1)
				{
					for (int i=0; i<Model_MultiSeq_GetNumEntries(pContainer, bPrimary); i++)
					{
						if ( iSeqIndex == Model_MultiSeq_GetEntry(pContainer, i, bPrimary))
						{
							// got it, so is there another sequence after this?
							//
							if (++i <Model_MultiSeq_GetNumEntries(pContainer, bPrimary))
							{
								// yes, so adopt first frame of this this seq...
								//
								iSeqIndex = Model_MultiSeq_GetEntry(pContainer,i,bPrimary);
								if (iSeqIndex < pContainer->SequenceList.size())
								{
									if (bLegaliseToStart)
										return pContainer->SequenceList[iSeqIndex].iStartFrame;
									return (pContainer->SequenceList[iSeqIndex].iStartFrame +
											pContainer->SequenceList[iSeqIndex].iFrameCount)-1;

								}
								else
								{
									ErrorBox(va("ModelContainer_EnsureFrameLegal(): Logic error trying to legalise frame %d",++iFrame)); // ++ to counter -- aboveand therefore get original param
								}
							}
							else
							{
								// reached end of multi-lock seq list...
								//
								break;
							}						
						}
					}
				}

				// sod it, whatever happened, let's start at the beginning of the list again...
				//
				iSeqIndex = Model_MultiSeq_GetEntry(pContainer, 0, bPrimary);
				Sequence_t *pSeq = &pContainer->SequenceList[iSeqIndex];
				iFrame = bLegaliseToStart ? pSeq->iStartFrame : (pSeq->iStartFrame + pSeq->iFrameCount)-1;
			}
			else
			{
				// iFrame is within the legal range of one of our multilocked sequences, so don't worry about it...
				//
			}
		}
		else
		{
			// no entries in table, so whatever frame we're on is legal...
			//
		}
	}
	else
	{
		// normal frame locking?...
		//
		const int iSequenceLockNumber = bPrimary ? pContainer->iSequenceLockNumber_Primary : pContainer->iSequenceLockNumber_Secondary;
		const Sequence_t *pSequence	= (iSequenceLockNumber == -1) ? NULL : &pContainer->SequenceList[iSequenceLockNumber];

		if (pSequence)
		{
			// anim-locked to this sequence...
			//
			if (iFrame > (pSequence->iStartFrame + pSequence->iFrameCount)-1)
			{
				// OOR above, loop or stop?...
				//
				if (pSequence->iLoopFrame != -1)
				{
					// account for loop frame...
					//
					iFrame = pSequence->iStartFrame + pSequence->iLoopFrame;
				}
				else
				{
					// stop...
					//
					if (AppVars.bForceWrapWhenAnimating)
					{
						iFrame = pSequence->iStartFrame;	// wrap
					}
					else
					{
						iFrame =(pSequence->iStartFrame + pSequence->iFrameCount)-1;	// stop at end
					}
				}
			}
			else
			if (iFrame < pSequence->iStartFrame)
			{
				// OOR below, legalise only...
				//
				iFrame = (pSequence->iFPS >= 0 && bLegaliseToStart)?pSequence->iStartFrame:(pSequence->iStartFrame + pSequence->iFrameCount)-1;
				//iFrame = pSequence->iStartFrame;
			}
		}
	}

	// sanity...
	//
	if (iFrame >= pContainer->iNumFrames)
	{
		iFrame = 0;
	}
	if (iFrame < 0)
		iFrame = pContainer->iNumFrames-1;

	return iFrame;
}


static void R_ModelContainer_CallBack_AffectedByLerping(ModelContainer_t* pContainer, void *pvData )
{
	bool *pbFeedbackIfTrue = (bool*) pvData;

	if (pContainer->iOldFrame_Primary != pContainer->iCurrentFrame_Primary)	// is this model lerping between 2 frames?
	{
		*pbFeedbackIfTrue = true;
	}
	else	// else = optional really
	if (Model_SecondaryAnimLockingActive(pContainer) &&
		pContainer->iOldFrame_Secondary != pContainer->iCurrentFrame_Secondary
		)
	{
		*pbFeedbackIfTrue = true;
	}
}
// called only for speed-opt check for rendering. This just says whether all models are unaffected by lerping,
//	eg if every loaded model has the currentframe and oldframe set to the same thing...
//
static bool ModelList_AffectedByLerping(void)
{
	bool bReturn = false;

	R_ModelContainer_Apply(&AppVars.Container, R_ModelContainer_CallBack_AffectedByLerping, &bReturn);

	return bReturn;
}




static void R_ModelContainer_CallBack_LegaliseFrame(ModelContainer_t* pContainer, void *pvData )
{
	bool *pbModelUpdated = (bool *) pvData;

	// primary...
	//
	int iFrame = ModelContainer_EnsureFrameLegal(pContainer,pContainer->iCurrentFrame_Primary,true, false);
	if (pContainer->iCurrentFrame_Primary != iFrame)
	{
		pContainer->iCurrentFrame_Primary  = iFrame;
		*pbModelUpdated = true;
	}
	// primary old...  (added to catch backlerping errors)
	iFrame = ModelContainer_EnsureFrameLegal(pContainer,pContainer->iOldFrame_Primary,true, true);
	if (pContainer->iOldFrame_Primary != iFrame)
	{
		pContainer->iOldFrame_Primary  = iFrame;
		*pbModelUpdated = true;
	}


	// secondary...
	//
	iFrame = ModelContainer_EnsureFrameLegal(pContainer, pContainer->iCurrentFrame_Secondary,false, false);
	if (pContainer->iCurrentFrame_Secondary != iFrame)
	{
		pContainer->iCurrentFrame_Secondary  = iFrame;
		*pbModelUpdated = true;
	}
	// secondary old... (added to catch backlerping errors)
	iFrame = ModelContainer_EnsureFrameLegal(pContainer, pContainer->iOldFrame_Secondary,false, true);
	if (pContainer->iOldFrame_Secondary != iFrame)
	{
		pContainer->iOldFrame_Secondary  = iFrame;
		*pbModelUpdated = true;
	}
}

// called by timer @ 100fps
//
//	return true == screen update wanted because of anim change
//
bool gbRedrawNeeded = false;
bool ModelList_Animation(void)
{
	if (!gbRenderInhibit)
	{
		bool bModelUpdated	= false;
		bool bFinalReturn	= false;
		bool bNothingUpdated= false;

		if (Model_Loaded())
		{
			//
			// sanity check, legalise any locked frames in case they were missed elsewhere (dunno why/how that would occur, but...)
			//
			R_ModelContainer_Apply(&AppVars.Container, R_ModelContainer_CallBack_LegaliseFrame, &bModelUpdated);

			// ok, now do animation...
			//
			if (!AppVars.bAnimate)
			{
				float fPrevFrac = AppVars.fFramefrac;
				if (AppVars.bInterpolate)
					AppVars.fFramefrac = 0.5f;
				else
					AppVars.fFramefrac = 0.0f;

				if (fPrevFrac != AppVars.fFramefrac)
					bModelUpdated = true;
			}
			else
			{
				double dTimeStamp2 = getDoubleTime();
				AppVars.fFramefrac = (float)((dTimeStamp2 - AppVars.dTimeStamp1) / AppVars.dAnimSpeed);

				if (AppVars.fFramefrac > 1.0f) 
				{
					bModelUpdated = true;
					AppVars.fFramefrac = 0;
					AppVars.dTimeStamp1 = dTimeStamp2;

					bNothingUpdated = !ModelList_StepFrame(1, false);				
				}
				else
				{
					bNothingUpdated = !ModelList_AffectedByLerping();
				}
			}
			
			bFinalReturn = (AppVars.bAnimate && AppVars.bInterpolate);
		}
		

		// bleurgh...
		//
		bFinalReturn = bNothingUpdated ? gbRedrawNeeded/*false*/ : (bFinalReturn || gbRedrawNeeded || bModelUpdated);

		gbRedrawNeeded = false;

		return bFinalReturn;
	}

	return false;
}



// sometimes MFC & C++ are a real fucking pain as regards what can talk to what, so...

void ModelList_ForceRedraw(void)
{
	gbRedrawNeeded = true;	// this was put in so model updated ok when not animating and treeview toggled a surface
}



// called whenever qdir changes (to avoid using cached textures from other dirs), or on program shutdown
void Media_Delete(void)
{		
	Model_Delete();
	RE_ModelBinCache_DeleteAll();
	TextureList_DeleteAll();
	KillAllShaderFiles();	
	Skins_KillPreCacheInfo();
}


// UI-Query function...
//
bool Model_SecondaryAnimLockingActive(ModelHandle_t hModel)
{
	ModelContainer_t *pContainer = ModelContainer_FindFromModelHandle( hModel );

	assert(pContainer);

	if (pContainer)
	{		
		return Model_SecondaryAnimLockingActive(pContainer);
	}

	return false;
}
bool Model_SecondaryAnimLockingActive(const ModelContainer_t *pContainer)
{
	return (pContainer->iBoneNum_SecondaryStart != -1);
}


// gallery query function...
//
// returns NULL if no lock present...
//
LPCSTR Model_Sequence_GetLockedName( ModelHandle_t hModel, bool bPrimary)
{
	ModelContainer_t *pContainer = ModelContainer_FindFromModelHandle( hModel );

	assert(pContainer);

	if (pContainer)
	{
		int iSeqLockNumber = bPrimary ? pContainer->iSequenceLockNumber_Primary : pContainer->iSequenceLockNumber_Secondary;

		if (iSeqLockNumber != -1)
		{
			return Model_Sequence_GetName( pContainer, iSeqLockNumber );
		}		
	}

	return NULL;
}


// UI query function.
//
// Note, since -1 = no locked sequence, you can pass this in to see if NO sequences are locked...
//
bool Model_Sequence_IsLocked( ModelHandle_t hModel, int iSequenceNumber, bool bPrimary)
{
	ModelContainer_t *pContainer = ModelContainer_FindFromModelHandle( hModel );

	assert(pContainer);

	if (pContainer)
	{		
		if (bPrimary)
		{
			return (pContainer->iSequenceLockNumber_Primary == iSequenceNumber);
		}
		return (pContainer->iSequenceLockNumber_Secondary == iSequenceNumber);
	}

	return false;
}

// UI query function.  ('bUsedForDisplay' signifies that return can be prepended with '-' if a reverse sequence)
//
LPCSTR Model_Sequence_GetName(ModelHandle_t hModel, int iSequenceNumber, bool bUsedForDisplay /* = false */)
{
	ModelContainer_t *pContainer = ModelContainer_FindFromModelHandle( hModel );

	assert(pContainer);

	if (pContainer)
	{		
		return Model_Sequence_GetName(pContainer, iSequenceNumber, bUsedForDisplay);
	}

	return sERROR_CONTAINER_NOT_FOUND;
}
// ('bUsedForDisplay' signifies that return can be prepended with '-' if a reverse sequence)
//
LPCSTR Model_Sequence_GetName(ModelContainer_t *pContainer, int iSequenceNumber, bool bUsedForDisplay /* = false */)
{
	if (iSequenceNumber < pContainer->SequenceList.size())
	{
		return Sequence_GetName(&pContainer->SequenceList[iSequenceNumber], bUsedForDisplay);
	}
	else
	{
		// could be fun if this appears in a dialogue... :-)
		//
		return va("Model_Sequence_GetName(): Illegal index %d, max = %d!",iSequenceNumber, pContainer->SequenceList.size()-1);
	}
}

// returns seq index for supplied name, else -1 for error...
//
int Model_Sequence_IndexForName(ModelContainer_t *pContainer, LPCSTR psSeqName)
{
	for (int iSeq=0; iSeq<pContainer->SequenceList.size(); iSeq++)
	{
		if (!stricmp(pContainer->SequenceList[iSeq].sName, psSeqName))
			return iSeq;
	}

	ErrorBox(va("Model_Sequence_IndexForName(): Unable to resolve index for \"%s\"",psSeqName));
	return -1;
}


// re-eval function for when toggling full path names on/off (Jarrod request)
//
LPCSTR Model_Sequence_GetTreeName(ModelHandle_t hModel, int iSequenceNumber)
{
	ModelContainer_t *pContainer = ModelContainer_FindFromModelHandle( hModel );

	assert(pContainer);

	if (pContainer)
	{		
		if (iSequenceNumber < pContainer->SequenceList.size())
		{				
			return Sequence_CreateTreeName(&pContainer->SequenceList[iSequenceNumber]);
		}
		else
		{
			// could be fun if this appears in a treeitem... :-)
			//
			return va("Model_Sequence_GetTreeName(): Illegal index %d, max = %d!",iSequenceNumber, pContainer->SequenceList.size()-1);
		}
	}

	return sERROR_CONTAINER_NOT_FOUND;
}


// a fairly inefficient function because of what it calls, but it's only used in "bursts" rather than every loop,
//	so who cares?...
//
bool Model_Sequence_Lock( ModelHandle_t hModel, LPCSTR psSequenceName, bool bPrimary, bool bOktoShowErrorBox /* = true */)
{
	int iTotSequences = Model_GetNumSequences(hModel);

	for (int i=0; i<iTotSequences; i++)
	{
		if (!stricmp(Model_Sequence_GetName	( hModel, i), psSequenceName))
			return Model_Sequence_Lock( hModel, i, bPrimary);
	}

	if (bOktoShowErrorBox)
	{
		ErrorBox(va("Model_Sequence_Lock(): Unable to find index for sequence \"%s\"!",psSequenceName));
	}
	return false;
}

bool Model_Sequence_Lock( ModelHandle_t hModel, int iSequenceNumber, bool bPrimary)
{
	bool bReturn = false;

	ModelContainer_t *pContainer = ModelContainer_FindFromModelHandle( hModel );

	assert(pContainer);

	if (pContainer)
	{		
		if (iSequenceNumber < pContainer->SequenceList.size())
		{
			if (bPrimary)
				pContainer->iSequenceLockNumber_Primary = iSequenceNumber;
			else
			{
				if (!Model_SecondaryAnimLockingActive(pContainer))
					iSequenceNumber = -1;	// don't allow secondary locking if no scondary bone anim start is set
				pContainer->iSequenceLockNumber_Secondary = iSequenceNumber;
			}

			ModelList_StepFrame(0, false);	// to legalise old frame so if no anim and you lock to another sequence then you don't try and lerp to an old one outside that sequence
			bReturn = true;
		}
		else
		{
			ErrorBox(va("Attempting to lock sequence # %d when max is %d!",iSequenceNumber, pContainer->SequenceList.size()-1));
		}
	}
	else
	{
		assert(0);
		ErrorBox(sERROR_CONTAINER_NOT_FOUND);
	}

	ModelList_ForceRedraw();

	return bReturn;
}

bool Model_Sequence_UnLock( ModelHandle_t hModel, bool bPrimary)
{
	bool bReturn = false;

	ModelContainer_t *pContainer = ModelContainer_FindFromModelHandle( hModel );

	assert(pContainer);

	if (pContainer)
	{
		if (bPrimary)
			pContainer->iSequenceLockNumber_Primary = -1;
		else
			pContainer->iSequenceLockNumber_Secondary = -1;
		bReturn = true;
	}
	else
	{
		assert(0);
		ErrorBox(sERROR_CONTAINER_NOT_FOUND);
	}

	ModelList_ForceRedraw();	// only needed to update lock stats onscr

	return bReturn;
}



static void R_ModelContainer_CallBack_Rewind(ModelContainer_t* pContainer, void *pvData )
{
	pContainer->iCurrentFrame_Primary   = ModelContainer_EnsureFrameLegal(pContainer, 0, true,  false);
	pContainer->iCurrentFrame_Secondary = ModelContainer_EnsureFrameLegal(pContainer, 0, false, false);
}
// rewind all loaded models...
//
void ModelList_Rewind()
{
	R_ModelContainer_Apply(&AppVars.Container, R_ModelContainer_CallBack_Rewind);

	ModelList_ForceRedraw();
}


static void R_ModelContainer_CallBack_GoToEndFrame(ModelContainer_t* pContainer, void *pvData )
{
	pContainer->iCurrentFrame_Primary   = ModelContainer_EnsureFrameLegal(pContainer, -1, true,  false);
	pContainer->iCurrentFrame_Secondary = ModelContainer_EnsureFrameLegal(pContainer, -1, false, false);
}

void ModelList_GoToEndFrame()
{
	R_ModelContainer_Apply(&AppVars.Container, R_ModelContainer_CallBack_GoToEndFrame);

	ModelList_ForceRedraw();
}


typedef struct	// one-off struct for data passing during StepFrame recursion
{
	int		iStepVal;
	bool	bSomethingHasUpdated;

} StepFrameData_t;

static void R_ModelContainer_CallBack_UpdateFrame(ModelContainer_t* pContainer, void *pvData )
{
	StepFrameData_t *pStepFrameData = (StepFrameData_t *) pvData;

	// primary...
	//
	{
		int iPrevOldFrame	= pContainer->iOldFrame_Primary;
		int	iPrevFrame		= pContainer->iCurrentFrame_Primary;

		// this is the only place in the code where this gets updated...
		//
		pContainer->iOldFrame_Primary		= ModelContainer_EnsureFrameLegal(pContainer, pContainer->iCurrentFrame_Primary, true, true);	// for locking	
		pContainer->iCurrentFrame_Primary	= Model_MultiSeq_GetNextFrame	 (pContainer, pContainer->iCurrentFrame_Primary, pStepFrameData->iStepVal, true);
		pContainer->iCurrentFrame_Primary	= ModelContainer_EnsureFrameLegal(pContainer, pContainer->iCurrentFrame_Primary, true, false);

		if (iPrevOldFrame != pContainer->iOldFrame_Primary ||
			iPrevFrame	  != pContainer->iCurrentFrame_Primary
			)
		{
			pStepFrameData->bSomethingHasUpdated = true;
		}
	}

	// secondary...
	//
	if (Model_SecondaryAnimLockingActive(pContainer))
	{
		int iPrevOldFrame	= pContainer->iOldFrame_Secondary;
		int	iPrevFrame		= pContainer->iCurrentFrame_Secondary;

		// this is the only place in the code where this gets updated...
		//
		pContainer->iOldFrame_Secondary		= ModelContainer_EnsureFrameLegal(pContainer, pContainer->iCurrentFrame_Secondary, false, true);	// for locking	
		pContainer->iCurrentFrame_Secondary	= Model_MultiSeq_GetNextFrame	 (pContainer, pContainer->iCurrentFrame_Secondary, pStepFrameData->iStepVal, false);
		pContainer->iCurrentFrame_Secondary	= ModelContainer_EnsureFrameLegal(pContainer, pContainer->iCurrentFrame_Secondary, false, false);

		if (iPrevOldFrame != pContainer->iOldFrame_Secondary ||
			iPrevFrame	  != pContainer->iCurrentFrame_Secondary
			)
		{
			pStepFrameData->bSomethingHasUpdated = true;
		}
	}
}

// advance/decrease all loaded model frames...
//
// return = true if at least one model has changed frames...
//
bool ModelList_StepFrame(int iStepVal, bool bAutoAnimOff /* = true */)
{
	StepFrameData_t StepFrameData;
					StepFrameData.iStepVal = iStepVal;
					StepFrameData.bSomethingHasUpdated = false;	// used for draw-optimising so when anim is on but all models are at non-wrap endframe, then no screen update

	R_ModelContainer_Apply(&AppVars.Container, R_ModelContainer_CallBack_UpdateFrame, &StepFrameData);

	if (bAutoAnimOff)
	{
		AppVars.bAnimate	= false;
		AppVars.bInterpolate= false;
	}

	if (bAutoAnimOff || (!bAutoAnimOff && StepFrameData.bSomethingHasUpdated))
	{
		ModelList_ForceRedraw();
	}

	return StepFrameData.bSomethingHasUpdated;
}

// remote helper func...
//
int Model_GetNumBoneAliases(ModelHandle_t hModel)
{
	ModelContainer_t *pContainer = ModelContainer_FindFromModelHandle(hModel);

	if (pContainer)
	{
		return pContainer->Aliases.size();
	}

	assert(0);
	ErrorBox(sERROR_CONTAINER_NOT_FOUND);
	return false;
}


// note that it's only valid to call this straight after you've called Model_GetNumAliases(), if you add
//	any more to this then the indexes are invalidated. ie, only call during an interrogate loop, and don't
//	store the indexes anywhere!
//
bool Model_GetBoneAliasPair(ModelHandle_t hModel, int iAliasIndex, string &strRealName,string &strAliasName)
{
	ModelContainer_t *pContainer = ModelContainer_FindFromModelHandle(hModel);

	if (pContainer)
	{
		return ModelContainer_GetBoneAliasPair(pContainer, iAliasIndex, strRealName, strAliasName);
	}

	assert(0);
	ErrorBox(sERROR_CONTAINER_NOT_FOUND);
	return false;
}

// note that it's only valid to call this straight after you've called Model_GetNumAliases(), if you add
//	any more to this then the indexes are invalidated. ie, only call during an interrogate loop, and don't
//	store the indexes anywhere!
//
bool ModelContainer_GetBoneAliasPair(ModelContainer_t *pContainer, int iAliasIndex, string &strRealName, string &strAliasName)
{
	int iLocalAliasIndex = iAliasIndex;

	for (MappedString_t::iterator it = pContainer->Aliases.begin(); it != pContainer->Aliases.end(); ++it)
	{
		if (!iLocalAliasIndex--)
		{
			strRealName = ((*it).first);
			strAliasName= ((*it).second);
			return true;
		}
	}

	assert(0);
	ErrorBox(va("Illegal alias index %d, max = %d",iAliasIndex,pContainer->Aliases.size()));
	return false;
}


// remote helper func only...
//
int Model_GetNumSequences(ModelHandle_t hModel)
{
	ModelContainer_t *pContainer = ModelContainer_FindFromModelHandle(hModel);

	if (pContainer)
	{
		return pContainer->SequenceList.size();
	}

	assert(0);
	ErrorBox(sERROR_CONTAINER_NOT_FOUND);
	return 0;
}


// remote helper func only...    returns a string with sequence name, startframe, duration etc
//
// NULL return = error
//
LPCSTR Model_GetSequenceString(ModelHandle_t hModel, int iSequenceNum)
{
	ModelContainer_t *pContainer = ModelContainer_FindFromModelHandle(hModel);

	if (pContainer && iSequenceNum < pContainer->SequenceList.size())
	{
		return Sequence_ReturnRemoteQueryString(&pContainer->SequenceList[iSequenceNum]);
	}

	assert(0);
	return NULL;
}


// I don't bother with validating the index because if it's wrong then it just doesn't highlight anything...
//
bool Model_SetSurfaceHighlight(ModelHandle_t hModel, int iSurfaceIndex)
{
	ModelContainer_t *pContainer = ModelContainer_FindFromModelHandle(hModel);

	if (pContainer)
	{
		pContainer->iSurfaceHighlightNumber = iSurfaceIndex;
		AppVars.iSurfaceNumToHighlight		= iSurfaceIndex;	// a bit tacky, but saves passing container info down the callstack
		AppVars.hModelToHighLight			= hModel;
		ModelList_ForceRedraw();
		return true;
	}

	assert(0);
	return false;
}

// this version uses bone names instead, used for easier remote calling...
//
bool Model_SetSurfaceHighlight(ModelHandle_t hModel, LPCSTR psSurfaceName)
{
	ModelContainer_t *pContainer = ModelContainer_FindFromModelHandle(hModel);
	
	if (pContainer)
	{
		// find this surface (fortunately, all names are unique within a model)...
		//
		for (int iSurface=0; iSurface<pContainer->iNumSurfaces; iSurface++)
		{
			if (!stricmp(psSurfaceName, pContainer->pModelGetSurfaceNameFunction( hModel, iSurface )))
			{
				Model_SetSurfaceHighlight(hModel, iSurface);
				return true;
			}
		}
	}

	return false;
}


// I don't bother with validating the index because if it's wrong then it just doesn't highlight anything...
//
bool Model_SetBoneHighlight(ModelHandle_t hModel, int iBoneIndex)
{
	ModelContainer_t *pContainer = ModelContainer_FindFromModelHandle(hModel);

	if (pContainer)
	{
		pContainer->iBoneHighlightNumber = iBoneIndex;
		ModelList_ForceRedraw();
		return true;
	}

	assert(0);
	return false;
}




// returns -1 for error, else bone index
//
// this is now slightly recursive, at least if you supply an aliased bone name...
//
int ModelContainer_BoneIndexFromName(ModelContainer_t *pContainer, LPCSTR psBoneName)
{
	int iBone = 0;

	// find this bone (fortunately, all names are unique within a model)...
	//
	for (iBone=0; iBone<pContainer->iNumBones; iBone++)
	{
		if (!stricmp(psBoneName, pContainer->pModelGetBoneNameFunction( pContainer->hModel, iBone )))
		{
			return iBone;
		}
	}

	static bool bAlreadyHere = false;
	if (!bAlreadyHere)
	{
		// new bit, if we didn't find the bone name, check the aliases (I could interrogate the tree, but this
		//	seems nicer...
		//
		// (Note that the alias <map> is opposite to what you'd think, ie [realname] = aliasname. 
		//
		// This makes for slower name lookup on these occasional calls, but faster is-alias-present-for-this-bone
		//	checks during rendering.
		//		
		/*
		for (MappedString_t::iterator it = pContainer->Aliases.begin(); it != pContainer->Aliases.end(); ++it)
		{
			CString strBoneName_Real (((*it).first).c_str());
			CString strBoneName_Alias(((*it).second).c_str());

			if (!stricmp(((*it).second).c_str(), psBoneName))	// bone name arg matches this alias?
			{
				bAlreadyHere = true;
				iBone = ModelContainer_BoneIndexFromName(pContainer, ((*it).first).c_str());	// pass real bone name
				bAlreadyHere = false;

				if (iBone != -1)
					return iBone;

				break;
			}
		}
		*/
		int iAliases = Model_GetNumBoneAliases(pContainer->hModel);
		for (int i=0; i<iAliases; i++)
		{
			string strBoneNameReal;
			string strBoneNameAlias;

			if (ModelContainer_GetBoneAliasPair(pContainer, i, strBoneNameReal, strBoneNameAlias))
			{
				if (!stricmp(strBoneNameAlias.c_str(), psBoneName))
				{
					bAlreadyHere = true;
					iBone = ModelContainer_BoneIndexFromName(pContainer, strBoneNameReal.c_str());	// pass real bone name
					bAlreadyHere = false;

					if (iBone != -1)
						return iBone;

					break;
				}
			}
			else
			{
				break;
			}
		}
	}

	assert(0);
	return -1;
}


// this version uses bone names instead, used for easier remote calling...
//
bool Model_SetBoneHighlight(ModelHandle_t hModel, LPCSTR psBoneName)
{
	ModelContainer_t *pContainer = ModelContainer_FindFromModelHandle(hModel);
	
	if (pContainer)
	{
		int iBoneIndex = ModelContainer_BoneIndexFromName(pContainer, psBoneName);
		if (iBoneIndex != -1)
		{
			Model_SetBoneHighlight(hModel, iBoneIndex);
			return true;
		}
	}

	return false;
}


void Model_StartAnim(bool bForceWrap /* = false */)
{
	AppVars.bAnimate = true;
	AppVars.bForceWrapWhenAnimating = bForceWrap;
}


void Model_StopAnim()
{
	AppVars.bAnimate = false;
}


// for use with script files and remote control, since it's safer to use bone names rather than indexes, in case they change 
//	 after the script was saved out or something
//
bool Model_SetSecondaryAnimStart(ModelHandle_t hModel, LPCSTR psBoneName)
{
	bool bReturn = false;

	ModelContainer_t *pContainer = ModelContainer_FindFromModelHandle(hModel);
	
	if (pContainer)
	{
		int iBoneNum = Model_GetBoltIndex( hModel, psBoneName, true);

		return Model_SetSecondaryAnimStart(hModel, iBoneNum );
	}
	else
	{
		assert(0);
		ErrorBox(sERROR_CONTAINER_NOT_FOUND);
	}

	ModelList_ForceRedraw();
	return bReturn;
}

// iBoneNum of -1 = clear secondary anim start
//
bool Model_SetSecondaryAnimStart(ModelHandle_t hModel, int iBoneNum)
{
	bool bReturn = false;

	ModelContainer_t *pContainer = ModelContainer_FindFromModelHandle(hModel);
	
	if (pContainer)
	{
		if (iBoneNum == 0)	// UI-update check on popup menu will not allow 0 to be sent, but remote-commands etc can do it.
		{
			WarningBox("Model_SetSecondaryAnimStart(): Attempting to start seconday anim from bone 0\n\nThis is probably bad, and is almost certainly pointless anyway");
			iBoneNum = -1;	// clear it
		}
		else
		if  (iBoneNum >= pContainer->iNumBones)
		{
			ErrorBox(va("Model_SetSecondaryAnimStart(): attempting to set start on bone %d, but max = %d!",iBoneNum,pContainer->iNumBones-1));
			iBoneNum = -1;	// clear it
		}
		else
		{
			bReturn = true;
		}
		pContainer->iBoneNum_SecondaryStart = iBoneNum;
	}
	else
	{
		assert(0);
		ErrorBox(sERROR_CONTAINER_NOT_FOUND);
	}

	ModelList_ForceRedraw();
	return bReturn;
}

int Model_GetSecondaryAnimStart(ModelHandle_t hModel)
{
	ModelContainer_t *pContainer = ModelContainer_FindFromModelHandle(hModel);
	
	if (pContainer)
	{
		return pContainer->iBoneNum_SecondaryStart;
	}
	else
	{
		assert(0);
		ErrorBox(sERROR_CONTAINER_NOT_FOUND);
	}

	return -1;	// return "no secondary anim start"
}

bool Model_MultiSeq_AlreadyContains(ModelContainer_t *pContainer, int iSequenceNumber, bool bPrimary)
{	
	for (int i=0; i<Model_MultiSeq_GetNumEntries(pContainer,bPrimary); i++)
	{
		if (iSequenceNumber == Model_MultiSeq_GetEntry(pContainer,i,bPrimary))
		{
			return true;
		}
	}

	return false;
}

bool Model_MultiSeq_AlreadyContains(ModelHandle_t hModel, int iSequenceNumber, bool bPrimary)
{
	ModelContainer_t *pContainer = ModelContainer_FindFromModelHandle(hModel);

	if (pContainer)
	{
		return Model_MultiSeq_AlreadyContains(pContainer, iSequenceNumber, bPrimary);
	}
	else
	{
		assert(0);
		ErrorBox(sERROR_CONTAINER_NOT_FOUND);
	}

	return false;
}

bool Model_MultiSeq_Add(ModelHandle_t hModel, int iSequenceNumber, bool bPrimary, bool bActivate /* = true */)
{
	ModelContainer_t *pContainer = ModelContainer_FindFromModelHandle(hModel);
	
	if (pContainer)
	{
		if (iSequenceNumber<pContainer->SequenceList.size())
		{
			if (!Model_MultiSeq_AlreadyContains(pContainer, iSequenceNumber, bPrimary))
			{
				if (bPrimary)
				{
					pContainer->SeqMultiLock_Primary.push_back(iSequenceNumber);
					if (bActivate)
					{
						pContainer->bSeqMultiLock_Primary_Active = true;
					}
				}
				else
				{
					pContainer->SeqMultiLock_Secondary.push_back(iSequenceNumber);
					if (bActivate)
					{
						pContainer->bSeqMultiLock_Secondary_Active = true;
					}
				}
				ModelList_ForceRedraw();	// because of lerping
			}
			return true;
		}
		else
		{
			ErrorBox(va("Model_MultiSeqLock_Add(): Illegal sequence number %d",iSequenceNumber));			
		}
	}
	else
	{
		assert(0);
		ErrorBox(sERROR_CONTAINER_NOT_FOUND);
	}

	return false;
}



// returns -1 else surface number to use instead of root...
//
int Model_GetG2SurfaceRootOverride(ModelContainer_t *pContainer)
{
	return pContainer->iSurfaceNum_RootOverride;
}


// returns -1 else surface number to use instead of root...
//
int Model_GetG2SurfaceRootOverride(ModelHandle_t hModel)
{
	ModelContainer_t *pContainer = ModelContainer_FindFromModelHandle( hModel );

	if ( pContainer )
	{
		return Model_GetG2SurfaceRootOverride( pContainer );
	}

	assert(0);
	return -1;
}


bool Model_SetG2SurfaceRootOverride	(ModelContainer_t *pContainer, LPCSTR psSurfaceName)
{
	for (int iSurface = 0; iSurface<pContainer->iNumSurfaces; iSurface++)
	{
		LPCSTR psScannedSurfaceName = Model_GetSurfaceName( pContainer, iSurface );

		if (!stricmp(psScannedSurfaceName,psSurfaceName))
		{
			Model_SetG2SurfaceRootOverride(pContainer, iSurface);
			return true;
		}
	}

	return false;
}

// iSurfaceNum of -1 = clear surface root override
//
bool Model_SetG2SurfaceRootOverride(ModelContainer_t *pContainer, int iSurfaceNum)
{
	bool bReturn = false;

	if  (iSurfaceNum >= pContainer->iNumSurfaces)
	{
		ErrorBox(va("Model_SetG2SurfaceRootOverride(): attempting to set surface %d as root, but max = %d!",iSurfaceNum, pContainer->iNumSurfaces-1));		
	}
	else
	{
		pContainer->iSurfaceNum_RootOverride = iSurfaceNum;
		bReturn = true;
	}

	ModelList_ForceRedraw();
	return bReturn;
}

bool Model_SetG2SurfaceRootOverride(ModelHandle_t hModel, int iSurfaceNum)
{
	ModelContainer_t *pContainer = ModelContainer_FindFromModelHandle( hModel );

	if ( pContainer )
	{
		return Model_SetG2SurfaceRootOverride( pContainer, iSurfaceNum );
	}
	else
	{
		assert(0);
		ErrorBox(sERROR_CONTAINER_NOT_FOUND);
	}

	ModelList_ForceRedraw();
	return false;
}



// ui-query...
//
bool Model_MultiSeq_IsActive(ModelHandle_t hModel, bool bPrimary)	
{
	ModelContainer_t *pContainer = ModelContainer_FindFromModelHandle(hModel);
	
	if (pContainer)
	{
		return Model_MultiSeq_IsActive(pContainer, bPrimary);
	}

	assert(0);
	return false;
}

bool Model_MultiSeq_IsActive(ModelContainer_t *pContainer, bool bPrimary)	
{
	if (bPrimary)
	{
		return pContainer->bSeqMultiLock_Primary_Active;
	}
	else
	{
		// secondary has special rule...
		//
		if (Model_SecondaryAnimLockingActive(pContainer))
			return pContainer->bSeqMultiLock_Secondary_Active;
		return false;
	}	
}

int Model_MultiSeq_GetEntry(ModelContainer_t *pContainer, int iEntry, bool bPrimary)
{
	if (bPrimary)
	{
		if (iEntry < pContainer->SeqMultiLock_Primary.size())
			return pContainer->SeqMultiLock_Primary[iEntry];
	}
	else
	{
		if (iEntry < pContainer->SeqMultiLock_Secondary.size())
			return pContainer->SeqMultiLock_Secondary[iEntry];
	}

	ErrorBox(va("Model_MultiSeq_GetEntry() illegal %s index %d ( max is %d )",bPrimary?"primary":"secondary",iEntry,bPrimary?pContainer->SeqMultiLock_Primary.size()-1:pContainer->SeqMultiLock_Secondary.size()-1));
	return NULL;
}

int Model_MultiSeq_GetNumEntries(ModelContainer_t *pContainer, bool bPrimary)	
{
	if (bPrimary)
	{
		return pContainer->SeqMultiLock_Primary.size();
	}
	else
	{
		return pContainer->SeqMultiLock_Secondary.size();
	}
}

// note that this can have entries but still be inactive
//
int Model_MultiSeq_GetNumEntries(ModelHandle_t hModel, bool bPrimary)	
{
	ModelContainer_t *pContainer = ModelContainer_FindFromModelHandle(hModel);
	
	if (pContainer)
	{
		return Model_MultiSeq_GetNumEntries(pContainer, bPrimary);
	}

	assert(0);
	ErrorBox(sERROR_CONTAINER_NOT_FOUND);
	return 0;
}

void Model_MultiSeq_Clear(ModelHandle_t hModel, bool bPrimary)	
{
	ModelContainer_t *pContainer = ModelContainer_FindFromModelHandle(hModel);
	
	if (pContainer)
	{
		if (bPrimary)
		{
			pContainer->SeqMultiLock_Primary.clear();			
		}
		else
		{
			pContainer->SeqMultiLock_Secondary.clear();			
		}

		ModelList_ForceRedraw();	// needed because of lerping
		return;
	}

	assert(0);	
	ErrorBox(sERROR_CONTAINER_NOT_FOUND);
}

// this is a tacky way of getting round the fact of not having a proper GUI for editing these lists,
//	basically you just delete the last thing entered if you make a mistake...
//
void Model_MultiSeq_DeleteLast(ModelHandle_t hModel, bool bPrimary)
{
	ModelContainer_t *pContainer = ModelContainer_FindFromModelHandle(hModel);
	
	if (pContainer)
	{
		if (bPrimary)
		{
			if (pContainer->SeqMultiLock_Primary.size() >= 1)
				pContainer->SeqMultiLock_Primary.pop_back();		
		}
		else
		{
			if (pContainer->SeqMultiLock_Secondary.size() >= 1)
				pContainer->SeqMultiLock_Secondary.pop_back();			
		}

		ModelList_ForceRedraw();	// important, in case current frame was within locked-seq we just deleted from set
		return;
	}

	assert(0);
	ErrorBox(sERROR_CONTAINER_NOT_FOUND);
}

void Model_MultiSeq_Delete( ModelHandle_t hModel, int iSeqIndex, bool bPrimary)
{
	ModelContainer_t *pContainer = ModelContainer_FindFromModelHandle(hModel);

	if (pContainer)
	{
		// now we have to turn this sequence index into a multilock list index...
		//
		for (int i=0; i<Model_MultiSeq_GetNumEntries(pContainer,bPrimary); i++)
		{
			int iThisSeqIndex = Model_MultiSeq_GetEntry	( pContainer, i, bPrimary );

			if (iThisSeqIndex == iSeqIndex)
			{
				if (bPrimary)
				{
					if (pContainer->SeqMultiLock_Primary.size() > i)
					{
						pContainer->SeqMultiLock_Primary.erase(pContainer->SeqMultiLock_Primary.begin() + i);
					}
				}
				else
				{
					if (pContainer->SeqMultiLock_Secondary.size() > i)
					{
						pContainer->SeqMultiLock_Secondary.erase(pContainer->SeqMultiLock_Secondary.begin() + i);
					}
				}
			}
		}

		ModelList_ForceRedraw();	// important, in case current frame was within locked-seq we just deleted from set
		return;
	}

	assert(0);
	ErrorBox(sERROR_CONTAINER_NOT_FOUND);
}

void Model_MultiSeq_SetActive(ModelHandle_t hModel, bool bPrimary, bool bActive)	
{
	ModelContainer_t *pContainer = ModelContainer_FindFromModelHandle(hModel);
	
	if (pContainer)
	{
		if (bPrimary)
		{	
			pContainer->bSeqMultiLock_Primary_Active = bActive;
		}
		else
		{
			pContainer->bSeqMultiLock_Secondary_Active = bActive;
		}

		ModelList_ForceRedraw();	// even just unlocking can affect lerping, so redraw needed
		return;		
	}

	assert(0);
	ErrorBox(sERROR_CONTAINER_NOT_FOUND);
}


// return value is the corrected (if over the model limit) LOD level...
//
int Model_EnsureGenerated_VertEdgeInfo(ModelContainer_t *pContainer, int iLOD)
{		
	return GLMModel_EnsureGenerated_VertEdgeInfo(pContainer->hModel, iLOD, pContainer->SurfaceEdgeInfoPerLOD);
}

// return value is the corrected (if over the model limit) LOD level, -1 = failure !
//
int Model_EnsureGenerated_VertEdgeInfo(ModelHandle_t hModel, int iLOD)
{
	ModelContainer_t *pContainer = ModelContainer_FindFromModelHandle(hModel);
	
	if (pContainer)
	{
		return Model_EnsureGenerated_VertEdgeInfo(pContainer, iLOD);
	}

	assert(0);
	ErrorBox(sERROR_CONTAINER_NOT_FOUND);
	return -1;
}

static inline void CrossProduct( const vec3_t v1, const vec3_t v2, vec3_t cross )
{
	cross[0] = (v1[1] * v2[2]) - (v1[2] * v2[1]);
	cross[1] = (v1[2] * v2[0]) - (v1[0] * v2[2]);
	cross[2] = (v1[0] * v2[1]) - (v1[1] * v2[0]);
}

static inline vec_t VectorNormalize( vec3_t v ) {
	float	length, ilength;

	length = v[0]*v[0] + v[1]*v[1] + v[2]*v[2];
	length = sqrt (length);

	if ( length > 0.0001f ) {
		ilength = 1/length;
		v[0] *= ilength;
		v[1] *= ilength;
		v[2] *= ilength;
	}

	return length;
}

typedef struct {
	unsigned int	normal;
	short			vertCoords[3];
	unsigned int	tangent;
	unsigned int	uiNmWeightsAndBoneIndexes;	
	byte			BoneWeightings[iMAX_G2_BONEWEIGHTS_PER_VERT];
} mdxmVertexComp_t;

typedef struct {
	short			texCoords[2];
} mdxmVertexTexCoordComp_t;

static vec3_t _tangents[1000];

static void BuildTangentVectors(mdxmVertex_t *v, int numVerts, mdxmTriangle_t *tri, int numTriangles)
{
	memset(_tangents, 0, sizeof(vec3_t) * 1000);

	mdxmVertexTexCoord_t *pTex = (mdxmVertexTexCoord_t *) &v[numVerts];

	int i = 0;

	for(i = 0; i < numTriangles; i ++)
	{
		vec3_t vec1, vec2, du, dv, cp;

		vec1[0] = v[tri[i].indexes[1]].vertCoords[0] - v[tri[i].indexes[0]].vertCoords[0];
		vec1[1] = pTex[tri[i].indexes[1]].texCoords[0] - pTex[tri[i].indexes[0]].texCoords[0];
		vec1[2] = pTex[tri[i].indexes[1]].texCoords[1] - pTex[tri[i].indexes[0]].texCoords[1];

		vec2[0] = v[tri[i].indexes[2]].vertCoords[0] - v[tri[i].indexes[0]].vertCoords[0];
		vec2[1] = pTex[tri[i].indexes[2]].texCoords[0] - pTex[tri[i].indexes[0]].texCoords[0];
		vec2[2] = pTex[tri[i].indexes[2]].texCoords[1] - pTex[tri[i].indexes[0]].texCoords[1];

		CrossProduct(vec1, vec2, cp);

		if(cp[0] == 0.0f)
			cp[0] = 0.001f;

		du[0] = -cp[1] / cp[0];
		dv[0] = -cp[2] / cp[0];

		vec1[0] = v[tri[i].indexes[1]].vertCoords[1] - v[tri[i].indexes[0]].vertCoords[1];
		vec1[1] = pTex[tri[i].indexes[1]].texCoords[0] - pTex[tri[i].indexes[0]].texCoords[0];
		vec1[2] = pTex[tri[i].indexes[1]].texCoords[1] - pTex[tri[i].indexes[0]].texCoords[1];

		vec2[0] = v[tri[i].indexes[2]].vertCoords[1] - v[tri[i].indexes[0]].vertCoords[1];
		vec2[1] = pTex[tri[i].indexes[2]].texCoords[0] - pTex[tri[i].indexes[0]].texCoords[0];
		vec2[2] = pTex[tri[i].indexes[2]].texCoords[1] - pTex[tri[i].indexes[0]].texCoords[1];

		CrossProduct(vec1, vec2, cp);

		if(cp[0] == 0.0f)
			cp[0] = 0.001f;

		du[1] = -cp[1] / cp[0];
		dv[1] = -cp[2] / cp[0];

		vec1[0] = v[tri[i].indexes[1]].vertCoords[2] - v[tri[i].indexes[0]].vertCoords[2];
		vec1[1] = pTex[tri[i].indexes[1]].texCoords[0] - pTex[tri[i].indexes[0]].texCoords[0];
		vec1[2] = pTex[tri[i].indexes[1]].texCoords[1] - pTex[tri[i].indexes[0]].texCoords[1];

		vec2[0] = v[tri[i].indexes[2]].vertCoords[2] - v[tri[i].indexes[0]].vertCoords[2];
		vec2[1] = pTex[tri[i].indexes[2]].texCoords[0] - pTex[tri[i].indexes[0]].texCoords[0];
		vec2[2] = pTex[tri[i].indexes[2]].texCoords[1] - pTex[tri[i].indexes[0]].texCoords[1];

		CrossProduct(vec1, vec2, cp);

		if(cp[0] == 0.0f)
			cp[0] = 0.001f;

		du[2] = -cp[1] / cp[0];
		dv[2] = -cp[2] / cp[0];

		_tangents[tri[i].indexes[0]][0] += du[0];
		_tangents[tri[i].indexes[0]][1] += du[0];
		_tangents[tri[i].indexes[0]][2] += du[0];

		_tangents[tri[i].indexes[1]][0] += du[0];
		_tangents[tri[i].indexes[1]][1] += du[0];
		_tangents[tri[i].indexes[1]][2] += du[0];

		_tangents[tri[i].indexes[1]][0] += du[0];
		_tangents[tri[i].indexes[1]][1] += du[0];
		_tangents[tri[i].indexes[1]][2] += du[0];
	}

	for(i = 0; i < numVerts; i++)
	{
		VectorNormalize(_tangents[i]);
	}
}

bool Model_Save(LPCSTR psFullPathedFilename)
{
	long pos;
	mdxmLOD_t*	lod;
	int l, i;
	mdxmTriangle_t*	tri;
	mdxmSurface_t*	surf;
	mdxmVertex_t*	v;
	int lodOfsEnd, headOfsEnd;
	mdxmLODSurfOffset_t* newindexes;

	mdxmHeader_t *pHeader = (mdxmHeader_t *) RE_GetModelData(AppVars.hModelLastLoaded);
	if(!pHeader)
		return false;

	headOfsEnd = pHeader->ofsEnd;

	FILE *pFile = fopen(psFullPathedFilename, "wb");
	if(!pFile)
		return false;
	FILE *flog = fopen( "C:\\modlog.txt", "a" );

	// Skipping thru the header at this point, because we have to change
	// header values as we go thru the model
	fseek(pFile, sizeof(*pHeader), SEEK_SET);

	// Write the surface heirarchy stuff
	// We dont change anything here, so its just a raw write
	byte *ptr = ((byte *) pHeader + sizeof(*pHeader));
	fwrite(ptr, 1, pHeader->ofsLODs - sizeof(*pHeader), pFile);

	// Offset to the first LOD
	lod = (mdxmLOD_t *) ( (byte *)pHeader + pHeader->ofsLODs );

	for ( l = 0 ; l < pHeader->numLODs ; l++)
	{
		int	triCount = 0;

		lodOfsEnd = lod->ofsEnd;

		mdxmLODSurfOffset_t *indexes = (mdxmLODSurfOffset_t *)((char*)lod + sizeof(mdxmLOD_t));
		newindexes = new mdxmLODSurfOffset_t[pHeader->numSurfaces];
		memcpy(newindexes, indexes, sizeof(mdxmLODSurfOffset_t) * pHeader->numSurfaces);

		// iterate thru the surfaces
		surf = (mdxmSurface_t *) ( (byte *)lod + sizeof (mdxmLOD_t) + (pHeader->numSurfaces * sizeof(mdxmLODSurfOffset_t)) );

		for ( i = 0 ; i < pHeader->numSurfaces ; i++) 
		{
			// Calculate the memory savings for this surface
			int save = (surf->numVerts * (sizeof(mdxmVertex_t) + sizeof(mdxmVertexTexCoord_t))) -
					   (surf->numVerts * (sizeof(mdxmVertexComp_t) + sizeof(mdxmVertexTexCoordComp_t)));

			lodOfsEnd -= save;
			headOfsEnd -= save;

			for ( int s = i + 1; s < pHeader->numSurfaces; s++ )
			{
				newindexes[s].offsets[0] -= save;
			}
			
			surf = (mdxmSurface_t *)( (byte *)surf + surf->ofsEnd );
		}

		// We've calculate our new offsets, so write out this LOD
		// Write the new LOD offset
		mdxmLOD_t newlod;
		newlod.ofsEnd = lodOfsEnd;
		fwrite(&newlod, 1, sizeof(newlod), pFile);

		pos = ftell(pFile);

		// Write the surface offset array
		fwrite(newindexes, 1, sizeof(mdxmLODSurfOffset_t) * pHeader->numSurfaces, pFile);
		delete [] newindexes;

		pos = ftell(pFile);

		// Write each surface
		surf = (mdxmSurface_t *) ( (byte *)lod + sizeof (mdxmLOD_t) + (pHeader->numSurfaces * sizeof(mdxmLODSurfOffset_t)) );
		for ( i = 0; i < pHeader->numSurfaces; i++)
		{
			// Calculate the memory savings for this surface
			int save = (surf->numVerts * (sizeof(mdxmVertex_t) + sizeof(mdxmVertexTexCoord_t))) -
					   (surf->numVerts * (sizeof(mdxmVertexComp_t) + sizeof(mdxmVertexTexCoordComp_t)));

			mdxmSurface_t newsurf;
			memcpy(&newsurf, surf, sizeof(mdxmSurface_t));

			newsurf.ofsEnd -= save;	
			newsurf.ofsBoneReferences -= save;

			// Write this surface struct
			fwrite(&newsurf, 1, sizeof(mdxmSurface_t), pFile);

			pos = ftell(pFile);

			// Write the triangles
			tri = (mdxmTriangle_t *) ( (byte *)surf + surf->ofsTriangles );
			fwrite(tri, 1, sizeof(mdxmTriangle_t) * surf->numTriangles, pFile);

			pos = ftell(pFile);

			// Write the vertices
			v = (mdxmVertex_t *) ( (byte *)surf + surf->ofsVerts );

			// Compute tangent vectors
			BuildTangentVectors(v, surf->numVerts, tri, surf->numTriangles);

			v = (mdxmVertex_t *) ( (byte *)surf + surf->ofsVerts );

			for(int ver = 0; ver < surf->numVerts; ver++)
			{
				mdxmVertexComp_t compv;
				compv.uiNmWeightsAndBoneIndexes = v->uiNmWeightsAndBoneIndexes;

				for(int m = 0; m < iMAX_G2_BONEWEIGHTS_PER_VERT; m++)
				{
					compv.BoneWeightings[m] = v->BoneWeightings[m];
				}

				compv.vertCoords[0] = (short)(LittleFloat( v->vertCoords[0] ) * POINT_SCALE );
				compv.vertCoords[1] = (short)(LittleFloat( v->vertCoords[1] ) * POINT_SCALE );
				compv.vertCoords[2] = (short)(LittleFloat( v->vertCoords[2] ) * POINT_SCALE );

				/*compv.normal[0] = (short)(LittleFloat( v->normal[0] ) * POINT_SCALE );
				compv.normal[1] = (short)(LittleFloat( v->normal[1] ) * POINT_SCALE );
				compv.normal[2] = (short)(LittleFloat( v->normal[2] ) * POINT_SCALE );*/

				/*compv.normal = ( ( ((DWORD)(v->normal[2] *  511.0f)) & 0x3ff ) << 22L ) |
							   ( ( ((DWORD)(v->normal[1] * 1023.0f)) & 0x7ff ) << 11L ) |
							   ( ( ((DWORD)(v->normal[0] * 1023.0f)) & 0x7ff ) <<  0L );

				compv.tangent = ( ( ((DWORD)(_tangents[ver][2] *  511.0f)) & 0x3ff ) << 22L ) |
							    ( ( ((DWORD)(_tangents[ver][1] * 1023.0f)) & 0x7ff ) << 11L ) |
							    ( ( ((DWORD)(_tangents[ver][0] * 1023.0f)) & 0x7ff ) <<  0L );*/

				unsigned int ix = unsigned int( (v->normal[0] * 127.f) + 128.f );
				unsigned int iy = unsigned int( (v->normal[1] * 127.f) + 128.f );
				unsigned int iz = unsigned int( (v->normal[2] * 127.f) + 128.f );
				unsigned int iw = 0; // we don't use the w component (just padding)

				compv.normal = (iw << 24) | (ix << 16) | (iy << 8) | (iz << 0);

				ix = unsigned int( (_tangents[ver][0] * 127.f) + 128.f );
				iy = unsigned int( (_tangents[ver][1] * 127.f) + 128.f );
				iz = unsigned int( (_tangents[ver][2] * 127.f) + 128.f );

				compv.tangent = (iw << 24) | (ix << 16) | (iy << 8) | (iz << 0);


				fwrite(&compv, 1, sizeof(mdxmVertexComp_t), pFile);

				v++;
			}
 
			pos = ftell(pFile);
			
			// Write the texture coords
			v = (mdxmVertex_t *) ( (byte *)surf + surf->ofsVerts );
			mdxmVertexTexCoord_t *pTexCoords = (mdxmVertexTexCoord_t *) &v[surf->numVerts];
//			fwrite(pTexCoords, 1, sizeof(mdxmVertexTexCoord_t) * surf->numVerts, pFile);
			for(int m = 0; m < surf->numVerts; m++)
			{
				float tempTex[2];
				tempTex[0] = pTexCoords->texCoords[0] * POINT_ST_SCALE;
				tempTex[1] = pTexCoords->texCoords[1] * POINT_ST_SCALE;

				assert( tempTex[0] > -32768.0f && tempTex[0] < 32767.0f );
				assert( tempTex[1] > -32768.0f && tempTex[1] < 32767.0f );

				if (tempTex[0] < -32768.0f || tempTex[0] > 32767.0f)
					fprintf( flog, "ERROR: UV overflow in %s [%f]\n", psFullPathedFilename, tempTex[0] );
				if (tempTex[1] < -32768.0f || tempTex[1] > 32767.0f)
					fprintf( flog, "ERROR: UV overflow in %s [%f]\n", psFullPathedFilename, tempTex[1] );

				mdxmVertexTexCoordComp_t tex;
				tex.texCoords[0] = (short)( tempTex[0] );
				tex.texCoords[1] = (short)( tempTex[1] );

				fwrite(&tex, 1, sizeof(mdxmVertexTexCoordComp_t), pFile);

				pTexCoords++;
			}

			// Write the bone references
			int *boneref = (int *)((byte*)surf + surf->ofsBoneReferences);
			fwrite(boneref, 1, sizeof(int) * surf->numBoneReferences, pFile);

			pos = ftell(pFile);


			surf = (mdxmSurface_t *)( (byte *)surf + surf->ofsEnd );
		}

		lod = (mdxmLOD_t *)((byte *)lod + lod->ofsEnd);
	}

	pos = ftell(pFile);

	// Weve written all the data, so skip back up to the top
	// and write the model header
	fseek(pFile, 0, SEEK_SET);
	mdxmHeader_t head;
	memcpy(&head, pHeader, sizeof(mdxmHeader_t));
	head.ofsEnd = headOfsEnd;
	fwrite(&head, 1, sizeof(mdxmHeader_t), pFile);
		
	fclose(pFile);

	fclose( flog );
	return true;
}


///////////////// eof //////////////////

