// CModule.cpp

#include "Module.h"

// CMoudle

CModule::CModule()
{
	m_errHandler = NULL;
	m_lastError = 0;
	m_lastErrMsg = NULL;
}

CModule::~CModule()
{
	if (m_errHandler != NULL)
	{
		m_errHandler->SetModule(NULL);
	}
	if (m_lastErrMsg != NULL)
	{
		free(m_lastErrMsg);
		m_lastErrMsg = NULL;
	}
}

CErrHandler* CModule::InstallErrHandler(CErrHandler* errHandler)
{
	CErrHandler* retval = m_errHandler;
	m_errHandler = errHandler;
	return retval;
}

void CModule::SetErrHandler(CErrHandler* errHandler)
{
	errHandler->SetModule(this);
}

void CModule::ReportError(int theError, LPCTSTR errString)
{
	m_lastError = theError;
	if (m_lastErrMsg != NULL)
	{
		free(m_lastErrMsg);
		m_lastErrMsg = NULL;
	}
	if (errString != NULL)
	{
		m_lastErrMsg = (char*)malloc(strlen(errString) + 1);
		strcpy(m_lastErrMsg, errString);
	}
	if (m_errHandler != NULL)
	{
		m_errHandler->Error(theError, errString);
	}
}

int CModule::GetLastError(BOOL reset)
{
	int retval = m_lastError;
	if (reset)
	{
		if (m_lastErrMsg != NULL)
		{
			free(m_lastErrMsg);
			m_lastErrMsg = NULL;
		}
		m_lastError = 0;
	}
	return retval;
}

char* CModule::GetLastErrorMessage()
{
	return m_lastErrMsg;
}

// CErrHandler

CErrHandler::CErrHandler()
{
	Init();
}

void CErrHandler::Init()
{
	m_module = NULL;
	m_oldHandler = NULL;
}

CErrHandler::~CErrHandler()
{
	if (m_module != NULL)
	{
		m_module->InstallErrHandler(m_oldHandler);
	}
}

void CErrHandler::SetModule(CModule* module)
{
	m_module = module;
	if (module != NULL)
	{
		m_oldHandler = m_module->InstallErrHandler(this);
	}
}
