//Filename:-	oldskins.cpp
//
// module containing code for old ID/EF1/CHC style skins...
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

////#include "GenericParser.h"
#include "oldskins.h"


// file format:  (dead simple)
//
//armor_chest,models/players/stormtrooper/torso_legs.tga
//armor_neck_augment,models/players/stormtrooper/torso_legs.tga
//body_torso,models/players/stormtrooper/torso_legs.tga

OldSkinSets_t OldSkinsFound;


// returns NULL for all ok, else error string...
//
LPCSTR OldSkins_Parse(LPCSTR psSkinName, LPCSTR psText)
{
	CString strText(psText);
			strText.Replace(va("%c%c",0x0A,0x0D),va("%c",0x0A));	// file was read in binary mode, so account for 0x0A/0x0D pairs
			strText.Replace("\t","");
			strText.Replace(" ","");
			strText.MakeLower();
	
	while (!strText.IsEmpty())
	{
		CString strThisLine;
		
		int iLoc = strText.Find('\n');

		if (iLoc == -1)
		{
			strThisLine = strText;
			strText.Empty();
		}
		else
		{
			strThisLine = strText.Left(iLoc);
			strText = strText.Mid(iLoc+1);
		}

		if (!strThisLine.IsEmpty())
		{
			iLoc = strThisLine.Find(',');
			if (iLoc != -1)
			{	
				CString strSurfaceName	(strThisLine.Left(iLoc));						
				CString strTGAName		(strThisLine.Mid (iLoc+1));

				strSurfaceName.TrimLeft();
				strSurfaceName.TrimRight();
				strTGAName.TrimLeft();
				strTGAName.TrimRight();

				//
				// new bit to cope with faulty ID skin files, where they have spurious lines like "tag_torso,"
				//
				// ("tag" is "*" in ghoul2)
				//
				if (strSurfaceName.GetAt(0) == '*' || strTGAName.IsEmpty())
				{
					// crap line, so ignore it...
				}
				else
				{
					OldSkinsFound[psSkinName].push_back(StringPairVector_t::value_type((LPCSTR)strSurfaceName,(LPCSTR)strTGAName));
				}
			}
			else
			{
				return va("Error parsing line \"%s\" in skin \"%s\"!",(LPCSTR)strThisLine,psSkinName);
			}			
		}
	}

	return NULL;
}

// converts stuff like "<path>/stormtrooper_blue.skin" to "blue"...
//
LPCSTR OldSkins_FilenameToSkinDescription(string strLocalSkinFileName)
{
	static char sTemp[1024];

	CString strSkinName(Filename_WithoutPath(Filename_WithoutExt(strLocalSkinFileName.c_str())));

	strSkinName.MakeLower();
	int iLoc = strSkinName.Find('_');
	if (iLoc != -1)
	{
		strSkinName = strSkinName.Mid(iLoc+1);
	}

	sprintf(sTemp,strSkinName);
	return sTemp;
}


// returns true if at least one set of skin data was read, else false...
//
static bool OldSkins_Read(LPCSTR psLocalFilename_GLM)
{
	LPCSTR psError = NULL;

	CWaitCursor;

	OldSkinsFound.clear();
	
	LPCSTR psSkinsPath = va("%s%s",gamedir,Filename_PathOnly(psLocalFilename_GLM));

	if (psSkinsPath)
	{
		CString strSkinFileMustContainThisName( Filename_WithoutPath( Filename_WithoutExt( psLocalFilename_GLM )) );	
				strSkinFileMustContainThisName += "_";	// eg: "turret_canon_"
		char **ppsSkinFiles;
		int iSkinFiles;

		// scan for skin files...
		//
		ppsSkinFiles =	//ri.FS_ListFiles( "shaders", ".shader", &iSkinFiles );
						Sys_ListFiles(	psSkinsPath,// const char *directory, 
										".skin",	// const char *extension, 
										NULL,		// char *filter, 
										&iSkinFiles,// int *numfiles, 
										qfalse		// qboolean wantsubs 
										);

		if ( !ppsSkinFiles || !iSkinFiles )
		{
			return false;
		}

		if ( iSkinFiles > MAX_SKIN_FILES ) 
		{
			WarningBox(va("%d skin files found, capping to %d\n\n(tell me if this ever happens -Ste)", iSkinFiles, MAX_SKIN_FILES ));

			iSkinFiles = MAX_SKIN_FILES;
		}

		// load and parse skin files...
		//
		char *buffers[MAX_SKIN_FILES] = {0};
		long iTotalBytesLoaded = 0;
		int i = 0;

		for ( i=0; i<iSkinFiles && !psError; i++ )
		{
			char sFileName[MAX_QPATH];

			string strLocalSkinFileName(ppsSkinFiles[i]);

			// only look at skins that begin "modelname_skinvariation" for a given "modelname_"
			if (!strnicmp(strSkinFileMustContainThisName,strLocalSkinFileName.c_str(),strlen(strSkinFileMustContainThisName)))
			{
				Com_sprintf( sFileName, sizeof( sFileName ), "%s/%s", Filename_PathOnly(psLocalFilename_GLM), strLocalSkinFileName.c_str() );
				//ri.Printf( PRINT_ALL, "...loading '%s'\n", sFileName );

				iTotalBytesLoaded += ri.FS_ReadFile( sFileName, (void **)&buffers[i] );

				if ( !buffers[i] ) {
					ri.Error( ERR_DROP, "Couldn't load %s", sFileName );
				}

				char *psDataPtr = buffers[i];

				psError = OldSkins_Parse( OldSkins_FilenameToSkinDescription(strLocalSkinFileName), psDataPtr);
				if (psError)
				{
					ErrorBox(va("Skins_Read(): Error reading file \"%s\"!\n\n( Skins will be ignored for this model )\n\nError was:\n\n%s",sFileName,psError));
					OldSkinsFound.clear();
				}
			}
		}

		// free loaded skin files...
		//
		for ( i=0; i<iSkinFiles; i++ )
		{
			if (buffers[i])
			{
				ri.FS_FreeFile( buffers[i] );
			}
		}

		// ... and file list...
		//		
		Sys_FreeFileList( ppsSkinFiles );
	}

	if (psError)
	{
		return false;
	}

	return !!(OldSkinsFound.size());
}



int DaysSinceCompile(void);
bool OldSkins_FilesExist(LPCSTR psLocalFilename_GLM)
{		
	return OldSkins_Read(psLocalFilename_GLM);
}

bool OldSkins_Read(LPCSTR psLocalFilename_GLM, ModelContainer_t *pContainer)
{
	if (OldSkins_Read(psLocalFilename_GLM))
	{
		pContainer->OldSkinSets = OldSkinsFound;	// huge nested stl-copy

/*		for (OldSkinSets_t::iterator it = OldSkinsFound.begin(); it != OldSkinsFound.end(); ++it)
		{
			string strFile = (*it).first;
			StringPairVector_t &blah = (*it).second;

			for (StringPairVector_t::iterator it2 = blah.begin(); it2 != blah.end(); ++it2)
			{
				string str1 = (*it2).first;
				string str2 = (*it2).second;

				OutputDebugString(va("Skin %s:  %s, %s\n",strFile.c_str(),str1.c_str(),str2.c_str()));
				OutputDebugString(__DATE__);
				OutputDebugString("\n");
			}
		}
*/
		return true;
	}

	return false;
}



// psSkinName = "blue", or "default" etc...
//
// this fills in a modelcontainer's "MaterialBinds" and "MaterialShaders" fields (registering textures etc)
//	based on the skinset pointed at by pContainer->OldSkinSets and strSkinFile
//
bool OldSkins_Apply( ModelContainer_t *pContainer, LPCSTR psSkinName )
{
	CWaitCursor wait;

	bool bReturn = true;

	pContainer->strCurrentSkinFile	= psSkinName;
//	pContainer->strCurrentSkinEthnic= "";

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

//typedef vector< pair<string,string> > StringPairVector_t;
//typedef map<string,StringPairVector_t> OldSkinSets_t;	// map key = (eg) "blue", string-pairs

	OldSkinSets_t::iterator itOldSkins = pContainer->OldSkinSets.find(psSkinName);
	if (itOldSkins != pContainer->OldSkinSets.end())
	{
		StringPairVector_t &StringPairs = (*itOldSkins).second;

		for (int iSkinEntry = 0; iSkinEntry < StringPairs.size(); iSkinEntry++)
		{
			LPCSTR psMaterialName = StringPairs[iSkinEntry].first.c_str();
			LPCSTR psShaderName   = StringPairs[iSkinEntry].second.c_str();

			pContainer->MaterialShaders[psMaterialName] = psShaderName;

//			LPCSTR psLocalTexturePath = R_FindShader( psShaderName );		// shader->texture name
//			if (psLocalTexturePath && strlen(psLocalTexturePath))
//			{
//				TextureHandle_t hTexture = TextureHandle_ForName( psLocalTexturePath );
//
//				if (hTexture == -1)
//				{
//					hTexture = Texture_Load(psLocalTexturePath);
//				}
//
//				GLuint uiBind = Texture_GetGLBind( hTexture );
//
//				pContainer->MaterialBinds[psMaterialName] = uiBind;
//			}
			TextureHandle_t hTexture = Texture_Load(psShaderName);
			GLuint uiBind = Texture_GetGLBind( hTexture );
			pContainer->MaterialBinds[psMaterialName] = uiBind;
		}
	}

	return bReturn;
}



bool OldSkins_ApplyToTree(HTREEITEM hTreeItem_Parent, ModelContainer_t *pContainer)
{
	bool bReturn = false;

	if (pContainer->OldSkinSets.size())
	{
		bReturn = true;

		TreeItemData_t	TreeItemData={0};
						TreeItemData.iItemType		= TREEITEMTYPE_OLDSKINSHEADER;
						TreeItemData.iModelHandle	= pContainer->hModel;

		HTREEITEM hTreeItem_SkinsHeader = ModelTree_InsertItem("Skins available", hTreeItem_Parent, TreeItemData.uiData);

		// skins...
		//
		int iSkinNumber = 0;
		for (OldSkinSets_t::iterator itSkins = pContainer->OldSkinSets.begin(); itSkins != pContainer->OldSkinSets.end(); ++itSkins, iSkinNumber++)
		{
			string strSkinName((*itSkins).first);	// eg "blue"

			TreeItemData.iItemNumber	= iSkinNumber;
			TreeItemData.iItemType		= TREEITEMTYPE_OLDSKIN;
			
			HTREEITEM hTreeItem_ThisSkin = ModelTree_InsertItem(strSkinName.c_str(), hTreeItem_SkinsHeader, TreeItemData.uiData);
/*			
			// body parts...
			//
			StringPairVector_t &StringPairs = (*itSkins).second);
			int iMaterialNumber = 0;
			for (StringPairVector_t::iterator itMaterial = StringPairs.begin(); itMaterial != StringPairs.end(); ++itMaterial, iMaterialNumber++)
			{
				string strMaterialName((*itMaterial).first);	// eg "face"
				string strShaderName  ((*itMaterial).second);	// eg "face"

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
*/
		}
	}

	return bReturn;
}


// sets up valid skin tables based on first entries loaded, also registers/binds appropriate textures...
//
void OldSkins_ApplyDefault(ModelContainer_t *pContainer)
{
	string strCurrentSkin;

	// look for one called "default" first...
	//
	OldSkinSets_t::iterator itSkins = pContainer->OldSkinSets.find("default");
	if (itSkins != pContainer->OldSkinSets.end())
	{
		strCurrentSkin = (*itSkins).first;
	}
	else
	{
		// just use the first one we have...
		//
		for (itSkins = pContainer->OldSkinSets.begin(); itSkins != pContainer->OldSkinSets.end();)
		{
			strCurrentSkin = (*itSkins).first;
			break;
		}
	}

	// apply it, but don't barf if there wasn't one...
	//
	if (!strCurrentSkin.empty())
	{
		OldSkins_Apply(pContainer, strCurrentSkin.c_str());
	}
}


GLuint OldSkins_GetGLBind(ModelContainer_t *pContainer, LPCSTR psSurfaceName)
{
/*	debug only, do NOT leave in or massive performance hit!!

	int iSize = pContainer->MaterialBinds.size();
	for (MaterialBinds_t::iterator it = pContainer->MaterialBinds.begin(); it != pContainer->MaterialBinds.end(); ++it)
	{
		string str = (*it).first;
		GLuint gl  = (*it).second;

		OutputDebugString(va("%s (%d)\n",str.c_str(), gl));
	}
*/
	return pContainer->MaterialBinds[psSurfaceName];
}


extern bool g_bReportImageLoadErrors;
bool OldSkins_Validate( ModelContainer_t *pContainer, int iSkinNumber )
{
	bool bReturn = true;	
	bool bPREV_bReportImageLoadErrors = g_bReportImageLoadErrors;
										g_bReportImageLoadErrors = false;
	int iSurface_Other = 0;

	// build up a list of shaders used...
	//	
	StringSet_t UniqueSkinShaders;	
	//SkinFileMaterialsMissing_t SkinFileMaterialsMissing;
	int iThisSkinIndex = 0;

	CString strSkinFileSurfaceDiscrepancies;
	
	for (OldSkinSets_t::iterator itOldSkins = pContainer->OldSkinSets.begin(); itOldSkins != pContainer->OldSkinSets.end(); ++itOldSkins, iThisSkinIndex++)
	{					
		string strSkinName				= (*itOldSkins).first;
		StringPairVector_t &StringPairs = (*itOldSkins).second;

		if (iSkinNumber == iThisSkinIndex || iSkinNumber == -1)
		{
			for (int iSurface = 0; iSurface < StringPairs.size(); iSurface++)
			{
				string strSurface(StringPairs[iSurface].first);
				string strTGAName(StringPairs[iSurface].second);

				UniqueSkinShaders.insert(UniqueSkinShaders.end(),strTGAName);

				if (iSkinNumber == -1)
				{
					// compare the current material against every other skin file, and report any that don't contain it...
					//					
					for (OldSkinSets_t::iterator itOldSkins_Other = pContainer->OldSkinSets.begin(); itOldSkins_Other != pContainer->OldSkinSets.end(); ++itOldSkins_Other)
					{
						string strSkinName_Other				= (*itOldSkins_Other).first;
						StringPairVector_t &StringPairs_Other	= (*itOldSkins_Other).second;

						for (iSurface_Other = 0; iSurface_Other < StringPairs_Other.size(); iSurface_Other++)
						{
							string strSurface_Other(StringPairs_Other[iSurface_Other].first);							

							if (strSurface_Other == strSurface)
							{
								break;
							}
						}
						if (iSurface_Other == StringPairs_Other.size())
						{
							// surface not found in this file...
							//
							strSkinFileSurfaceDiscrepancies += va("Surface \"%s\" ( skin \"%s\" ) had no entry in skin \"%s\"\n",strSurface.c_str(),strSkinName.c_str(),strSkinName_Other.c_str());
						}
					}
				}

			}			
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

	
	// Now output results...

	// If too many lines to fit on screen (which is now happening), send 'em to notepad instead...
	//
	// ( tacky way of counting lines...)
	CString strTackyCount(strNotFoundList.c_str());
			strTackyCount += strFoundList.c_str();			

	int iLines = strTackyCount.Replace('\n','?');	// :-)

	#define MAX_BOX_LINES_HERE 50

	// new popup before the other ones...
	//
	if (!strSkinFileSurfaceDiscrepancies.IsEmpty())
	{
		strSkinFileSurfaceDiscrepancies.Insert(0,va("( \"%s\" )\n\nThe following skin file errors occured during cross-checking...\n\n",pContainer->sLocalPathName));

		if (GetYesNo(va("%s\n\nSend copy of report to Notepad?", (LPCSTR) strSkinFileSurfaceDiscrepancies)))
		{
			SendStringToNotepad( strSkinFileSurfaceDiscrepancies, "skinfile_discrepancies.txt");
		}
	}

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


/*
#include "sys/timeb.h"

typedef struct
{
	char *psName;
	int iDays;
} MonthInfo_t;

MonthInfo_t Months[]=
{
//	{"January",		31},
//	{"February",	28},	// fuck the leap years
//	{"March",		31},
//	{"April",		30},
//	{"May",			31},
//	{"June",		30},
//	{"July",		31},
//	{"August",		31},
//	{"September",	30},
//	{"October", 	31},
//	{"November",	30},
//	{"December",	31}
	{"Jan",	31},
	{"Feb",	28},
	{"Mar",	31},
	{"Apr",	30},
	{"May",	31},
	{"Jun",	30},
	{"Jul",	31},
	{"Aug",	31},
	{"Sep",	30},
	{"Oct", 31},
	{"Nov",	30},
	{"Dec",	31}
};
char sCompileDate[] = {__DATE__};
	
// returns # days since compile.
//
// If return is -ve, then someone's messing with their machine time, or there's an error working it out, so
//	make your own mind up what to do.
//
int DaysSinceCompile(void)
{
	int iCompileDay = 0;
	int iThisDay = 0;

	for (int iPass=0; iPass<2; iPass++)
	{			
		char sTestDate[1024];
		char _month[1024];
		int _day,_year;		

		if (!iPass){
			strcpy(sTestDate,sCompileDate);
		} else {
			time_t ltime;			
			_tzset();			
			time( &ltime );    
			struct tm *today = localtime(&ltime);
			strftime(sTestDate,sizeof(sTestDate),"%b %d %Y",today);			
		}

		sscanf(sTestDate,"%s %d %d",&_month,&_day,&_year);

		int iDay = 0;
		for (int i=0; i<12; i++)
		{
			if (!strnicmp(_month,Months[i].psName,strlen(Months[i].psName)))		
				break;

			iDay += Months[i].iDays;
		}
		if (i==12)
		{
			// ERROR: couldn't determine month
			// 
			return -1;
		}
		iDay += _day;
		iDay += _year*365;

		if (!iPass){
			iCompileDay = iDay;
		} else {
			iThisDay = iDay;
		}
	}

	return iThisDay - iCompileDay;
}

*/
////////////////// eof /////////////////


