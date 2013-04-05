/**********************************************************************
	Copyright (c) 1997 Immersion Corporation

	Permission to use, copy, modify, distribute, and sell this
	software and its documentation may be granted without fee;
	interested parties are encouraged to request permission from
		Immersion Corporation
		2158 Paragon Drive
		San Jose, CA 95131
		408-467-1900

	IMMERSION DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
	INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.
	IN NO EVENT SHALL IMMERSION BE LIABLE FOR ANY SPECIAL, INDIRECT OR
	CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
	LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
	NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
	CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

  FILE:		FeelDevice.h

  PURPOSE:	Abstract Base Device Class for Force Foundation Classes

  STARTED:	10/10/97

  NOTES/REVISIONS:
     3/2/99 jrm (Jeff Mallett): Force-->Feel renaming
	 3/15/99 jrm: __declspec(dllimport/dllexport) the whole class
	 3/16/99 jrm: Made abstract. Moved functionality to CFeelMouse/CFeelDXDevice

**********************************************************************/

#if !defined(AFX_FORCEDEVICE_H__135B88C4_4175_11D1_B049_0020AF30269A__INCLUDED_)
#define AFX_FORCEDEVICE_H__135B88C4_4175_11D1_B049_0020AF30269A__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef _FFCDLL_
#define DLLFFC __declspec(dllimport)
#else
#define DLLFFC __declspec(dllexport)
#endif

#include "dinput.h"
#include "FeelBaseTypes.h"

#ifdef FFC_EFFECT_CACHING
 #include "FeelEffectSuite.h"
#endif



//================================================================
// CFeelDevice
//================================================================

//
// ------ PUBLIC INTERFACE ------ 
//

class DLLFFC CFeelDevice  
{
    //
    // CONSTRUCTOR/DESCTRUCTOR
    //

    public:
    
    // Constructor
	CFeelDevice();

	// Destructor
	virtual
	~CFeelDevice();


    //
    // ATTRIBUTES
    //

    public:

	virtual LPIFEEL_API
    GetAPI() 
		= 0; // pure virtual function

	virtual LPIFEEL_DEVICE // Will actually return LPDIRECTINPUTDEVICE2 if non-FEELit
    GetDevice()
		= 0; // pure virtual function

	DWORD
    GetDeviceType() const
		{ return m_dwDeviceType; }


    //
    // OPERATIONS
    //

    public:

	static CFeelDevice *
	CreateDevice(HINSTANCE hinstApp, HWND hwndApp);
	
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

#ifdef FFC_EFFECT_CACHING
	public:

	void Cache_AddEffect(CFeelEffect *pFeelEffect);
	void Cache_RemoveEffect(const CFeelEffect *pFeelEffect);
	void Cache_SwapOutEffect();

    protected:

	void Cache_LoadEffectSuite(CFeelEffectSuite *pSuite, BOOL bCreateOnDevice);
	void Cache_UnloadEffectSuite(CFeelEffectSuite *pSuite, BOOL bUnloadFromDevice);

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
        LPDIDEVICEINSTANCE pForceDevInst, 
        LPVOID pv
        );

	static BOOL CALLBACK
    enum_devices_proc(
        LPFORCE_DEVICEINSTANCE pForceDevInst, 
        LPVOID pv
        );


    //
    // INTERNAL DATA
    //

    protected:
	
	BOOL m_bInitialized;
    DWORD m_dwDeviceType;
	GUID m_guidDevice;
	BOOL m_bGuidValid;

};


#endif // !defined(AFX_FORCEDEVICE_H__135B88C4_4175_11D1_B049_0020AF30269A__INCLUDED_)
