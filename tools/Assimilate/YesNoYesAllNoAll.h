#if !defined(AFX_YESNOYESALLNOALL_H__FCBCF4B4_7CCF_11D3_8A35_00500424438B__INCLUDED_)
#define AFX_YESNOYESALLNOALL_H__FCBCF4B4_7CCF_11D3_8A35_00500424438B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// YesNoYesAllNoAll.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CYesNoYesAllNoAll dialog

class CYesNoYesAllNoAll : public CDialog
{
// Construction
public:
	CYesNoYesAllNoAll(	LPCSTR psLine1 = NULL, // this is awful, see constructor body for comment
						LPCSTR psLine2 = NULL, 
						LPCSTR psLine3 = NULL, 
						LPCSTR psLine4 = NULL, 
						LPCSTR psLine5 = NULL, 
						LPCSTR psLine6 = NULL, 		
						CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CYesNoYesAllNoAll)
	enum { IDD = IDD_DIALOG_YESNOYESNOALL };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CYesNoYesAllNoAll)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	LPCSTR	m_psLine1;
	LPCSTR	m_psLine2;
	LPCSTR	m_psLine3;
	LPCSTR	m_psLine4;
	LPCSTR	m_psLine5;
	LPCSTR	m_psLine6;

	// Generated message map functions
	//{{AFX_MSG(CYesNoYesAllNoAll)
	afx_msg void OnNotoall();
	afx_msg void OnYestoall();
	virtual void OnOK();
	virtual void OnCancel();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_YESNOYESALLNOALL_H__FCBCF4B4_7CCF_11D3_8A35_00500424438B__INCLUDED_)
