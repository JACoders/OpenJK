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

  FILE:		FeelInertia.h

  PURPOSE:	Feelit API Inertia Effect Class

  STARTED:	12/29/97

  NOTES/REVISIONS:
     3/2/99 jrm (Jeff Mallett): Force-->Feel renaming
	 3/2/99 jrm: Added GetIsCompatibleGUID
	 3/15/99 jrm: __declspec(dllimport/dllexport) the whole class

**********************************************************************/


#if !defined(AFX_FEELInertia_H__135B88C4_4175_11D1_B049_0020AF30269A__INCLUDED_)
#define AFX_FEELInertia_H__135B88C4_4175_11D1_B049_0020AF30269A__INCLUDED_

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

#define     FEEL_INERTIA_DEFAULT_COEFFICIENT		2500
#define     FEEL_INERTIA_DEFAULT_SATURATION			10000
#define     FEEL_INERTIA_DEFAULT_MIN_ACCELERATION	0

//
// FORCE --> FEEL Wrappers
//
#define     FORCE_INERTIA_DEFAULT_COEFFICIENT		FEEL_INERTIA_DEFAULT_COEFFICIENT
#define     FORCE_INERTIA_DEFAULT_SATURATION		FEEL_INERTIA_DEFAULT_SATURATION
#define     FORCE_INERTIA_DEFAULT_MIN_ACCELERATION	FEEL_INERTIA_DEFAULT_MIN_ACCELERATION


//================================================================
// CFeelInertia
//================================================================

//
// ------ PUBLIC INTERFACE ------ 
//

class DLLFFC CFeelInertia : public CFeelCondition
{
    //
    // CONSTRUCTOR/DESTRUCTOR
    //

    public:
    
    // Constructor
	CFeelInertia();

	// Destructor
	virtual
	~CFeelInertia();


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
        DWORD dwCoefficient,
        DWORD dwSaturation = FEEL_EFFECT_DONT_CHANGE,
        DWORD dwMinAcceleration = FEEL_EFFECT_DONT_CHANGE,
        LONG lDirectionX = FEEL_EFFECT_DONT_CHANGE,
        LONG lDirectionY = FEEL_EFFECT_DONT_CHANGE
        );

    BOOL
    ChangeParametersPolar( 
        DWORD dwCoefficient,
        DWORD dwSaturation,
        DWORD dwMinAcceleration,
        LONG lAngle
        );


    //
    // OPERATIONS
    //

    public:

    virtual BOOL 
    Initialize( 
        CFeelDevice* pDevice, 
        DWORD dwCoefficient = FEEL_INERTIA_DEFAULT_COEFFICIENT,
        DWORD dwSaturation = FEEL_INERTIA_DEFAULT_SATURATION,
        DWORD dwMinAcceleration = FEEL_INERTIA_DEFAULT_MIN_ACCELERATION,
        DWORD dwfAxis = FEEL_EFFECT_AXIS_BOTH,
        LONG lDirectionX = FEEL_EFFECT_DEFAULT_DIRECTION_X,
        LONG lDirectionY = FEEL_EFFECT_DEFAULT_DIRECTION_Y
        );

    virtual BOOL 
    InitializePolar( 
        CFeelDevice* pDevice, 
        DWORD dwCoefficient,
        DWORD dwSaturation,
        DWORD dwMinAcceleration,
        LONG lAngle
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
CFeelInertia::GetIsCompatibleGUID(GUID &guid)
{
	return IsEqualGUID(guid, GUID_Feel_Inertia);
}


#endif // !defined(AFX_FEELInertia_H__135B88C4_4175_11D1_B049_0020AF30269A__INCLUDED_)
