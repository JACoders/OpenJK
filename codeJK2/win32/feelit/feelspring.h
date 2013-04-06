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

  FILE:		FeelSpring.h

  PURPOSE:	Feelit API Spring Class

  STARTED:	10/10/97

  NOTES/REVISIONS:
     3/2/99 jrm (Jeff Mallett): Force-->Feel renaming
	 3/2/99 jrm: Added GetIsCompatibleGUID
	 3/15/99 jrm: __declspec(dllimport/dllexport) the whole class

**********************************************************************/


#if !defined(AFX_FEELSPRING_H__135B88C4_4175_11D1_B049_0020AF30269A__INCLUDED_)
#define AFX_FEELSPRING_H__135B88C4_4175_11D1_B049_0020AF30269A__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef _FFCDLL_
#define DLLFFC __declspec(dllimport)
#else
#define DLLFFC __declspec(dllexport)
#endif

#include <windows.h>
#include "FeelCondition.h"


//================================================================
// Constants
//================================================================

#define     FEEL_SPRING_DEFAULT_STIFFNESS      2500
#define     FEEL_SPRING_DEFAULT_SATURATION     10000
#define     FEEL_SPRING_DEFAULT_DEADBAND       100
#define     FEEL_SPRING_DEFAULT_CENTER_POINT   FEEL_EFFECT_MOUSE_POS_AT_START
#define     FEEL_SPRING_DEFAULT_DIRECTION_X    1
#define     FEEL_SPRING_DEFAULT_DIRECTION_Y    0
#define     FEEL_SPRING_DEFAULT_ANGLE          0

//
// FORCE --> FEEL Wrappers
//
#define     FORCE_SPRING_DEFAULT_STIFFNESS      FEEL_SPRING_DEFAULT_STIFFNESS
#define     FORCE_SPRING_DEFAULT_SATURATION     FEEL_SPRING_DEFAULT_SATURATION
#define     FORCE_SPRING_DEFAULT_DEADBAND       FEEL_SPRING_DEFAULT_DEADBAND
#define     FORCE_SPRING_DEFAULT_CENTER_POINT   FEEL_SPRING_DEFAULT_CENTER_POINT
#define     FORCE_SPRING_DEFAULT_DIRECTION_X    FEEL_SPRING_DEFAULT_DIRECTION_X
#define     FORCE_SPRING_DEFAULT_DIRECTION_Y    FEEL_SPRING_DEFAULT_DIRECTION_Y
#define     FORCE_SPRING_DEFAULT_ANGLE          FEEL_SPRING_DEFAULT_ANGLE


//================================================================
// CFeelSpring
//================================================================

//
// ------ PUBLIC INTERFACE ------ 
//
        
class DLLFFC CFeelSpring : public CFeelCondition
{
    //
    // CONSTRUCTOR/DESTRUCTOR
    //

    public:
    
    // Constructor
	CFeelSpring();

	// Destructor
	virtual
	~CFeelSpring();


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
        LONG lStiffness = FEEL_EFFECT_DONT_CHANGE,
        DWORD dwSaturation = FEEL_EFFECT_DONT_CHANGE,
        DWORD dwDeadband = FEEL_EFFECT_DONT_CHANGE,
        LONG lDirectionX = FEEL_EFFECT_DONT_CHANGE,
        LONG lDirectionY = FEEL_EFFECT_DONT_CHANGE
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
        CFeelDevice* pDevice, 
        LONG lStiffness = FEEL_SPRING_DEFAULT_STIFFNESS,
        DWORD dwSaturation = FEEL_SPRING_DEFAULT_SATURATION,
        DWORD dwDeadband = FEEL_SPRING_DEFAULT_DEADBAND,
        DWORD dwfAxis = FEEL_EFFECT_AXIS_BOTH, 
        POINT pntCenter = FEEL_SPRING_DEFAULT_CENTER_POINT,
        LONG lDirectionX = FEEL_SPRING_DEFAULT_DIRECTION_X,
        LONG lDirectionY = FEEL_SPRING_DEFAULT_DIRECTION_Y,
		BOOL bUseDeviceCoordinates = FALSE
		);

    virtual BOOL 
    InitializePolar( 
        CFeelDevice* pDevice, 
        LONG lStiffness,
        DWORD dwSaturation,
        DWORD dwDeadband,
        POINT pntCenter,
        LONG lAngle,
        BOOL bUseDeviceCoordinates = FALSE
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
CFeelSpring::GetIsCompatibleGUID(GUID &guid)
{
	return IsEqualGUID(guid, GUID_Feel_Spring);
}

#endif // !defined(AFX_FEELSPRING_H__135B88C4_4175_11D1_B049_0020AF30269A__INCLUDED_)

