/**********************************************************************
	Copyright (c) 2000 Immersion Corporation

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

  FILE:		ImmDevices.h

  PURPOSE:	Abstract Base Device Class for Immersion Foundation Classes

  STARTED:	3/29/00

  NOTES/REVISIONS:
     3/29/00 jrm (Jeff Mallett): Started

**********************************************************************/

#if !defined(AFX_FORCEDEVICES_H__135B88C4_4175_11D1_B049_0020AF30269A__INCLUDED_)
#define AFX_FORCEDEVICES_H__135B88C4_4175_11D1_B049_0020AF30269A__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef _IFCDLL_
#define DLLIFC __declspec(dllimport)
#else
#define DLLIFC __declspec(dllexport)
#endif

#include "ImmBaseTypes.h"



typedef enum {
	IMM_ENUMERATE_IMM_DEVICES = 0x00000001,
	IMM_ENUMERATE_DX_DEVICES  = 0x00000002,
	IMM_ENUMERATE_ALL         = 0xFFFFFFFF
} IMM_ENUMERATE;

typedef enum {
	IMM_NO_PREFERENCE      = 0x00000000,
	IMM_PREFER_IMM_DEVICES = 0x00000001,
	IMM_PREFER_DX_DEVICES  = 0x00000002
} IMM_ENUMERATE_PREFERENCE;

class CImmDevices;
class CInitializeEnum {
public:
	HANDLE m_hinstApp;
	HANDLE m_hwndApp;
	DWORD m_dwCooperativeFlag;
	CImmDevices *m_pDevices;
	long m_lMaximumDevices;
};

//================================================================
// CImmDevices
//================================================================

typedef class CImmDevice * IMM_DEVICE_PTR;


//
// ------ PUBLIC INTERFACE ------ 
//

class DLLIFC CImmDevices {

    //
    // CONSTRUCTOR/DESCTRUCTOR
    //

    public:

	CImmDevices();
	~CImmDevices();


    //
    // ATTRIBUTES
    //

    public:

	long
	GetNumDevices()
		{ return m_lNumDevices; }

	IMM_DEVICE_PTR
	GetDevice(long lIndex);


    //
    // OPERATIONS
    //

    public:

	void
	AddDevice(IMM_DEVICE_PTR pDevice);

	long
	CreateDevices(
		HINSTANCE hinstApp,
		HWND hwndApp,
		long lMaximumDevices = -1, // means "all"
		IMM_ENUMERATE type = IMM_ENUMERATE_ALL,
		IMM_ENUMERATE_PREFERENCE preference = IMM_NO_PREFERENCE
		);


//
// ------ PRIVATE INTERFACE ------ 
//

    //
    // HELPERS
    //

protected:

	BOOL
	enumerate_dx_devices(CInitializeEnum *pIE);

	BOOL
	enumerate_imm_devices(CInitializeEnum *pIE);

	void
	clean_up();

    //
    // INTERNAL DATA
    //
    
    protected:

	long m_lNumDevices;
	IMM_DEVICE_PTR *m_DeviceArray;
};

#endif // !defined(AFX_FORCEDEVICE_H__135B88C4_4175_11D1_B049_0020AF30269A__INCLUDED_)

