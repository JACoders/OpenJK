// MainFrm.cpp : implementation of the CMainFrame class
//
#include "stdafx.h"	 
#include "commtest.h"
#include "../tools/modview/wintalk.h"

#include "commtestDoc.h"
#include "commtestview.h"
#include <assert.h>

#include "bits.h"

#include "MainFrm.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
	ON_COMMAND(ID_EDIT_PASTE, OnEditPaste)
	ON_COMMAND(ID_EDIT_CUT, OnEditCut)
	ON_COMMAND(ID_ERRORBOX_OFF, OnErrorboxOff)
	ON_COMMAND(ID_ERRORBOX_ON, OnErrorboxOn)
	ON_COMMAND(ID_EDIT_M4, OnEditM4)
	ON_COMMAND(ID_STARTANIM, OnStartanim)
	ON_COMMAND(ID_STARTANIMWRAP, OnStartanimwrap)
	ON_COMMAND(ID_STOPANIM, OnStopanim)
	ON_COMMAND(ID_LOCK_SEQUENCES, OnLockSequences)
	ON_COMMAND(ID_UNLOCKALLSEQS, OnUnlockallseqs)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
	// TODO: add member initialization code here
	
}

CMainFrame::~CMainFrame()
{
}

CMainFrame* gCMainFrame = NULL;

HWND g_hWnd;
int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP
		| CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
		!m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}

	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators,
		  sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}

	// TODO: Delete these three lines if you don't want the toolbar to
	//  be dockable
	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	EnableDocking(CBRS_ALIGN_ANY);
	DockControlBar(&m_wndToolBar);
	
	g_hWnd = m_hWnd;

	gCMainFrame = this;	// eeeuuwww!!!!!

	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CFrameWnd::PreCreateWindow(cs) )
		return FALSE;
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers



char	*va(char *format, ...);

void R_OurTree_AddRemoteItemToThisParent(TreeItemData_t	TreeItemData, HTREEITEM hTreeItem_Parent, HTREEITEM hTreeItem_Remote )
{
	LPCSTR psAnswer;

	// get remote item text...
	//
	if (!WinTalk_IssueCommand(va("modeltree_getitemtextpure %d",hTreeItem_Remote),NULL,0,&psAnswer))
	{
		// should never happen...
		//
		assert(0);
		psAnswer = "Error!";
	}
	// generate new tree item...
	//
	HTREEITEM hTreeItem_This =	gpCommTestView->GetTreeCtrl().InsertItem ( psAnswer, hTreeItem_Parent);
								gpCommTestView->GetTreeCtrl().SetItemData( hTreeItem_This, TreeItemData.uiData);

	// check remote version of this for children...
	//
	if (WinTalk_IssueCommand(va("modeltree_getchilditem %d",hTreeItem_Remote),NULL,0,&psAnswer))
	{
		HTREEITEM hTreeItemRemote_Child = (HTREEITEM) atoi(psAnswer);

		if (hTreeItemRemote_Child)
		{
			R_OurTree_AddRemoteItemToThisParent( TreeItemData, hTreeItem_This, hTreeItemRemote_Child);			
		}
	}

	// check remote version of this for sibling...
	//
	if (WinTalk_IssueCommand(va("modeltree_getnextsiblingitem %d",hTreeItem_Remote),NULL,0,&psAnswer))
	{
		HTREEITEM hTreeItemRemote_Sibling = (HTREEITEM) atoi(psAnswer);

		if (hTreeItemRemote_Sibling)
		{
			R_OurTree_AddRemoteItemToThisParent( TreeItemData, hTreeItem_Parent, hTreeItemRemote_Sibling);			
		}
	}
}



//
void ExpandTreeItem( HTREEITEM hTreeItem )
{
	gpCommTestView->GetTreeCtrl().Expand( hTreeItem, TVE_EXPAND );
}


void R_ApplyToTreeItem( void (*pFunction) ( HTREEITEM hTreeItem ), HTREEITEM hTreeItem )
{
	if (hTreeItem)
	{
		// process item...
		//
		pFunction(hTreeItem);


		// recurse child...
		//
		R_ApplyToTreeItem( pFunction, gpCommTestView->GetTreeCtrl().GetChildItem( hTreeItem ) );
		

		// recurse sibling...
		//
		R_ApplyToTreeItem( pFunction, gpCommTestView->GetTreeCtrl().GetNextSiblingItem( hTreeItem ) );
	}
}


bool R_IsDescendantOf(HTREEITEM hTreeItem_This, HTREEITEM hTreeItem_Ancestor)
{
	if (hTreeItem_This == hTreeItem_Ancestor)
		return true;

	HTREEITEM hTreeItem_Parent = gpCommTestView->GetTreeCtrl().GetParentItem(hTreeItem_This);
	if (hTreeItem_Parent)
	{
		return R_IsDescendantOf(hTreeItem_Parent, hTreeItem_Ancestor);
	}

	return false;
}


// made global so they can be accessed from another module...
//
HTREEITEM ghTreeItem_ModelName	= NULL;
HTREEITEM ghTreeItem_Surfaces= NULL;
HTREEITEM ghTreeItem_Bones	= NULL;

ModelHandle_t ghLastLoadedModel = NULL;
ModelHandle_t ghPrimaryModel = NULL;
int giPrimaryModel_NumSequences=0;

void OurTree_Generate(LPCSTR psModelName, ModelHandle_t hModel, HTREEITEM hTreeItemParent = TVI_ROOT);
void OurTree_Generate(LPCSTR psModelName, ModelHandle_t hModel, HTREEITEM hTreeItemParent)
{
	ghLastLoadedModel = hModel;

	if (hTreeItemParent == TVI_ROOT)	// loading primary model?
		gpCommTestView->GetTreeCtrl().DeleteAllItems();

	TreeItemData_t	TreeItemData={0};
					TreeItemData.iModelHandle = hModel;

	TreeItemData.iItemType	=	TREEITEMTYPE_MODELNAME;
	ghTreeItem_ModelName	=	gpCommTestView->GetTreeCtrl().InsertItem(psModelName,	hTreeItemParent);
								gpCommTestView->GetTreeCtrl().SetItemData(ghTreeItem_ModelName,TreeItemData.uiData);

	TreeItemData.iItemType	=	TREEITEMTYPE_SURFACEHEADER;
	ghTreeItem_Surfaces		=	gpCommTestView->GetTreeCtrl().InsertItem("Surfaces",	ghTreeItem_ModelName);
								gpCommTestView->GetTreeCtrl().SetItemData(ghTreeItem_Surfaces, TreeItemData.uiData);

	TreeItemData.iItemType	=	TREEITEMTYPE_BONEHEADER;
	ghTreeItem_Bones		=	gpCommTestView->GetTreeCtrl().InsertItem("Bones",		ghTreeItem_ModelName);
								gpCommTestView->GetTreeCtrl().SetItemData(ghTreeItem_Bones, TreeItemData.uiData);

	TreeItemData.iItemType	=	TREEITEMTYPE_BONEALIASHEADER;
	HTREEITEM hTreeItem_Aliases=gpCommTestView->GetTreeCtrl().InsertItem("Bone Aliases",ghTreeItem_ModelName);
								gpCommTestView->GetTreeCtrl().SetItemData(hTreeItem_Aliases, TreeItemData.uiData);

	// ok, now we need to remote-ask ModView for the HTREEITEM values for its Bone and Surface items...
	//
	LPCSTR psAnswer = NULL;
//	if (WinTalk_IssueCommand(va("modeltree_getrootsurface %s",va("%d",hModel)),NULL,0,&psAnswer))
	if (WinTalk_IssueCommand(va("modeltree_getrootsurface %d",hModel),NULL,0,&psAnswer))
	{
		HTREEITEM hTreeItemRemote_RootSurface = (HTREEITEM) atoi(psAnswer);

//		if (WinTalk_IssueCommand(va("modeltree_getrootbone %s",va("%d",hModel)),NULL,0,&psAnswer))
		if (WinTalk_IssueCommand(va("modeltree_getrootbone %d",hModel),NULL,0,&psAnswer))
		{
			HTREEITEM hTreeItemRemote_RootBone = (HTREEITEM) atoi(psAnswer);

			TreeItemData.iItemType = TREEITEMTYPE_GLM_SURFACE;
			R_OurTree_AddRemoteItemToThisParent(TreeItemData, ghTreeItem_Surfaces,	hTreeItemRemote_RootSurface	);
			TreeItemData.iItemType = TREEITEMTYPE_GLM_BONE;
			R_OurTree_AddRemoteItemToThisParent(TreeItemData, ghTreeItem_Bones,		hTreeItemRemote_RootBone	);

			// add bone aliases...
			if (WinTalk_IssueCommand(va("model_getnumbonealiases %d",hModel),NULL,0,&psAnswer))
			{
				int iNumBoneAliases = atoi(psAnswer);

				for (int iBoneAlias = 0; iBoneAlias < iNumBoneAliases; iBoneAlias++)
				{
					if (WinTalk_IssueCommand(va("model_getbonealias %d %d",hModel,iBoneAlias),NULL,0,&psAnswer))
					{
						CString		strRealName(psAnswer);
						int iLoc =	strRealName.Find(" ",0);

						if (iLoc!=-1)
						{
							CString strAliasName(strRealName.Mid(iLoc+1));

							strRealName = strRealName.Left(iLoc);	// I don't actually use this in this app, but FYI.

							OutputDebugString(va("alias %d/%d: real = '%s', alias = '%s'\n",iBoneAlias,iNumBoneAliases,(LPCSTR)strRealName,(LPCSTR)strAliasName));

							TreeItemData.iItemType = TREEITEMTYPE_GLM_BONEALIAS;
							HTREEITEM hTreeItemTemp = gpCommTestView->GetTreeCtrl().InsertItem(strAliasName, hTreeItem_Aliases);
							gpCommTestView->GetTreeCtrl().SetItemData( hTreeItemTemp, TreeItemData.uiData);
						}
						else
						{
							assert(0);
						}
					}
					else
					{
						assert(0);
					}
				}
			}
			else
			{
				assert(0);
			}

			// query sequence info, and add to tree if present...
			//
			if (WinTalk_IssueCommand(va("model_getnumsequences %s",va("%d",hModel)),NULL,0,&psAnswer))
			{
				int iNumSequences = atoi(psAnswer);

				if (hModel == ghPrimaryModel)
				{
					giPrimaryModel_NumSequences = iNumSequences;
				}

				if (iNumSequences)
				{
					TreeItemData.iItemType = TREEITEMTYPE_SEQUENCEHEADER;
					HTREEITEM hTreeItem_Sequences = gpCommTestView->GetTreeCtrl().InsertItem("Sequences",ghTreeItem_ModelName);
													gpCommTestView->GetTreeCtrl().SetItemData(hTreeItem_Sequences, TreeItemData.uiData);
						
					for (int iSeq=0; iSeq<iNumSequences; iSeq++)
					{
						if (WinTalk_IssueCommand(va("model_getsequence %s %d",va("%d",hModel),iSeq),NULL,0,&psAnswer))
						{
							TreeItemData.iItemType	= TREEITEMTYPE_SEQUENCE;
							TreeItemData.iItemNumber= iSeq;
							HTREEITEM hTreeItem_Sequence =	gpCommTestView->GetTreeCtrl().InsertItem (psAnswer, hTreeItem_Sequences);
															gpCommTestView->GetTreeCtrl().SetItemData(hTreeItem_Sequence, TreeItemData.uiData);
						}
						else
						{
							assert(0);
						}
					}
					TreeItemData.iItemNumber = 0;
				}

				// add BoltOns tree item header...
				//
				TreeItemData.iItemType	=	TREEITEMTYPE_BOLTONSHEADER;
				HTREEITEM hTreeItemBolts=	gpCommTestView->GetTreeCtrl().InsertItem("BoltOns",	ghTreeItem_ModelName);
											gpCommTestView->GetTreeCtrl().SetItemData(hTreeItemBolts, TreeItemData.uiData);
			}
			else
			{
				assert(0);
			}
		}
		else
		{
			assert(0);
		}
	}
	else
	{
		assert(0);
	}


	R_ApplyToTreeItem( ExpandTreeItem, gpCommTestView->GetTreeCtrl().GetRootItem() );
	gpCommTestView->GetTreeCtrl().SelectSetFirstVisible(gpCommTestView->GetTreeCtrl().GetRootItem());
}

// returns actual filename only, no path
//
char *Filename_WithoutPath(LPCSTR psFilename)
{
	static char sString[MAX_PATH];
/*
	LPCSTR p = strrchr(psFilename,'\\');

  	if (!p++)
	{
		p = strrchr(psFilename,'/');
		if (!p++)
			p=psFilename;
	}

	strcpy(sString,p);
*/

	LPCSTR psCopyPos = psFilename;
	
	while (*psFilename)
	{
		if (*psFilename == '/' || *psFilename == '\\')
			psCopyPos = psFilename+1;
		psFilename++;
	}

	strcpy(sString,psCopyPos);

	return sString;

}


bool gbInhibit = false;
void CMainFrame::LoadModel(LPCSTR psFullPathedFileName)
{
	gbInhibit = true;

	LPCSTR psAnswer = NULL;

	if (WinTalk_IssueCommand(va("model_loadprimary %s",psFullPathedFileName),NULL,0,&psAnswer))
	{			
		ModelHandle_t hModel = (ModelHandle_t) atoi(psAnswer);
		ghPrimaryModel = hModel;	// do this BEFORE OurTree_Generate()
		OurTree_Generate(Filename_WithoutPath(psFullPathedFileName),hModel);
	}
	else
	{
		ghPrimaryModel = NULL;		// do this BEFORE OurTree_Generate()
		gpCommTestView->GetTreeCtrl().DeleteAllItems();
	}

	gbInhibit = false;
}

extern HTREEITEM R_ModelTree_FindItemWithThisData(HTREEITEM hTreeItem, UINT32 uiData2Match);
void CMainFrame::BoltModel(ModelHandle_t hModel, LPCSTR psBoltName, LPCSTR psFullPathedName)
{
	LPCSTR psAnswer = NULL;

	if (WinTalk_IssueCommand(va("model_loadbolton %d %s %s",hModel,psBoltName,psFullPathedName),NULL,0,&psAnswer))
	{
		ModelHandle_t hModel_BoltOn = (ModelHandle_t) atoi(psAnswer);

		TreeItemData_t	TreeItemData = {0};
						TreeItemData.iModelHandle	= hModel;
						TreeItemData.iItemType		= TREEITEMTYPE_BOLTONSHEADER;

		HTREEITEM hTreeItem_BoltOns = R_ModelTree_FindItemWithThisData(NULL, TreeItemData.uiData);

		OurTree_Generate(Filename_WithoutPath(psFullPathedName),hModel_BoltOn,hTreeItem_BoltOns);
	}
}

void CMainFrame::OnEditCopy() 
{
	LoadModel("S:/base/models/test/conetree4/conetree4.glm");
}

void CMainFrame::OnEditPaste() 
{
	LoadModel("S:/base/models/test/jake/jake.glm");
}

void CMainFrame::OnEditCut() 
{
	LoadModel("S:/base/models/test/baddir/badname.glm");
}

void CMainFrame::OnEditM4() 
{
	LoadModel("S:/base/models/test/m4/m4.glm");
}

void CMainFrame::OnErrorboxOff() 
{
	WinTalk_IssueCommand("errorbox_disable");	
}

void CMainFrame::OnErrorboxOn() 
{
	WinTalk_IssueCommand("errorbox_enable");	
}


void CMainFrame::OnStartanim() 
{
	WinTalk_IssueCommand("startanim");
}

void CMainFrame::OnStartanimwrap() 
{
	WinTalk_IssueCommand("startanimwrap");
}

void CMainFrame::OnStopanim() 
{
	WinTalk_IssueCommand("stopanim");
}

void CMainFrame::OnLockSequences() 
{
	if (ghPrimaryModel && giPrimaryModel_NumSequences)
	{
		static int iLockSeq = -1;

		if (++iLockSeq >= giPrimaryModel_NumSequences)
		{
			iLockSeq = 0;
		}
		
		WinTalk_IssueCommand(va("model_locksequence %d, %d", ghPrimaryModel, iLockSeq));
	}
}

void CMainFrame::OnUnlockallseqs() 
{
	if (ghPrimaryModel)
	{
		WinTalk_IssueCommand(va("model_unlocksequences %d",ghPrimaryModel));
	}
}

