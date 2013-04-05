// Filename:-	script.cpp
//
// file to control model-loading scripts (scripts are purely a viewer convenience for multiple-bolt stuff)
//
#include "stdafx.h"
#include "includes.h"
#include "files.h"
#include "r_common.h"
//#include "ModViewTreeView.h"
//#include "glm_code.h"
//#include "R_Model.h"
//#include "R_Surface.h"
#include "textures.h"
//#include "TEXT.H"
//#include "sequence.h"
//#include "model.h"

#include "GenericParser2.h"
#include "skins.h"
#include "stl.h"
SkinSets_t				CurrentSkins;
SkinSetsSurfacePrefs_t	CurrentSkinsSurfacePrefs;
typedef map <string, map<string, int> >	G2SkinModelPrefs_t;		// <skinfile name, <modelname, junk>>  to see if a file contains a model's name
										G2SkinModelPrefs_t G2SkinModelPrefs;


// called from Media_Delete...
//
void Skins_KillPreCacheInfo(void)
{
	G2SkinModelPrefs.clear();
	//
	// these 2 aren't really needed here, but wtf?
	//
	CurrentSkins.clear();
	CurrentSkinsSurfacePrefs.clear();
}

/*
prefs
{
	models
	{
		1	"snow"
	}

  	surfaces_off
	{
		name1	"head_side_r_avmed_off"
		name2	"head_side_l_avmed_off"
		name3	"head_side_r_off"
		name4	"head_side_l_off"
	}

	surfaces_on
	{
		name1	"head_bck_uppr_r_avmed_off"
		name2	"head_bck_uppr_l_avmed_off"
		name3	"head_bck_lwr_r_avmed_off"
		name4	"head_bck_lwr_l_avmed_off"
		name5	"head_bck_lwr_r"
		name6	"head_bck_lwr_l"
	}
}

material
{
	name	"face"

	group
	{
		name	"white"

		shader1	"models/characters/face_average/face"	
	}

	group
	{
		name	"black"

		texture1	"models/characters/average_face/f_marine_b1"	
	}
}

material
{
	name	"face_2sided"

	group
	{
		name	"white"

		shader1	"models/characters/face_average/face_2sided"	
	}

	group
	{
		name	"black"

		texture1	"models/characters/average_face/f_marine_b1"	
	}
}

material
{
	name	"head"

	group
	{
		name "white"

		shader1	"models/characters/face_average/head"
	}

	group
	{
		name "black"

		texture1	"models/characters/average_face/h_marine_b1"
	}
}

material
{
	name	"arms"

	group
	{
		name "white"

		shader1	"models/characters/average_sleeves/arms"
	}

	group
	{
		name "black"

		texture1	"models/characters/average_sleeves/a_marine_b1"
	}
}
*/

#define sSKINKEYWORD_MATERIAL				"material"
#define sSKINKEYWORD_GROUP					"group"
#define sSKINKEYWORD_NAME					"name"
#define sSKINKEYWORD_PREFS					"prefs"
#define sSKINKEYWORD_MODELSUSING			"models"
#define sSKINKEYWORD_SURFACES_ON			"surfaces_on"
#define sSKINKEYWORD_SURFACES_OFF			"surfaces_off"
#define sSKINKEYWORD_SURFACES_OFFNOCHILDREN	"surfaces_offnochildren"

static LPCSTR Skins_Parse(string strThisSkinFileName, CGPGroup *pFileGroup, CGPGroup *pParseGroup_Prefs)
{
	LPCSTR psError = NULL;

	// read any optional surface on/off blocks...
	//
	for (int iSurfaceOnOffType = 0; iSurfaceOnOffType<3; iSurfaceOnOffType++)
	{
		CGPGroup *pSurfaceParseGroup = NULL;
		switch (iSurfaceOnOffType)
		{
			case 0: pSurfaceParseGroup = pParseGroup_Prefs->FindSubGroup(sSKINKEYWORD_SURFACES_ON);				break;
			case 1: pSurfaceParseGroup = pParseGroup_Prefs->FindSubGroup(sSKINKEYWORD_SURFACES_OFF);			break;
			case 2: pSurfaceParseGroup = pParseGroup_Prefs->FindSubGroup(sSKINKEYWORD_SURFACES_OFFNOCHILDREN);	break;
			default: assert(0);	break;
		}

		if (pSurfaceParseGroup)
		{
			CGPValue *pValue = pSurfaceParseGroup->GetPairs();
			while (pValue)
			{			
//				string str1 = (*it).first;	// junk, eg "name1"
				string str2 = pValue->GetTopValue();
				
				switch (iSurfaceOnOffType)
				{
					case 0: CurrentSkinsSurfacePrefs[strThisSkinFileName].vSurfacesOn.push_back(str2);	break;
					case 1: CurrentSkinsSurfacePrefs[strThisSkinFileName].vSurfacesOff.push_back(str2);	break;
					case 2: CurrentSkinsSurfacePrefs[strThisSkinFileName].vSurfacesOffNoChildren.push_back(str2);	break;
					default: assert(0);	break;
				}

				pValue = pValue->GetNext();
			}
		}
	}

	// find all the materials and add them to the skin set...
	//
	int iMaterialDeclarationIndex = 0;
	for (CGPGroup *pMaterialGroup = pFileGroup->GetSubGroups(); pMaterialGroup; pMaterialGroup = pMaterialGroup->GetNext(), iMaterialDeclarationIndex++)
	{
		string strKeyWord = pMaterialGroup->GetName();

		if (strKeyWord == sSKINKEYWORD_MATERIAL)
		{
			string strMaterialName(pMaterialGroup->FindPairValue(sSKINKEYWORD_NAME,""));

			if (strMaterialName == "")
			{
				psError = va("%s[%d] had no \"%s\" field!\n",sSKINKEYWORD_MATERIAL, iMaterialDeclarationIndex, sSKINKEYWORD_NAME);
				return psError;
			}

			// now iterate through the ethnic group variants of this material...
			//
			int iEthnicGroupIndex = 0;
			for (CGPGroup *pEthnicGroup = pMaterialGroup->GetSubGroups(); pEthnicGroup; pEthnicGroup = pEthnicGroup->GetNext(), iEthnicGroupIndex++)
			{
				strKeyWord = pEthnicGroup->GetName();

				if (strKeyWord == sSKINKEYWORD_GROUP)
				{
					string strEthnicGroupName(pEthnicGroup->FindPairValue(sSKINKEYWORD_NAME,""));

					if (strEthnicGroupName == "")
					{
						psError = va("%s[%d] %s[%d] had no \"%s\" field!\n",sSKINKEYWORD_MATERIAL, iMaterialDeclarationIndex, sSKINKEYWORD_GROUP, iEthnicGroupIndex, sSKINKEYWORD_NAME);
						return psError;
					}

					// now iterate through the shader variants for this ethnic version of this material...  (is anyone reading this...?)
					//
					int iAlternateShaderIndex = 0;
					for (CGPValue *pValue = pEthnicGroup->GetPairs(); pValue; pValue = pValue->GetNext())
					{
						string strField(pValue->GetName());

						if (strField != sSKINKEYWORD_NAME)
						{
							// ... then it should be a shader...
							//
							string strShader(pValue->GetTopValue());

							CurrentSkins[strThisSkinFileName][strEthnicGroupName][strMaterialName].push_back(strShader);
						}
					}
				}
			}
		}
	}

	return psError;
}



// returns NULL if error, else usable (local) path...
//
static LPCSTR Skins_ModelNameToSkinPath(LPCSTR psModelFilename)
{
	static CString str;
			str = psModelFilename;
			str.MakeLower();
 	while  (str.Replace('\\','/')){}	

	// special case for anything in the "objects" dir...
	//
	int i = str.Find("models/objects/");
	if (i>=0)
	{
		str = str.Left(i+strlen("models/objects/"));
		str+= "skins";
		return (LPCSTR) str;
	}

	// or anything else...
	//
	i = str.Find("models/");
	if (i>=0)
	{
		str = str.Left(i+strlen("models/"));
		str+= "characters/skins";		
		return (LPCSTR) str;
	}

	return NULL;
}


// (params should both be const, but GP1 has crap proto args)
//
static bool Skins_ParseThisFile(CGenericParser2 &SkinParser,
								/*const*/char *psSkinFileData, string &strThisModelBaseName,
								CGPGroup	*&pFileGroup, CGPGroup *&pParseGroup_Prefs,
								LPCSTR psSkinFileName, G2SkinModelPrefs_t &G2SkinModelPrefs
								)
{
	bool bParseThisSkinFile = false;

	char *psDataPtr = psSkinFileData;
	
	if (SkinParser.Parse(&psDataPtr, true))
	{
		pFileGroup = SkinParser.GetBaseParseGroup();
		if (pFileGroup)
		{
			pParseGroup_Prefs = pFileGroup->FindSubGroup(sSKINKEYWORD_PREFS);//, true);
			if (pParseGroup_Prefs)
			{
				// see if our current model is amongst those that use this skin...
				//
				// look for a "models" field which says who uses this skin file...
				//
				CGPGroup *pParseGroup_ModelsUsing = pParseGroup_Prefs->FindSubGroup(sSKINKEYWORD_MODELSUSING);

				if (pParseGroup_ModelsUsing)
				{
					// ok, now search this subgroup for our model name...
					//
					for (CGPValue *pValue = pParseGroup_ModelsUsing->GetPairs(); pValue; pValue = pValue->GetNext())
					{			
						string str1 = pValue->GetName();
						string str2 = pValue->GetTopValue();

						if (str2 == strThisModelBaseName)
						{
							bParseThisSkinFile = true;						
							//break;	// do NOT stop scanning now
						}

						// record this model entry...
						//
						G2SkinModelPrefs[ psSkinFileName ][ str2 ] = 1;	// any old junk, as long as we make an entry
					}
				}
			}			
		}
	}
	else
	{
		ErrorBox(va("{} - Brace mismatch error in file \"%s\"!",psSkinFileName));
	}

	return bParseThisSkinFile;
}

typedef struct FT_s
{
	FILETIME ft;
	bool bValid;

	FT_s()
	{
		bValid = false;
	}
} FT_t;
typedef map <string,FT_t> SkinFileTimeDates_t;
static SkinFileTimeDates_t SkinFileTimeDates;

// returns true if at least one set of skin data was read, else false...
//
static bool Skins_Read(LPCSTR psModelFilename)
{
	LPCSTR psError = NULL;

	CWaitCursor;

	LPCSTR psSkinsPath = Skins_ModelNameToSkinPath(psModelFilename);	// eg "models/characters/skins"

	if (psSkinsPath)
	{
		string strThisModelBaseName(String_ToLower(Filename_WithoutExt(Filename_WithoutPath(psModelFilename))));

		char **ppsSkinFiles;
		int iSkinFiles;

		// scan for skin files...
		//
		ppsSkinFiles =	//ri.FS_ListFiles( "shaders", ".shader", &iSkinFiles );
						Sys_ListFiles(	va("%s%s",gamedir,psSkinsPath),// const char *directory, 
										".g2skin",	// const char *extension, 
										NULL,		// char *filter, 
										&iSkinFiles,// int *numfiles, 
										qfalse		// qboolean wantsubs 
										);

		if ( !ppsSkinFiles || !iSkinFiles )
		{
			CurrentSkins.clear();	
			CurrentSkinsSurfacePrefs.clear();
			return false;
		}

		if ( iSkinFiles > MAX_SKIN_FILES ) 
		{
			WarningBox(va("%d skin files found, capping to %d\n\n(tell me if this ever happens -Ste)", iSkinFiles, MAX_SKIN_FILES ));

			iSkinFiles = MAX_SKIN_FILES;
		}

		// load and parse skin files...
		//					
		// for now, I just scan each file and if it's out of date then I invalidate it's model-prefs info...
		//
		extern bool GetFileTime(LPCSTR psFileName, FILETIME &ft);
		for ( int i=0; i<iSkinFiles; i++)
		{
			bool bReParseThisFile = false;

			char sFileName[MAX_QPATH];
			LPCSTR psFileName = ppsSkinFiles[i];
			Com_sprintf( sFileName, sizeof( sFileName ), "%s/%s", psSkinsPath, psFileName );
			psFileName = &sFileName[0];
											  
			// have a go at getting this time/date stamp if not already present...
			//
			if (!SkinFileTimeDates[psFileName].bValid)
			{
				FILETIME ft;
				if (GetFileTime(psFileName, ft))
				{
					SkinFileTimeDates[psFileName].ft = ft;
					SkinFileTimeDates[psFileName].bValid = true;
				}
			}

			// now see if there's a valid time-stamp, and use it if so, else give up and re-scan all files...
			//
			if (SkinFileTimeDates[psFileName].bValid)
			{
				FILETIME ft;
				if (GetFileTime(psFileName, ft))
				{
					LONG l = CompareFileTime( &SkinFileTimeDates[psFileName].ft, &ft);

					bReParseThisFile = (l<0);
				}
				else
				{
					bReParseThisFile = true;
				}
			}
			else
			{
				bReParseThisFile = true;
			}	
			
			if (bReParseThisFile)
			{
				G2SkinModelPrefs[sFileName].clear();
			}
		}		
		
		if (1)//bReParseSkinFiles || !CurrentSkins.size())
		{
			CurrentSkins.clear();	
			CurrentSkinsSurfacePrefs.clear();

			char *buffers[MAX_SKIN_FILES]={0};
//			long iTotalBytesLoaded = 0;
			for ( int i=0; i<iSkinFiles && !psError; i++ )
			{
				char sFileName[MAX_QPATH];

				string strThisSkinFile(ppsSkinFiles[i]);

				Com_sprintf( sFileName, sizeof( sFileName ), "%s/%s", psSkinsPath, strThisSkinFile.c_str() );
				StatusMessage( va("Scanning skin %d/%d: \"%s\"...",i+1,iSkinFiles,sFileName));

				//ri.Printf( PRINT_ALL, "...loading '%s'\n", sFileName );

				bool _bDiskLoadOccured = false;	// debug use only, but wtf?

#define LOAD_SKIN_FILE									\
/*iTotalBytesLoaded += */ ri.FS_ReadFile( sFileName, (void **)&buffers[i] );	\
if ( !buffers[i] )										\
{														\
	CurrentSkins.clear();								\
	CurrentSkinsSurfacePrefs.clear();					\
														\
	ri.Error( ERR_DROP, "Couldn't load %s", sFileName );\
}														\
_bDiskLoadOccured = true;


				// see if we should pay attention to the contents of this file...
				//
				CGPGroup	*pFileGroup			= NULL;
				CGPGroup *pParseGroup_Prefs	= NULL;
				CGenericParser2 SkinParser;
				//
				bool bParseThisFile = false;
				//
				// if we have any information about this skin file as regards what models it refers to, use the info...
				//
				if (G2SkinModelPrefs[sFileName].size())
				{
					map<string, int>::iterator it = G2SkinModelPrefs[sFileName].find( strThisModelBaseName );
					if (it != G2SkinModelPrefs[sFileName].end())
					{
						// this skin file contains this entry, so just check that we can setup the parse groups ok...
						//
						LOAD_SKIN_FILE;

						char *psDataPtr = buffers[i];
						if (SkinParser.Parse(&psDataPtr, true))
						{
							pFileGroup = SkinParser.GetBaseParseGroup();
							if (pFileGroup)
							{
								pParseGroup_Prefs = pFileGroup->FindSubGroup(sSKINKEYWORD_PREFS);//, true);
								if (pParseGroup_Prefs)
								{
									bParseThisFile = true;
								}
							}
						}
						else
						{
							ErrorBox(va("{} - Brace mismatch error in file \"%s\"!",sFileName));
						}
					}
				}
				else
				{
					// no size info for this file, so check it manually...
					//
					LOAD_SKIN_FILE;

					if (Skins_ParseThisFile(SkinParser, buffers[i], strThisModelBaseName, pFileGroup, pParseGroup_Prefs, 
											sFileName, G2SkinModelPrefs)
						)
					{
						bParseThisFile = true;
					}
				}

				if (bParseThisFile)
				{
					psError = Skins_Parse( strThisSkinFile, pFileGroup, pParseGroup_Prefs);
					if (psError)
					{
						ErrorBox(va("Skins_Read(): Error reading file \"%s\"!\n\n( Skins will be ignored for this model )\n\nError was:\n\n",sFileName,psError));
					}
				}
				else
				{
					//OutputDebugString(va("Skipping parse of file \"%s\" %s\n",sFileName, _bDiskLoadOccured?"":"( and no load! )"));
				}
			}
			//
			// free loaded skin files...
			//
			for ( int i=0; i<iSkinFiles; i++ )
			{
				if (buffers[i])
				{
					ri.FS_FreeFile( buffers[i] );
				}
			}
		}

		StatusMessage(NULL);
		Sys_FreeFileList( ppsSkinFiles );
	}
	else
	{
		CurrentSkins.clear();	
		CurrentSkinsSurfacePrefs.clear();
	}

	if (psError)
	{
		return false;
	}

	return !!(CurrentSkins.size());
}



// ask if there are any skins available for the supplied GLM model path...
//
bool Skins_FilesExist(LPCSTR psModelFilename)
{
	return Skins_Read(psModelFilename);
}



static void Skins_ApplyVariant(ModelContainer_t *pContainer, SkinSet_t::iterator itEthnic, ShadersForMaterial_t::iterator itMaterialShaders, string strMaterialName, int iVariant)
{
	if (itMaterialShaders != (*itEthnic).second.end())
	{
		LPCSTR psShaderName	= (*itMaterialShaders).second[iVariant].c_str();	// shader name to load from skinset

		pContainer->MaterialShaders[strMaterialName] = psShaderName;

		LPCSTR psLocalTexturePath = R_FindShader( psShaderName );		// shader->texture name

		if (psLocalTexturePath && strlen(psLocalTexturePath))
		{
			TextureHandle_t hTexture = TextureHandle_ForName( psLocalTexturePath );

			if (hTexture == -1)
			{
				hTexture = Texture_Load(psLocalTexturePath);
			}

			GLuint uiBind = Texture_GetGLBind( hTexture );

			pContainer->MaterialBinds[strMaterialName] = uiBind;
		}
	}
}

static void Skins_SetSurfacePrefs(ModelContainer_t *pContainer, StringVector_t &SurfaceNames, SurfaceOnOff_t eStatus)
{
	bool bFailureOccured = false;

	for (int i=0; i<SurfaceNames.size() && !bFailureOccured; i++)
	{
		string strSurfaceName = SurfaceNames[i];

		if (!Model_GLMSurface_SetStatus( pContainer->hModel, strSurfaceName.c_str(), eStatus))
			bFailureOccured = true;
	}
}

// this fills in a modelcontainer's "MaterialBinds" and "MaterialShaders" fields (registering textures etc)
//	based on the skinset pointed at by pContainer->SkinSets and strSkinFile
//
bool Skins_ApplySkinFile(ModelContainer_t *pContainer, string strSkinFile, string strEthnic, 
						 bool bApplySurfacePrefs, bool bDefaultSurfaces,
						 string strMaterial, int iVariant)	// optional params, else "" and -1
{
	CWaitCursor wait;

	bool bReturn = true;


	// new bit, if surface prefs active, then enable/disable model surfaces...
	//
	if (bApplySurfacePrefs || bDefaultSurfaces)
	{
		Model_GLMSurfaces_DefaultAll(pContainer->hModel);
	}

	if (bApplySurfacePrefs)
	{
		// now go through surface prefs list and turn them on/off accordingly...
		//
		SkinSetsSurfacePrefs_t::iterator itSurfPrefs = pContainer->SkinSetsSurfacePrefs.find(strSkinFile.c_str());
		if (itSurfPrefs != pContainer->SkinSetsSurfacePrefs.end())
		{
			SurfaceOnOffPrefs_t &prefs = (*itSurfPrefs).second;

			Skins_SetSurfacePrefs(pContainer, prefs.vSurfacesOn, SURF_ON);
			Skins_SetSurfacePrefs(pContainer, prefs.vSurfacesOff, SURF_OFF);
			Skins_SetSurfacePrefs(pContainer, prefs.vSurfacesOffNoChildren, SURF_NO_DESCENDANTS);
		}	
	}

	// setup a map of material names first, which we can then read from and lookup shaders for...
	//
	if (iVariant != -1)
	{
		// just clear this one...
		//
		assert(!strMaterial.empty());
		pContainer->MaterialShaders [strMaterial] = "";
		pContainer->MaterialBinds	[strMaterial] = (GLuint) 0;
	}
	else
	{
		pContainer->strCurrentSkinFile	= strSkinFile;
		pContainer->strCurrentSkinEthnic= strEthnic;

		pContainer->MaterialBinds.clear();
		pContainer->MaterialShaders.clear();

		for (int iSurface = 0; iSurface<pContainer->iNumSurfaces; iSurface++)
		{
			// when we're at this point we know it's GLM model, and that the shader name is in fact a material name...
			//
			LPCSTR psMaterialName = GLMModel_GetSurfaceShaderName( pContainer->hModel, iSurface );

			pContainer->MaterialShaders	[psMaterialName] = "";			// just insert the key for now, so the map<> is legit.
			pContainer->MaterialBinds	[psMaterialName] = (GLuint) 0;	// default to gl-white-notfound texture
		}
	}

	SkinSets_t::iterator itSkins = pContainer->SkinSets.find(strSkinFile);
	if (itSkins != pContainer->SkinSets.end())
	{
		SkinSet_t::iterator itEthnic = (*itSkins).second.find(strEthnic);

		// new bit, auto-hack the ethnic-variant if we don't find it to use the first one (so lazy NPC->MVS code works)...
		//
		if (itEthnic == (*itSkins).second.end())
		{				
			itEthnic = (*itSkins).second.begin();
			string strNewEthnic = (*itEthnic).first;
			pContainer->strCurrentSkinEthnic= strNewEthnic;
		}

		if (itEthnic != (*itSkins).second.end())
		{
			// ok, we've found the ethnic skin set specified, now use it to look up the shaders for the materials...
			//
			// query these materials, and find the appropriate skinfile/ethnic shader (variant 0 for now)...
			//			
			if (iVariant != -1)
			{
				ShadersForMaterial_t::iterator itMaterialShaders = (*itEthnic).second.find(strMaterial);

				Skins_ApplyVariant(pContainer, itEthnic, itMaterialShaders, strMaterial, iVariant);
			}
			else
			{
				for (MappedString_t::iterator itMaterial = pContainer->MaterialShaders.begin(); itMaterial != pContainer->MaterialShaders.end(); ++itMaterial)
				{
					LPCSTR psMaterialName = (*itMaterial).first.c_str();

					// find this material in the skinset...
					//
					ShadersForMaterial_t::iterator itMaterialShaders = (*itEthnic).second.find((*itMaterial).first);

					Skins_ApplyVariant(pContainer, itEthnic, itMaterialShaders, psMaterialName, 0);
				}
			}
		}
	}

	return bReturn;
}

// sets up valid skin tables based on first entries loaded, also registers/binds appropriate textures...
//
void Skins_ApplyDefault(ModelContainer_t *pContainer)
{
	for (SkinSets_t::iterator itSkins = pContainer->SkinSets.begin(); itSkins != pContainer->SkinSets.end(); ++itSkins)
	{
		string strCurrentSkinFile = (*itSkins).first;

		for (SkinSet_t::iterator itEthnic = (*itSkins).second.begin(); itEthnic != (*itSkins).second.end(); ++itEthnic)
		{
			string strCurrentEthnic = (*itEthnic).first;	// eg "white"

			Skins_ApplySkinFile(pContainer, strCurrentSkinFile, strCurrentEthnic, true, true);
			return;	// :-)
		}
	}
}


// returns true if at least one set of skin data was read, else false...
//
bool Skins_Read(LPCSTR psModelFilename, ModelContainer_t *pContainer)
{
	if (Skins_Read(psModelFilename))
	{
		pContainer->SkinSets = CurrentSkins;	// huge stl-nested-map copy		
		pContainer->SkinSetsSurfacePrefs = CurrentSkinsSurfacePrefs;
		return true;
	}

	return false;
}


typedef map <string, StringVector_t> EthnicMaterials_t;
typedef map < string, EthnicMaterials_t > SkinFileMaterialsMissing_t;	// [skinfile][ethnic][missing materials]
static void SkinSet_Validate_BuildList(StringSet_t &strUniqueSkinShaders, SkinSets_t::iterator itSkins, StringSet_t &MaterialsPresentInModel, SkinFileMaterialsMissing_t &SkinFileMaterialsMissing)
{	
	string strSkinName((*itSkins).first);	// eg "thug1"

	// race variants...
	//
	for (SkinSet_t::iterator itEthnic = (*itSkins).second.begin(); itEthnic != (*itSkins).second.end(); ++itEthnic)
	{
		string strEthnicName((*itEthnic).first);	// eg "white"

		map <string, int> MaterialsDefinedInThisEthnicVariant;

		// materials...
		//			
		for (ShadersForMaterial_t::iterator itMaterial = (*itEthnic).second.begin(); itMaterial != (*itEthnic).second.end(); ++itMaterial)
		{
			string strMaterialName((*itMaterial).first);	// eg "face"

			// keep a note of all materials defined in this file...
			//
			MaterialsDefinedInThisEthnicVariant[strMaterialName]=1;	// 1 = whatever

			// available shader variants for this material...
			//
			for (int iShaderVariantIndex=0; iShaderVariantIndex<(*itMaterial).second.size(); iShaderVariantIndex++)
			{
				string strShaderVariantName((*itMaterial).second[iShaderVariantIndex]);	// eg "models/characters/average_sleeves/face"

				strUniqueSkinShaders.insert(strUniqueSkinShaders.end(),strShaderVariantName);
			}			
		}

		// now scan through the materials defined in this ethnic variant, and see if there were any materials 
		//	mentioned/referenced by the model which weren't defined here...
		//
		for (StringSet_t::iterator itMaterialsReferenced = MaterialsPresentInModel.begin(); itMaterialsReferenced != MaterialsPresentInModel.end(); ++itMaterialsReferenced)
		{
			string strModelMaterial = (*itMaterialsReferenced);

			if (!strModelMaterial.empty() && stricmp(strModelMaterial.c_str(),"[NoMaterial]"))
			{
				if (MaterialsDefinedInThisEthnicVariant.find(strModelMaterial) == MaterialsDefinedInThisEthnicVariant.end())
				{
					SkinFileMaterialsMissing[strSkinName][strEthnicName].push_back(strModelMaterial);
					//
//					OutputDebugString(va("strModelMaterial = %s\n",strModelMaterial.c_str()));
				}
			}
		}
	}
}



// UI-code...
//	
bool Skins_FileHasSurfacePrefs(ModelContainer_t *pContainer, LPCSTR psSkin)
{
	SkinSetsSurfacePrefs_t::iterator itSurfPrefs = pContainer->SkinSetsSurfacePrefs.find(psSkin);
	if (itSurfPrefs != pContainer->SkinSetsSurfacePrefs.end())
	{
		SurfaceOnOffPrefs_t &prefs = (*itSurfPrefs).second;

		return	prefs.vSurfacesOn.size()			||
				prefs.vSurfacesOff.size()			||
				prefs.vSurfacesOffNoChildren.size()
				;
	}
	
	return false;
}

bool Skins_ApplyEthnic( ModelContainer_t *pContainer, LPCSTR psSkin, LPCSTR psEthnic, bool bApplySurfacePrefs, bool bDefaultSurfaces)
{
	return Skins_ApplySkinFile(pContainer, psSkin, psEthnic, bApplySurfacePrefs, bDefaultSurfaces);
}

bool Skins_ApplySkinShaderVariant(ModelContainer_t *pContainer, LPCSTR psSkin, LPCSTR psEthnic, LPCSTR psMaterial, int iVariant )
{
	return Skins_ApplySkinFile(pContainer, psSkin, psEthnic, false, false, psMaterial, iVariant );
}


// returns false for "at least one shader or material missing", else true for all ok...
//
extern bool g_bReportImageLoadErrors;
bool Skins_Validate( ModelContainer_t *pContainer, int iSkinNumber )
{
	bool bReturn = true;	
	bool bPREV_bReportImageLoadErrors = g_bReportImageLoadErrors;
										g_bReportImageLoadErrors = false;

	bool bCheckMissingMaterials = true;//GetYesNo("Check for materials referenced by model but missing in skinfile(s)?\n\n( Note: This can give false alarms for skins which don't use (eg) \"scarf\" )");

	// first, build up a list of all model materials...
	//	
	StringSet_t MaterialsPresentInModel;
	for (int iSurface = 0; iSurface < pContainer->iNumSurfaces; iSurface++)
	{
		bool bOnOff = GLMModel_SurfaceIsON(pContainer->hModel, iSurface);

		if (bOnOff)
		{
			LPCSTR psMaterial = GLMModel_GetSurfaceShaderName( pContainer->hModel, iSurface);

			MaterialsPresentInModel.insert(MaterialsPresentInModel.end(),psMaterial);
		}
	}

	// build up a list of shaders used...
	//	
	StringSet_t UniqueSkinShaders;	
	SkinFileMaterialsMissing_t SkinFileMaterialsMissing;
	int iThisSkinIndex = 0;
	for (SkinSets_t::iterator itSkins = pContainer->SkinSets.begin(); itSkins != pContainer->SkinSets.end(); ++itSkins, iThisSkinIndex++)
	{
		if (iSkinNumber == iThisSkinIndex || iSkinNumber == -1)
		{
			SkinSet_Validate_BuildList(UniqueSkinShaders, itSkins, MaterialsPresentInModel, SkinFileMaterialsMissing);
		}
	}

	// now process the unique list we've just built...
	//
	CWaitCursor wait;
	string strFoundList;
	string strNotFoundList;
	int iUniqueIndex = 0;
	for (StringSet_t::iterator it = UniqueSkinShaders.begin(); it != UniqueSkinShaders.end(); ++it, iUniqueIndex++)
	{			
		string strShader(*it);

		StatusMessage(va("Processing shader %d/%d: \"%s\"\n",iUniqueIndex,UniqueSkinShaders.size(),strShader.c_str()));

		OutputDebugString(va("Unique: \"%s\"... ",strShader.c_str()));

		int iTextureHandle = Texture_Load(strShader.c_str(), true);	// bInhibitStatus

		GLuint uiGLBind = Texture_GetGLBind( iTextureHandle );

		if (uiGLBind == 0)
		{
			OutputDebugString("NOT FOUND\n");
			
			strNotFoundList += strShader;
			strNotFoundList += "\n";
		}
		else
		{
			OutputDebugString("found\n");

			strFoundList += strShader;
			strFoundList += "\n";
		}
	}

	StatusMessage(NULL);

	
	// see if we were missing any model materials in these skins...
	//
	CString strModelMaterialsMissing;
	if (SkinFileMaterialsMissing.size())
	{
		for (SkinFileMaterialsMissing_t::iterator itSkinFileMaterialsMissing = SkinFileMaterialsMissing.begin(); itSkinFileMaterialsMissing != SkinFileMaterialsMissing.end(); ++itSkinFileMaterialsMissing)
		{
			string strSkinFileName((*itSkinFileMaterialsMissing).first);

			if (iSkinNumber == -1)
			{
				strModelMaterialsMissing += va("\nSkin \"%s\":\n",strSkinFileName.c_str());
			}
																					 
			for (EthnicMaterials_t::iterator itSkinFile = (*itSkinFileMaterialsMissing).second.begin(); itSkinFile != (*itSkinFileMaterialsMissing).second.end(); ++itSkinFile)
			{
				string strEthnicFileName((*itSkinFile).first);

				strModelMaterialsMissing += va("Ethnic \"%s\":   ",strEthnicFileName.c_str());

				StringVector_t& MaterialStrings = (*itSkinFile).second;

				for (int iMaterial = 0; iMaterial != MaterialStrings.size(); ++iMaterial)
				{
					string strMaterial(MaterialStrings[iMaterial]);

					strModelMaterialsMissing += va("%s\"%s\"",(iMaterial==0)?"":", ",strMaterial.c_str());
				}
				strModelMaterialsMissing += "\n";
			}
		}
	}
	if (!strModelMaterialsMissing.IsEmpty())
	{
		if (iSkinNumber == -1)
		{
			strModelMaterialsMissing.Insert(0, "One or more skin files are missing some material definitions referenced by this model's currently-active surfaces.\nList follows...\n\n");
		}
		else
		{
			strModelMaterialsMissing.Insert(0, "This skin file is missing one or more material definitions referenced by this model's currently-active surfaces.\nList follows...\n\n");
		}
	}

	
	if (!strModelMaterialsMissing.IsEmpty())
	{
		if (bCheckMissingMaterials)
		{
			WarningBox(va("Summary Part 1: Missing materials\n\n%s",(LPCSTR)strModelMaterialsMissing));
		}
	}


	// Now output results...

	// If too many lines to fit on screen (which is now happening), send 'em to notepad instead...
	//
	// ( tacky way of counting lines...)
	CString strTackyCount(strNotFoundList.c_str());
			strTackyCount += strFoundList.c_str();

	int iLines = strTackyCount.Replace('\n','?');	// :-)

	#define MAX_BOX_LINES_HERE 50

	if (strNotFoundList.empty())
	{
		if (iLines > MAX_BOX_LINES_HERE)
		{
			if (GetYesNo(va("All shaders found...    :-)\n\nList has > %d entries, send to Notepad?",MAX_BOX_LINES_HERE)))
			{
				SendStringToNotepad(va("All shaders found...    :-)\n\nList follows:\n\n%s",strFoundList.c_str()),"found_shaders.txt");
			}
		}
		else
		{
			InfoBox(va("All shaders found...    :-)\n\nList follows:\n\n%s",strFoundList.c_str()));
		}
	}
	else
	{
		if (iLines > MAX_BOX_LINES_HERE)
		{
			if (GetYesNo(va("Some missing shader, some found, but list is > %d entries, send to Notepad?",MAX_BOX_LINES_HERE)))
			{
				SendStringToNotepad(va("Missing shaders:\n\n%s\n\nFound shaders:\n\n%s",strNotFoundList.c_str(),strFoundList.c_str()),"found_shaders.txt");
			}
		}
		else
		{
			WarningBox(va("Missing shaders:\n\n%s\n\nFound shaders:\n\n%s",strNotFoundList.c_str(),strFoundList.c_str()));
		}
		bReturn = false;
	}


	g_bReportImageLoadErrors = bPREV_bReportImageLoadErrors;
	return bReturn;
}



bool Skins_ApplyToTree (HTREEITEM hTreeItem_Parent, ModelContainer_t *pContainer)
{
	bool bReturn = false;

	if (pContainer->SkinSets.size())
	{
		bReturn = true;

		TreeItemData_t	TreeItemData={0};
						TreeItemData.iItemType		= TREEITEMTYPE_SKINSHEADER;
						TreeItemData.iModelHandle	= pContainer->hModel;

		HTREEITEM hTreeItem_SkinsHeader = ModelTree_InsertItem("G2Skins available", hTreeItem_Parent, TreeItemData.uiData);

		// skins...
		//
		int iSkinNumber = 0;
		for (SkinSets_t::iterator itSkins = pContainer->SkinSets.begin(); itSkins != pContainer->SkinSets.end(); ++itSkins, iSkinNumber++)
		{
			string strSkinName((*itSkins).first);	// eg "thug1"

			TreeItemData.iItemNumber	= iSkinNumber;
			TreeItemData.iItemType		= TREEITEMTYPE_SKIN;
			
			HTREEITEM hTreeItem_ThisSkin = ModelTree_InsertItem(strSkinName.c_str(), hTreeItem_SkinsHeader, TreeItemData.uiData);
			
			// race variants...
			//
			int iEthnicNumber = 0;
			for (SkinSet_t::iterator itEthnic = (*itSkins).second.begin(); itEthnic != (*itSkins).second.end(); ++itEthnic, iEthnicNumber++)
			{
				string strEthnicName((*itEthnic).first);	// eg "white"

				TreeItemData.iItemNumber	= iEthnicNumber;
				TreeItemData.iItemType		= TREEITEMTYPE_SKINETHNIC;
				
				HTREEITEM hTreeItem_ThisEthnic = ModelTree_InsertItem(strEthnicName.c_str(), hTreeItem_ThisSkin, TreeItemData.uiData);

				// materials...
				//
				int iMaterialNumber = 0;
				for (ShadersForMaterial_t::iterator itMaterial = (*itEthnic).second.begin(); itMaterial != (*itEthnic).second.end(); ++itMaterial, iMaterialNumber++)
				{
					string strMaterialName((*itMaterial).first);	// eg "face"

					TreeItemData.iItemNumber	= iMaterialNumber;
					TreeItemData.iItemType		= TREEITEMTYPE_SKINMATERIAL;

					HTREEITEM hTreeItem_ThisMaterial = ModelTree_InsertItem(strMaterialName.c_str(), hTreeItem_ThisEthnic, TreeItemData.uiData);

					// available shader variants for this material...
					//
					for (int iShaderVariantIndex=0; iShaderVariantIndex<(*itMaterial).second.size(); iShaderVariantIndex++)
					{
						string strShaderVariantName((*itMaterial).second[iShaderVariantIndex]);	// eg "models/characters/average_sleeves/face"

						TreeItemData.iItemNumber	= iShaderVariantIndex;
						TreeItemData.iItemType		= TREEITEMTYPE_SKINMATERIALSHADER;

						HTREEITEM hTreeItem_ThisShaderVariant = ModelTree_InsertItem(strShaderVariantName.c_str(), hTreeItem_ThisMaterial, TreeItemData.uiData);
					}
				}
			}
		}
	}

	return bReturn;
}




// called directly from renderer...
//
extern ModelContainer_t* gpContainerBeingRendered;
GLuint AnySkin_GetGLBind( ModelHandle_t hModel, LPCSTR psMaterialName, LPCSTR psSurfaceName )
{
	ModelContainer_t *pContainer = gpContainerBeingRendered;	// considerably faster during rendering process :-)

	if (pContainer == NULL)
	{
		pContainer = ModelContainer_FindFromModelHandle(hModel);
	}

	if (pContainer)
	{
		if (pContainer->SkinSets.size())
		{
			return pContainer->MaterialBinds[psMaterialName];
		}
		return OldSkins_GetGLBind(pContainer,psSurfaceName);
	}

	return (GLuint) 0;
}

/////////////////// eof /////////////////


