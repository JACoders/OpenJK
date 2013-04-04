// commtestView.cpp : implementation of the CCommtestView class
//

#include "stdafx.h"
#include "commtest.h"
#include <assert.h>
#include "commtestDoc.h"
#include "commtestView.h"
#include "../tools/modview/wintalk.h"
#include "bits.h"
#include "mainfrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCommtestView

IMPLEMENT_DYNCREATE(CCommtestView, CTreeView)

BEGIN_MESSAGE_MAP(CCommtestView, CTreeView)
	//{{AFX_MSG_MAP(CCommtestView)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_TIMER()
	ON_NOTIFY_REFLECT(TVN_SELCHANGED, OnSelchanged)
	ON_NOTIFY_REFLECT(NM_DBLCLK, OnDblclk)
	//}}AFX_MSG_MAP
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, CTreeView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, CTreeView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CTreeView::OnFilePrintPreview)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCommtestView construction/destruction

CCommtestView* gpCommTestView = NULL;
CCommtestView::CCommtestView()
{		
}

CCommtestView::~CCommtestView()
{
}

BOOL CCommtestView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	if (!CTreeView::PreCreateWindow(cs))
	{
		return FALSE;
	}
	cs.style |= TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | TVS_SHOWSELALWAYS/* | TVS_EDITLABELS*/;
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CCommtestView drawing



/////////////////////////////////////////////////////////////////////////////
// CCommtestView printing

BOOL CCommtestView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CCommtestView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void CCommtestView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}

/////////////////////////////////////////////////////////////////////////////
// CCommtestView diagnostics

#ifdef _DEBUG
void CCommtestView::AssertValid() const
{
	CTreeView::AssertValid();
}

void CCommtestView::Dump(CDumpContext& dc) const
{
	CTreeView::Dump(dc);
}

CCommtestDoc* CCommtestView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CCommtestDoc)));
	return (CCommtestDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CCommtestView message handlers

enum eTimerHandles
{
	th_DO_NOT_USE = 0,	// system reserved for NULL timer
	th_100FPS = 1,
};


int CCommtestView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CTreeView::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	m_TimerHandle_Update100FPS = SetTimer(	th_100FPS,	// UINT nIDEvent, 
											200,				// UINT nElapse, (in milliseconds)
											NULL				// void (CALLBACK EXPORT* lpfnTimer)(HWND, UINT, UINT, DWORD) 
										);
	
	return 0;
}

BOOL CCommtestView::Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext) 
{
	m_TimerHandle_Update100FPS = 0;
	
	BOOL b = CWnd::Create(lpszClassName, lpszWindowName, dwStyle, rect, pParentWnd, nID, pContext);

	gpCommTestView = this;

	HTREEITEM h =	GetTreeCtrl().InsertItem("test_root_item", NULL);
					GetTreeCtrl().InsertItem("child", h);

					return b;
}

void CCommtestView::OnDestroy() 
{
	if (m_TimerHandle_Update100FPS)
	{
		bool bOk = !!KillTimer(m_TimerHandle_Update100FPS);	// NZ return if good
		if (bOk)
		{
			m_TimerHandle_Update100FPS = NULL;
		}
		else
		{
		//	ErrorBox("Error killing timer!");	// should never happen
		}
	}


	CTreeView::OnDestroy();	
}

void CCommtestView::OnTimer(UINT nIDEvent) 
{
	if (nIDEvent != th_100FPS)
	{
		CTreeView::OnTimer(nIDEvent);
		return;
	}
	
//	WinTalk_HandleMessages();
}

//DEL void CCommtestView::OnDraw(CDC* pDC) 
//DEL {
//DEL 	CCommtestDoc* pDoc = GetDocument();
//DEL 	ASSERT_VALID(pDoc);
//DEL 	// TODO: add draw code for native data here
//DEL }


extern char	*va(char *format, ...);
extern bool gbCommInitialised;
extern bool R_IsDescendantOf(HTREEITEM hTreeItem_This, HTREEITEM hTreeItem_Ancestor);
extern bool gbInhibit;



// search tree for an item whose userdata matches what's been passed...
//
// hTreeItem = tree item to start from, pass NULL to start from root
//
HTREEITEM R_ModelTree_FindItemWithThisData(HTREEITEM hTreeItem, UINT32 uiData2Match)
{
	if (!hTreeItem)
		 hTreeItem = gpCommTestView->GetTreeCtrl().GetRootItem();

	if (hTreeItem)
	{
		CString str(gpCommTestView->GetTreeCtrl().GetItemText(hTreeItem));
		OutputDebugString(va("Scanning item %X (%s)\n",hTreeItem,(LPCSTR)str));

		if (str=="BoltOns")
		{
			int x=1;
		}

		// check this tree item...
		//
		TreeItemData_t	TreeItemData;
						TreeItemData.uiData = gpCommTestView->GetTreeCtrl().GetItemData(hTreeItem);

		// match?...
		//
		if (TreeItemData.uiData == uiData2Match)
			return hTreeItem;

		// check child...
		//
		HTREEITEM hTreeItem_Child = gpCommTestView->GetTreeCtrl().GetChildItem(hTreeItem);
		if (hTreeItem_Child)
		{
			HTREEITEM hTreeItemFound = R_ModelTree_FindItemWithThisData(hTreeItem_Child, uiData2Match);
			if (hTreeItemFound)
				return hTreeItemFound;
		}
		
		// process siblings...
		//
		HTREEITEM hTreeItem_Sibling = gpCommTestView->GetTreeCtrl().GetNextSiblingItem(hTreeItem);
		if (hTreeItem_Sibling)
		{
			HTREEITEM hTreeItemFound = R_ModelTree_FindItemWithThisData(hTreeItem_Sibling, uiData2Match);
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



void CCommtestView::OnSelchanged(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;

	if (gbCommInitialised && GetTreeCtrl().GetCount() > 3 /*?*/ && !gbInhibit)
	{
		HTREEITEM hTreeItem = GetTreeCtrl().GetSelectedItem();

		CString Text =	GetTreeCtrl().GetItemText(hTreeItem);

		TreeItemData_t	TreeItemData;
						TreeItemData.uiData = GetTreeCtrl().GetItemData(hTreeItem);

		LPCSTR psArg = Text;
		if (Text == "Bones")
			psArg = "#all";
		if (Text == "Surfaces")
			psArg = "#all";

//		LPCSTR psAnswer;
		bool bOk = false;		

/*		if (R_IsDescendantOf(hTreeItem, ghTreeItem_Surfaces))
		{
				  WinTalk_IssueCommand(   "model_highlightbone #none");
			bOk = WinTalk_IssueCommand(va("model_highlightsurface %s",psArg),NULL,0,&psAnswer);
		}
		else
		if (R_IsDescendantOf(hTreeItem, ghTreeItem_Bones))
		{
				  WinTalk_IssueCommand(   "model_highlightsurface #none");
			bOk = WinTalk_IssueCommand(va("model_highlightbone %s",psArg),NULL,0,&psAnswer);
		}
		else
		{
			WinTalk_IssueCommand(   "model_highlightbone #none");
			WinTalk_IssueCommand(   "model_highlightsurface #none");
		}
*/

		WinTalk_IssueCommand(va("model_highlightbone %d #none",TreeItemData.iModelHandle));
		WinTalk_IssueCommand(va("model_highlightsurface %d #none",TreeItemData.iModelHandle));

		switch (TreeItemData.iItemType)
		{
			case TREEITEMTYPE_SURFACEHEADER:	// "surfaces"
			{
				bOk = WinTalk_IssueCommand(va("model_highlightsurface %d #all",TreeItemData.iModelHandle));
			}
			break;

			case TREEITEMTYPE_BONEHEADER:		// "bones"
			{
				bOk = WinTalk_IssueCommand(va("model_highlightbone %d #all",TreeItemData.iModelHandle));
			}
			break;

			case TREEITEMTYPE_GLM_SURFACE:		// a surface
			{
				bOk = WinTalk_IssueCommand(va("model_highlightsurface %d %s",TreeItemData.iModelHandle, psArg));
			}
			break;

			case TREEITEMTYPE_BONEALIASHEADER:	// alias header
			{
				bOk = WinTalk_IssueCommand(va("model_highlightbone %d #aliased",TreeItemData.iModelHandle));
			}
			break;

			case TREEITEMTYPE_GLM_BONEALIAS:	// a bone alias
			case TREEITEMTYPE_GLM_BONE:			// a bone
			{
//					  WinTalk_IssueCommand(va("model_highlightsurface %d #none",TreeItemData.iModelHandle));
				bOk = WinTalk_IssueCommand(va("model_highlightbone %d %s",TreeItemData.iModelHandle, psArg));
			}
			break;	
		}

		if (bOk)
		{
			//
		}
		else
		{
//			assert(0);
		}
	}
	
	*pResult = 0;
}


// "psInitialLoadName" param can be "" if not bothered, "psInitialDir" can be NULL to open to last browse-dir
//
// psFilter example:		TEXT("Model files (*.glm)|*.glm|All Files(*.*)|*.*||")	// LPCSTR psFilter
//
char *InputLoadFileName(LPCSTR psInitialLoadName, LPCSTR psCaption, LPCSTR psInitialDir, LPCSTR psFilter)
{
	CFileStatus Filestatus;
	CFile File;
	static char name[MAX_PATH];	
		
	CFileDialog FileDlg(TRUE, NULL, NULL,
		 OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST,
		 //TEXT("Map Project Files (*.mpj)|*.lit|Other Map Files (*.smd/*.sc2)|*.sc2;*.smd|FastMap Files (*.fmf)|*.fmf|All Files(*.*)|*.*||"), //Map Object Files|*.sms||"),
		 psFilter, //Map Object Files|*.sms||"),		 
		 AfxGetMainWnd());				   		 
		
	static CString strInitialDir;
	if (psInitialDir)
		strInitialDir = psInitialDir;

	FileDlg.m_ofn.lpstrInitialDir = (LPCSTR) strInitialDir;
	FileDlg.m_ofn.lpstrTitle=psCaption;	// Load Editor Model";  
	strcpy(name,psInitialLoadName);
	FileDlg.m_ofn.lpstrFile=name;
		
   	if (FileDlg.DoModal() == IDOK)
	{
		return name;
	}
	
	return NULL;
}

LPCSTR Model_GetSupportedTypesFilter(void)
{
	return "Model files (*.glm)|*.glm|All Files(*.*)|*.*||";
}



void CCommtestView::OnDblclk(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;

	if (gbCommInitialised && GetTreeCtrl().GetCount() > 3 /*?*/ && !gbInhibit)
	{	
		HTREEITEM hTreeItem = GetTreeCtrl().GetSelectedItem();

		CString Text =	GetTreeCtrl().GetItemText(hTreeItem);

		TreeItemData_t	TreeItemData;
						TreeItemData.uiData = GetTreeCtrl().GetItemData(hTreeItem);

		bool bOk = false;		

/*		if (R_IsDescendantOf(hTreeItem, ghTreeItem_Surfaces))
		{
				  WinTalk_IssueCommand(   "model_highlightbone #none");
			bOk = WinTalk_IssueCommand(va("model_highlightsurface %s",psArg),NULL,0,&psAnswer);
		}
		else
		if (R_IsDescendantOf(hTreeItem, ghTreeItem_Bones))
		{
				  WinTalk_IssueCommand(   "model_highlightsurface #none");
			bOk = WinTalk_IssueCommand(va("model_highlightbone %s",psArg),NULL,0,&psAnswer);
		}
		else
		{
			WinTalk_IssueCommand(   "model_highlightbone #none");
			WinTalk_IssueCommand(   "model_highlightsurface #none");
		}
*/

		switch (TreeItemData.iItemType)
		{
			case TREEITEMTYPE_MODELNAME:
			{
				// is this a bolted model or the root (find out by seeing if its parent is a "BoltOns" treeitem)...
				//
				HTREEITEM hTreeItemParent = GetTreeCtrl().GetParentItem(hTreeItem);
				if (hTreeItemParent)
				{
					TreeItemData_t	TreeItemData_Parent;
									TreeItemData_Parent.uiData = GetTreeCtrl().GetItemData(hTreeItemParent);
					if (TreeItemData_Parent.iItemType == TREEITEMTYPE_BOLTONSHEADER)
					{
						//if (GetYesNo("Unbolt/discard this model?"))
						{
							//<modelhandle of bolted thing to delete>
							WinTalk_IssueCommand(va("model_deletebolton %d",TreeItemData.iModelHandle));
							GetTreeCtrl().DeleteItem(hTreeItem);
						}
					}
				}
			}
			break;

			case TREEITEMTYPE_GLM_BONE:			// a bone
			{
				extern CMainFrame* gCMainFrame;
				if (gCMainFrame)
				{
					char *psFullPathedFilename = InputLoadFileName(	"",				// LPCSTR psInitialLoadName, 
																	"Load Model",	// LPCSTR psCaption,
																	"S:\\base\\models\\test\\conetree4",	// LPCSTR psInitialDir, 
																	Model_GetSupportedTypesFilter()			// LPCSTR psFilter
																	);
					if (psFullPathedFilename)
					{
						gCMainFrame->BoltModel(TreeItemData.iModelHandle, Text, psFullPathedFilename);
					}
				}
			}
			break;
		}

		if (bOk)
		{
			//
		}
		else
		{
//			assert(0);
		}
	}
	
	*pResult = 0;
}
