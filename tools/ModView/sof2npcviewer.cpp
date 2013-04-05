// SOF2NPCViewer.cpp : implementation file
//
// ( Note:  this file is actually for both SOF2 NPCs and JK2 Bots now )

#include "stdafx.h"
#include "includes.h"
#include "modview.h"
#include "files.h"
#include "r_common.h"
#include "generic_stuff.h"
#include "GenericParser2.h"
#include "script.h"				// so we can access ModView script keyword #defines
#include "modviewtreeview.h"	// for GetString()
//
#include "SOF2NPCViewer.h"


#include "stl.h"
/*
#include "disablewarnings.h"
#pragma warning( push, 3 )
#include <map>
#include <string>
#include <vector>
#pragma warning( pop )
using namespace std;
#include "disablewarnings.h"
*/

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

bool gbDoingGallery = false;
CString gstrGalleryOutputDir;
CString gstrGallerySequenceToLock;
CString *gpFeedback = NULL;
LPCSTR gpsGameDir = NULL;
/////////////////////////////////////////////////////////////////////////////
// CSOF2NPCViewer dialog


CSOF2NPCViewer::CSOF2NPCViewer(bool bSOF2Mode, CString *pFeedback, LPCSTR psGameDir, CWnd* pParent /*=NULL*/)
	: CDialog(CSOF2NPCViewer::IDD, pParent)
{
	//{{AFX_DATA_INIT(CSOF2NPCViewer)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	gpFeedback = pFeedback;
	gpsGameDir = psGameDir;

	m_bSOF2Mode = bSOF2Mode;

	Gallery_Done();
}


void CSOF2NPCViewer::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSOF2NPCViewer)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSOF2NPCViewer, CDialog)
	//{{AFX_MSG_MAP(CSOF2NPCViewer)
	ON_BN_CLICKED(IDREFRESH, OnRefresh)
	ON_LBN_DBLCLK(IDC_LIST_NPCS, OnDblclkListNpcs)
	ON_LBN_SELCHANGE(IDC_LIST_NPCS, OnSelchangeListNpcs)
	ON_BN_CLICKED(IDGALLERY, OnGallery)
	ON_BN_CLICKED(IDVALIDATE, OnValidate)
	ON_BN_CLICKED(IDC_BUTTON_GENERATE_LIST, OnButtonGenerateList)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSOF2NPCViewer message handlers

BOOL CSOF2NPCViewer::OnInitDialog() 
{
	CDialog::OnInitDialog();	

	// CG: The following block was added by the ToolTips component.
	{
		// Create the ToolTip control.
		m_tooltip.Create(this);
		m_tooltip.Activate(TRUE);

		// TODO: Use one of the following forms to add controls:
		// m_tooltip.AddTool(GetDlgItem(IDC_<name>), <string-table-id>);
		// m_tooltip.AddTool(GetDlgItem(IDC_<name>), "<text>");

		m_tooltip.AddTool(GetDlgItem(IDOK),		 "Close");
		m_tooltip.AddTool(GetDlgItem(IDREFRESH), "Refresh data from source files");
		m_tooltip.AddTool(GetDlgItem(IDVALIDATE), "Validate");
		m_tooltip.AddTool(GetDlgItem(IDGALLERY),  "Generate gallery images");
	}
	
	if (m_bSOF2Mode)
	{
		// SOF2...
		//
		NPC_ScanFiles(false);
		NPC_FillList();
	}
	else
	{
		// JK2...
		//
		BOT_ScanFiles(false);
		BOT_FillList();

		GetDlgItem(IDVALIDATE)->EnableWindow(false);
	}

	// my machine only... :-)
	//
//	GetDlgItem(IDGALLERY)->EnableWindow(!stricmp(scGetUserName(),"scork"));	
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CSOF2NPCViewer::OnRefresh() 
{
	if (m_bSOF2Mode)
	{
		// SOF2...
		//
		NPC_ScanFiles(true);
		NPC_FillList();
	}
	else
	{
		// JK2...
		//
		BOT_ScanFiles(true);
		BOT_FillList();
	}
}

typedef struct
{
	// fields from "ext_data/sof2.item"...
	//
	string	strName;			// 	name		"US SOCOM"
	string	strBoltPoint;		// boltpoint	"*hand_r"
	string	strHolstModel;		// holstmodel	"models\characters\bolt_ons\holster.glm"
								// holstsurf	"gun"

	// fields from "ext_data/sof2.wpn"

	string	strModel;			// model		"models/weapons/ussocom/ussocom.glm"

} NPC_Weapon_t;


typedef struct
{
	// fields from "ext_data/sof2.item"...
	//
	string	strName;					// name			"chem_taylor"
	vector	<string> vSurfaces_Off;		// offsurf1		"head_bck_lwr_l"
	vector	<string> vSurfaces_On;		// onsurf1		"fhead_bck_lwr_l_off"
	string	strModel;					// model		"models\characters\bolt_ons\backpack_radio.glm"

} NPC_Item_t;

typedef map <string,NPC_Weapon_t>	TheNPCWeapons_t;
typedef map <string,NPC_Item_t>		TheNPCItems_t;

typedef struct
{
	string	strName;			// Name		"US SOCOM"
	string	strBoltPoint;		// Bolt		"*hip_r"

} NPC_INV_Weapon_t;

typedef struct
{
	string	strName;			// Name		"ponytail"
	string	strBoltPoint;		// Bolt		"*hip_r"

} NPC_INV_Item_t;

typedef vector <NPC_INV_Weapon_t>	NPC_INV_Weapons_t;
typedef vector <NPC_INV_Item_t>		NPC_INV_Items_t;

typedef struct
{
	NPC_INV_Weapons_t	Weapons;
	NPC_INV_Items_t		Items;

} NPC_INV_t;

typedef struct
{
	string		strFile;			// SkinFile		"nurse_w1" or [File	"marine_camo2"]

	// optional weapons and items only available in this skin...
	//
	NPC_INV_t	Inventory;

} NPC_Skin_t;

typedef map <string, NPC_Skin_t> NPC_Skins_t;

typedef struct
{
	string		strName;			// Name			"NPC_Taylor_Jungle"
	string		strComments;		// comments		"Madeline Taylor in Jungle Gear"
	string		strModel;			// Model		"models/characters/female_pants/female_pants.glm"		

//	Team		"The Shop"
//	Rank		"Private"
//	Occupation	"ScriptGuy"
//	Health		"90 100"

	// optional weapons and items at the global level...
	//
	NPC_INV_t	Inventory;
	NPC_Skins_t Skins;

} NPC_CharacterTemplate_t;

typedef map <string,NPC_CharacterTemplate_t> NPC_CharacterTemplates_t;

typedef struct
{
//	Skeleton	"female_pants.skl"
	string	strParentTemplate;				// ParentTemplate	"NPC_Base_Female"

} NPC_GroupInfo_t;

typedef struct
{
	NPC_GroupInfo_t				NPC_GroupInfo;	
	NPC_CharacterTemplates_t	NPC_CharacterTemplates;

} NPCFile_t;


typedef map <string, NPCFile_t> NPCFiles_t;

NPCFiles_t		TheNPCFiles;
TheNPCWeapons_t TheNPCWeapons;
TheNPCItems_t	TheNPCItems;



// text defines for ext_data/sof2.item...
//
#define sKEYWORD_ITEM_WEAPON		"weapon"
#define sKEYWORD_ITEM_ITEM			"item"
#define sKEYWORD_ITEM_NAME			"name"
#define sKEYWORD_ITEM_MODEL			"model"
#define sKEYWORD_ITEM_BOLTPOINT		"boltpoint"
#define sKEYWORD_ITEM_HOLSTMODEL	"holstmodel"
#define sKEYWORD_ITEM_ONSURF		"onsurf"	// "onsurf",	"onsurf1",	"onsurf2" etc
#define sKEYWORD_ITEM_OFFSURF		"offsurf"	// "offsurf",	"offsurf1", "offsurf2" etc


// confusingly, this actually means load both weapons & items, but parse them from the file "ext_data/sof2.item"
//
// return true to continue processing, else false because of an error and the user said "don't continue"...
//
static bool NPC_LoadItems(void)
{
	bool bReturn = false;

	char sFileName[MAX_QPATH];
	
	Com_sprintf( sFileName, sizeof( sFileName ), "ext_data/sof2.item");
	StatusMessage( va("Scanning file: \"%s\"...", sFileName));
			
	char *pText = NULL;
	int iTotalBytesLoaded = ri.FS_ReadFile( sFileName, (void **)&pText );
			
	if ( pText ) 
	{
		CGenericParser2 Parser;
		char *psDataPtr = pText;	// because ptr gets advanced, so we supply a clone that GP1 can alter
		if (Parser.Parse(&psDataPtr, true))
		{				
			CGPGroup *pFileGroup	= Parser.GetBaseParseGroup();
			if (pFileGroup)
			{
				CGPGroup *pSubGroup = pFileGroup->GetSubGroups();
				while (pSubGroup)
				{
					string strParseGroupName = pSubGroup->GetName();

					if (strParseGroupName == sKEYWORD_ITEM_WEAPON)
					{						
						// read weapon fields...
						//
						CGPGroup *&pWeaponGroup = pSubGroup;	// saves search/replace during GP1->GP conversion
						string strName = pWeaponGroup->FindPairValue(sKEYWORD_ITEM_NAME, "");
						if (!strName.empty())
						{
							// insert into map...
							//
							TheNPCWeapons[strName].strName = strName;	// looks pointless, but init's the map entry
							//
							// now parse other fields...
							//
							string 
							str = pWeaponGroup->FindPairValue(sKEYWORD_ITEM_BOLTPOINT, "");
							if (!str.empty())
							{
								TheNPCWeapons[strName].strBoltPoint = str;
							}

							str = pWeaponGroup->FindPairValue(sKEYWORD_ITEM_HOLSTMODEL, "");
							if (!str.empty())
							{
								TheNPCWeapons[strName].strHolstModel = str;
							}
						}
					}
					else
					if (strParseGroupName == sKEYWORD_ITEM_ITEM)
					{
						// read item fields...
						//
						CGPGroup *&pItemGroup = pSubGroup;

						string strName = pItemGroup->FindPairValue(sKEYWORD_ITEM_NAME, "");
						if (!strName.empty())
						{
							// insert into map...
							//						
							TheNPCItems[strName].strName = strName;	// looks pointless, but init's the map entry
							//
							// now parse other fields...
							//
							string 
							str = pItemGroup->FindPairValue(sKEYWORD_ITEM_MODEL, "");
							if (!str.empty())
							{
								TheNPCItems[strName].strModel = str;
							}

							//
							// parse fields with non-precise names...
							//
							CGPValue *pValue = pSubGroup->GetPairs();
							while (pValue)
							{
								string strKey	= pValue->GetName();
								string strValue = pValue->GetTopValue();

								if (!strncmp(strKey.c_str(), sKEYWORD_ITEM_ONSURF, strlen(sKEYWORD_ITEM_ONSURF)))
								{								
									TheNPCItems[strName].vSurfaces_On.push_back(strValue);
								}

								if (!strncmp(strKey.c_str(), sKEYWORD_ITEM_OFFSURF, strlen(sKEYWORD_ITEM_OFFSURF)))
								{								
									TheNPCItems[strName].vSurfaces_Off.push_back(strValue);
								}

								pValue = pValue->GetNext();
							}
						}
					}

					pSubGroup = (CGPGroup *)pSubGroup->GetNext();	
				}
			}
		}
		else
		{
			ErrorBox(va("{} - Brace mismatch error in file \"%s\"!",sFileName));
		}

		ri.FS_FreeFile( pText );
		bReturn = true;
	}
	else
	{
		bReturn = GetYesNo(va("Unable to open file \"%s\", continue anyway?",sFileName));
	}
	StatusMessage(NULL);

	return bReturn;
}



#define sKEYWORD_WPN_WEAPON		"weapon"
#define sKEYWORD_WPN_NAME		"name"
#define sKEYWORD_WPN_MODEL		"model"

// this parses weapon info from the file "ext_data/sof2.wpn"
//
// return true to continue processing, else false because of an error and the user said "don't continue"...
//
static bool NPC_LoadWeapons(void)
{
	bool bReturn = false;

	char sFileName[MAX_QPATH];
	
	Com_sprintf( sFileName, sizeof( sFileName ), "ext_data/sof2.wpn");
	StatusMessage( va("Scanning file: \"%s\"...", sFileName));
			
	char *pText = NULL;
	int iTotalBytesLoaded = ri.FS_ReadFile( sFileName, (void **)&pText );
			
	if ( pText ) 
	{
		CGenericParser2 Parser;
		char *psDataPtr = pText;
		if (Parser.Parse(&psDataPtr, true))
		{
			CGPGroup	*pFileGroup	= Parser.GetBaseParseGroup();
			if (pFileGroup)
			{
				CGPGroup *pSubGroup = pFileGroup->GetSubGroups();

				while (pSubGroup)
				{
					string strParseGroupName = pSubGroup->GetName();

					if (strParseGroupName == sKEYWORD_WPN_WEAPON)
					{						
						// read weapon fields...
						//
						CGPGroup *&pWeaponGroup = pSubGroup;

						string strName = pWeaponGroup->FindPairValue(sKEYWORD_WPN_NAME, "");
						if (!strName.empty())
						{
							// insert into map...
							//
							TheNPCWeapons[strName].strName = strName;	// looks pointless, but init's the map entry
							//
							// now parse other fields...
							//
							string 
							str = pWeaponGroup->FindPairValue(sKEYWORD_WPN_MODEL, "");						
							if (!str.empty())
							{
								TheNPCWeapons[strName].strModel = str;
							}
						}
					}
					pSubGroup = pSubGroup->GetNext();
				}
			}
		}
		else
		{
			ErrorBox(va("{} - Brace mismatch error in file \"%s\"!",sFileName));
		}

		ri.FS_FreeFile( pText );
		bReturn = true;
	}
	else
	{
		bReturn = GetYesNo(va("Unable to open file \"%s\", continue anyway?",sFileName));
	}
	StatusMessage(NULL);

	return bReturn;
}



#define sKEYWORD_NPC_GROUPINFO			"GroupInfo"
#define sKEYWORD_NPC_CHARACTERTEMPLATE	"CharacterTemplate"
#define sKEYWORD_NPC_PARENTTEMPLATE		"ParentTemplate"
#define sKEYWORD_NPC_NAME				"Name"
#define sKEYWORD_NPC_MODEL				"Model"
#define sKEYWORD_NPC_COMMENTS			"comments"
#define sKEYWORD_NPC_SKINFILE			"SkinFile"
#define sKEYWORD_NPC_INVENTORY			"Inventory"
#define sKEYWORD_NPC_ITEM				"Item"
#define sKEYWORD_NPC_WEAPON				"Weapon"
#define sKEYWORD_NPC_BOLT				"Bolt"
#define sKEYWORD_NPC_SKIN				"Skin"
#define sKEYWORD_NPC_FILE				"File"

static bool NPC_ParseInventory(NPC_INV_t &Inventory, CGPGroup *pParseGroup)
{
	bool bReturn = false;

	CGPGroup *pSubGroup = pParseGroup->GetSubGroups();
	while (pSubGroup)
	{
		string strSubGroupName = pSubGroup->GetName();

		if (strSubGroupName == sKEYWORD_NPC_WEAPON)
		{						
			// read weapon fields...
			//
			NPC_INV_Weapon_t Weapon;

			CGPValue *pValue = pSubGroup->GetPairs();
			while (pValue)
			{
				string strKey	= pValue->GetName();
				string strValue = pValue->GetTopValue();

				if (strKey == sKEYWORD_NPC_NAME)
				{
					Weapon.strName = strValue;
				}
				else
				if (strKey == sKEYWORD_NPC_BOLT)
				{
					Weapon.strBoltPoint = strValue;
				}

				pValue = pValue->GetNext();
			}

			Inventory.Weapons.push_back( Weapon );
			bReturn = true;
		}
		else
		if (strSubGroupName == sKEYWORD_NPC_ITEM)
		{						
			// read item fields...
			//
			NPC_INV_Item_t Item;

			CGPValue *pValue = pSubGroup->GetPairs();
			while (pValue)
			{
				string strKey	= pValue->GetName();
				string strValue = pValue->GetTopValue();

				if (strKey == sKEYWORD_NPC_NAME)
				{
					Item.strName = strValue;
				}
				else
				if (strKey == sKEYWORD_NPC_BOLT)
				{
					Item.strBoltPoint = strValue;
				}

				pValue = pValue->GetNext();
			}

			Inventory.Items.push_back( Item );
			bReturn = true;
		}

		pSubGroup = pSubGroup->GetNext();
	}

	return bReturn;
}


//
static bool NPC_ParseNPCFiles(void)
{
	char **ppsNPCFiles;
	int iNPCFiles;
	int i = 0;

	// scan for NPC files...
	//
	#define sNPC_DIR va("%snpcs",gpsGameDir)

	ppsNPCFiles =	//ri.FS_ListFiles( "shaders", ".shader", &iSkinFiles );
					Sys_ListFiles(	sNPC_DIR,		// const char *directory, 
									".npc",		// const char *extension, 
									NULL,		// char *filter, 
									&iNPCFiles,	// int *numfiles, 
									qfalse		// qboolean wantsubs 
									);

	if ( ppsNPCFiles && iNPCFiles )
	{
		#define MAX_NPC_FILES 100	// any old large-ish size for array declaration...
		//
		if ( iNPCFiles > MAX_NPC_FILES ) 
		{
			WarningBox(va("%d NPC files found, capping to %d\n\n(tell me if this ever happens -Ste)", iNPCFiles, MAX_NPC_FILES ));

			iNPCFiles =  MAX_NPC_FILES;
		}

		// load and parse skin files...
		//					
		char *buffers[MAX_NPC_FILES];
		long iTotalBytesLoaded = 0;
		for ( i=0; i<iNPCFiles; i++ )
		{
			char sFileName[MAX_QPATH];

			string strThisNPCFile(ppsNPCFiles[i]);
			string strThisNPCFileNoExt(Filename_WithoutExt(strThisNPCFile.c_str()));	// laziness

			Com_sprintf( sFileName, sizeof( sFileName ), "%s/%s", "npcs", strThisNPCFile.c_str() );
			StatusMessage( va("Scanning NPC file %d/%d: \"%s\"...",i+1,iNPCFiles,sFileName));
									
			iTotalBytesLoaded += ri.FS_ReadFile( sFileName, (void **)&buffers[i] );
			
			if ( !buffers[i] ) {
				ri.Error( ERR_DROP, "Couldn't load %s", sFileName );
			}

			char *psDataPtr = buffers[i];

			CGenericParser2 Parser;
			if (Parser.Parse(&psDataPtr, true))
			{
				CGPGroup *pFileGroup = Parser.GetBaseParseGroup();
				if (pFileGroup)
				{
					CGPGroup *pSubGroup = pFileGroup->GetSubGroups();
					while (pSubGroup)
					{
						string strParseGroupName = pSubGroup->GetName();

						if (strParseGroupName == sKEYWORD_NPC_GROUPINFO)
						{
							// only want one field from this group...
							//
							string str = pSubGroup->FindPairValue(sKEYWORD_NPC_PARENTTEMPLATE, "");
							if (!str.empty())
							{							
								TheNPCFiles[strThisNPCFileNoExt].NPC_GroupInfo.strParentTemplate = str;
							}
						}
						else
						if (strParseGroupName == sKEYWORD_NPC_CHARACTERTEMPLATE)
						{
							NPC_CharacterTemplate_t CharacterTemplate;						
							//
							// see which of the pairs in here interest us...
							//
							CGPValue *pValue = pSubGroup->GetPairs();
							while (pValue)
							{
								string strKey	= pValue->GetName();
								string strValue = pValue->GetTopValue();

								if (!stricmp(strKey.c_str(), sKEYWORD_NPC_NAME))
								{								
									//OutputDebugString(va("%s\n",strValue.c_str()));
									CharacterTemplate.strName = strValue;
								}
								else
								if (!stricmp(strKey.c_str(), sKEYWORD_NPC_MODEL))
								{								
									CharacterTemplate.strModel = strValue;
								}
								else
								if (!stricmp(strKey.c_str(), sKEYWORD_NPC_COMMENTS))
								{								
									CharacterTemplate.strComments = strValue;
								}
								else
								if (!stricmp(strKey.c_str(), sKEYWORD_NPC_SKINFILE))
								{								
									NPC_Skin_t	Skin;
												Skin.strFile = strValue;								
									CharacterTemplate.Skins[ Skin.strFile ] = Skin;
								}
								pValue = pValue->GetNext();
							}

							// scan for whichever subgroups we're interested in...
							//
							CGPGroup *pSubGroupTemplate = pSubGroup->GetSubGroups();
							while (pSubGroupTemplate)
							{
								string strSubGroupName = pSubGroupTemplate->GetName();

								if (strSubGroupName == sKEYWORD_NPC_INVENTORY)
								{
									NPC_INV_t Inventory;

									if (NPC_ParseInventory(Inventory,pSubGroupTemplate))
									{
										CharacterTemplate.Inventory = Inventory;
									}
								}
								else
								if (strSubGroupName == sKEYWORD_NPC_SKIN)
								{
									// must have a "File" pair-entry...
									//
									string strFile = pSubGroupTemplate->FindPairValue(sKEYWORD_NPC_FILE,"");
									if (!strFile.empty())
									{
										NPC_Skin_t	Skin;
													Skin.strFile = strFile;

										// now look for an optional Inventory subgroup...
										//
										CGPGroup *pSubGroupInv = pSubGroupTemplate->FindSubGroup(sKEYWORD_NPC_INVENTORY);
										if (pSubGroupInv)
										{
											NPC_INV_t Inventory;

											if (NPC_ParseInventory(Inventory,pSubGroupInv))
											{
												Skin.Inventory = Inventory;
											}
										}

										CharacterTemplate.Skins[ Skin.strFile ] = Skin;
									}
								}
								pSubGroupTemplate = pSubGroupTemplate->GetNext();
							}

							// finally...
							//						
							TheNPCFiles[strThisNPCFileNoExt].NPC_CharacterTemplates[CharacterTemplate.strName] = CharacterTemplate;
						}

						pSubGroup = pSubGroup->GetNext();
					}
				}
			}
			else
			{
				ri.Error( ERR_DROP, "{} - Brace mismatch error in file \"%s\"!",sFileName);				
			}
		}
		StatusMessage(NULL);

		// free loaded files...
		//
		for ( i=0; i<iNPCFiles; i++ )
		{
			ri.FS_FreeFile( buffers[i] );
		}

		// ... and file list...
		//		
		Sys_FreeFileList( ppsNPCFiles );
	}
	else
	{
		WarningBox(va("WARNING: no NPC files found in '%s'\n",sNPC_DIR ));
	}

	return !!TheNPCFiles.size();
}


/*
// Dark Side bots
// Currently, all DS bots carry red lightsabers
name		"SW-967"
model		Swamptrooper
color1		0
personality	/botfiles/Swamptrooper.jkb
//Swamptrooper is aligned with no one

*/
typedef struct BOTFile_s
{
	string	strName;	// name		"SW-967"
	string	strModel;	// model	Swamptrooper	// (will be 1:1 dir name match)
	int		iColor1;	// color1	0				// lightsaber color, FWIW (dark = 0, light = 4 so far)
	string	strComment;	////Swamptrooper is aligned with no one

	BOTFile_s()
	{
		iColor1 = 0;
	}

} BOTFile_t;

typedef map <string, BOTFile_t> BOTFiles_t;

BOTFiles_t TheBOTFiles;


static void BOT_ScanSkins(BOTFile_t &Bot, set <string> &SkinVariants)
{
	char **ppsFiles;
	int iFiles;

	// scan for NPC files...
	//
	#define sBOTSKINS_DIR va("%smodels/players/%s",gpsGameDir, Bot.strModel.c_str())

	ppsFiles =	//ri.FS_ListFiles( "shaders", ".shader", &iSkinFiles );
				Sys_ListFiles(	sBOTSKINS_DIR,	// const char *directory, 
								".skin",		// const char *extension, 
								NULL,		// char *filter, 
								&iFiles,	// int *numfiles, 
								qfalse		// qboolean wantsubs 
								);

	if ( ppsFiles && iFiles )
	{
		for ( int i=0; i<iFiles; i++ )
		{
			CString strThisFile( Filename_WithoutExt(ppsFiles[i]) );

			if (!strnicmp(strThisFile,"model_",6))
			{
				string s = (LPCSTR)(strThisFile.Mid(6));
				SkinVariants.insert(s);
			}
		}
		StatusMessage(NULL);

		// free file list...
		//		
		Sys_FreeFileList( ppsFiles );
	}
}


void CSOF2NPCViewer::BOT_ScanFiles(bool bForceRefresh)
{
	if (TheBOTFiles.size() && !bForceRefresh)
		return;

	CWaitCursor wait;

	TheBOTFiles.clear();
	
	CString strFileName( va("%sbotfiles\\bots.txt",gpsGameDir) );
	CString strBOTFile;
	if (TextFile_Read(strBOTFile, strFileName, false, true))
	{
		int iLoc;

		bool bStopHere = false;
		while ( !bStopHere && (iLoc = strBOTFile.Find("{")) != -1)		/*}*/
		{
			/*{*/
			int iLoc2 = strBOTFile.Find("}",iLoc);

			if (iLoc2 != -1)
			{
				CString strThisBot( strBOTFile.Left(iLoc2) );
						strThisBot = strThisBot.Mid(iLoc+1);

				strBOTFile = strBOTFile.Mid(iLoc2+1);

				BOTFile_t Bot;

				// scan this bot file on a line by line basis...
				//
				/*
						{
						name		"IW-322"
						model		Imperial_worker
						color1		0
						personality	/botfiles/Imperial_worker.jkb
						//Imperial_worker is aligned with no one
						}
				*/

				while (!strThisBot.IsEmpty())
				{
					CString strLine;

					iLoc = strThisBot.FindOneOf("\n\r");
					if (iLoc != -1)
					{
						strLine = strThisBot.Left(iLoc);
						strThisBot = strThisBot.Mid(iLoc+1);
					}
					else
					{
						strLine = strThisBot;
						strThisBot.Empty();
					}

					strLine.TrimLeft();
					strLine.TrimRight();

					if (!strLine.IsEmpty())
					{
						if (!strnicmp(strLine,"name", 4))
						{
							CString str(strLine.Mid(4));
									str.TrimLeft();
									str.Replace("\"","");

							if (!str.CompareNoCase("@STOPHERE"))
							{
								bStopHere = true;
								break;
							}

							Bot.strName = (LPCSTR) str;
							//OutputDebugString(va("Bot name: \"%s\"\n",(LPCSTR) str));
						}
						else
						if (!strnicmp(strLine,"model",5))
						{
							CString str(strLine.Mid(5));
									str.TrimLeft();
									str.Replace("\"","");

							Bot.strModel = (LPCSTR) str;
							//OutputDebugString(va("Bot model: \"%s\"\n",(LPCSTR) str));
						}
						else
						if (!strnicmp(strLine,"color1",6))
						{
							Bot.iColor1 = atoi(&((LPCSTR)strLine)[6]);

							//OutputDebugString(va("Bot color1: %d\n",Bot.iColor1));
						}
						else
						if (!strnicmp(strLine,"//",2))
						{
							CString str( strLine.Mid(2) );
									str.TrimLeft();

							Bot.strComment = (LPCSTR) str;

							//OutputDebugString(va("Bot comment: \"%s\"\n",Bot.strComment.c_str()));
						}
					}
				}

				if (!bStopHere && !Bot.strModel.empty())
				{
					TheBOTFiles[ Bot.strModel.c_str() ] = Bot;					
				}
			}
			else
			{
				ErrorBox(va("Mismatching braces in \"%s\"!",(LPCSTR)strFileName));
				strBOTFile.Empty();
			}
		}
	}
	else
	{
		ErrorBox(va("Failed to open file \"%s\"!",(LPCSTR)strFileName));
	}
}

void CSOF2NPCViewer::BOT_FillList(void)
{
//	ListBoxLookups.clear();

	CListBox* pListBox = (CListBox*)GetDlgItem(IDC_LIST_NPCS);
	assert(pListBox);
	if (pListBox)
	{
		pListBox->ResetContent();

		for (BOTFiles_t::iterator itBots = TheBOTFiles.begin(); itBots != TheBOTFiles.end(); ++itBots)
		{
			BOTFile_t &Bot = (*itBots).second;

			pListBox->InsertString(-1, Bot.strModel.c_str());
/*
				string strListEntry( NPC_CreateListEntry( pTemplate->strName.c_str(), pSkin->strFile.c_str() ) );
					pListBox->InsertString(-1, strListEntry.c_str());

					ListBoxLookups[ strListEntry ] = NPC_DataLookup;	// hehehehe, STL rules for some stuff
*/

		}
	}


	CStatic *pStatic = (CStatic *)GetDlgItem(IDC_STATIC_COMMENT);
	if (pStatic)
	{
		pStatic->SetWindowText("");
	}
}


// builds up all NPC structures...
//
void CSOF2NPCViewer::NPC_ScanFiles(bool bForceRefresh)
{
	if (TheNPCFiles.size() && !bForceRefresh)
		return;

	CWaitCursor wait;

	TheNPCFiles.clear();
	TheNPCWeapons.clear();
	TheNPCItems.clear();

	// scan weapons and items in first...
	//
	bool bUserWantsToCancel = true;
	if (NPC_LoadItems())
	{
		if (NPC_LoadWeapons())
		{
			if (NPC_ParseNPCFiles())
			{
				bUserWantsToCancel = false;
	/*
				// quick debug thing...
				//
				for (TheNPCWeapons_t::iterator itW = TheNPCWeapons.begin(); itW != TheNPCWeapons.end(); ++itW)
				{
					NPC_Weapon_t &Weapon = (*itW).second;

					OutputDebugString(va("Weapon: \"%s\"\n",Weapon.strName.c_str()));
					OutputDebugString(va("        strModel:      \"%s\"\n",Weapon.strModel.c_str()));
					OutputDebugString(va("        strBoltPoint:  \"%s\"\n",Weapon.strBoltPoint.c_str()));
					OutputDebugString(va("        strHolstModel: \"%s\"\n",Weapon.strHolstModel.c_str()));
					OutputDebugString("\n");
				}

				for (TheNPCItems_t::iterator itI = TheNPCItems.begin(); itI != TheNPCItems.end(); ++itI)
				{
					NPC_Item_t &Item = (*itI).second;

					OutputDebugString(va("Item: \"%s\"\n",Item.strName.c_str()));
					OutputDebugString(va("        strModel:      \"%s\"\n",Item.strModel.c_str()));
					if (Item.vSurfaces_On.size())
					{
						for (int i=0; i<Item.vSurfaces_On.size(); i++)
						{
							OutputDebugString(va("On: \"%s\"\n",Item.vSurfaces_On[i].c_str()));
						}
					}
					if (Item.vSurfaces_Off.size())
					{
						for (int i=0; i<Item.vSurfaces_Off.size(); i++)
						{
							OutputDebugString(va("Off: \"%s\"\n",Item.vSurfaces_Off[i].c_str()));
						}
					}				
				}
	*/
			}
		}
	}	

	if (bUserWantsToCancel)
	{
		OnCancel();
	}
}





typedef struct
{
	string	strNPCFileNameBase;
	string	strTemplateName;
	string	strSkinName;	

} NPC_DataLookup_t;

typedef map<string, NPC_DataLookup_t>	ListBoxLookups_t;
										ListBoxLookups_t ListBoxLookups;

// subroutined so that the ModView Script generator can generate a 100%-compatible string for doing a data lookup...
//
static LPCSTR NPC_CreateListEntry(LPCSTR psTemplateName, LPCSTR psSkinName)
{
#define MIN_NAME_LEN 40
	static string strListEntry;
	strListEntry = va("%s\t ( %s )", String_EnsureMinLength(psTemplateName, MIN_NAME_LEN), psSkinName);
	return strListEntry.c_str();
}

// applies NPC structures to GUI picker...
//
void CSOF2NPCViewer::NPC_FillList()
{	
	ListBoxLookups.clear();

	CListBox* pListBox = (CListBox*)GetDlgItem(IDC_LIST_NPCS);	
	assert(pListBox);
	if (pListBox)
	{
		pListBox->ResetContent();
		
		for (NPCFiles_t::iterator itFiles = TheNPCFiles.begin(); itFiles != TheNPCFiles.end(); ++itFiles)
		{
			string strNPCFileNameBase	= (*itFiles).first;
			NPCFile_t *pNPCFile			= &(*itFiles).second;

			NPC_GroupInfo_t *pNPC_GroupInfo = &pNPCFile->NPC_GroupInfo;

			for (NPC_CharacterTemplates_t::iterator itTemplate = pNPCFile->NPC_CharacterTemplates.begin(); itTemplate != pNPCFile->NPC_CharacterTemplates.end(); ++itTemplate)
			{
				NPC_CharacterTemplate_t *pTemplate = &(*itTemplate).second;

				for (NPC_Skins_t::iterator itSkin = pTemplate->Skins.begin(); itSkin != pTemplate->Skins.end(); ++itSkin)
				{
					NPC_Skin_t *pSkin = &(*itSkin).second;

					string strListEntry( NPC_CreateListEntry( pTemplate->strName.c_str(), pSkin->strFile.c_str() ) );
					pListBox->InsertString(-1, strListEntry.c_str());

					NPC_DataLookup_t	NPC_DataLookup;
										NPC_DataLookup.strNPCFileNameBase	= strNPCFileNameBase;
										NPC_DataLookup.strTemplateName		= pTemplate->strName;
										NPC_DataLookup.strSkinName			= pSkin->strFile;

					ListBoxLookups[ strListEntry ] = NPC_DataLookup;	// hehehehe, STL rules for some stuff
				}
			}
		}
	}


	CStatic *pStatic = (CStatic *)GetDlgItem(IDC_STATIC_COMMENT);
	if (pStatic)
	{
		pStatic->SetWindowText("");
	}
}



// turns a listbox caption into data ptrs...
//
static bool NPC_DataLookup(LPCSTR psCaption, NPCFile_t &NPCFile, NPC_CharacterTemplate_t &Template, NPC_Skin_t &Skin)
{
	ListBoxLookups_t::iterator it = ListBoxLookups.find(psCaption);
	if (it != ListBoxLookups.end())
	{
		NPC_DataLookup_t &DataLookUp = (*it).second;

		// NPCFile...
		//
		NPCFiles_t::iterator itFile = TheNPCFiles.find( DataLookUp.strNPCFileNameBase );
		if (itFile != TheNPCFiles.end())
		{
			NPCFile = (*itFile).second;

			// CharacterTemplate...
			//
			NPC_CharacterTemplates_t::iterator itTemplate = NPCFile.NPC_CharacterTemplates.find( DataLookUp.strTemplateName );
			if (itTemplate != NPCFile.NPC_CharacterTemplates.end())
			{
				Template = (*itTemplate).second;

				// Skin...
				//
				NPC_Skins_t::iterator itSkin = Template.Skins.find( DataLookUp.strSkinName );
				if (itSkin != Template.Skins.end())
				{
					Skin = (*itSkin).second;
					return true;
				}
			}
		}
	}

	return false;
}

void CSOF2NPCViewer::OnSelchangeListNpcs() 
{
	CListBox* pListBox = (CListBox*)GetDlgItem(IDC_LIST_NPCS);
	assert(pListBox);	
	if (pListBox)
	{
		int iSelected = pListBox->GetCurSel();

		if (iSelected != LB_ERR)
		{
			CString strCaption;

			pListBox->GetText(iSelected, strCaption);

			// work out comment...
			//
			CStatic *pStatic = (CStatic *)GetDlgItem(IDC_STATIC_COMMENT);
			if (pStatic)
			{
				LPCSTR psStaticText = NULL;
				if (m_bSOF2Mode)
				{
					// SOF2...
					//
					NPCFile_t				NPCFile;
					NPC_CharacterTemplate_t Template;
					NPC_Skin_t				Skin;
					if (NPC_DataLookup(strCaption, NPCFile, Template, Skin) && !Template.strComments.empty())
					{
						psStaticText = va("( Comment: \"%s\" )",Template.strComments.c_str());
					}
				}
				else
				{
					// JK2...
					//
					BOTFiles_t::iterator itBotFile = TheBOTFiles.find( (LPCSTR) strCaption);
					if (itBotFile != TheBOTFiles.end())
					{
//						psStaticText = va("( Comment: \"%s\"     Model: \"%s\" )",(*itBotFile).second.strComment.c_str(), (*itBotFile).second.strModel.c_str());
						psStaticText = va("( Comment: \"%s\" )",(*itBotFile).second.strComment.c_str(), (*itBotFile).second.strModel.c_str());
					}
				}

				pStatic->SetWindowText( psStaticText ? psStaticText : "( No comment available )" );
			}
		}
	}
}




static
void NPC_CreateModViewScript_ParseInventory(CString &strCaptionForErrorPrinting,
											NPC_INV_t &Inventory, 
											vector	<string> &vSurfaces_On,
											vector	<string> &vSurfaces_Off,
											vector  < pair< string, string> > &vBoltData
											)
{
	// items...
	//
	NPC_INV_Items_t &Items = Inventory.Items;
	for (int iItem = 0; iItem < Items.size(); iItem++)
	{	
		LPCSTR psName = Items[iItem].strName.c_str();

		string strBoltPoint = Items[iItem].strBoltPoint.c_str();
		string strBoltModel;

		TheNPCItems_t::iterator itItem = TheNPCItems.find(psName);
		if (itItem != TheNPCItems.end())
		{
			NPC_Item_t &Item = (*itItem).second;

			vSurfaces_On.insert (vSurfaces_On.begin(), Item.vSurfaces_On.begin(), Item.vSurfaces_On.end());
			vSurfaces_Off.insert(vSurfaces_Off.begin(),Item.vSurfaces_Off.begin(),Item.vSurfaces_Off.end());

			strBoltModel = Item.strModel;
		}
		else
		{
			WarningBox(va("Unable to find item entry \"%s\"\n\nCharacter: \"%s\"\n\n( Try hitting 'Refresh', and if still missing, tell Joe K )",psName,(LPCSTR)strCaptionForErrorPrinting));
		}

		if (!strBoltPoint.empty() && !strBoltModel.empty())
		{
			vBoltData.push_back( pair<string,string>(String_ForwardSlash(strBoltModel.c_str()),strBoltPoint) );
		}
	}

	// weapons...
	//
	NPC_INV_Weapons_t &InvWeapons = Inventory.Weapons;
	for (int iWeapon = 0; iWeapon < InvWeapons.size(); iWeapon++)
	{	
		NPC_INV_Weapon_t &InvWeapon = InvWeapons[iWeapon];

		string strName		= InvWeapon.strName;
		string strBoltPoint = InvWeapon.strBoltPoint;	// will be blank if a held weapon, else filled in for holstered
		string strBoltModel;

		TheNPCWeapons_t::iterator itWeapon = TheNPCWeapons.find(strName);
		if (itWeapon != TheNPCWeapons.end())
		{
			NPC_Weapon_t &Weapon = (*itWeapon).second;					

			// some slightly interesting logic here, ok, first, let's see if it was a holstered weapon or not...
			//
			if (!strBoltPoint.empty())
			{
				// holstered weapon...
				//
				strBoltModel = Weapon.strHolstModel;
			}
			else
			{
				// currently-held weapon...
				//
				strBoltPoint = Weapon.strBoltPoint;
				strBoltModel = Weapon.strModel;
			}
		}
		else
		{				
			WarningBox(va("Unable to find weapon entry \"%s\"\n\nCharacter: \"%s\"\n\n( Try hitting 'Refresh', and if still missing, tell Joe )",strName.c_str(),(LPCSTR)strCaptionForErrorPrinting));
		}

		if (!strBoltPoint.empty() && !strBoltModel.empty())
		{
			vBoltData.push_back( pair<string,string>(String_ForwardSlash(strBoltModel.c_str()),strBoltPoint) );
		}
	}
}
 
// this is pretty quick and dirty for now, it'll fail if you have two identical models loaded, each of which has 
//	something bolted to it (which the real script-writer compensates for correctly).
//
// Oh well, I can tackle that if it ever arises...
//
static bool NPC_CreateModViewScript(CString &strCaptionForErrorPrinting, CString &strScript, NPCFile_t &NPCFile, NPC_CharacterTemplate_t &Template, NPC_Skin_t &Skin)
{
	bool bReturn = false;

	CTextPool	*pTextPool = new CTextPool(40960);		// any old number, it'll expand internally

	CGenericParser2	OutputParser;						// .. this should be interesting...
					OutputParser.SetWriteable(true);	// itu?

	CGPGroup *pModelGroup = OutputParser.GetBaseParseGroup()->AddGroup( sSCRIPTKEYWORD_LOADMODEL, &pTextPool );
	if (pModelGroup)
	{
		if (gpsGameDir)
		{
			pModelGroup->AddPair(sSCRIPTKEYWORD_BASEDIR, gpsGameDir, &pTextPool);
		}
		string strParentName( Template.strName );
		//
		// name "average_sleeves"...	(this field is more of a label really, for bolting purposes). See comment at top.
		//
		pModelGroup->AddPair(sSCRIPTKEYWORD_NAME, strParentName.c_str(), &pTextPool);
		

		// 	modelfile	"models/characters/average_sleeves/average_sleeves.glm"
		//
		string strModel( Template.strModel );
		if (strModel.empty())
		{
			string strParentTemplate = NPCFile.NPC_GroupInfo.strParentTemplate;
			//fixme: need to look this up!!!!

			// need to find which NPC file has this template in...
			//
			for (NPCFiles_t::iterator itNPC = TheNPCFiles.begin(); itNPC != TheNPCFiles.end(); ++itNPC)
			{
				NPCFile_t &_NPCFile = (*itNPC).second;
				NPC_CharacterTemplates_t::iterator itTemplate = _NPCFile.NPC_CharacterTemplates.find(strParentTemplate);
				if (itTemplate != _NPCFile.NPC_CharacterTemplates.end())
				{
					// found the template, so adopt the model name from there...
					//
					NPC_CharacterTemplate_t &_Template = (*itTemplate).second;
					strModel = _Template.strModel;
				}
			}

			if (strModel.empty())
			{
				ErrorBox(va("Unable to work out model name for character template \"%s\"!",Template.strName.c_str()));
				return false;
			}
		}
		pModelGroup->AddPair(sSCRIPTKEYWORD_MODELFILE, strModel.c_str(), &pTextPool);

		// skinfile	"col_rebel_h1.g2skin"
		//
		pModelGroup->AddPair(sSCRIPTKEYWORD_SKINFILE, va("%s.g2skin",Skin.strFile.c_str()), &pTextPool);


		// ethnic		"white"
		// (currently the NPC file system has no provision for this (unlike MVS files :-)
		// but i stilll have to have one or it won't apply the skin, so...
		//
		pModelGroup->AddPair(sSCRIPTKEYWORD_ETHNIC, "white", &pTextPool);

		// now build up lists of surfaces on and off. This is going to be messy, since they do theirs as part of 
		//	the inventory data (sigh)...
		//		
		vector	<string> vSurfaces_On;
		vector	<string> vSurfaces_Off;
		vector  < pair< string, string> > vBoltData;	// <model name,boltpoint>
		//
		// check global inventory first...
		//
		NPC_CreateModViewScript_ParseInventory(strCaptionForErrorPrinting, Template.Inventory,	vSurfaces_On, vSurfaces_Off, vBoltData);
		//
		// now check the inventory inside this skin...
		//
		NPC_CreateModViewScript_ParseInventory(strCaptionForErrorPrinting, Skin.Inventory,		vSurfaces_On, vSurfaces_Off, vBoltData);

		/*
		surfaces_on
		{
			name0	"scarf_off"
			name1	"head_side_r_avmedhat_off"
			name2	"head_side_l_avmedhat_off"
		}
		*/
		if (vSurfaces_On.size())
		{
			CGPGroup *pSurfaceGroup = pModelGroup->AddGroup( sSCRIPTKEYWORD_SURFACES_ON, &pTextPool );
			if (pSurfaceGroup)
			{
				for (int iSurface = 0; iSurface < vSurfaces_On.size(); iSurface++)
				{
					pSurfaceGroup->AddPair(va(sSCRIPTKEYWORD_NAME "%d",iSurface), vSurfaces_On[iSurface].c_str(), &pTextPool);
				}
			}
		}

		/*
		surfaces_off
		{
			name0	"head_side_r"
			name1	"head_side_l"
			name2	"head_bck_uppr_r"
			name3	"head_bck_uppr_l"
		}
		*/
		if (vSurfaces_Off.size())
		{
			CGPGroup *pSurfaceGroup = pModelGroup->AddGroup( sSCRIPTKEYWORD_SURFACES_OFF, &pTextPool);			
			if (pSurfaceGroup)
			{
				for (int iSurface = 0; iSurface < vSurfaces_Off.size(); iSurface++)
				{
					pSurfaceGroup->AddPair(va(sSCRIPTKEYWORD_NAME "%d",iSurface), vSurfaces_Off[iSurface].c_str(), &pTextPool);
				}
			}
		}

		/*
		boltmodel
		{
			name	"ak74world"
			modelfile	"models/weapons/ak74/world/ak74world.glm"
			parent	"average_sleeves"
			bolttosurface	"*hand_r"
		}
		*/
		if (vBoltData.size())
		{
			for (int iBoltOn=0; iBoltOn<vBoltData.size(); iBoltOn++)
			{
				CGPGroup *pBoltGroup = pModelGroup->AddGroup( sSCRIPTKEYWORD_BOLTMODEL, &pTextPool );
				if (pBoltGroup)
				{
					string strModelName = vBoltData[iBoltOn].first.c_str();
					string strBoltPoint = vBoltData[iBoltOn].second.c_str();

					pBoltGroup->AddPair(sSCRIPTKEYWORD_NAME, Filename_WithoutPath(Filename_WithoutExt(strModelName.c_str())),&pTextPool);
					pBoltGroup->AddPair(sSCRIPTKEYWORD_MODELFILE, strModelName.c_str(),&pTextPool);
					pBoltGroup->AddPair(sSCRIPTKEYWORD_PARENT, strParentName.c_str(),&pTextPool);
					pBoltGroup->AddPair(sSCRIPTKEYWORD_BOLTTOSURFACE, strBoltPoint.c_str(),&pTextPool);
				}
			}
		}

		// now here's hoping Rick's code works...
		//

		// create the text pool...
		//
		CTextPool *pOutputTextPool = new CTextPool(40960);	// any old number, will expand if necessary
		OutputParser.Write(pOutputTextPool);

		strScript = pOutputTextPool->GetPool();	// feed script back to main program as text

		CleanTextPool(pOutputTextPool);
		bReturn = true;
	}

	CleanTextPool(pTextPool);
	return bReturn;
}



// this is pretty quick and dirty for now, it'll fail if you have two identical models loaded, each of which has 
//	something bolted to it (which the real script-writer compensates for correctly).
//
// Oh well, I can tackle that if it ever arises...
//
static bool BOT_CreateModViewScript(CString &strCaptionForErrorPrinting, CString &strScript, BOTFile_t &BOTFile, set <string> &SkinVariants)
{
	bool bReturn = false;

	CTextPool	*pTextPool = new CTextPool(40960);		// any old number, it'll expand internally

	CGenericParser2	OutputParser;						// .. this should be interesting...
					OutputParser.SetWriteable(true);	// itu?

	CGPGroup *pModelGroup = OutputParser.GetBaseParseGroup()->AddGroup( sSCRIPTKEYWORD_LOADMODEL, &pTextPool );
	if (pModelGroup)
	{
		if (gpsGameDir)
		{
			pModelGroup->AddPair(sSCRIPTKEYWORD_BASEDIR, gpsGameDir, &pTextPool);
		}
		// name "SW-967"...	(this field is more of a label really, for bolting purposes). See comment at top.
		//
		pModelGroup->AddPair(sSCRIPTKEYWORD_NAME, BOTFile.strName.c_str(), &pTextPool);

		// 	modelfile	"models/players/swamptrooper/model.glm"
		//
		pModelGroup->AddPair(sSCRIPTKEYWORD_MODELFILE, va("models/players/%s/model.glm",BOTFile.strModel.c_str()), &pTextPool);

		// if any skin variants supplied, grab the one off the top and use it...
		//
		if (!SkinVariants.empty())
		{
			// oldskinfile	"commander"
			//
			pModelGroup->AddPair(sSCRIPTKEYWORD_OLDSKINFILE, (*SkinVariants.begin()).c_str(), &pTextPool);
			SkinVariants.erase(SkinVariants.begin());
		}
  
		// now here's hoping Rick's code works...
		//

		// create the text pool...
		//
		CTextPool *pOutputTextPool = new CTextPool(40960);	// any old number, will expand if necessary
		OutputParser.Write(pOutputTextPool);

		strScript = pOutputTextPool->GetPool();	// feed script back to main program as text

		CleanTextPool(pOutputTextPool);
		bReturn = true;
	}

	CleanTextPool(pTextPool);
	return bReturn;
}	


void CSOF2NPCViewer::OnDblclkListNpcs() 
{
	CListBox* pListBox = (CListBox*)GetDlgItem(IDC_LIST_NPCS);
	assert(pListBox);	
	if (pListBox)
	{
		int iSelected = pListBox->GetCurSel();

		if (iSelected != LB_ERR)
		{
			CString strCaption;

			pListBox->GetText(iSelected, strCaption);

			if (m_bSOF2Mode)
			{
				// SOF2...
				//
				NPCFile_t				NPCFile;
				NPC_CharacterTemplate_t Template;
				NPC_Skin_t				Skin;
				if (NPC_DataLookup(strCaption, NPCFile, Template, Skin))
				{
					CString strScript;

					if (NPC_CreateModViewScript(strCaption, strScript, NPCFile, Template, Skin))
					{
						*gpFeedback = strScript;
						OnOK();
					}
				}
			}
			else
			{
				// JK2...
				//
				BOTFiles_t::iterator itBotFile = TheBOTFiles.find( (LPCSTR) strCaption );
				if (itBotFile != TheBOTFiles.end())
				{
					BOTFile_t &Bot = (*itBotFile).second;

					set <string> SkinVariants;	// leave it empty for this call

					CString strScript;
					if (BOT_CreateModViewScript(strCaption, strScript, Bot, SkinVariants))
					{
						*gpFeedback = strScript;
						OnOK();
					}
				}
			}
		}
	}
}



#include "direct.h"
void Q_mkdir (const char *path)
{
#ifdef WIN32
	if (_mkdir (path) != -1)
		return;
#else
	if (mkdir (path, 0777) != -1)
		return;
#endif
	if (errno != EEXIST)
	{
		ErrorBox (va("mkdir %s: %s",path, strerror(errno)));
	}
}



void CreatePath (const char *path)
{
	const char	*ofs;
	char		c;
	char		dir[1024];

#ifdef _WIN32
	int		olddrive = -1;

	if ( path[1] == ':' )
	{
		olddrive = _getdrive();
		_chdrive( toupper( path[0] ) - 'A' + 1 );
	}
#endif

	if (path[1] == ':')
		path += 2;

	for (ofs = path+1 ; *ofs ; ofs++)
	{
		c = *ofs;
		if (c == '/' || c == '\\')
		{	// create the directory
			memcpy( dir, path, ofs - path );
			dir[ ofs - path ] = 0;
			Q_mkdir( dir );
		}
	}

#ifdef _WIN32
	if ( olddrive != -1 )
	{
		_chdrive( olddrive );
	}
#endif
}

map <string, CString> AllScripts;	// string("NPC caption from list box"), CString(modviewscript)
bool Gallery_Active(void)
{
	return gbDoingGallery;
}
void Gallery_Done(void)
{
	gbDoingGallery = false;
}
LPCSTR Gallery_GetOutputDir(void)
{
	return gstrGalleryOutputDir;
}
LPCSTR Gallery_GetSeqToLock(void)
{
	return gstrGallerySequenceToLock;
}
// reads first entry and removes it.
//
// returns 0 if fail, else number-remaining-plus-one
//
int GalleryRead_ExtractEntry(CString &strCaption, CString &strScript)
{
	map <string, CString>::iterator itGallery = AllScripts.begin();
	if (itGallery == AllScripts.end())
	{
		gbDoingGallery = false;
		return 0;
	}
	
	strCaption  = (*itGallery).first.c_str();
	strScript	= (*itGallery).second;

	AllScripts.erase(itGallery);
	return AllScripts.size() + 1;
}

void CSOF2NPCViewer::OnGallery() 
{
	AllScripts.clear();

#define sDEFAULT_SOF2_SCREENSHOTS_DIR	"c:\\ravenlocal\\sof2\\gallery"
#define sDEFAULT_JK2_SCREENSHOTS_DIR	"c:\\ravenlocal\\jk2\\gallery"	
	
	if (GetYesNo("This will load & screenshot every character in the list\n\nThis can take a *LONG* time. Proceed?"))
	{
		LPCSTR psOutputPath = GetString("Enter destination dir for screenshots (will be created if not found)", m_bSOF2Mode ? sDEFAULT_SOF2_SCREENSHOTS_DIR : sDEFAULT_JK2_SCREENSHOTS_DIR);

		if (psOutputPath)
		{
			gstrGalleryOutputDir = psOutputPath;

			// now for the fun (and mega-tacky!!) part, go through every item in the listbox, 
			//	and generate a complete script for it... :-)    Heh, CPU-abuse!!
			//
			CListBox* pListBox = (CListBox*)GetDlgItem(IDC_LIST_NPCS);
			assert(pListBox);
			if (pListBox)
			{
				int iListItems = pListBox->GetCount();

				bool bGenerateEveryJK2SkinVariant = false;
				if (!m_bSOF2Mode)
				{
					bGenerateEveryJK2SkinVariant = GetYesNo("Generate every available skin variant as a seperate gallery entry?\n\n( 'NO' will just generate one gallery entry per list entry )");
				}

				// work out which sequence to lock...
				//
				if (Model_Loaded())
				{
					ModelHandle_t hModel = AppVars.Container.hModel;

					gstrGallerySequenceToLock = m_bSOF2Mode ? "idle01" : "BOTH_STAND1";

					LPCSTR psSeqLockName = Model_Sequence_GetLockedName( hModel, true);					

					if (psSeqLockName)
					{
						if (GetYesNo(va("Currently-loaded model is locked to sequence \"%s\", use this for all models?\n\n( 'NO' will lock to \"%s\" instead )",psSeqLockName,(LPCSTR) gstrGallerySequenceToLock)))
						{
							gstrGallerySequenceToLock = psSeqLockName;
						}
					}
				}

				// off we go...
				//
				for (int iListItem = 0; iListItem < iListItems; iListItem++)
				{
					StatusMessage( va("Creating script %d/%d...", iListItem, iListItems) );

					CString strCaption;

					pListBox->GetText(iListItem, strCaption);

					if (m_bSOF2Mode)
					{
						// SOF2...
						//
						NPCFile_t				NPCFile;
						NPC_CharacterTemplate_t Template;
						NPC_Skin_t				Skin;
						if (NPC_DataLookup(strCaption, NPCFile, Template, Skin))
						{
							CString strScript;

							if (NPC_CreateModViewScript(strCaption, strScript, NPCFile, Template, Skin))
							{
								AllScripts[ (LPCSTR)strCaption ] = strScript;
							}
						}
					}
					else
					{
						// JK2...
						//
						BOTFiles_t::iterator itBotFile = TheBOTFiles.find( (LPCSTR) strCaption );
						if (itBotFile != TheBOTFiles.end())
						{
							BOTFile_t &Bot = (*itBotFile).second;

							// grab all the skin variants?...
							//
							set <string> SkinVariants;	// eg "model_blue", "model_red"
										 SkinVariants.clear();

							if (bGenerateEveryJK2SkinVariant)
							{
								BOT_ScanSkins(Bot, SkinVariants);
							}

							do
							{
								CString strScript;

								CString strThisSkinVariant;
								if (!SkinVariants.empty())
								{
									strThisSkinVariant = (*SkinVariants.begin()).c_str();
								}

								if (BOT_CreateModViewScript(strCaption, strScript, Bot, SkinVariants))
								{
									CString strEntryName( (LPCSTR)strCaption );

									if (!strThisSkinVariant.IsEmpty())
									{
										strEntryName += va("_%s",(LPCSTR)strThisSkinVariant);
									}

									AllScripts[ (LPCSTR) strEntryName ] = strScript;
								}
							}
							while (!SkinVariants.empty());
						}

					}
				}
				StatusMessage( NULL );

				gbDoingGallery = true;
				OnOK();
			}
		}
	}
}


// simply build a modview script (in memory, then disposes) of every NPC template so that
//	any error messages can popup as they require...
//
// (basically a hack out of the OnGallery() function above)
//
// ( user sees messages by writing them down from popup boxes )
//
void CSOF2NPCViewer::OnValidate() 
{
	CListBox* pListBox = (CListBox*)GetDlgItem(IDC_LIST_NPCS);
	assert(pListBox);
	if (pListBox)
	{
		int iListItems = pListBox->GetCount();

		for (int iListItem = 0; iListItem < iListItems; iListItem++)
		{
			StatusMessage( va("validating template %d/%d...", iListItem, iListItems) );

			CString strCaption;

			pListBox->GetText(iListItem, strCaption);

			NPCFile_t				NPCFile;
			NPC_CharacterTemplate_t Template;
			NPC_Skin_t				Skin;
			if (NPC_DataLookup(strCaption, NPCFile, Template, Skin))
			{
				CString strScript;

				if (NPC_CreateModViewScript(strCaption, strScript, NPCFile, Template, Skin))
				{
					// do nothing...
				}
			}
		}
		StatusMessage( NULL );

		InfoBox("Done");
	}
}

BOOL CSOF2NPCViewer::PreTranslateMessage(MSG* pMsg)
{
	// CG: The following block was added by the ToolTips component.
	{
		// Let the ToolTip process this message.
		m_tooltip.RelayEvent(pMsg);
	}
	return CDialog::PreTranslateMessage(pMsg);	// CG: This was added by the ToolTips component.
}


void CSOF2NPCViewer::OnButtonGenerateList() 
{
	CListBox* pListBox = (CListBox*)GetDlgItem(IDC_LIST_NPCS);
	assert(pListBox);	
	if (pListBox)
	{
		StringSet_t strEntries;

		int iListItems = pListBox->GetCount();
		for (int iListItem = 0; iListItem < iListItems; iListItem++)
		{
			CString strCaption;

			pListBox->GetText(iListItem, strCaption);

			int iLoc = strCaption.FindOneOf(" \t");
			if (iLoc != -1)
			{
				strCaption = strCaption.Left( iLoc );
			}

			strEntries.insert( (LPCSTR) strCaption );
		}

		CString strOutput;

		for (StringSet_t::iterator it = strEntries.begin(); it != strEntries.end(); ++it)
		{
			strOutput += (*it).c_str();
			strOutput += "\n";
		}

		if (!strOutput.IsEmpty())
		{
			SendStringToNotepad(strOutput, "entries.txt");
		}
		else
		{
			ErrorBox("No entries to send!");
		}
	}
}
