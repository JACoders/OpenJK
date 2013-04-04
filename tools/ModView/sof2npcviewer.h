#if !defined(AFX_SOF2NPCVIEWER_H__8ED96028_14DD_47EE_88B9_0426D26FB683__INCLUDED_)
#define AFX_SOF2NPCVIEWER_H__8ED96028_14DD_47EE_88B9_0426D26FB683__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SOF2NPCViewer.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSOF2NPCViewer dialog

class CSOF2NPCViewer : public CDialog
{
// Construction
public:
	CSOF2NPCViewer(bool bSOF2Mode, CString *pFeedback, LPCSTR psGameDir, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CSOF2NPCViewer)
	enum { IDD = IDD_DIALOG_NPCS };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSOF2NPCViewer)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CToolTipCtrl m_tooltip;
	bool		m_bSOF2Mode;

	// Generated message map functions
	//{{AFX_MSG(CSOF2NPCViewer)
	virtual BOOL OnInitDialog();
	afx_msg void OnRefresh();
	afx_msg void OnDblclkListNpcs();
	afx_msg void OnSelchangeListNpcs();
	afx_msg void OnGallery();
	afx_msg void OnValidate();
	afx_msg void OnButtonGenerateList();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	void NPC_ScanFiles(bool bForceRefresh);
	void NPC_FillList(void);

	void BOT_ScanFiles(bool bForceRefresh);
	void BOT_FillList(void);


	BOOL PreTranslateMessage(MSG* pMsg);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.


bool Gallery_Active(void);
void Gallery_Done(void);
LPCSTR Gallery_GetOutputDir(void);
LPCSTR Gallery_GetSeqToLock(void);
int GalleryRead_ExtractEntry(CString &strCaption, CString &strScript);


#endif // !defined(AFX_SOF2NPCVIEWER_H__8ED96028_14DD_47EE_88B9_0426D26FB683__INCLUDED_)
