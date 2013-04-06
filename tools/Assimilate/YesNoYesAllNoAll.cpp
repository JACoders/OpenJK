// YesNoYesAllNoAll.cpp : implementation file
//

// Ok, ok, I kjnow this is a stupid name for a file, I didn't notice that a class name I was 
//	filling in in a classwizard dialog was also adding to a filename field, and I couldn't be 
//	bothered trying to undo it all afterwards, ok?   :-/

#include "stdafx.h"
//#include "assimilate.h"
#include "includes.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CYesNoYesAllNoAll dialog


// this is really really awful, but if I make on large static text box the damn thing won't wrap text, no matter what
//	flagfs are set in the RC, so I'll do it this way for now (another MS classic...)
//
CYesNoYesAllNoAll::CYesNoYesAllNoAll(	LPCSTR psLine1, 
										LPCSTR psLine2, 
										LPCSTR psLine3, 
										LPCSTR psLine4, 
										LPCSTR psLine5, 
										LPCSTR psLine6, 
										CWnd* pParent /*=NULL*/)
	: CDialog(CYesNoYesAllNoAll::IDD, pParent)
{
	//{{AFX_DATA_INIT(CYesNoYesAllNoAll)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	m_psLine1 = psLine1;
	m_psLine2 = psLine2;
	m_psLine3 = psLine3;
	m_psLine4 = psLine4;
	m_psLine5 = psLine5;
	m_psLine6 = psLine6;
}


void CYesNoYesAllNoAll::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CYesNoYesAllNoAll)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CYesNoYesAllNoAll, CDialog)
	//{{AFX_MSG_MAP(CYesNoYesAllNoAll)
	ON_BN_CLICKED(IDNOTOALL, OnNotoall)
	ON_BN_CLICKED(IDYESTOALL, OnYestoall)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CYesNoYesAllNoAll message handlers

void CYesNoYesAllNoAll::OnNotoall() 
{
	EndDialog(NO_ALL);
}

void CYesNoYesAllNoAll::OnYestoall() 
{
	EndDialog(YES_ALL);
}

void CYesNoYesAllNoAll::OnOK() 
{
	EndDialog(YES);
}

void CYesNoYesAllNoAll::OnCancel() 
{
	EndDialog(NO);
}


BOOL CYesNoYesAllNoAll::OnInitDialog() 
{
	CDialog::OnInitDialog();	

	GetDlgItem(IDC_STATIC1)->SetWindowText(m_psLine1?m_psLine1:"");
	GetDlgItem(IDC_STATIC2)->SetWindowText(m_psLine2?m_psLine2:"");
	GetDlgItem(IDC_STATIC3)->SetWindowText(m_psLine3?m_psLine3:"");
	GetDlgItem(IDC_STATIC4)->SetWindowText(m_psLine4?m_psLine4:"");
	GetDlgItem(IDC_STATIC5)->SetWindowText(m_psLine5?m_psLine5:"");
	GetDlgItem(IDC_STATIC6)->SetWindowText(m_psLine6?m_psLine6:"");
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
