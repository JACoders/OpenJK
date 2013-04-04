// ModViewDoc.cpp : implementation of the CModViewDoc class
//

#include "stdafx.h"
#include "includes.h"
#include "ModView.h"
#include "script.h"

#include "ModViewDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



/////////////////////////////////////////////////////////////////////////////
// CModViewDoc

IMPLEMENT_DYNCREATE(CModViewDoc, CDocument)

BEGIN_MESSAGE_MAP(CModViewDoc, CDocument)
	//{{AFX_MSG_MAP(CModViewDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CModViewDoc construction/destruction
CModViewDoc* gpDocument = NULL;
CModViewDoc::CModViewDoc()
{		
	gpDocument = this;
}

CModViewDoc::~CModViewDoc()
{
}

BOOL CModViewDoc::OnNewDocument()
{
	Model_Delete();

	return CDocument::OnNewDocument();
}



/////////////////////////////////////////////////////////////////////////////
// CModViewDoc serialization

void CModViewDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}

/////////////////////////////////////////////////////////////////////////////
// CModViewDoc diagnostics

#ifdef _DEBUG
void CModViewDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CModViewDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CModViewDoc commands

CString strLastRealDocName;

CModViewDoc* gpLastOpenedModViewDoc = NULL;
void FuckingWellSetTheDocumentNameAndDontBloodyIgnoreMeYouCunt(LPCSTR psDocName)
{
	if (gpLastOpenedModViewDoc)
	{
		// make absolutely fucking sure this bastard does as it's told...
		//
		gpLastOpenedModViewDoc->SetPathName(psDocName,false);
		gpLastOpenedModViewDoc->SetTitle   (psDocName);
	}
}

LPCSTR LengthenFilenameW95W98(LPCSTR psFilename)
{
	// need to do this for Xmen models when running from a batch file under W95/98...
	//
	static char sDest[1024];
	sDest[0]='\0';
	DWORD dwCount = GetLongPathName(psFilename,sDest,sizeof(sDest));
	
	return sDest;
}

//
// function will now recurse back into here if loading a script! (*.mvs)
//
BOOL CModViewDoc::OnOpenDocument(LPCTSTR lpszPathName) 
{
	string str = LengthenFilenameW95W98(lpszPathName);
	if (str.empty())	// if it was empty, then arg didn't eval to a filename (eg "#startminimized"), so return original
	{
		str = lpszPathName;
	}
	lpszPathName = str.c_str();

	if (strstr(lpszPathName,"#startminimized"))
	{
		extern bool gbStartMinimized;
		gbStartMinimized = true;		

		OnNewDocument();

		// None of this shit works, because whatever you set the current document to MS override it with a derived name,
		//	and since the CWinApp class can't even ask what it's own fucking document pointer is without doing a hundred
		//	lines of shit deep within MFC then I'm going to fuck the whole lot off by storing a pointer which I can then
		//	use later in the CWinApp class to override the doc name. 
		//
		// All this fucking bollocks was because MS insist on doing their own switch-comparing so I can't pass in 'real'
		//	switches, I have to use this '#' crap. Stupid fucking incompetent MS dickheads. Like how hard would it be to
		//	pass command line switches to the app instead of just filenames?
		//
		strLastRealDocName = "Untitled";
		SetPathName(strLastRealDocName, false);	// I shouldn't have to do this, but MFC doesn't do it for some reason
		SetTitle(strLastRealDocName);
		gpLastOpenedModViewDoc = this;	
		return true;
	}

//	if (!CDocument::OnOpenDocument(lpszPathName))
//		return FALSE;

	// check for script file first...
	//
	if (lpszPathName && !stricmp(&lpszPathName[strlen(lpszPathName)-4],".mvs"))
	{
		Script_Read(lpszPathName);				// this will recurse back into this function
		SetPathName(lpszPathName, true);		// add script file to MRU
		SetPathName(strLastRealDocName);	// DOESN'T WORK!: set doc/app name to last real model load, not the script name
		return true;
	}

	
	if (lpszPathName && Model_LoadPrimary(lpszPathName))
	{
		strLastRealDocName = lpszPathName;
		strLastRealDocName.Replace("/","\\");
		SetPathName(strLastRealDocName, true);
		return true;
	}
	// model existed, but had some sort of error...
	//

	OnNewDocument();

	strLastRealDocName = "Untitled";
	SetPathName(strLastRealDocName, false);	// I shouldn't have to do this, but MFC doesn't do it for some reason
	return false;	
}

// allows main doc code to be called by wintalk command
//
bool Document_ModelLoadPrimary(LPCSTR psFilename)
{
	return !!(!gpDocument ? NULL : gpDocument->OnOpenDocument(psFilename));
}