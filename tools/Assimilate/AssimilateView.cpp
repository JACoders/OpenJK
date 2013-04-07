// AssimilateView.cpp : implementation of the CAssimilateView class
//

#include "stdafx.h"
#include "Includes.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CAssimilateView* ghAssimilateView = NULL;

/////////////////////////////////////////////////////////////////////////////
// CAssimilateView

IMPLEMENT_DYNCREATE(CAssimilateView, CTreeView)

BEGIN_MESSAGE_MAP(CAssimilateView, CTreeView)
	//{{AFX_MSG_MAP(CAssimilateView)
	ON_NOTIFY_REFLECT(NM_DBLCLK, OnDblclk)
	ON_WM_DESTROY()
	ON_NOTIFY_REFLECT(TVN_ITEMEXPANDED, OnItemexpanded)
	ON_WM_RBUTTONDBLCLK()
	ON_WM_KEYDOWN()
	ON_NOTIFY_REFLECT(TVN_SELCHANGED, OnSelchanged)
	//}}AFX_MSG_MAP
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, CTreeView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, CTreeView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CTreeView::OnFilePrintPreview)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAssimilateView construction/destruction

CAssimilateView::CAssimilateView()
{
	ghAssimilateView = this;
}

CAssimilateView::~CAssimilateView()
{
}

BOOL CAssimilateView::PreCreateWindow(CREATESTRUCT& cs)
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
// CAssimilateView drawing

void CAssimilateView::OnDraw(CDC* pDC)
{
	CAssimilateDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	// TODO: add draw code for native data here
}

/////////////////////////////////////////////////////////////////////////////
// CAssimilateView printing

BOOL CAssimilateView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CAssimilateView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void CAssimilateView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}

/////////////////////////////////////////////////////////////////////////////
// CAssimilateView diagnostics

#ifdef _DEBUG
void CAssimilateView::AssertValid() const
{
	CTreeView::AssertValid();
}

void CAssimilateView::Dump(CDumpContext& dc) const
{
	CTreeView::Dump(dc);
}

CAssimilateDoc* CAssimilateView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CAssimilateDoc)));
	return (CAssimilateDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CAssimilateView message handlers

void CAssimilateView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{	
	// TODO: Add your specialized code here and/or call the base class
	CTreeCtrl& tree = GetTreeCtrl();

	switch(lHint)
	{
	case AS_DELETECONTENTS:		
		tree.DeleteAllItems();
		break;
	case AS_NEWFILE:
		// findme: hmmm, not sure about this. Adding files zaps the tree? Would look odd to user...
		tree.DeleteAllItems();
		BuildTree();
		UpdateTree();
		break;
	case AS_FILESUPDATED:		
		UpdateTree();	// sorts tree according to enum names, sorts model according to new tree, re-draws tree
		break;
	default:
		break;
	}

	SetFirstModelTitleAndMode();
}



// in practise, the Assimilate app will only ever use one sort type, but wtf...
//
typedef enum
{
	TS_BY_ENUMTYPE = 0,
} TREESORTTYPE;

//
// tree item compare callback,
//
// returns (to windows):- (so do NOT change these defines)
//
#define ITEM1_BEFORE_ITEM2 -1
#define ITEM1_MATCHES_ITEM2 0
#define ITEM1_AFTER_ITEM2   1
//
int CALLBACK TreeCompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	CSequence* seq1 = (CSequence*) lParam1;
	CSequence* seq2 = (CSequence*) lParam2;

	switch (lParamSort)
	{
		case TS_BY_ENUMTYPE:
		{
			// enumtypes are enum'd lower-numbers-more-important...
			//
			if (seq1->GetEnumType() < seq2->GetEnumType())
			{
				return ITEM1_BEFORE_ITEM2;
			}

			if (seq1->GetEnumType() > seq2->GetEnumType())
			{
				return ITEM1_AFTER_ITEM2;
			}

			// must be same type, so sort alphabetically...
			// (strcmp almost returns correct values, except the results are signed anynums, not explicitly -1/0/1)
			//
			return ITEM1_MATCHES_ITEM2;
/*
			int i = strcmp(seq1->GetEnum(),seq2->GetEnum());

			if (i<0)
			{
				return ITEM1_BEFORE_ITEM2;
			}

			if (i>0)
			{
				return ITEM1_AFTER_ITEM2;
			}

			return ITEM1_MATCHES_ITEM2;
*/
		}
		break;
	}

	ASSERT(0);
	return ITEM1_MATCHES_ITEM2;	
}

bool TreeSort(CTreeCtrl& tree, TREESORTTYPE ts)
{
	HTREEITEM hTreeItem_Model = tree.GetRootItem();	// "klingon"

	while (hTreeItem_Model)
	{
		TVSORTCB sortParams;

		sortParams.hParent		= hTreeItem_Model;
		sortParams.lpfnCompare	= TreeCompareFunc;
		sortParams.lParam		= ts;

		tree.SortChildrenCB(&sortParams);

		hTreeItem_Model = tree.GetNextSiblingItem(hTreeItem_Model);				
	}

	return true;
}

// returns true if any delete has taken place (sequence or model)...
//
bool CAssimilateView::DeleteCurrentItem(bool bNoQueryForSequenceDelete)
{
//	OutputDebugString(va("DeleteCurrentItem %s\n",bNoQueryForSequenceDelete?"CTRL":""));

	// ok, what *is* the current item?
	//	
	CTreeCtrl& tree = GetTreeCtrl();
	HTREEITEM theItem = tree.GetSelectedItem();
	if (theItem == NULL)
	{
		return false;
	}

	// ok, so it's either a model or a sequence...
	//
	CModel* curModel = NULL;
	if (tree.GetParentItem(theItem) == NULL)
	{
		curModel = (CModel*)tree.GetItemData(theItem);
		if (curModel != NULL)
		{
			if (GetYesNo(va("Delete whole model \"%s\"?",curModel->GetName())))
			{				
				tree.DeleteItem(theItem);
				GetDocument()->DeleteModel(curModel);
				GetDocument()->SetModifiedFlag();
				return true;
			}			
		}
	}
	else
	{
		curModel = (CModel*)tree.GetItemData(tree.GetParentItem(theItem));
		CSequence* curSequence = (CSequence*)tree.GetItemData(theItem);
		if (curSequence != NULL)
		{
			// ask them if they're sure, unless the ctrl key is also pressed...
			//
			if (bNoQueryForSequenceDelete || GetYesNo(va("Delete sequence \"%s\"?",curSequence->GetName())))
			{
				tree.DeleteItem(theItem);
				curModel->DeleteSequence(curSequence);
				GetDocument()->SetModifiedFlag();
				return true;
			}
		}
	}

	return false;
}

// also re-orders model(s) sequences to match tree order...
//
void CAssimilateView::SortTree()
{
	TreeSort(GetTreeCtrl(),TS_BY_ENUMTYPE);

	// now update models to reflect their positions within the tree struct...
	//
	HTREEITEM hTreeItem_Model = GetTreeCtrl().GetRootItem();	// "klingon"

	while (hTreeItem_Model)
	{
		CModel* curModel = (CModel*) GetTreeCtrl().GetItemData(hTreeItem_Model);
//		OutputDebugString(va("item = '%s'\n",(LPCSTR) GetTreeCtrl().GetItemText(hTreeItem_Model)));

		int iSequenceNum = 0;

		HTREEITEM hTreeItem_Sequence = GetTreeCtrl().GetChildItem(hTreeItem_Model);

		while (hTreeItem_Sequence)
		{
			CSequence* curSequence = (CSequence*) GetTreeCtrl().GetItemData(hTreeItem_Sequence);

			curSequence->m_iSequenceNumber = iSequenceNum++;

			hTreeItem_Sequence = GetTreeCtrl().GetNextSiblingItem(hTreeItem_Sequence);
		}

		curModel->ReOrderSequences();

		hTreeItem_Model = GetTreeCtrl().GetNextSiblingItem(hTreeItem_Model);		
	}

	// sod it, too much work to check if re-ordering actually took place. Let's assume it did... :-)
	//
	GetDocument()->SetModifiedFlag();
}

// sorts tree according to enum names, sorts model according to new tree, re-draws tree
//	(re-draw = changes icons & text according to enum-validity and prefs)
//
void CAssimilateView::UpdateTree()
{
	/* DT EDIT - This re-sorts the .car files animation list in alphabetical order which can be very bad. Commenting this out 
	             leaves the .car file animation list order as-is which is needed for Raven's supplied "humanoid.car" so animations 
				 aren't broken ingame.*/
	//SortTree();		// sorts tree according to enum names, sorts model according to new tree

	HTREEITEM hTreeItem_Model = GetTreeCtrl().GetRootItem();	// "klingon"

	while (hTreeItem_Model)
	{
		CModel* curModel = (CModel*) GetTreeCtrl().GetItemData(hTreeItem_Model);
//		OutputDebugString(va("item = '%s'\n",(LPCSTR) GetTreeCtrl().GetItemText(hTreeItem_Model)));

		HTREEITEM hTreeItem_Sequence = GetTreeCtrl().GetChildItem(hTreeItem_Model);

		while (hTreeItem_Sequence)
		{			
			CSequence* curSequence = (CSequence*) GetTreeCtrl().GetItemData(hTreeItem_Sequence);

//			OutputDebugString(va("item = '%s'\n",(LPCSTR) GetTreeCtrl().GetItemText(hTreeItem_Sequence)));

//			gQuakeHelperTreeViewhandle->GetTreeCtrl().SetItemState(hTreeItem, bMatch?TVIS_SELECTED:0, TVIS_SELECTED);					

			// update this tree item accordingly...
			//
			CDC *pDC = GetDC();
			GetTreeCtrl().SetItemText(hTreeItem_Sequence, curSequence->GetDisplayNameForTree(curModel,gbViewAnimEnums,gbViewFrameDetails,gbViewFrameDetails_Additional,pDC));
			ReleaseDC(pDC);
			int iIcon = curSequence->GetDisplayIconForTree(curModel);
			GetTreeCtrl().SetItemImage(hTreeItem_Sequence, iIcon, iIcon);
			hTreeItem_Sequence = GetTreeCtrl().GetNextSiblingItem(hTreeItem_Sequence);
		}

		hTreeItem_Model = GetTreeCtrl().GetNextSiblingItem(hTreeItem_Model);		
	}
}


PlayerMode_e CAssimilateView::GetSingleOrMultiPlayerMode()
{
	PlayerMode_e ePlayerMode = eMODE_BAD;

	LPCSTR psEnumFilename = ((CAssimilateApp*)AfxGetApp())->GetEnumFilename();

	if (psEnumFilename && !stricmp(psEnumFilename,sDEFAULT_ENUM_FILENAME))
		ePlayerMode = eMODE_SINGLE;
	else
	if (psEnumFilename && !stricmp(psEnumFilename,sDEFAULT_ENUM_FILENAME_MULTI))
		ePlayerMode = eMODE_MULTI;

	return ePlayerMode;
}

void CAssimilateView::SetFirstModelTitleAndMode()
{
	PlayerMode_e ePlayerMode = GetSingleOrMultiPlayerMode();

	CTreeCtrl& tree = GetTreeCtrl();
	CModel* curModel = GetDocument()->GetFirstModel();
	if (curModel)
	{
		HTREEITEM hTreeItem = tree.GetRootItem();
		tree.SetItemText(hTreeItem, va("%s: ( %s )",curModel->GetName(), (ePlayerMode == eMODE_SINGLE)?"Single-Player Mode":(ePlayerMode == eMODE_MULTI)?"Multi-Player Mode":"Unknown Mode - Prefs not setup?"));
	}
}

void CAssimilateView::BuildTree()
{	
	CTreeCtrl& tree = GetTreeCtrl();

	CModel* curModel = GetDocument()->GetFirstModel();
	while(curModel != NULL)
	{
		HTREEITEM item = tree.InsertItem(curModel->GetName());
		tree.SetItemData(item, DWORD(curModel));
		tree.SetItemImage(item, ObjID_Folder, ObjID_Folder);
		CSequence* curSequence = curModel->GetFirstSequence();
		while(curSequence != NULL)
		{			
			CDC *pDC = GetDC();
			HTREEITEM seqItem = tree.InsertItem(curSequence->GetDisplayNameForTree(curModel,gbViewAnimEnums,gbViewFrameDetails,gbViewFrameDetails_Additional,pDC), item);
			ReleaseDC(pDC);
			tree.SetItemData(seqItem, DWORD(curSequence));
/*			if (curSequence->ValidEnum())
			{
				tree.SetItemImage(seqItem, ObjID_Sequence, ObjID_Sequence);
			}
			else
			{
				tree.SetItemImage(seqItem, ObjID_MissingEnum, ObjID_MissingEnum);
			}*/
			int iIcon = curSequence->GetDisplayIconForTree(curModel);
			tree.SetItemImage(seqItem, iIcon, iIcon);

			curSequence = curSequence->GetNext();
		}
		curModel = curModel->GetNext();
	}

	SetFirstModelTitleAndMode();
}

void CAssimilateView::OnDblclk(NMHDR* pNMHDR, LRESULT* pResult) 
{	
//	OutputDebugString("double\n");

	*pResult = 0;

	CTreeCtrl& tree = GetTreeCtrl();
	HTREEITEM theItem = tree.GetSelectedItem();
	if (theItem == NULL)
	{
		return;
	}
	if (tree.GetParentItem(theItem) == NULL)
	{
		CModel* theModel = (CModel*)tree.GetItemData(theItem);
		if (theModel != NULL)
		{
			if (theModel->DoProperties())
			{
				GetDocument()->SetModifiedFlag();
			}
			*pResult = 1;
		}
	}
	else
	{
		CSequence* theSequence = (CSequence*)tree.GetItemData(theItem);
		if (theSequence != NULL)
		{
			if (theSequence->DoProperties())
			{
				GetDocument()->SetModifiedFlag();
			}
			*pResult = 1;
		}
	}
}


void CAssimilateView::OnInitialUpdate() 
{
	CTreeView::OnInitialUpdate();
	
	// TODO: Add your specialized code here and/or call the base class
	CImageList *pimagelist;
	pimagelist = new CImageList();
	pimagelist->Create(IDB_TREEIMAGES, 16, FALSE, 0x00ffffff);
	CTreeCtrl& theTree = GetTreeCtrl();
	CImageList* oldImageList;
	oldImageList = theTree.GetImageList(TVSIL_NORMAL);
	if (oldImageList != NULL)
		delete(oldImageList);
	theTree.SetImageList(pimagelist, TVSIL_NORMAL);	
}

void CAssimilateView::OnDestroy() 
{
	CTreeCtrl& theTree = GetTreeCtrl();
	CImageList *pimagelist;
	pimagelist = theTree.GetImageList(TVSIL_NORMAL);
	if (pimagelist != NULL)
	{
		pimagelist->DeleteImageList();
		delete pimagelist;
	}
	CTreeView::OnDestroy();
	
	// TODO: Add your message handler code here
	
}

void CAssimilateView::OnItemexpanded(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	// TODO: Add your control notification handler code here
	CTreeCtrl& theTree = GetTreeCtrl();
	HTREEITEM curSelection = pNMTreeView->itemNew.hItem;
	if (theTree.GetParentItem(curSelection) == NULL)
	{
		int expandState = theTree.GetItemState(curSelection, TVIS_EXPANDED);
		if (theTree.GetItemState(curSelection, TVIS_EXPANDED) & TVIS_EXPANDED)
		{
			theTree.SetItemImage(curSelection, ObjID_OpenFolder, ObjID_OpenFolder);
		}
		else
		{
			theTree.SetItemImage(curSelection, ObjID_Folder, ObjID_Folder);
		}
	}
	*pResult = 0;
}

void CAssimilateView::OnRButtonDblClk(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	CTreeCtrl& tree = GetTreeCtrl();
	HTREEITEM theItem = tree.GetSelectedItem();
	if (theItem == NULL)
	{
		CTreeView::OnRButtonDblClk(nFlags, point);
		return;	//don't have an item, don't go on
	}
	if (tree.GetParentItem(theItem) == NULL)
	{
		CModel* theModel = (CModel*)tree.GetItemData(theItem);
		if (theModel != NULL)
		{
			if (theModel->DoProperties())
			{
				GetDocument()->SetModifiedFlag();
			}
		}
	}
	else
	{
		/*	// fixme: update this stuff some other time
		CSequence* theSequence = (CSequence*)tree.GetItemData(theItem);
		if (theSequence != NULL)
		{
			if (theSequence->Parse())
			{
				GetDocument()->SetModifiedFlag();
			}
		}
		*/
	}
}




void CAssimilateView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	switch (nChar)
	{
		case VK_DELETE:
		{			
			if (DeleteCurrentItem(!!(GetAsyncKeyState(VK_CONTROL))))	// NoQueryForSequenceDelete if CTRL pressed
			{
				UpdateTree();	// sorts tree according to enum names, sorts model according to new tree, re-draws tree
			}
		}
		break;
	}
	
	CTreeView::OnKeyDown(nChar, nRepCnt, nFlags);
}



// currently tells the document which model is the one the user considers current (ie which one is blue in tree)
//
void CAssimilateView::OnSelchanged(NMHDR* pNMHDR, LRESULT* pResult) 
{
//	OutputDebugString("OnSelchanged TOP\n");
	GetDocument()->ClearModelUserSelectionBools();

	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;	
	HTREEITEM theItem = pNMTreeView->itemNew.hItem;	

	if (!theItem)
	{
		return;
	}

	CTreeCtrl& tree = GetTreeCtrl();

	CModel *theModel = NULL;
	if (tree.GetParentItem(theItem) == NULL)
	{
		theModel = (CModel*)tree.GetItemData(theItem);
		ASSERT(theModel);
		if (theModel)
		{
			theModel->SetUserSelectionBool();
		}
	}
	else
	{
		theModel = (CModel*)tree.GetItemData(tree.GetParentItem(theItem));
		ASSERT(theModel);
		if (theModel)
		{
			theModel->SetUserSelectionBool();
		}
	}

//	OutputDebugString("OnSelchanged\n");
//	if (theModel)
//	{
//		OutputDebugString(va("Model = %s\n",theModel->GetName()));
//	}
}






