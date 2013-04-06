// AlertErrHandler.cpp

#include <afxwin.h> 
#include "Module.h"
#include "AlertErrHandler.h"

bool gbParseError = false;

void CAlertErrHandler::Error(int theError, LPCTSTR errString)
{
	gbParseError = true;
	AfxMessageBox(errString);
}

