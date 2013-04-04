// GetString.cpp : implementation file
//

#include "stdafx.h"
#include "modview.h"
#include "GetString.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGetString dialog


CGetString::CGetString(LPCSTR psPrompt, CString *pFeedback, LPCSTR psDefault /* = NULL */, CWnd* pParent /*=NULL*/)
	: CDialog(CGetString::IDD, pParent)
{
	//{{AFX_DATA_INIT(CGetString)
	m_strEditBox = _T("");
	//}}AFX_DATA_INIT

	m_pFeedback = pFeedback;
	m_pPrompt	= psPrompt;
	m_pDefault	= psDefault;
}


void CGetString::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGetString)
	DDX_Text(pDX, IDC_EDIT1, m_strEditBox);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CGetString, CDialog)
	//{{AFX_MSG_MAP(CGetString)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGetString message handlers

BOOL CGetString::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	GetDlgItem(IDC_GETSTRING_PROMPT)->SetWindowText(m_pPrompt);
	GetDlgItem(IDC_EDIT1)->SetWindowText(m_pDefault?m_pDefault:"");
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CGetString::OnOK() 
{
	UpdateData(DIALOG_TO_DATA);

	*m_pFeedback = m_strEditBox;
	
	CDialog::OnOK();
}
