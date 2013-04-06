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

  FILE:		ImmRamp.h

  PURPOSE:	Base Ramp Force Class for Immersion Foundation Classes

  STARTED:	12/11/97

  NOTES/REVISIONS:
     3/2/99 jrm (Jeff Mallett): Force-->Feel renaming
	 3/2/99 jrm: Added GetIsCompatibleGUID
	 3/15/99 jrm: __declspec(dllimport/dllexport) the whole class
	 11/15/99 sdr (Steve Rank): Converted to IFC

**********************************************************************/


#if !defined(AFX_IMMRAMP_H__135B88C4_4175_11D1_B049_0020AF30269A__INCLUDED_)
#define AFX_IMMRAMP_H__135B88C4_4175_11D1_B049_0020AF30269A__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef _IFCDLL_
#define DLLIFC __declspec(dllimport)
#else
#define DLLIFC __declspec(dllexport)
#endif

#include "ImmBaseTypes.h"
#include "ImmEffect.h"



//================================================================
// Constants
//================================================================

#define     IMM_RAMP_DEFAULT_DURATION			1000 // Milliseconds
#define     IMM_RAMP_DEFAULT_MAGNITUDE_START	0
#define     IMM_RAMP_DEFAULT_MAGNITUDE_END		10000



//================================================================
// CImmRamp
//================================================================

//
// ------ PUBLIC INTERFACE ------ 
//
        
class DLLIFC CImmRamp : public CImmEffect
{
    //
    // CONSTRUCTOR/DESTRUCTOR
    //

    public:
    
    // Constructor
	CImmRamp();

	// Destructor
	virtual
	~CImmRamp();


    //
    // ATTRIBUTES
    //

    public:

	virtual BOOL
	GetIsCompatibleGUID(
		GUID &guid
		);

	virtual DWORD GetEffectType()
		{ return IMM_EFFECTTYPE_RAMPFORCE; }

    BOOL
    ChangeParameters( 
        LONG lDirectionX,
        LONG lDirectionY,
        DWORD dwDuration = IMM_EFFECT_DONT_CHANGE,
        LONG lMagStart = IMM_EFFECT_DONT_CHANGE,
        LONG lMagEnd = IMM_EFFECT_DONT_CHANGE,
        LPIMM_ENVELOPE pEnvelope = (LPIMM_ENVELOPE) IMM_EFFECT_DONT_CHANGE_PTR
        );

    BOOL
    ChangeParametersPolar( 
        LONG lAngle,
        DWORD dwDuration = IMM_EFFECT_DONT_CHANGE,
        LONG lMagStart = IMM_EFFECT_DONT_CHANGE,
        LONG lMagEnd = IMM_EFFECT_DONT_CHANGE,
        LPIMM_ENVELOPE pEnvelope = (LPIMM_ENVELOPE) IMM_EFFECT_DONT_CHANGE_PTR
        );

	BOOL ChangeStartMagnitude( LONG lMagStart );
	BOOL ChangeEndMagnitude( LONG lMagEnd );

	BOOL GetStartMagnitude( LONG &lMagStart );
	BOOL GetEndMagnitude( LONG &lMagEnd );

    //
    // OPERATIONS
    //

    public:

	virtual BOOL
	Initialize( 
		CImmDevice* pDevice, 
		const IMM_EFFECT &effect,
		DWORD dwFlags = 0
    );

    virtual BOOL
    Initialize( 
        CImmDevice* pDevice, 
        LONG lDirectionX = IMM_EFFECT_DEFAULT_DIRECTION_X,
        LONG lDirectionY = IMM_EFFECT_DEFAULT_DIRECTION_Y,
        DWORD dwDuration = IMM_RAMP_DEFAULT_DURATION,
        LONG lMagStart = IMM_RAMP_DEFAULT_MAGNITUDE_START,
        LONG lMagEnd = IMM_RAMP_DEFAULT_MAGNITUDE_END,
        LPIMM_ENVELOPE pEnvelope = NULL,
		DWORD dwFlags = 0
        );

    virtual BOOL
    InitializePolar( 
        CImmDevice* pDevice, 
        LONG lAngle = IMM_EFFECT_DEFAULT_ANGLE,
        DWORD dwDuration = IMM_RAMP_DEFAULT_DURATION,
        LONG lMagStart = IMM_RAMP_DEFAULT_MAGNITUDE_START,
        LONG lMagEnd = IMM_RAMP_DEFAULT_MAGNITUDE_END,
        LPIMM_ENVELOPE pEnvelope = NULL,
		DWORD dwFlags = 0
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
        LONG lMagStart,
        LONG lMagEnd,
        LPIMM_ENVELOPE pEnvelope
        );

	DWORD
	change_parameters( 
		LONG lDirection0,
		LONG lDirection1,
		DWORD dwDuration,
		LONG lMagStart,
		LONG lMagEnd,
		LPIMM_ENVELOPE pEnvelope
		);
	
	int
	buffer_ifr_data(
		TCHAR* pData
		);
	
	BOOL
	get_ffe_data(
		LPDIEFFECT pdiEffect
		);

    //
    // INTERNAL DATA
    //

    protected:

    IMM_RAMPFORCE m_RampForce;
    
};


//
// INLINES
//

inline BOOL
CImmRamp::GetIsCompatibleGUID(GUID &guid)
{
	return IsEqualGUID(guid, GUID_Imm_RampForce);
}

#endif // !defined(AFX_IMMRAMP_H__135B88C4_4175_11D1_B049_0020AF30269A__INCLUDED_)
