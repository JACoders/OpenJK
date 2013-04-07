// AlertErrHandler.h

class CAlertErrHandler :  public CErrHandler
{
public:
	CAlertErrHandler() {Init();};

	virtual void Error(int theError, LPCTSTR errString);
};

extern bool gbParseError;
