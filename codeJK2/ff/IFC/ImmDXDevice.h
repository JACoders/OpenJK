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

  FILE:		ImmDXDevice.h

  PURPOSE:	Abstraction of DirectX Force Feedback device

  STARTED:	10/10/97

  NOTES/REVISIONS:
     3/2/99 jrm (Jeff Mallett): Force-->Feel renaming
	 3/15/99 jrm: __declspec(dllimport/dllexport) the whole class

**********************************************************************/

#ifndef ImmDXDevice_h
#define ImmDXDevice_h

#ifndef _IFCDLL_
#define DLLIFC __declspec(dllimport)
#else
#define DLLIFC __declspec(dllexport)
#endif

#include "ImmDevice.h"


//================================================================
// CImmDXDevice
//================================================================

//
// ------ PUBLIC INTERFACE ------ 
//

class DLLIFC CImmDXDevice : public CImmDevice
{

    //
    // CONSTRUCTOR/DESCTRUCTOR
    //

    public:
    
    // Constructor
	CImmDXDevice();

	// Destructor
    virtual
	~CImmDXDevice();


    //
    // ATTRIBUTES
    //

    public:

	virtual LPIIMM_API
    GetAPI() 
	{ return (LPIIMM_API) m_piApi; } // actually LPDIRECTINPUT

	virtual LPIIMM_DEVICE
    GetDevice() 
	{ return (LPIIMM_DEVICE) m_piDevice; } // actually LPDIRECTINPUTDEVICE2

	virtual DWORD GetProductType();

	virtual BOOL GetDriverVersion(
		DWORD &dwFFDriverVersion, 
		DWORD &dwFirmwareRevision, 
		DWORD &dwHardwareRevision);

	virtual int GetProductName(LPTSTR lpszProductName, int nMaxCount);
	virtual int GetProductGUIDString(LPTSTR lpszGUID, int nMaxCount);
	virtual GUID GetProductGUID();

    //
    // OPERATIONS
    //

	public:

	BOOL
	Initialize(
		HANDLE hinstApp,
		HANDLE hwndApp,
		LPDIRECTINPUT pDI = NULL,
		LPDIRECTINPUTDEVICE2 piDevice = NULL,
		BOOL bEnumerate = TRUE
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

	friend class CImmDevices;
	static BOOL CALLBACK
    devices_enum_proc(
        LPDIDEVICEINSTANCE pImmDevInst, 
        LPVOID pv
        );


    //
    // INTERNAL DATA
    //

    protected:

		// TODO: these are unused... delete them in future rev
	BOOL m_bpDIPreExist;
	BOOL m_bpDIDevicePreExist;
		// end of useless variables

	LPDIRECTINPUT m_piApi;
	LPDIRECTINPUTDEVICE2 m_piDevice;
};

#endif // ForceDXDevice_h
