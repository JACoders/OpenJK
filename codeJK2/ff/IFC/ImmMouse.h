/**********************************************************************
	Copyright (c) 1997 - 2000 Immersion Corporation

	Permission to use, copy, modify, distribute, and sell this
	software and its documentation may be granted without fee;
	interested parties are encouraged to request permission from
		Immersion Corporation
		801 Fox Lane
		San Jose, CA 95131
		408-467-1900

	IMMERSION DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
	INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.
	IN NO EVENT SHALL IMMERSION BE LIABLE FOR ANY SPECIAL, INDIRECT OR
	CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
	LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
	NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
	CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

  FILE:		ImmMouse.h

  PURPOSE:	Abstraction of Feelit mouse device

  STARTED:	10/10/97

  NOTES/REVISIONS:
     3/2/99 jrm (Jeff Mallett): Force-->Feel renaming
	 3/15/99 jrm: __declspec(dllimport/dllexport) the whole class

**********************************************************************/

#ifndef ImmMouse_h
#define ImmMouse_h

#ifndef _IFCDLL_
#define DLLIFC __declspec(dllimport)
#else
#define DLLIFC __declspec(dllexport)
#endif

#include "ImmDevice.h"


//================================================================
// CImmMouse
//================================================================

//
// ------ PUBLIC INTERFACE ------ 
//

class DLLIFC CImmMouse : public CImmDevice
{

    //
    // CONSTRUCTOR/DESCTRUCTOR
    //

    public:
    
    // Constructor
	CImmMouse();

	// Destructor
    virtual 
	~CImmMouse();


    //
    // ATTRIBUTES
    //

    public:

	virtual LPIIMM_API
    GetAPI() 
		{ return m_piApi; }

	virtual LPIIMM_DEVICE
    GetDevice() 
		{ return m_piDevice; }

	virtual DWORD GetProductType();

	virtual BOOL GetDriverVersion(
		DWORD &dwFFDriverVersion, 
		DWORD &dwFirmwareRevision, 
		DWORD &dwHardwareRevision);

	virtual int GetProductName(LPTSTR lpszProductName, int nMaxCount);
	virtual int GetProductGUIDString(LPTSTR lpszGUID, int nMaxCount);
	virtual GUID GetProductGUID();

	BOOL 
    HaveImmMouse() 
		{ return m_piDevice != NULL; }


    //
    // OPERATIONS
    //

	public:

	BOOL
	Initialize(
		HANDLE hinstApp,
		HANDLE hwndApp,
		DWORD dwCooperativeFlag = IMM_COOPLEVEL_FOREGROUND,
		BOOL bEnumerate = TRUE
		);

	virtual BOOL 
	ChangeScreenResolution(
		BOOL bAutoSet,
		DWORD dwXScreenSize = 0,
		DWORD dwYScreenSize = 0
		);

	// Another syntax for SwitchToAbsoluteMode.
	// The default is Absolute mode.  Call only to switch to Relative mode or
	// to switch back to Absolute mode.
	virtual BOOL
	SwitchToAbsoluteMode(
		BOOL bAbsMode
		);

	virtual BOOL
	GetCurrentPosition( long &lXPos, long &lYPos );

//
// ------ PRIVATE INTERFACE ------ 
//

	//
	// HELPERS
	//

	protected:

	virtual void
	reset();

	virtual BOOL 
	prepare_device();

	friend class CImmDevices;
	static BOOL CALLBACK
    devices_enum_proc(
        LPIMM_DEVICEINSTANCE pImmDevInst, 
        LPVOID pv
        );

    //
    // INTERNAL DATA
    //

    protected:

    LPIIMM_API m_piApi;
    LPIIMM_DEVICE m_piDevice;
};

#endif // ImmMouse_h