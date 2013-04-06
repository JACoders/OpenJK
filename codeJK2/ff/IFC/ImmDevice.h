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

  FILE:		ImmDevice.h

  PURPOSE:	Abstract Base Device Class for Force Foundation Classes

  STARTED:	10/10/97

  NOTES/REVISIONS:
     3/2/99 jrm (Jeff Mallett): Force-->Feel renaming
	 3/15/99 jrm: __declspec(dllimport/dllexport) the whole class
	 3/16/99 jrm: Made abstract. Moved functionality to CImmMouse/CImmDXDevice

**********************************************************************/

#if !defined(AFX_FORCEDEVICE_H__135B88C4_4175_11D1_B049_0020AF30269A__INCLUDED_)
#define AFX_FORCEDEVICE_H__135B88C4_4175_11D1_B049_0020AF30269A__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef _IFCDLL_
#define DLLIFC __declspec(dllimport)
#else
#define DLLIFC __declspec(dllexport)
#endif

#include <dinput.h>
#include "ImmBaseTypes.h"

#ifdef IFC_EFFECT_CACHING
 #include "ImmEffectSuite.h"
#endif


//================================================================
// Device and Technology types
//================================================================

//Company IDs
const DWORD IMM_OTHERPARTNER	=0x01000000;
const DWORD IMM_IMMERSION		=0x02000000;
const DWORD IMM_ACTLABS			=0x03000000;
const DWORD IMM_ANKO			=0x04000000;
const DWORD IMM_AVB				=0x05000000;
const DWORD IMM_BOEDER			=0x06000000;
const DWORD IMM_CHPRODUCTS		=0x07000000;
const DWORD IMM_CHIC			=0x08000000;
const DWORD IMM_GUILLEMOT		=0x09000000;
const DWORD IMM_GENIUS			=0x0a000000;
const DWORD IMM_HAPP			=0x0b000000;
const DWORD IMM_INTERACT		=0x0c000000;
const DWORD IMM_INTERACTIVEIO	=0x0d000000;
const DWORD IMM_KYE				=0x0e000000;
const DWORD IMM_LMP				=0x0f000000;
const DWORD IMM_LOGITECH		=0x10000000;
const DWORD IMM_MADCATZ			=0x11000000;
const DWORD IMM_MICROSOFT		=0x12000000;
const DWORD IMM_PADIX			=0x13000000;
const DWORD IMM_PRIMAX			=0x14000000;
const DWORD IMM_QUANTUM3D		=0x15000000;
const DWORD IMM_ROCKFIRE		=0x16000000;
const DWORD IMM_SCT				=0x17000000;
const DWORD IMM_SMELECTRONIC	=0x18000000;
const DWORD IMM_SYSGRATION		=0x19000000;
const DWORD IMM_THRUSTMASTER	=0x1a000000;
const DWORD IMM_TRUST			=0x1b000000;
const DWORD IMM_VIKINGS			=0x1c000000;

// Device IDs
const DWORD IMM_OTHERDEVICE		=0x00000001;
const DWORD IMM_JOYSTICK		=0x00000002;
const DWORD IMM_WHEEL			=0x00000003;
const DWORD IMM_GAMEPAD			=0x00000004;
const DWORD IMM_ABSMOUSE		=0x00000005;
const DWORD IMM_RELMOUSE		=0x00000006;

//Technology IDs
//Note that these are bit masks
const DWORD IMM_OTHERTECH		=0x00000001;
const DWORD IMM_FULLFF			=0x00000002;
const DWORD IMM_IHDFF			=0x00000004;
const DWORD IMM_VIBROFF			=0x00000008;

//Product Types (not to be confused with product GUIDs)
const DWORD IMM_JOYSTICK_FULLFF	= MAKELONG(IMM_FULLFF, IMM_JOYSTICK);
const DWORD IMM_WHEEL_FULLFF	= MAKELONG(IMM_FULLFF, IMM_WHEEL); 
const DWORD IMM_GAMEPAD_FULLFF	= MAKELONG(IMM_FULLFF, IMM_GAMEPAD);
const DWORD IMM_ABSMOUSE_FULLFF	= MAKELONG(IMM_FULLFF, IMM_ABSMOUSE);

const DWORD IMM_JOYSTICK_IHDFF	= MAKELONG(IMM_IHDFF, IMM_JOYSTICK);
const DWORD IMM_WHEEL_IHDFF		= MAKELONG(IMM_IHDFF, IMM_WHEEL);
const DWORD IMM_GAMEPAD_IHDFF	= MAKELONG(IMM_IHDFF, IMM_GAMEPAD);
const DWORD IMM_RELMOUSE_IHDFF	= MAKELONG(IMM_IHDFF, IMM_RELMOUSE);

const DWORD IMM_JOYSTICK_VIBROFF= MAKELONG(IMM_VIBROFF, IMM_JOYSTICK);
const DWORD IMM_WHEEL_VIBROFF	= MAKELONG(IMM_VIBROFF, IMM_WHEEL);
const DWORD IMM_GAMEPAD_VIBROFF	= MAKELONG(IMM_VIBROFF, IMM_GAMEPAD);
const DWORD IMM_RELMOUSE_VIBROFF= MAKELONG(IMM_VIBROFF, IMM_RELMOUSE);

//================================================================
// CImmDevice
//================================================================

//
// ------ PUBLIC INTERFACE ------ 
//

class DLLIFC CImmDevice  
{
    //
    // CONSTRUCTOR/DESCTRUCTOR
    //

    public:
    
    // Constructor
	CImmDevice();

	// Destructor
	virtual
	~CImmDevice();


    //
    // ATTRIBUTES
    //

    public:

	virtual LPIIMM_API
    GetAPI() 
		= 0; // pure virtual function

	virtual LPIIMM_DEVICE // Will actually return LPDIRECTINPUTDEVICE2 for DX supported device
    GetDevice()
		= 0; // pure virtual function

	DWORD
    GetDeviceType() const
		{ return m_dwDeviceType; }

	virtual DWORD GetProductType() = 0;
	virtual BOOL GetDriverVersion(
		DWORD &dwFFDriverVersion, 
		DWORD &dwFirmwareRevision, 
		DWORD &dwHardwareRevision) 
		= 0;

	virtual int GetProductName(LPTSTR lpszProductName, int nMaxCount) = 0;
	virtual int GetProductGUIDString(LPTSTR lpszGUID, int nMaxCount) = 0;
	virtual GUID GetProductGUID() = 0;

	static BOOL GetIFCVersion(DWORD &dwMajor, DWORD &dwMinor, DWORD &dwBuild, DWORD &dwBuildMinor);
	static BOOL GetImmAPIVersion(DWORD &dwMajor, DWORD &dwMinor, DWORD &dwBuild, DWORD &dwBuildMinor);
	static BOOL GetDXVersion(DWORD &dwMajor, DWORD &dwMinor, DWORD &dwBuild, DWORD &dwBuildMinor);

    //
    // OPERATIONS
    //

    public:

	static CImmDevice *
	CreateDevice(HINSTANCE hinstApp, HWND hwndApp);
	
	virtual BOOL
	GetCurrentPosition( long &lXPos, long &lYPos )
		= 0; // pure virtual function

    virtual BOOL 
    ChangeScreenResolution( 
		BOOL bAutoSet,
		DWORD dwXScreenSize = 0,
		DWORD dwYScreenSize = 0
		);

	// The default state is using standard Win32 Mouse messages (e.g., WM_MOUSEMOVE)  
	// and functions (e.g, GetCursorPos).  Call only to switch to relative mode
	// if not using standard Win32 Mouse services (e.g., DirectInput) for mouse
	// input.  
	BOOL
	UsesWin32MouseServices(
		BOOL bWin32MouseServ
		);

	// Another syntax for SwitchToAbsoluteMode.
	// The default is Absolute mode.  Call only to switch to Relative mode or
	// to switch back to Absolute mode.
	virtual BOOL
	SwitchToAbsoluteMode(
		BOOL bAbsMode
		);

//
// ------ PRIVATE INTERFACE ------ 
//

    //
    // CACHING
    //

#ifdef IFC_EFFECT_CACHING
	public:

	void Cache_AddEffect(CImmEffect *pImmEffect);
	void Cache_RemoveEffect(const CImmEffect *pImmEffect);
	void Cache_SwapOutEffect();

    protected:

	void Cache_LoadEffectSuite(CImmEffectSuite *pSuite, BOOL bCreateOnDevice);
	void Cache_UnloadEffectSuite(CImmEffectSuite *pSuite, BOOL bUnloadFromDevice);

	CEffectList m_Cache; // List of all effects created on device
#endif

    //
    // HELPERS
    //

    protected:

    // Performs device preparation by setting the device's parameters
    virtual BOOL 
    prepare_device();

    virtual void
	reset()
		= 0; // pure virtual function

	static BOOL CALLBACK
    enum_didevices_proc(
        LPDIDEVICEINSTANCE pImmDevInst, 
        LPVOID pv
        );

	static BOOL CALLBACK
    enum_devices_proc(
        LPIMM_DEVICEINSTANCE pImmDevInst, 
        LPVOID pv
        );

	void
	detach_effects();

    //
    // INTERNAL DATA
    //

    protected:
	
	BOOL m_bInitialized;
    DWORD m_dwDeviceType;
	GUID m_guidDevice;
	BOOL m_bGuidValid;
	DWORD m_dwProductType;

};


#endif // !defined(AFX_FORCEDEVICE_H__135B88C4_4175_11D1_B049_0020AF30269A__INCLUDED_)
