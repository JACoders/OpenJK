// Assimilate.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "Includes.h"
#include "sourcesafe.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

bool gbViewAnimEnums = false;
bool gbViewFrameDetails = false;
bool gbViewFrameDetails_Additional = false;

/////////////////////////////////////////////////////////////////////////////
// CAssimilateApp

BEGIN_MESSAGE_MAP(CAssimilateApp, CWinApp)
	//{{AFX_MSG_MAP(CAssimilateApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	ON_COMMAND(IDM_PROPERTIES, OnProperties)
	//}}AFX_MSG_MAP
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
	// Standard print setup command
	ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)
END_MESSAGE_MAP()

keywordArray_t CAssimilateApp::s_Symbols[] = 
{
	"{",		TK_OBRACE,
	"}",		TK_CBRACE,
	",",		TK_COMMA,
	";",		TK_SEMICOLON,
	NULL,		TK_EOF,
};

keywordArray_t CAssimilateApp::s_Keywords[] = 
{
	"enum",					TK_ENUM,
	NULL,					TK_EOF,
};

/////////////////////////////////////////////////////////////////////////////
// CAssimilateApp construction

CAssimilateApp::CAssimilateApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CAssimilateApp object

CAssimilateApp theApp;

const TCHAR CAssimilateApp::c_prefSection[] = "Preferences";
const TCHAR CAssimilateApp::c_enumFilename[] = "Enum Filename";
const TCHAR CAssimilateApp::c_buffersize[] = "Buffer size";
const TCHAR CAssimilateApp::c_QDataFilename[] = "QData filename";

void CAssimilateApp::LoadRegistry()
{
	m_bMultiPlayerMode = !!GetProfileInt(c_prefSection, "m_bMultiPlayerMode", m_bMultiPlayerMode);
	m_enumFilename	= GetProfileString(c_prefSection, c_enumFilename, m_enumFilename);
	m_buffersize	= GetProfileInt	  (c_prefSection, c_buffersize, m_buffersize);
	m_QDataFilename = GetProfileString(c_prefSection, c_QDataFilename, m_QDataFilename);
	m_QuakeDir		= GetProfileString(c_prefSection, "m_QuakeDir", m_QuakeDir);
	gbViewAnimEnums = !!GetProfileInt(c_prefSection, "gbViewAnimEnums", gbViewAnimEnums);
	gbViewFrameDetails = !!GetProfileInt(c_prefSection, "gbViewFrameDetails", gbViewFrameDetails);
	gbViewFrameDetails_Additional = !!GetProfileInt(c_prefSection, "gbViewFrameDetails_Additional", gbViewFrameDetails_Additional);
	
}

void CAssimilateApp::SaveRegistry()
{
	WriteProfileInt(c_prefSection, "m_bMultiPlayerMode", m_bMultiPlayerMode);
	WriteProfileString(c_prefSection, c_enumFilename, m_enumFilename);
	WriteProfileInt	  (c_prefSection, c_buffersize, m_buffersize);
	WriteProfileString(c_prefSection, c_QDataFilename, m_QDataFilename);
	WriteProfileString(c_prefSection, "m_QuakeDir", m_QuakeDir);
	WriteProfileInt(c_prefSection, "gbViewAnimEnums", gbViewAnimEnums);
	WriteProfileInt(c_prefSection, "gbViewFrameDetails", gbViewFrameDetails);	
	WriteProfileInt(c_prefSection, "gbViewFrameDetails_Additional", gbViewFrameDetails_Additional);	
}

/////////////////////////////////////////////////////////////////////////////
// CAssimilateApp initialization

BOOL CAssimilateApp::InitInstance()
{
	m_bMultiPlayerMode = bDEFAULT_MULTIPLAYER_MODE;
	m_enumFilename	= sDEFAULT_ENUM_FILENAME;
	m_buffersize	= dwDEFAULT_BUFFERSIZE;
	m_QDataFilename = sDEFAULT_QDATA_LOCATION;
	m_QuakeDir		= sDEFAULT_QUAKEDIR;
	
	AfxEnableControlContainer();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

	// Change the registry key under which our settings are stored.
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization.
	SetRegistryKey(_T("Local AppWizard-Generated Applications"));

	LoadStdProfileSettings(12);  // Load standard INI file options (including MRU)
	LoadRegistry();
	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views.

	CSingleDocTemplate* pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(CAssimilateDoc),
		RUNTIME_CLASS(CMainFrame),       // main SDI frame window
		RUNTIME_CLASS(CAssimilateView));
	AddDocTemplate(pDocTemplate);

	// Enable DDE Execute open
	EnableShellOpen();
	RegisterShellFileTypes(TRUE);

	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	LoadEnumTable(m_enumFilename);

	CString strCommandLineFile = cmdInfo.m_strFileName;
	cmdInfo.m_strFileName = "";
	cmdInfo.m_nShellCommand = CCommandLineInfo::FileNew;	// :-)

	// Dispatch commands specified on the command line
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;

	// The one and only window has been initialized, so show and update it.
	m_pMainWnd->ShowWindow(SW_SHOW);
	m_pMainWnd->UpdateWindow();

	// Enable drag/drop open
	m_pMainWnd->DragAcceptFiles();


//===============
	//
	// a bit hacky but wtf for utils, right?...
	//	
	SS_SetString_Ini	("\\\\RAVEND\\VSS_PROJECTS\\StarWars\\SRCSAFE.INI");
	SS_SetString_Project("$/base/");
//===============

	if (strCommandLineFile.GetLength())
	{
		if (!OpenDocumentFile(strCommandLineFile))
			return FALSE;
	}

	return TRUE;
}

// this asks whether or not a name is present in the enum table, not whether or not it's really valid (just so I remember)
//
bool CAssimilateApp::ValidEnum(LPCTSTR name)
{
	for (EnumTable_t::iterator i = m_enumTable.begin(); i!= m_enumTable.end(); ++i)
	{
		if (!stricmp((*i).first,name))
		{
			return true;
		}
	}

	return false;	//	return (m_enumTable.FindSymbol(name) != NULL);
}

// return any comment associated with a given enum... (may be NULL)
//
LPCSTR CAssimilateApp::GetEnumComment(LPCSTR psEnum)
{
	for (EnumTable_t::iterator i = m_enumTable.begin(); i!= m_enumTable.end(); ++i)
	{
		if (!stricmp((*i).first,psEnum))
		{
			LPCSTR p = (*i).second;

			if (strlen(p))
				return p;
			return NULL;
		}
	}

	return NULL;
}

bool CAssimilateApp::GetMultiPlayerMode()
{
	return m_bMultiPlayerMode;
}

LPCTSTR CAssimilateApp::GetEnumFilename()
{
	return m_enumFilename;
}

LPCTSTR CAssimilateApp::GetQDataFilename()
{
	return m_QDataFilename;
}

LPCTSTR CAssimilateApp::GetQuakeDir()
{
	return m_QuakeDir;
}

DWORD CAssimilateApp::GetBufferSize()
{
	return m_buffersize;
}

int CAssimilateApp::GetEnumTableEntries()
{
	return m_enumTable.size();
}
LPCSTR CAssimilateApp::GetEnumEntry(int iIndex)
{
	for (EnumTable_t::iterator i = m_enumTable.begin(); i!= m_enumTable.end(); ++i)
	{
		if (!iIndex--)
		{			
			return (*i).first;
		}
	}

	return NULL;
}

bool CAssimilateApp::SetMultiPlayerMode(bool bMultiPlayerMode)
{
	if (bMultiPlayerMode != m_bMultiPlayerMode)
	{
		m_bMultiPlayerMode = bMultiPlayerMode;
		return true;
	}
	return false;
}

bool CAssimilateApp::SetEnumFilename(LPCTSTR filename)
{
	CString name = filename;
	name.MakeLower();
	name.Replace('\\', '/');
	if (name.Compare(m_enumFilename) == 0)
	{
		return false;
	}
	m_enumFilename = name;
	LoadEnumTable(m_enumFilename);
	return true;
}

bool CAssimilateApp::SetQuakeDir(LPCTSTR psQuakeDir)
{
	CString name = psQuakeDir;
	name.MakeLower();
	name.Replace('\\','/');
	if (name.Compare(m_QuakeDir) == 0)
	{
		return false;
	}
	m_QuakeDir = name;
	return true;
}
	
bool CAssimilateApp::SetQDataFilename(LPCTSTR filename)
{
	CString name = filename;
	name.MakeLower();
	name.Replace('\\', '/');
	if (name.Compare(m_QDataFilename) == 0)
	{
		return false;
	}
	m_QDataFilename = name;
	return true;
}
	
bool CAssimilateApp::SetBufferSize(DWORD buffersize)
{
	if (buffersize != m_buffersize)
	{
		m_buffersize = buffersize;
		return true;
	}
	return false;
}

void CAssimilateApp::LoadEnumTable(LPCTSTR filename)
{
	int iIndex_Both = 0,
		iIndex_Torso = 0,
		iIndex_Legs = 0;

	m_enumTable.clear();	
	CTokenizer* tokenizer = CTokenizer::Create();
	tokenizer->AddParseFile(filename);
	tokenizer->SetKeywords(s_Keywords);
	tokenizer->SetSymbols(s_Symbols);
	CToken* curToken = tokenizer->GetToken(TKF_NODIRECTIVES|TKF_IGNOREDIRECTIVES);

	bool bFinished = false;
	while(curToken->GetType() != TK_EOF && !bFinished)
	{
		switch (curToken->GetType())
		{
		case TK_ENUM:
			while((curToken->GetType() != TK_EOF) && (curToken->GetType() != TK_OBRACE))
			{
				curToken->Delete();
				curToken = tokenizer->GetToken(NULL, TKF_NODIRECTIVES|TKF_IGNOREDIRECTIVES|TKF_USES_EOL|TKF_COMMENTTOKENS, 0);
			}
			while ((curToken->GetType() != TK_EOF) && (curToken->GetType() != TK_CBRACE))
			{
				if (curToken->GetType() == TK_IDENTIFIER)
				{
					CString curEnumName = curToken->GetStringValue();	// useful later

					curToken->Delete();
					curToken = tokenizer->GetToken(NULL, TKF_NODIRECTIVES|TKF_IGNOREDIRECTIVES|TKF_USES_EOL|TKF_COMMENTTOKENS, 0);

					// only add the kind of symbols we know about...
					//
					if (GetEnumTypeFromString(curEnumName) != ET_INVALID)
					{
						if (!ValidEnum(curEnumName))	// ie if not already in the table...
						{							
							// now skip past any crud like " = 0," and check for comments...
							//
							CString Comment;

							while (	(curToken->GetType() != TK_EOF) &&
									(curToken->GetType() != TK_EOL) &&
									(curToken->GetType() != TK_CBRACE)
									)
							{
								if (curToken->GetType() == TK_COMMENT)
								{
									LPCSTR psComment = curToken->GetStringValue();

									// only read comment like "//#"...
									//
									if (psComment[0]=='#')
									{
										psComment++;
										while (isspace(*psComment)) psComment++;

										// see if there's anything left after skipping whitespace...
										//
										if (strlen(psComment))
										{
											Comment = psComment;
										}
									}
								}

								curToken->Delete();
								curToken = tokenizer->GetToken(NULL, TKF_NODIRECTIVES|TKF_IGNOREDIRECTIVES|TKF_USES_EOL|TKF_COMMENTTOKENS, 0);						
							}

							pair<CString,CString> pr(curEnumName,Comment);
							m_enumTable.push_back(pr);	
						}
					}
				}
				else
				{
					// is this one of the new seperator comments?
					//
					if (curToken->GetType() == TK_COMMENT)
					{
						LPCSTR psComment = curToken->GetStringValue();
						
						// only count comments beginning "//#"...
						//
						if (psComment[0] == '#')
						{
							psComment++;
							while (isspace(*psComment)) psComment++;

							CString curEnumName = psComment;	// but this will include potential comments, so...
							CString comment;

							int loc = curEnumName.Find("#");
							if (loc>=0)
							{
								comment = curEnumName.Mid(loc+1);
								curEnumName = curEnumName.Left(loc);
								comment.TrimLeft();
							}
							curEnumName.TrimRight();	// lose trailing whitespace
							comment.TrimRight();

							// the first part of the comment (a pseudo-enum) must be a valid type...
							//
							if (GetEnumTypeFromString(curEnumName) != ET_INVALID)
							{
								if (!ValidEnum(curEnumName))	// ie if not already in the table...
								{
									pair <CString,CString> pr(curEnumName,comment);
									m_enumTable.push_back(pr);	
								}
							}
						}
					}

					curToken->Delete();
					curToken = tokenizer->GetToken(NULL, TKF_NODIRECTIVES|TKF_IGNOREDIRECTIVES|TKF_USES_EOL|TKF_COMMENTTOKENS, 0);						
				}
			}
			bFinished = true;
			break;
		default:
			break;
		}
		if (curToken->GetType() != TK_EOF)
		{
			curToken->Delete();
			curToken = tokenizer->GetToken(TKF_NODIRECTIVES|TKF_IGNOREDIRECTIVES);
		}
	}
	if (curToken->GetType() != TK_EOF)
	{
		curToken->Delete();
	}
	tokenizer->Delete();
}


/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
		// No message handlers
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// App command to run the dialog
void CAssimilateApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
// CAssimilateApp message handlers


int CAssimilateApp::ExitInstance() 
{
	m_enumTable.clear();
	SaveRegistry();

	return CWinApp::ExitInstance();
}

void CAssimilateApp::OnProperties() 
{
	CString strOldEnumFilename(m_enumFilename);
		
	bool dirty = false;
	CPropertySheet* propSheet = new CPropertySheet("Assimilate");
	CAssimilatePropPage* propPage = new CAssimilatePropPage();
	propPage->m_soilFlag = &dirty;
	propSheet->AddPage(propPage);

	propSheet->DoModal();

	delete propPage;
	delete propSheet;

	if (dirty)
	{
		SaveRegistry();

		if (strOldEnumFilename.CompareNoCase(m_enumFilename)	// if enum table name changed...
			&&
			ghAssimilateView->GetDocument()->GetNumModels()		// ...and any models loaded
			)
		{				
			//if (GetYesNo("Enum table changed, do you want to re-enumerate?\n\n( You should nearly always answer YES here )"))
				ghAssimilateView->GetDocument()->Resequence();

			PlayerMode_e ePlayerMode = ghAssimilateView->GetSingleOrMultiPlayerMode();

			if (ePlayerMode == eMODE_SINGLE)
			{
				InfoBox("Since you're now using the single player enum table but you weren't previously, be warned that certain sequences may be sorted slightly differently.\n\nTHIS WILL STILL WORK FINE, but may affect (eg) WinDiff, or cause non-alphabetical sort order which could make things harder to find\n\n\nIf this bothers you, do a NEW (without saving) and reload this model now");
			}
			else
			if (ePlayerMode == eMODE_MULTI)
			{
				InfoBox("Attention! Multiplayer sequence order is fixed, but switching to the multiplayer settings from different settings can result in badly-sorted sequence order.\n\nYou should now do a NEW (without saving) and reload this model");
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
// CAssimilatePropPage property page

IMPLEMENT_DYNCREATE(CAssimilatePropPage, CPropertyPage)

CAssimilatePropPage::CAssimilatePropPage() : CPropertyPage(CAssimilatePropPage::IDD)
{
	//{{AFX_DATA_INIT(CAssimilatePropPage)
	m_buffsize = 0;
	m_enumfilename = _T("");
	m_qdata = _T("");
	m_csQuakeDir = _T("");
	m_bMultiPlayer = FALSE;
	//}}AFX_DATA_INIT
}

CAssimilatePropPage::~CAssimilatePropPage()
{
}

void CAssimilatePropPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAssimilatePropPage)
	DDX_Text(pDX, IDC_BUFFSIZE, m_buffsize);
	DDX_Text(pDX, IDC_ENUM, m_enumfilename);
	DDX_Text(pDX, IDC_QDATA, m_qdata);
	DDX_Text(pDX, IDC_QUAKEDIR, m_csQuakeDir);
	DDX_Check(pDX, IDC_CHECK_MULTIPLAYER, m_bMultiPlayer);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAssimilatePropPage, CPropertyPage)
	//{{AFX_MSG_MAP(CAssimilatePropPage)
	ON_BN_CLICKED(IDC_ENUM_BROWSE, OnEnumBrowse)
	ON_BN_CLICKED(IDC_QDATA_BROWSE, OnQdataBrowse)
	ON_BN_CLICKED(IDC_BUTTON_DEFAULTS, OnButtonDefaults)
	ON_BN_CLICKED(IDC_BUTTON_DEFAULTS_MULTI, OnButtonDefaultsMulti)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAssimilatePropPage message handlers

void CAssimilatePropPage::OnEnumBrowse() 
{
	// TODO: Add your control notification handler code here
	CFileDialog theDialog(true, ".h", NULL, OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST, _T("Enum File (*.h)|*.h|All Files|*.*||"), NULL);
	int result = theDialog.DoModal();
	if (result != IDOK)
	{
		return;
	}
	m_enumfilename = theDialog.GetPathName();
	UpdateData(FALSE);
}

void CAssimilatePropPage::OnQdataBrowse() 
{
	// TODO: Add your control notification handler code here
	CFileDialog theDialog(true, ".exe", NULL, OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST, _T("QData File (*.exe)|*.exe|All Files|*.*||"), NULL);
	int result = theDialog.DoModal();
	if (result != IDOK)
	{
		return;
	}
	m_qdata = theDialog.GetPathName();
	UpdateData(FALSE);
}

void CAssimilatePropPage::OnOK() 
{
	// TODO: Add your specialized code here and/or call the base class
	CAssimilateApp* app = (CAssimilateApp*)AfxGetApp();
	*m_soilFlag =	!!
					(
					app->SetEnumFilename(m_enumfilename)	|	// note single OR rather than double, else not all execute
					app->SetQDataFilename(m_qdata)			|
					app->SetBufferSize(m_buffsize)			|
					app->SetQuakeDir(m_csQuakeDir)			|
					app->SetMultiPlayerMode(!!m_bMultiPlayer)
					);

	CPropertyPage::OnOK();
}

BOOL CAssimilatePropPage::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
	// TODO: Add extra initialization here
	CAssimilateApp* app = (CAssimilateApp*)AfxGetApp();
	m_bMultiPlayer  = app->GetMultiPlayerMode();
	m_enumfilename	= app->GetEnumFilename();
	m_qdata			= app->GetQDataFilename();
	m_buffsize		= app->GetBufferSize();
	m_csQuakeDir	= app->GetQuakeDir();

	UpdateData(FALSE);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CAssimilatePropPage::OnButtonDefaults() 
{
	UpdateData(true);	// dialog -> vars

	if (m_enumfilename.IsEmpty()	||
		m_qdata.IsEmpty()			||
		m_csQuakeDir.IsEmpty()		||
		GetYesNo("Override all fields with single player defaults,\n\nAre you Sure?")
		)
	{
		m_bMultiPlayer  = false;
		m_enumfilename	= sDEFAULT_ENUM_FILENAME;
		m_qdata			= sDEFAULT_QDATA_LOCATION;
		m_buffsize		= dwDEFAULT_BUFFERSIZE;
		m_csQuakeDir	= sDEFAULT_QUAKEDIR;	

		UpdateData(false);	// vars -> dialog
	}	
}


void CAssimilatePropPage::OnButtonDefaultsMulti() 
{
	UpdateData(true);	// dialog -> vars

	if (m_enumfilename.IsEmpty()	||
		m_qdata.IsEmpty()			||
		m_csQuakeDir.IsEmpty()		||
		GetYesNo("Override all fields with multi player defaults,\n\nAre you Sure?")
		)
	{
		m_bMultiPlayer  = true;
		m_enumfilename	= sDEFAULT_ENUM_FILENAME_MULTI;
		m_qdata			= sDEFAULT_QDATA_LOCATION;
		m_buffsize		= dwDEFAULT_BUFFERSIZE;
		m_csQuakeDir	= sDEFAULT_QUAKEDIR;	

		UpdateData(false);	// vars -> dialog
	}	
}
