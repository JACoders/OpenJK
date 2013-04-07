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

  FILE:		FeelMouse.h

  PURPOSE:	Abstraction of Feelit mouse device

  STARTED:	10/10/97

  NOTES/REVISIONS:
     3/2/99 jrm (Jeff Mallett): Force-->Feel renaming
	 3/15/99 jrm: __declspec(dllimport/dllexport) the whole class

**********************************************************************/

#ifndef FeelMouse_h
#define FeelMouse_h

#ifndef _FFCDLL_
#define DLLFFC __declspec(dllimport)
#else
#define DLLFFC __declspec(dllexport)
#endif

#include "FeelDevice.h"


//================================================================
// CFeelMouse
//================================================================

//
// ------ PUBLIC INTERFACE ------ 
//

class DLLFFC CFeelMouse : public CFeelDevice
{

    //
    // CONSTRUCTOR/DESCTRUCTOR
    //

    public:
    
    // Constructor
	CFeelMouse();

	// Destructor
    virtual 
	~CFeelMouse();


    //
    // ATTRIBUTES
    //

    public:

	virtual LPIFEEL_API
    GetAPI() 
		{ return m_piApi; }

	virtual LPIFEEL_DEVICE
    GetDevice() 
		{ return m_piDevice; }

	BOOL 
    HaveForceFeelitMouse() 
		{ return m_piDevice != NULL; }


    //
    // OPERATIONS
    //

	public:

	BOOL
	Initialize(
		HANDLE hinstApp,
		HANDLE hwndApp,
		DWORD dwCooperativeFlag = FEELIT_FCOOPLEVEL_FOREGROUND
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


    //
    // INTERNAL DATA
    //

    protected:

    LPIFEEL_API m_piApi;
    LPIFEEL_DEVICE m_piDevice;
};

#endif // ForceFeelitMouse_h