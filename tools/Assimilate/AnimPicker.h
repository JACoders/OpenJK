#if !defined(AFX_ANIMPICKER_H__A3E6F523_45D1_11D3_8A22_00500424438B__INCLUDED_)
#define AFX_ANIMPICKER_H__A3E6F523_45D1_11D3_8A22_00500424438B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// AnimPicker.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAnimPicker dialog

class CAnimPicker : public CDialog
{
// Construction
public:
	CAnimPicker(char *psReturnString, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CAnimPicker)
	enum { IDD = IDD_ANIMPICKER };
	BOOL	m_bFilterOutUsed;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAnimPicker)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	char*		m_psReturnString;	
	bool		ReturnStringIsValid();
	void		HandleSelectionChange(CListBox* listbox);
	
	// Generated message map functions
	//{{AFX_MSG(CAnimPicker)
	afx_msg void OnDblclkListBoth();
	afx_msg void OnDblclkListLegs();
	afx_msg void OnDblclkListTorso();
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangeListLegs();
	afx_msg void OnSelchangeListTorso();
	afx_msg void OnSelchangeListBoth();
	afx_msg void OnCheckFilteroutused();
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	void FillListBoxes();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ANIMPICKER_H__A3E6F523_45D1_11D3_8A22_00500424438B__INCLUDED_)
