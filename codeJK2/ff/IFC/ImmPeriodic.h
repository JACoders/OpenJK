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

  FILE:		ImmPeriodic.h

  PURPOSE:	Base Periodic Class for Immersion Foundation Classes

  STARTED:	11/03/97

  NOTES/REVISIONS:
     3/2/99 jrm (Jeff Mallett): Force-->Feel renaming
	 3/2/99 jrm: Added GetIsCompatibleGUID
	 3/15/99 jrm: __declspec(dllimport/dllexport) the whole class
	 11/15/99 sdr (Steve Rank): Converted to IFC

**********************************************************************/


#if !defined(AFX_IMMPERIODIC_H__135B88C4_4175_11D1_B049_0020AF30269A__INCLUDED_)
#define AFX_IMMPERIODIC_H__135B88C4_4175_11D1_B049_0020AF30269A__INCLUDED_

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

#define     IMM_PERIODIC_DEFAULT_DURATION       1000 // Milliseconds
#define     IMM_PERIODIC_DEFAULT_MAGNITUDE      5000
#define     IMM_PERIODIC_DEFAULT_PERIOD         100  // Milliseconds
#define     IMM_PERIODIC_DEFAULT_OFFSET         0
#define     IMM_PERIODIC_DEFAULT_PHASE          0    // Degrees
#define     IMM_PERIODIC_DEFAULT_DIRECTION_X    1    // Pixels
#define     IMM_PERIODIC_DEFAULT_DIRECTION_Y    0    // Pixels
#define     IMM_PERIODIC_DEFAULT_ANGLE          9000 // 100ths of degrees




//================================================================
// CImmPeriodic
//================================================================

//
// ------ PUBLIC INTERFACE ------ 
//

class DLLIFC CImmPeriodic : public CImmEffect
{
    //
    // CONSTRUCTOR/DESTRUCTOR
    //

    public:
    
    // Constructors

	// You may use this form if you will immediately initialize it
	//   from an IFR file...
	CImmPeriodic();

	// Otherwise use this form...
	CImmPeriodic(
		const GUID& rguidEffect
		);

	// Destructor
	virtual
	~CImmPeriodic();


    //
    // ATTRIBUTES
    //

    public:

	virtual BOOL
	GetIsCompatibleGUID(
		GUID &guid
		);

	virtual DWORD GetEffectType()
		{ return IMM_EFFECTTYPE_PERIODIC; }

    BOOL
    ChangeParameters( 
        DWORD dwMagnitude,
        DWORD dwPeriod = IMM_EFFECT_DONT_CHANGE,
        DWORD dwDuration = IMM_EFFECT_DONT_CHANGE,
        LONG lDirectionX = IMM_EFFECT_DONT_CHANGE,
        LONG lDirectionY = IMM_EFFECT_DONT_CHANGE,
        LONG  lOffset = IMM_EFFECT_DONT_CHANGE,
        DWORD dwPhase = IMM_EFFECT_DONT_CHANGE,
        LPIMM_ENVELOPE pEnvelope = (LPIMM_ENVELOPE) IMM_EFFECT_DONT_CHANGE_PTR
        );

    BOOL
    ChangeParametersPolar( 
        DWORD dwMagnitude,
        DWORD dwPeriod = IMM_EFFECT_DONT_CHANGE,
        DWORD dwDuration = IMM_EFFECT_DONT_CHANGE,
        LONG lAngle = IMM_EFFECT_DONT_CHANGE,
        LONG  lOffset = IMM_EFFECT_DONT_CHANGE,
        DWORD dwPhase = IMM_EFFECT_DONT_CHANGE,
        LPIMM_ENVELOPE pEnvelope = (LPIMM_ENVELOPE) IMM_EFFECT_DONT_CHANGE_PTR
        );

	BOOL ChangeMagnitude( DWORD dwMagnitude );
	BOOL ChangePeriod( DWORD dwPeriod );
	BOOL ChangeOffset( LONG lOffset );
	BOOL ChangePhase( DWORD dwPhase );

	BOOL GetMagnitude( DWORD &dwMagnitude );
	BOOL GetPeriod( DWORD &dwPeriod );
	BOOL GetOffset( LONG &lOffset );
	BOOL GetPhase( DWORD &dwPhase );

    //
    // OPERATIONS
    //

    public:

	virtual BOOL
	Initialize( 
		CImmDevice* pDevice, 
		const IMM_EFFECT &effect,
		DWORD dwNoDownload = 0
    );

    virtual BOOL
    Initialize( 
        CImmDevice* pDevice, 
        DWORD dwMagnitude = IMM_PERIODIC_DEFAULT_MAGNITUDE,
        DWORD dwPeriod = IMM_PERIODIC_DEFAULT_PERIOD,
        DWORD dwDuration = IMM_PERIODIC_DEFAULT_DURATION,
        LONG lDirectionX = IMM_PERIODIC_DEFAULT_DIRECTION_X,
        LONG lDirectionY = IMM_PERIODIC_DEFAULT_DIRECTION_Y,
        LONG  lOffset = IMM_PERIODIC_DEFAULT_OFFSET,
        DWORD dwPhase = IMM_PERIODIC_DEFAULT_PHASE,
        LPIMM_ENVELOPE pEnvelope = NULL,
		DWORD dwNoDownload = 0
        );

    virtual BOOL
    InitializePolar( 
        CImmDevice* pDevice, 
        DWORD dwMagnitude = IMM_PERIODIC_DEFAULT_MAGNITUDE,
        DWORD dwPeriod = IMM_PERIODIC_DEFAULT_PERIOD,
        DWORD dwDuration = IMM_PERIODIC_DEFAULT_DURATION,
        LONG lAngle = IMM_PERIODIC_DEFAULT_ANGLE,
        LONG  lOffset = IMM_PERIODIC_DEFAULT_OFFSET,
        DWORD dwPhase = IMM_PERIODIC_DEFAULT_PHASE,
        LPIMM_ENVELOPE pEnvelope = NULL,
		DWORD dwNoDownload = 0
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
        LPIMM_ENVELOPE pEnvelope
        );

	DWORD
	change_parameters( 
		LONG lDirection0,
		LONG lDirection1,
		DWORD dwDuration,
		DWORD dwMagnitude,
		DWORD dwPeriod,
		LONG  lOffset,
		DWORD dwPhase,
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

    IMM_PERIODIC m_Periodic;
    
};


//
// INLINES
//

inline BOOL
CImmPeriodic::GetIsCompatibleGUID(GUID &guid)
{
	return	IsEqualGUID(guid, GUID_Imm_Sine) ||
			IsEqualGUID(guid, GUID_Imm_Square) ||
			IsEqualGUID(guid, GUID_Imm_Triangle) ||
			IsEqualGUID(guid, GUID_Imm_SawtoothUp) ||
			IsEqualGUID(guid, GUID_Imm_SawtoothDown);
}

#endif // !defined(AFX_IMMPERIODIC_H__135B88C4_4175_11D1_B049_0020AF30269A__INCLUDED_)
