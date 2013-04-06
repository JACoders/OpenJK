// CModule.h
#ifndef USES_MODULES
#define USES_MODULES

#include <windows.h>

class CModule;

class CErrHandler
{
public:
	CErrHandler();
	virtual ~CErrHandler();

	void SetModule(CModule* module);
	virtual void Error(int theError, LPCTSTR errString) {};

protected:
	virtual void Init();

	CModule*			m_module;
	CErrHandler*		m_oldHandler;
};

class CModule
{
public:
	CModule();
	virtual ~CModule();

	void SetErrHandler(CErrHandler* errHandler);
	void ReportError(int theError, LPCTSTR errString);
	int GetLastError(BOOL reset = TRUE);
	char* GetLastErrorMessage();

	friend class CErrHandler;

protected:
	CErrHandler* InstallErrHandler(CErrHandler* errHandler);

	CErrHandler*			m_errHandler;
	int						m_lastError;
	char*					m_lastErrMsg;
};

#endif // USES_MODULES