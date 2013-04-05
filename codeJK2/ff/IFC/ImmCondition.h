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

  FILE:		ImmCondition.h

  PURPOSE:	Immersion Foundation Classes Base Condition Effect
  
  STARTED:	Oct.10.97

  NOTES/REVISIONS:
     Mar.02.99 jrm (Jeff Mallett): Force-->Feel renaming
	 Mar.02.99 jrm: Added GetIsCompatibleGUID
	 Mar.15.99 jrm: __declspec(dllimport/dllexport) the whole class
	 Nov.15.99 efw (Evan Wies): Converted to IFC

**********************************************************************/


#if !defined(AFX_IMMCondition_H__135B88C4_4175_11D1_B049_0020AF30269A__INCLUDED_)
#define AFX_IMMCondition_H__135B88C4_4175_11D1_B049_0020AF30269A__INCLUDED_

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

const POINT IMM_CONDITION_PT_NULL = { 0, 0 };

#define     IMM_CONDITION_DEFAULT_COEFFICIENT    2500
#define     IMM_CONDITION_DEFAULT_SATURATION     10000
#define     IMM_CONDITION_DEFAULT_DEADBAND       100
#define     IMM_CONDITION_DEFAULT_CENTER_POINT   IMM_EFFECT_MOUSE_POS_AT_START
#define     IMM_CONDITION_DEFAULT_DURATION       INFINITE

typedef enum {
	IC_NULL = 0,
	IC_POSITIVE_COEFFICIENT,
	IC_NEGATIVE_COEFFICIENT,
	IC_POSITIVE_SATURATION,
	IC_NEGATIVE_SATURATION,
	IC_DEAD_BAND,
	IC_AXIS,
	IC_CENTER,
	IC_DIRECTION_X,
	IC_DIRECTION_Y,
	IC_ANGLE,
	IC_CONDITION_X,
	IC_CONDITION_Y
} IC_ArgumentType;

#define IC_CONDITION	IC_CONDITION_X



//================================================================
// CImmCondition
//================================================================

//
// ------ PUBLIC INTERFACE ------ 
//

class DLLIFC CImmCondition : public CImmEffect
{
    //
    // CONSTRUCTOR/DESCTRUCTOR
    //

    public:
    
    // Constructor
	CImmCondition(
		const GUID& rguidEffect
		);

	// Destructor
	virtual
	~CImmCondition();


    //
    // ATTRIBUTES
    //

    public:

	virtual BOOL
	GetIsCompatibleGUID(
		GUID &guid
		);

	virtual DWORD GetEffectType()
		{ return IMM_EFFECTTYPE_CONDITION; }

    // Use this form for single-axis and dual-axis effects
	BOOL
    ChangeConditionParams( 
        LPCIMM_CONDITION pConditionX,
        LPCIMM_CONDITION pConditionY
        );

    // Use this form for directional effects
    BOOL
	ChangeConditionParams( 
        LPCIMM_CONDITION pCondition,
        LONG lDirectionX,
        LONG lDirectionY
        );

    // Use this form for directional effects
    BOOL
	ChangeConditionParamsPolar( 
        LPCIMM_CONDITION pCondition,
        LONG lAngle
        );

    // Use this form for single-axis, dual-axis, or directional effects
    BOOL
	ChangeConditionParamsX( 
        LONG lPositiveCoefficient,
        LONG lNegativeCoefficient = IMM_EFFECT_DONT_CHANGE,
        DWORD dwPositiveSaturation = IMM_EFFECT_DONT_CHANGE,
        DWORD dwNegativeSaturation = IMM_EFFECT_DONT_CHANGE,
        LONG lDeadBand = IMM_EFFECT_DONT_CHANGE,
        POINT pntCenter = IMM_EFFECT_DONT_CHANGE_POINT,
        LONG lDirectionX = IMM_EFFECT_DONT_CHANGE,
        LONG lDirectionY = IMM_EFFECT_DONT_CHANGE
        );

    BOOL
	ChangeConditionParamsPolarX( 
        LONG lPositiveCoefficient,
        LONG lNegativeCoefficient,
        DWORD dwPositiveSaturation,
        DWORD dwNegativeSaturation,
        LONG lDeadBand,
        POINT pntCenter,
        LONG lAngle
        );

    BOOL
	ChangeConditionParamsY( 
        LONG lPositiveCoefficient,
        LONG lNegativeCoefficient = IMM_EFFECT_DONT_CHANGE,
        DWORD dwPositiveSaturation = IMM_EFFECT_DONT_CHANGE,
        DWORD dwNegativeSaturation = IMM_EFFECT_DONT_CHANGE,
        LONG lDeadBand = IMM_EFFECT_DONT_CHANGE,
        POINT pntCenter = IMM_EFFECT_DONT_CHANGE_POINT,
        LONG lDirectionX = IMM_EFFECT_DONT_CHANGE,
        LONG lDirectionY = IMM_EFFECT_DONT_CHANGE
        );

    BOOL
	ChangeConditionParamsPolarY( 
        LONG lPositiveCoefficient,
        LONG lNegativeCoefficient,
        DWORD dwPositiveSaturation,
        DWORD dwNegativeSaturation,
        LONG lDeadBand,
        POINT pntCenter,
        LONG lAngle
        );

    // Use this form for single-axis, dual-axis symetrical, or directional effects
    BOOL
	ChangeConditionParams( 
        LONG lPositiveCoefficient,
        LONG lNegativeCoefficient = IMM_EFFECT_DONT_CHANGE,
        DWORD dwPositiveSaturation = IMM_EFFECT_DONT_CHANGE,
        DWORD dwNegativeSaturation = IMM_EFFECT_DONT_CHANGE,
        LONG lDeadBand = IMM_EFFECT_DONT_CHANGE,
        POINT pntCenter = IMM_EFFECT_DONT_CHANGE_POINT,
        LONG lDirectionX = IMM_EFFECT_DONT_CHANGE,
        LONG lDirectionY = IMM_EFFECT_DONT_CHANGE
        );

    BOOL
	ChangeConditionParamsPolar( 
        LONG lPositiveCoefficient,
        LONG lNegativeCoefficient,
        DWORD dwPositiveSaturation,
        DWORD dwNegativeSaturation,
        LONG lDeadBand,
        POINT pntCenter,
        LONG lAngle
        );

	BOOL ChangePositiveCoefficientX( LONG lPositiveCoefficient );
	BOOL ChangeNegativeCoefficientX( LONG lNegativeCoefficient );
	BOOL ChangePositiveSaturationX( DWORD dwPositiveSaturation );
	BOOL ChangeNegativeSaturationX( DWORD dwNegativeSaturation );
	BOOL ChangeDeadBandX( LONG lDeadBand );

	BOOL ChangePositiveCoefficientY( LONG lPositiveCoefficient );
	BOOL ChangeNegativeCoefficientY( LONG lNegativeCoefficient );
	BOOL ChangePositiveSaturationY( DWORD dwPositiveSaturation );
	BOOL ChangeNegativeSaturationY( DWORD dwNegativeSaturation );
	BOOL ChangeDeadBandY( LONG lDeadBand );

	BOOL ChangePositiveCoefficient(	LONG lPositiveCoefficient );
	BOOL ChangeNegativeCoefficient(	LONG lNegativeCoefficient );
	BOOL ChangePositiveSaturation( DWORD dwPositiveSaturation );
	BOOL ChangeNegativeSaturation( DWORD dwNegativeSaturation );
	BOOL ChangeDeadBand( LONG lDeadBand	);

    BOOL
	SetCenter( 
        POINT pntCenter 
        );

	BOOL 
	ChangeConditionParams2(
		IC_ArgumentType type,
		...
	);

	BOOL GetPositiveCoefficientX( LONG &lPositiveCoefficient );
	BOOL GetNegativeCoefficientX( LONG &lNegativeCoefficient );
	BOOL GetPositiveSaturationX( DWORD &dwPositiveSaturation );
	BOOL GetNegativeSaturationX( DWORD &dwNegativeSaturation );
	BOOL GetDeadBandX( LONG &lDeadBand );

	BOOL GetPositiveCoefficientY( LONG &lPositiveCoefficient );
	BOOL GetNegativeCoefficientY( LONG &lNegativeCoefficient );
	BOOL GetPositiveSaturationY( DWORD &dwPositiveSaturation );
	BOOL GetNegativeSaturationY( DWORD &dwNegativeSaturation );
	BOOL GetDeadBandY( LONG &lDeadBand );

	BOOL GetAxis( DWORD &dwfAxis );
    BOOL GetCenter( POINT &pntCenter );

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

	// Use this form for single-axis and dual-axis effects
    BOOL 
	InitCondition( 
        CImmDevice* pDevice, 
        LPCIMM_CONDITION pConditionX,
        LPCIMM_CONDITION pConditionY,
        BOOL bUseDeviceCoordinates = FALSE,
		DWORD dwNoDownload = 0
		);


    // Use this form for directional effects
    BOOL 
	InitCondition( 
        CImmDevice* pDevice, 
        LPCIMM_CONDITION pCondition,
        LONG lDirectionX,
        LONG lDirectionY,
        BOOL bUseDeviceCoordinates = FALSE,
		DWORD dwNoDownload = 0
		);


    // Use this form for directional effects
    BOOL 
    InitConditionPolar( 
        CImmDevice* pDevice, 
        LPCIMM_CONDITION pCondition,
        LONG lAngle,
        BOOL bUseDeviceCoordinates = FALSE,
		DWORD dwNoDownload = 0
		);


    // Use this form for single-axis, dual-axis symetrical, or directional effects
    BOOL 
    InitCondition( 
        CImmDevice* pDevice, 
        LONG lPositiveCoefficient = IMM_CONDITION_DEFAULT_COEFFICIENT,
        LONG lNegativeCoefficient = IMM_CONDITION_DEFAULT_COEFFICIENT,
        DWORD dwPositiveSaturation = IMM_CONDITION_DEFAULT_SATURATION,
        DWORD dwNegativeSaturation = IMM_CONDITION_DEFAULT_SATURATION,
        LONG lDeadBand = IMM_CONDITION_DEFAULT_DEADBAND,
        DWORD dwfAxis = IMM_EFFECT_AXIS_BOTH,
        POINT pntCenter = IMM_CONDITION_DEFAULT_CENTER_POINT,
        LONG lDirectionX = IMM_EFFECT_DEFAULT_DIRECTION_X,
        LONG lDirectionY = IMM_EFFECT_DEFAULT_DIRECTION_Y,
        BOOL bUseDeviceCoordinates = FALSE,
		DWORD dwNoDownload = 0
		);

    // Use this form for directional effects
    BOOL 
    InitConditionPolar( 
        CImmDevice* pDevice, 
        LONG lPositiveCoefficient = IMM_CONDITION_DEFAULT_COEFFICIENT,
        LONG lNegativeCoefficient = IMM_CONDITION_DEFAULT_COEFFICIENT,
        DWORD dwPositiveSaturation = IMM_CONDITION_DEFAULT_SATURATION,
        DWORD dwNegativeSaturation = IMM_CONDITION_DEFAULT_SATURATION,
        LONG lDeadBand = IMM_CONDITION_DEFAULT_DEADBAND,
        POINT pntCenter = IMM_CONDITION_DEFAULT_CENTER_POINT,
        LONG lAngle = IMM_EFFECT_DEFAULT_ANGLE,
        BOOL bUseDeviceCoordinates = FALSE,
		DWORD dwNoDownload = 0
		);


    virtual BOOL
    Start(
        DWORD dwIterations = 1,
        DWORD dwFlags = 0,
		BOOL bAllowStartDelayEmulation = true
        );

        
//
// ------ PRIVATE INTERFACE ------ 
//

    //
    // HELPERS
    //

    protected:

	void
	convert_line_point_to_offset(
		POINT pntOnLine
		);

    BOOL
    set_parameters( 
        DWORD dwfAxis,
        DWORD dwfCoordinates,
        LONG lDirection0,
        LONG lDirection1,
        LPCIMM_CONDITION pConditionX, 
        LPCIMM_CONDITION pConditionY
        );

    BOOL
    set_parameters( 
        DWORD dwfAxis,
        DWORD dwfCoordinates,
        LONG lDirection0,
        LONG lDirection1,
        LONG lPositiveCoefficient,
        LONG lNegativeCoefficient,
        DWORD dwPositiveSaturation,
        DWORD dwNegativeSaturation,
        LONG lDeadBand,
        POINT pntCenter
        );

	DWORD
	change_parameters( 
		LONG lDirection0,
		LONG lDirection1,
		LONG lPositiveCoefficient,
		LONG lNegativeCoefficient,
		DWORD dwPositiveSaturation,
		DWORD dwNegativeSaturation,
		LONG lDeadBand,
		POINT pntCenter,
		int fAxis
		);

	DWORD
	change_parameters( 
		LONG lDirection0,
		LONG lDirection1,
		LPCIMM_CONDITION pConditionX, 
		LPCIMM_CONDITION pConditionY
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

    IMM_CONDITION m_aCondition[2];
    DWORD m_dwfAxis;
    BOOL m_bUseMousePosAtStart;
    
    protected:
	    BOOL m_bUseDeviceCoordinates;

};


//
// INLINES
//

inline BOOL
CImmCondition::GetIsCompatibleGUID(GUID &guid)
{
	return  IsEqualGUID(guid, GUID_Imm_Spring) ||
			IsEqualGUID(guid, GUID_Imm_DeviceSpring) ||
			IsEqualGUID(guid, GUID_Imm_Damper) ||
			IsEqualGUID(guid, GUID_Imm_Inertia) ||
			IsEqualGUID(guid, GUID_Imm_Friction) ||
			IsEqualGUID(guid, GUID_Imm_Texture) ||
			IsEqualGUID(guid, GUID_Imm_Grid);
}

#endif // !defined(AFX_IMMCondition_H__135B88C4_4175_11D1_B049_0020AF30269A__INCLUDED_)

