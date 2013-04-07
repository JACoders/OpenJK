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

  FILE:		ImmSpring.h

  PURPOSE:	Immersion Foundation Classes Spring Effect
  
  STARTED:	Oct.10.97

  NOTES/REVISIONS:
     Mar.02.99 jrm (Jeff Mallett): Force-->Feel renaming
	 Mar.02.99 jrm: Added GetIsCompatibleGUID
	 Mar.15.99 jrm: __declspec(dllimport/dllexport) the whole class
	 Nov.15.99 efw (Evan Wies): Converted to IFC

**********************************************************************/


#if !defined(AFX_IMMSpring_H__135B88C4_4175_11D1_B049_0020AF30269A__INCLUDED_)
#define AFX_IMMSpring_H__135B88C4_4175_11D1_B049_0020AF30269A__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef _IFCDLL_
#define DLLIFC __declspec(dllimport)
#else
#define DLLIFC __declspec(dllexport)
#endif

#include <windows.h>
#include "ImmCondition.h"


//================================================================
// Constants
//================================================================

#define     IMM_SPRING_DEFAULT_STIFFNESS      2500
#define     IMM_SPRING_DEFAULT_SATURATION     10000
#define     IMM_SPRING_DEFAULT_DEADBAND       100
#define     IMM_SPRING_DEFAULT_CENTER_POINT   IMM_EFFECT_MOUSE_POS_AT_START
#define     IMM_SPRING_DEFAULT_DIRECTION_X    1
#define     IMM_SPRING_DEFAULT_DIRECTION_Y    0
#define     IMM_SPRING_DEFAULT_ANGLE          0


//================================================================
// CImmSpring
//================================================================

//
// ------ PUBLIC INTERFACE ------ 
//
        
class DLLIFC CImmSpring : public CImmCondition
{
    //
    // CONSTRUCTOR/DESTRUCTOR
    //

    public:
    
    // Constructor
	CImmSpring();

	// Destructor
	virtual
	~CImmSpring();


    //
    // ATTRIBUTES
    //

    public:

	virtual BOOL
	GetIsCompatibleGUID(
		GUID &guid
		);

    BOOL
    ChangeParameters( 
        POINT pntCenter,
        LONG lStiffness = IMM_EFFECT_DONT_CHANGE,
        DWORD dwSaturation = IMM_EFFECT_DONT_CHANGE,
        DWORD dwDeadband = IMM_EFFECT_DONT_CHANGE,
        LONG lDirectionX = IMM_EFFECT_DONT_CHANGE,
        LONG lDirectionY = IMM_EFFECT_DONT_CHANGE
        );

    BOOL
    ChangeParametersPolar( 
        POINT pntCenter,
        LONG lStiffness,
        DWORD dwSaturation,
        DWORD dwDeadband,
        LONG lAngle
        );

    //
    // OPERATIONS
    //

    public:

    virtual BOOL 
    Initialize( 
        CImmDevice* pDevice, 
        LONG lStiffness = IMM_SPRING_DEFAULT_STIFFNESS,
        DWORD dwSaturation = IMM_SPRING_DEFAULT_SATURATION,
        DWORD dwDeadband = IMM_SPRING_DEFAULT_DEADBAND,
        DWORD dwfAxis = IMM_EFFECT_AXIS_BOTH, 
        POINT pntCenter = IMM_SPRING_DEFAULT_CENTER_POINT,
        LONG lDirectionX = IMM_SPRING_DEFAULT_DIRECTION_X,
        LONG lDirectionY = IMM_SPRING_DEFAULT_DIRECTION_Y,
		BOOL bUseDeviceCoordinates = FALSE,
		DWORD dwNoDownload = 0
		);

    virtual BOOL 
    InitializePolar( 
        CImmDevice* pDevice, 
        LONG lStiffness,
        DWORD dwSaturation,
        DWORD dwDeadband,
        POINT pntCenter,
        LONG lAngle,
        BOOL bUseDeviceCoordinates = FALSE,
		DWORD dwNoDownload = 0
		);


//
// ------ PRIVATE INTERFACE ------ 
//
            
    //
    // HELPERS
    //

    protected:

    //
    // INTERNAL DATA
    //
    
    protected:

};



//
// INLINES
//

inline BOOL
CImmSpring::GetIsCompatibleGUID(GUID &guid)
{
	return IsEqualGUID(guid, GUID_Imm_Spring);
}

#endif // !defined(AFX_IMMSpring_H__135B88C4_4175_11D1_B049_0020AF30269A__INCLUDED_)

