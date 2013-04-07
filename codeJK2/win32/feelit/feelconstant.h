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

  FILE:		FeelConstant.h

  PURPOSE:	Base Constant Class for Feelit API Foundation Classes

  STARTED:	11/03/97

  NOTES/REVISIONS:
     3/2/99 jrm (Jeff Mallett): Force-->Feel renaming
	 3/2/99 jrm: Added GetIsCompatibleGUID
	 3/15/99 jrm: __declspec(dllimport/dllexport) the whole class

**********************************************************************/


#if !defined(AFX_FEELCONSTANT_H__135B88C4_4175_11D1_B049_0020AF30269A__INCLUDED_)
#define AFX_FEELCONSTANT_H__135B88C4_4175_11D1_B049_0020AF30269A__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef _FFCDLL_
#define DLLFFC __declspec(dllimport)
#else
#define DLLFFC __declspec(dllexport)
#endif

#include "FeelBaseTypes.h"
#include "FeelEffect.h"



//================================================================
// Constants
//================================================================

const POINT FEEL_CONSTANT_DEFAULT_DIRECTION = { 1, 0 };
#define     FEEL_CONSTANT_DEFAULT_DURATION       1000 // Milliseconds
#define     FEEL_CONSTANT_DEFAULT_MAGNITUDE      5000

//
// FORCE --> FEEL Wrappers
//
#define FORCE_CONSTANT_DEFAULT_DIRECTION	FEEL_CONSTANT_DEFAULT_DIRECTION
#define FORCE_CONSTANT_DEFAULT_DURATION		FEEL_CONSTANT_DEFAULT_DURATION
#define FORCE_CONSTANT_DEFAULT_MAGNITUDE	FEEL_CONSTANT_DEFAULT_MAGNITUDE


//================================================================
// CFeelConstant
//================================================================

//
// ------ PUBLIC INTERFACE ------ 
//

class DLLFFC CFeelConstant : public CFeelEffect
{
    //
    // CONSTRUCTOR/DESTRUCTOR
    //

    public:
    
    // Constructor
	CFeelConstant();

	// Destructor
	virtual
	~CFeelConstant();


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
        LONG lDirectionX,
        LONG lDirectionY,
        DWORD dwDuration = FEEL_EFFECT_DONT_CHANGE,
        LONG lMagnitude = FEEL_EFFECT_DONT_CHANGE,
        LPFEEL_ENVELOPE pEnvelope = (LPFEEL_ENVELOPE) FEEL_EFFECT_DONT_CHANGE_PTR
        );

    BOOL
    ChangeParametersPolar( 
        LONG lAngle,
        DWORD dwDuration = FEEL_EFFECT_DONT_CHANGE,
        LONG lMagnitude = FEEL_EFFECT_DONT_CHANGE,
        LPFEEL_ENVELOPE pEnvelope = (LPFEEL_ENVELOPE) FEEL_EFFECT_DONT_CHANGE_PTR
        );


    //
    // OPERATIONS
    //

    public:

    virtual
	BOOL
    Initialize( 
        CFeelDevice* pDevice, 
        LONG lDirectionX = FEEL_EFFECT_DEFAULT_DIRECTION_X,
        LONG lDirectionY = FEEL_EFFECT_DEFAULT_DIRECTION_Y,
        DWORD dwDuration = FEEL_CONSTANT_DEFAULT_DURATION,
        LONG lMagnitude = FEEL_CONSTANT_DEFAULT_MAGNITUDE,
        LPFEEL_ENVELOPE pEnvelope = NULL
        );

    virtual
	BOOL
    InitializePolar( 
        CFeelDevice* pDevice, 
        LONG lArray = FEEL_EFFECT_DEFAULT_ANGLE,
        DWORD dwDuration = FEEL_CONSTANT_DEFAULT_DURATION,
        LONG lMagnitude = FEEL_CONSTANT_DEFAULT_MAGNITUDE,
        LPFEEL_ENVELOPE pEnvelope = NULL
        );

        
//
// ------ PRIVATE INTERFACE ------ 
//

    //
    // HELPERS
    //

    protected:

    BOOL
    set_parameters( 
        DWORD dwfCoordinates,
        LONG lDirection0,
        LONG lDirection1,
        DWORD dwDuration,
        LONG lMagnitude,
        LPFEEL_ENVELOPE pEnvelope
        );


    //
    // INTERNAL DATA
    //

    FEEL_CONSTANTFORCE m_ConstantForce;
    
    protected:

};



//
// INLINES
//

inline BOOL
CFeelConstant::GetIsCompatibleGUID(GUID &guid)
{
	return IsEqualGUID(guid, GUID_Feel_ConstantForce);
}


#endif // !defined(AFX_FEELCONSTANT_H__135B88C4_4175_11D1_B049_0020AF30269A__INCLUDED_)
