// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__EFFD4A56_9FB9_11D4_8A94_00500424438B__INCLUDED_)
#define AFX_STDAFX_H__EFFD4A56_9FB9_11D4_8A94_00500424438B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <afxcview.h>

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

// use these with the UpdateData() dialog command, since I can never remember which way round it works...
//
#define DATA_TO_DIALOG FALSE
#define DIALOG_TO_DATA TRUE


#endif // !defined(AFX_STDAFX_H__EFFD4A56_9FB9_11D4_8A94_00500424438B__INCLUDED_)
