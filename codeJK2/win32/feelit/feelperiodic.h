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

  FILE:		FeelPeriodic.h

  PURPOSE:	Base Periodic Class for Feelit API Foundation Classes

  STARTED:	11/03/97

  NOTES/REVISIONS:
     3/2/99 jrm (Jeff Mallett): Force-->Feel renaming
	 3/2/99 jrm: Added GetIsCompatibleGUID
	 3/15/99 jrm: __declspec(dllimport/dllexport) the whole class

**********************************************************************/


#if !defined(AFX_FEELPERIODIC_H__135B88C4_4175_11D1_B049_0020AF30269A__INCLUDED_)
#define AFX_FEELPERIODIC_H__135B88C4_4175_11D1_B049_0020AF30269A__INCLUDED_

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

#define     FEEL_PERIODIC_DEFAULT_DURATION       1000 // Milliseconds
#define     FEEL_PERIODIC_DEFAULT_MAGNITUDE      5000
#define     FEEL_PERIODIC_DEFAULT_PERIOD         100  // Milliseconds
#define     FEEL_PERIODIC_DEFAULT_OFFSET         0
#define     FEEL_PERIODIC_DEFAULT_PHASE          0    // Degrees
#define     FEEL_PERIODIC_DEFAULT_DIRECTION_X    1    // Pixels
#define     FEEL_PERIODIC_DEFAULT_DIRECTION_Y    0    // Pixels
#define     FEEL_PERIODIC_DEFAULT_ANGLE          9000 // 100ths of degrees

//
// FORCE --> FEEL Wrappers
//
#define     FORCE_PERIODIC_DEFAULT_DURATION       FEEL_PERIODIC_DEFAULT_DURATION
#define     FORCE_PERIODIC_DEFAULT_MAGNITUDE      FEEL_PERIODIC_DEFAULT_MAGNITUDE
#define     FORCE_PERIODIC_DEFAULT_PERIOD         FEEL_PERIODIC_DEFAULT_PERIOD
#define     FORCE_PERIODIC_DEFAULT_OFFSET         FEEL_PERIODIC_DEFAULT_OFFSET
#define     FORCE_PERIODIC_DEFAULT_PHASE          FEEL_PERIODIC_DEFAULT_PHASE
#define     FORCE_PERIODIC_DEFAULT_DIRECTION_X    FEEL_PERIODIC_DEFAULT_DIRECTION_X
#define     FORCE_PERIODIC_DEFAULT_DIRECTION_Y    FEEL_PERIODIC_DEFAULT_DIRECTION_Y
#define     FORCE_PERIODIC_DEFAULT_ANGLE          FEEL_PERIODIC_DEFAULT_ANGLE



//================================================================
// CFeelPeriodic
//================================================================

//
// ------ PUBLIC INTERFACE ------ 
//

class DLLFFC CFeelPeriodic : public CFeelEffect
{
    //
    // CONSTRUCTOR/DESTRUCTOR
    //

    public:
    
    // Constructors

	// You may use this form if you will immediately initialize it
	//   from an IFR file...
	CFeelPeriodic();

	// Otherwise use this form...
	CFeelPeriodic(
		const GUID& rguidEffect
		);

	// Destructor
	virtual
	~CFeelPeriodic();


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
        DWORD dwMagnitude,
        DWORD dwPeriod = FEEL_EFFECT_DONT_CHANGE,
        DWORD dwDuration = FEEL_EFFECT_DONT_CHANGE,
        LONG lDirectionX = FEEL_EFFECT_DONT_CHANGE,
        LONG lDirectionY = FEEL_EFFECT_DONT_CHANGE,
        LONG  lOffset = FEEL_EFFECT_DONT_CHANGE,
        DWORD dwPhase = FEEL_EFFECT_DONT_CHANGE,
        LPFEEL_ENVELOPE pEnvelope = (LPFEEL_ENVELOPE) FEEL_EFFECT_DONT_CHANGE_PTR
        );

    BOOL
    ChangeParametersPolar( 
        DWORD dwMagnitude,
        DWORD dwPeriod = FEEL_EFFECT_DONT_CHANGE,
        DWORD dwDuration = FEEL_EFFECT_DONT_CHANGE,
        LONG lAngle = FEEL_EFFECT_DONT_CHANGE,
        LONG  lOffset = FEEL_EFFECT_DONT_CHANGE,
        DWORD dwPhase = FEEL_EFFECT_DONT_CHANGE,
        LPFEEL_ENVELOPE pEnvelope = (LPFEEL_ENVELOPE) FEEL_EFFECT_DONT_CHANGE_PTR
        );


    //
    // OPERATIONS
    //

    public:

    virtual BOOL
    Initialize( 
        CFeelDevice* pDevice, 
        DWORD dwMagnitude = FEEL_PERIODIC_DEFAULT_MAGNITUDE,
        DWORD dwPeriod = FEEL_PERIODIC_DEFAULT_PERIOD,
        DWORD dwDuration = FEEL_PERIODIC_DEFAULT_DURATION,
        LONG lDirectionX = FEEL_PERIODIC_DEFAULT_DIRECTION_X,
        LONG lDirectionY = FEEL_PERIODIC_DEFAULT_DIRECTION_Y,
        LONG  lOffset = FEEL_PERIODIC_DEFAULT_OFFSET,
        DWORD dwPhase = FEEL_PERIODIC_DEFAULT_PHASE,
        LPFEEL_ENVELOPE pEnvelope = NULL
        );

    virtual BOOL
    InitializePolar( 
        CFeelDevice* pDevice, 
        DWORD dwMagnitude = FEEL_PERIODIC_DEFAULT_MAGNITUDE,
        DWORD dwPeriod = FEEL_PERIODIC_DEFAULT_PERIOD,
        DWORD dwDuration = FEEL_PERIODIC_DEFAULT_DURATION,
        LONG lAngle = FEEL_PERIODIC_DEFAULT_ANGLE,
        LONG  lOffset = FEEL_PERIODIC_DEFAULT_OFFSET,
        DWORD dwPhase = FEEL_PERIODIC_DEFAULT_PHASE,
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
        DWORD dwMagnitude,
        DWORD dwPeriod,
        LONG  lOffset,
        DWORD dwPhase,
        LPFEEL_ENVELOPE pEnvelope
        );


    //
    // INTERNAL DATA
    //

	protected:

    FEEL_PERIODIC m_Periodic;
    
};


//
// INLINES
//

inline BOOL
CFeelPeriodic::GetIsCompatibleGUID(GUID &guid)
{
	return	IsEqualGUID(guid, GUID_Feel_Sine) ||
			IsEqualGUID(guid, GUID_Feel_Square) ||
			IsEqualGUID(guid, GUID_Feel_Triangle) ||
			IsEqualGUID(guid, GUID_Feel_SawtoothUp) ||
			IsEqualGUID(guid, GUID_Feel_SawtoothDown);
}

#endif // !defined(AFX_FEELPERIODIC_H__135B88C4_4175_11D1_B049_0020AF30269A__INCLUDED_)
