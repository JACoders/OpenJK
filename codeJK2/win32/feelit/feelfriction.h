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

  FILE:		FeelFriction.h

  PURPOSE:	Feelit API Friction Effect Class

  STARTED:	12/29/97

  NOTES/REVISIONS:
     3/2/99 jrm (Jeff Mallett): Force-->Feel renaming
	 3/2/99 jrm: Added GetIsCompatibleGUID
	 3/15/99 jrm: __declspec(dllimport/dllexport) the whole class

**********************************************************************/


#if !defined(AFX_FEELFriction_H__135B88C4_4175_11D1_B049_0020AF30269A__INCLUDED_)
#define AFX_FEELFriction_H__135B88C4_4175_11D1_B049_0020AF30269A__INCLUDED_

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

#define     FEEL_FRICTION_DEFAULT_COEFFICIENT    2500
#define     FEEL_FRICTION_DEFAULT_MIN_VELOCITY   0

//
// FORCE --> FEEL Wrappers
//
#define     FORCE_FRICTION_DEFAULT_COEFFICIENT    FEEL_FRICTION_DEFAULT_COEFFICIENT
#define     FORCE_FRICTION_DEFAULT_MIN_VELOCITY   FEEL_FRICTION_DEFAULT_MIN_VELOCITY


//================================================================
// CFeelFriction
//================================================================

//
// ------ PUBLIC INTERFACE ------ 
//

class DLLFFC CFeelFriction : public CFeelCondition
{
    //
    // CONSTRUCTOR/DESTRUCTOR
    //

    public:
    
    // Constructor
	CFeelFriction();

	// Destructor
	virtual
	~CFeelFriction();


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
        DWORD dwMinVelocity = FEEL_EFFECT_DONT_CHANGE,
        LONG lDirectionX = FEEL_EFFECT_DONT_CHANGE,
        LONG lDirectionY = FEEL_EFFECT_DONT_CHANGE
        );

    BOOL
    ChangeParametersPolar( 
        DWORD dwCoefficient,
        DWORD dwMinVelocity,
        LONG lAngle
        );


    //
    // OPERATIONS
    //

    public:

    virtual BOOL 
    Initialize( 
        CFeelDevice* pDevice, 
        DWORD dwCoefficient = FEEL_FRICTION_DEFAULT_COEFFICIENT,
        DWORD dwMinVelocity = FEEL_FRICTION_DEFAULT_MIN_VELOCITY,
        DWORD dwfAxis = FEEL_EFFECT_AXIS_BOTH,
        LONG lDirectionX = FEEL_EFFECT_DEFAULT_DIRECTION_X,
        LONG lDirectionY = FEEL_EFFECT_DEFAULT_DIRECTION_Y
        );

    virtual BOOL 
    InitializePolar( 
        CFeelDevice* pDevice, 
        DWORD dwCoefficient,
        DWORD dwMinVelocity,
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
CFeelFriction::GetIsCompatibleGUID(GUID &guid)
{
	return IsEqualGUID(guid, GUID_Feel_Friction);
}


#endif // !defined(AFX_FEELFriction_H__135B88C4_4175_11D1_B049_0020AF30269A__INCLUDED_)
