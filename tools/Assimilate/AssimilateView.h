// AssimilateView.h : interface of the CAssimilateView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_ASSIMILATEVIEW_H__2CCA554C_2AD3_11D3_82E0_0000C0366FF2__INCLUDED_)
#define AFX_ASSIMILATEVIEW_H__2CCA554C_2AD3_11D3_82E0_0000C0366FF2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

enum
{
	ObjID_Folder,
	ObjID_Unused1,
	ObjID_Unused2,
	ObjID_Unused3,
	ObjID_Unused4,
	ObjID_Sequence,
	ObjID_ENUMINVALID,
	ObjID_OpenFolder,
	ObjID_Unused6,
	ObjID_ENUMBOTH,
	ObjID_ENUMTORSO,
	ObjID_ENUMLEGS,
	ObjID_ENUMG2,
	ObjID_ENUMG2INVALID,
	ObjID_ENUMG2GLA,
	ObjID_ENUMG2GLAINVALID
};

typedef enum
{
	eMODE_BAD=0,
	eMODE_SINGLE,
	eMODE_MULTI
} PlayerMode_e;

class CAssimilateView : public CTreeView
{
protected: // create from serialization only
	CAssimilateView();
	DECLARE_DYNCREATE(CAssimilateView)

// Attributes
public:
	CAssimilateDoc* GetDocument();

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAssimilateView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnInitialUpdate();
	protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CAssimilateView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	CModel *GetCurrentModel();
	PlayerMode_e GetSingleOrMultiPlayerMode();

protected:
	void BuildTree();
	void UpdateTree();
	void SortTree();
	bool DeleteCurrentItem(bool bNoQueryForSequenceDelete = false);
	void SetFirstModelTitleAndMode();

// Generated message map functions
protected:
	//{{AFX_MSG(CAssimilateView)
	afx_msg void OnDblclk(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDestroy();
	afx_msg void OnItemexpanded(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnSelchanged(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in AssimilateView.cpp
inline CAssimilateDoc* CAssimilateView::GetDocument()
   { return (CAssimilateDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

extern CAssimilateView* ghAssimilateView;

#endif // !defined(AFX_ASSIMILATEVIEW_H__2CCA554C_2AD3_11D3_82E0_0000C0366FF2__INCLUDED_)
