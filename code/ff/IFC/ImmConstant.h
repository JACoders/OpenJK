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

  FILE:		ImmConstant.h

  PURPOSE:	Base Constant Class for Immersion Foundation Classes

  STARTED:	11/03/97

  NOTES/REVISIONS:
     3/2/99 jrm (Jeff Mallett): Force-->Feel renaming
	 3/2/99 jrm: Added GetIsCompatibleGUID
	 3/15/99 jrm: __declspec(dllimport/dllexport) the whole class
	 11/15/99 sdr (Steve Rank): Converted to IFC
**********************************************************************/


#if !defined(AFX_IMMCONSTANT_H__135B88C4_4175_11D1_B049_0020AF30269A__INCLUDED_)
#define AFX_IMMCONSTANT_H__135B88C4_4175_11D1_B049_0020AF30269A__INCLUDED_

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

const POINT IMM_CONSTANT_DEFAULT_DIRECTION = { 1, 0 };
#define     IMM_CONSTANT_DEFAULT_DURATION       1000 // Milliseconds
#define     IMM_CONSTANT_DEFAULT_MAGNITUDE      5000



//================================================================
// CImmConstant
//================================================================

//
// ------ PUBLIC INTERFACE ------ 
//

class DLLIFC CImmConstant : public CImmEffect
{
    //
    // CONSTRUCTOR/DESTRUCTOR
    //

    public:
    
    // Constructor
	CImmConstant();

	// Destructor
	virtual
	~CImmConstant();


    //
    // ATTRIBUTES
    //

    public:

	virtual BOOL
	GetIsCompatibleGUID(
		GUID &guid
		);

	virtual DWORD GetEffectType()
		{ return IMM_EFFECTTYPE_CONSTANTFORCE; }

	BOOL
    ChangeParameters( 
        LONG lDirectionX,
        LONG lDirectionY,
        DWORD dwDuration = IMM_EFFECT_DONT_CHANGE,
        LONG lMagnitude = IMM_EFFECT_DONT_CHANGE,
        LPIMM_ENVELOPE pEnvelope = (LPIMM_ENVELOPE) IMM_EFFECT_DONT_CHANGE_PTR
        );

    BOOL
    ChangeParametersPolar( 
        LONG lAngle,
        DWORD dwDuration = IMM_EFFECT_DONT_CHANGE,
        LONG lMagnitude = IMM_EFFECT_DONT_CHANGE,
        LPIMM_ENVELOPE pEnvelope = (LPIMM_ENVELOPE) IMM_EFFECT_DONT_CHANGE_PTR
        );

	BOOL ChangeMagnitude( LONG lMagnitude );
	BOOL GetMagnitude( LONG &lMagnitude );

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

    virtual
	BOOL
    Initialize( 
        CImmDevice* pDevice, 
        LONG lDirectionX = IMM_EFFECT_DEFAULT_DIRECTION_X,
        LONG lDirectionY = IMM_EFFECT_DEFAULT_DIRECTION_Y,
        DWORD dwDuration = IMM_CONSTANT_DEFAULT_DURATION,
        LONG lMagnitude = IMM_CONSTANT_DEFAULT_MAGNITUDE,
        LPIMM_ENVELOPE pEnvelope = NULL,
		DWORD dwNoDownload = 0
        );

    virtual
	BOOL
    InitializePolar( 
        CImmDevice* pDevice, 
        LONG lAngle = IMM_EFFECT_DEFAULT_ANGLE,
        DWORD dwDuration = IMM_CONSTANT_DEFAULT_DURATION,
        LONG lMagnitude = IMM_CONSTANT_DEFAULT_MAGNITUDE,
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
        LONG lMagnitude,
        LPIMM_ENVELOPE pEnvelope
        );

	DWORD
	change_parameters( 
    LONG lDirection0,
    LONG lDirection1,
    DWORD dwDuration,
    LONG lMagnitude,
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

    IMM_CONSTANTFORCE m_ConstantForce;
    
    protected:

};



//
// INLINES
//

inline BOOL
CImmConstant::GetIsCompatibleGUID(GUID &guid)
{
	return IsEqualGUID(guid, GUID_Imm_ConstantForce);
}


#endif // !defined(AFX_IMMCONSTANT_H__135B88C4_4175_11D1_B049_0020AF30269A__INCLUDED_)
