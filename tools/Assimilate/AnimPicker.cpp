// AnimPicker.cpp : implementation file
//

#include "stdafx.h"
//#include "assimilate.h"
#include "includes.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAnimPicker dialog
static bool gbFilterOutUsed = false;

CAnimPicker::CAnimPicker(char *psReturnString,
						 CWnd* pParent /*=NULL*/)
	: CDialog(CAnimPicker::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAnimPicker)
	m_bFilterOutUsed = gbFilterOutUsed;
	//}}AFX_DATA_INIT

	m_psReturnString= psReturnString;
	*m_psReturnString = 0;
}


void CAnimPicker::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAnimPicker)
	DDX_Check(pDX, IDC_CHECK_FILTEROUTUSED, m_bFilterOutUsed);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAnimPicker, CDialog)
	//{{AFX_MSG_MAP(CAnimPicker)
	ON_LBN_DBLCLK(IDC_LIST_BOTH, OnDblclkListBoth)
	ON_LBN_DBLCLK(IDC_LIST_LEGS, OnDblclkListLegs)
	ON_LBN_DBLCLK(IDC_LIST_TORSO, OnDblclkListTorso)
	ON_LBN_SELCHANGE(IDC_LIST_LEGS, OnSelchangeListLegs)
	ON_LBN_SELCHANGE(IDC_LIST_TORSO, OnSelchangeListTorso)
	ON_LBN_SELCHANGE(IDC_LIST_BOTH, OnSelchangeListBoth)
	ON_BN_CLICKED(IDC_CHECK_FILTEROUTUSED, OnCheckFilteroutused)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAnimPicker message handlers

const char *sEnumUsedString = "* ";
const char *sEnumSeperatorString_Prefix = "====== ";	// (the spaces keep these unique/comparable)
const char *sEnumSeperatorString_Suffix = " ======";	//


void CAnimPicker::FillListBoxes()
{
	// fill in the enum list boxes...
	//
	CListBox* boxLegs = (CListBox*)GetDlgItem(IDC_LIST_LEGS);
	CListBox* boxTorso= (CListBox*)GetDlgItem(IDC_LIST_TORSO);
	CListBox* boxBoth = (CListBox*)GetDlgItem(IDC_LIST_BOTH);
	boxLegs->ResetContent();
	boxTorso->ResetContent();
	boxBoth->ResetContent();

	CModel* theModel = ghAssimilateView->GetDocument()->GetCurrentUserSelectedModel();
	ASSERT(theModel);
	if (theModel)
	{
		for (int i=0; ; i++)
		{
			LPCSTR p = ((CAssimilateApp*)AfxGetApp())->GetEnumEntry(i);	
			if (!p)
				break;

			CString string = p;			

			if (theModel->AnimEnumInUse(p))
			{
				if (m_bFilterOutUsed)
					continue;
				string.Insert(0,sEnumUsedString);
			}			

			if (IsEnumSeperator(p))
			{
				string = StripSeperatorStart(p);
				string.Insert(0,sEnumSeperatorString_Prefix);
				string+=sEnumSeperatorString_Suffix;
			}

			CListBox* listBoxPtr = NULL;		
			switch (GetEnumTypeFromString(p))	// note (p), *not* (string)
			{			
				case ET_BOTH:	listBoxPtr = boxBoth;	break;
				case ET_TORSO:	listBoxPtr = boxTorso;	break;
				case ET_LEGS:	listBoxPtr = boxLegs;	break;
				default:		ASSERT(0);				break;	
			}
			if (listBoxPtr)
			{
				// keep an index to the original enum for comment-diving reasons...
				//
				int iIndex = listBoxPtr->InsertString(-1,string);
				listBoxPtr->SetItemData(iIndex,(DWORD)p);
			}
		}
	}
}


BOOL CAnimPicker::OnInitDialog() 
{
	CDialog::OnInitDialog();	

	FillListBoxes();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void CAnimPicker::OnDblclkListBoth() 
{
// not needed now...
//
//	CListBox*	boxBoth = (CListBox*)GetDlgItem(IDC_LIST_BOTH);
//				boxBoth->GetText(boxBoth->GetCurSel(),m_psReturnString);

	if (ReturnStringIsValid())
	{
		EndDialog( IDOK );
	}
}

void CAnimPicker::OnDblclkListLegs() 
{
// not needed now...
//
//	CListBox*	boxLegs = (CListBox*)GetDlgItem(IDC_LIST_LEGS);
//				boxLegs->GetText(boxLegs->GetCurSel(),m_psReturnString);	

	if (ReturnStringIsValid())
	{
		EndDialog( IDOK );
	}
}

void CAnimPicker::OnDblclkListTorso() 
{
// not needed now...
//
//	CListBox*	boxTorso = (CListBox*)GetDlgItem(IDC_LIST_TORSO);	
//				boxTorso->GetText(boxTorso->GetCurSel(),m_psReturnString);	

	if (ReturnStringIsValid())
	{
		EndDialog( IDOK );
	}
}


bool CAnimPicker::ReturnStringIsValid()
{
	if (!strlen(m_psReturnString) ||
		!strnicmp(m_psReturnString,sEnumUsedString,				strlen(sEnumUsedString))	||
		!strnicmp(m_psReturnString,sEnumSeperatorString_Prefix,	strlen(sEnumSeperatorString_Prefix))
		)
	{
		return false;
	}
	
	return true;
}

// updates the return string for if needed later (so ENTER key can work), handles onscreen Comments.
//
void CAnimPicker::HandleSelectionChange(CListBox* listbox)
{	
	if (listbox->GetCurSel() != LB_ERR)
	{
		CString selectedString;
		listbox->GetText(listbox->GetCurSel(),selectedString);

		LPCSTR lpOriginalEnumName = (LPCSTR) listbox->GetItemData(listbox->GetCurSel());

		LPCSTR psComment = lpOriginalEnumName?((CAssimilateApp*)AfxGetApp())->GetEnumComment(lpOriginalEnumName):NULL;	
		
		GetDlgItem(IDC_STATIC_COMMENT)->SetWindowText(psComment?va("Comment: %s",psComment):"");

		strcpy(m_psReturnString,selectedString);

		if (!ReturnStringIsValid())
		{
			strcpy(m_psReturnString,"");
		}		
	}
}

void CAnimPicker::OnSelchangeListLegs() 
{
	CListBox* listbox = (CListBox*)GetDlgItem(IDC_LIST_LEGS);	

	HandleSelectionChange(listbox);
}

void CAnimPicker::OnSelchangeListTorso() 
{
	CListBox* listbox = (CListBox*)GetDlgItem(IDC_LIST_TORSO);

	HandleSelectionChange(listbox);
}

void CAnimPicker::OnSelchangeListBoth() 
{
	CListBox* listbox = (CListBox*)GetDlgItem(IDC_LIST_BOTH);
	
	HandleSelectionChange(listbox);
}


BOOL CAnimPicker::PreTranslateMessage(MSG* pMsg) 
{
	int i = VK_RETURN;

	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN)
	{
		if (ReturnStringIsValid())
		{
			EndDialog( IDOK );
		}
		return 1;	// disable the RETURN key
	}
	
	return CDialog::PreTranslateMessage(pMsg);
}


void CAnimPicker::OnCheckFilteroutused() 
{
	UpdateData(DIALOG_TO_DATA);
	FillListBoxes();
}

void CAnimPicker::OnDestroy() 
{
	CDialog::OnDestroy();

	gbFilterOutUsed = !!m_bFilterOutUsed;	
}

