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

  FILE:		FeelCondition.h

  PURPOSE:	Base Condition Class for Feelit API Foundation Classes

  STARTED:	10/10/97

  NOTES/REVISIONS:
     3/2/99 jrm (Jeff Mallett): Force-->Feel renaming
	 3/2/99 jrm: Added GetIsCompatibleGUID
	 3/15/99 jrm: __declspec(dllimport/dllexport) the whole class

**********************************************************************/


#if !defined(AFX_FEELCONDITION_H__135B88C4_4175_11D1_B049_0020AF30269A__INCLUDED_)
#define AFX_FEELCONDITION_H__135B88C4_4175_11D1_B049_0020AF30269A__INCLUDED_

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

const POINT FEEL_CONDITION_PT_NULL = { 0, 0 };

#define     FEEL_CONDITION_DEFAULT_COEFFICIENT    2500
#define     FEEL_CONDITION_DEFAULT_SATURATION     10000
#define     FEEL_CONDITION_DEFAULT_DEADBAND       100
#define     FEEL_CONDITION_DEFAULT_CENTER_POINT   FEEL_EFFECT_MOUSE_POS_AT_START
#define     FEEL_CONDITION_DEFAULT_DURATION       INFINITE

typedef enum {
	FC_NULL = 0,
	FC_POSITIVE_COEFFICIENT,
	FC_NEGATIVE_COEFFICIENT,
	FC_POSITIVE_SATURATION,
	FC_NEGATIVE_SATURATION,
	FC_DEAD_BAND,
	FC_AXIS,
	FC_CENTER,
	FC_DIRECTION_X,
	FC_DIRECTION_Y,
	FC_ANGLE,
	FC_CONDITION_X,
	FC_CONDITION_Y
} FC_ArgumentType;

#define FC_CONDITION	FC_CONDITION_X

//
// FORCE --> FEEL Wrappers
//
#define		FORCE_CONDITION_PT_NULL					FEEL_CONDITION_PT_NULL

#define     FORCE_CONDITION_DEFAULT_COEFFICIENT		FEEL_CONDITION_DEFAULT_COEFFICIENT
#define     FORCE_CONDITION_DEFAULT_SATURATION		FEEL_CONDITION_DEFAULT_SATURATION
#define     FORCE_CONDITION_DEFAULT_DEADBAND		FEEL_CONDITION_DEFAULT_DEADBAND
#define     FORCE_CONDITION_DEFAULT_CENTER_POINT	FEEL_CONDITION_DEFAULT_CENTER_POINT
#define     FORCE_CONDITION_DEFAULT_DURATION		FEEL_CONDITION_DEFAULT_DURATION



//================================================================
// CFeelCondition
//================================================================

//
// ------ PUBLIC INTERFACE ------ 
//

class DLLFFC CFeelCondition : public CFeelEffect
{
    //
    // CONSTRUCTOR/DESCTRUCTOR
    //

    public:
    
    // Constructor
	CFeelCondition(
		const GUID& rguidEffect
		);

	// Destructor
	virtual
	~CFeelCondition();


    //
    // ATTRIBUTES
    //

    public:

	virtual BOOL
	GetIsCompatibleGUID(
		GUID &guid
		);

    // Use this form for single-axis and dual-axis effects
	BOOL
    ChangeConditionParams( 
        LPCFEELIT_CONDITION pConditionX,
        LPCFEELIT_CONDITION pConditionY
        );

    // Use this form for directional effects
    BOOL
	ChangeConditionParams( 
        LPCFEELIT_CONDITION pCondition,
        LONG lDirectionX,
        LONG lDirectionY
        );

    // Use this form for directional effects
    BOOL
	ChangeConditionParamsPolar( 
        LPCFEELIT_CONDITION pCondition,
        LONG lAngle
        );

    // Use this form for single-axis, dual-axis symetrical, or directional effects
    BOOL
	ChangeConditionParams( 
        LONG lPositiveCoefficient,
        LONG lNegativeCoefficient = FEEL_EFFECT_DONT_CHANGE,
        DWORD dwPositiveSaturation = FEEL_EFFECT_DONT_CHANGE,
        DWORD dwNegativeSaturation = FEEL_EFFECT_DONT_CHANGE,
        LONG lDeadBand = FEEL_EFFECT_DONT_CHANGE,
        POINT pntCenter = FEEL_EFFECT_DONT_CHANGE_POINT,
        LONG lDirectionX = FEEL_EFFECT_DONT_CHANGE,
        LONG lDirectionY = FEEL_EFFECT_DONT_CHANGE
        );

    // Use this form for single-axis, dual-axis symetrical, or directional effects
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


    BOOL
	SetCenter( 
        POINT pntCenter 
        );

	BOOL 
	ChangeConditionParams2(
		FC_ArgumentType type,
		...
	);

    //
    // OPERATIONS
    //

    public:

	virtual BOOL
	Initialize( 
		CFeelDevice* pDevice, 
		const FEEL_EFFECT &effect
		);

	// Use this form for single-axis and dual-axis effects
    BOOL 
	InitCondition( 
        CFeelDevice* pDevice, 
        LPCFEELIT_CONDITION pConditionX,
        LPCFEELIT_CONDITION pConditionY,
        BOOL bUseDeviceCoordinates = FALSE
		);


    // Use this form for directional effects
    BOOL 
	InitCondition( 
        CFeelDevice* pDevice, 
        LPCFEELIT_CONDITION pCondition,
        LONG lDirectionX,
        LONG lDirectionY,
        BOOL bUseDeviceCoordinates = FALSE
		);


    // Use this form for directional effects
    BOOL 
    InitConditionPolar( 
        CFeelDevice* pDevice, 
        LPCFEELIT_CONDITION pCondition,
        LONG lAngle,
        BOOL bUseDeviceCoordinates = FALSE
		);


    // Use this form for single-axis, dual-axis symetrical, or directional effects
    BOOL 
    InitCondition( 
        CFeelDevice* pDevice, 
        LONG lPositiveCoefficient = FEEL_CONDITION_DEFAULT_COEFFICIENT,
        LONG lNegativeCoefficient = FEEL_CONDITION_DEFAULT_COEFFICIENT,
        DWORD dwPositiveSaturation = FEEL_CONDITION_DEFAULT_SATURATION,
        DWORD dwNegativeSaturation = FEEL_CONDITION_DEFAULT_SATURATION,
        LONG lDeadBand = FEEL_CONDITION_DEFAULT_DEADBAND,
        DWORD dwfAxis = FEEL_EFFECT_AXIS_BOTH,
        POINT pntCenter = FEEL_CONDITION_DEFAULT_CENTER_POINT,
        LONG lDirectionX = FEEL_EFFECT_DEFAULT_DIRECTION_X,
        LONG lDirectionY = FEEL_EFFECT_DEFAULT_DIRECTION_Y,
        BOOL bUseDeviceCoordinates = FALSE
		);

    // Use this form for directional effects
    BOOL 
    InitConditionPolar( 
        CFeelDevice* pDevice, 
        LONG lPositiveCoefficient = FEEL_CONDITION_DEFAULT_COEFFICIENT,
        LONG lNegativeCoefficient = FEEL_CONDITION_DEFAULT_COEFFICIENT,
        DWORD dwPositiveSaturation = FEEL_CONDITION_DEFAULT_SATURATION,
        DWORD dwNegativeSaturation = FEEL_CONDITION_DEFAULT_SATURATION,
        LONG lDeadBand = FEEL_CONDITION_DEFAULT_DEADBAND,
        POINT pntCenter = FEEL_CONDITION_DEFAULT_CENTER_POINT,
        LONG lAngle = FEEL_EFFECT_DEFAULT_ANGLE,
        BOOL bUseDeviceCoordinates = FALSE
		);


    virtual BOOL
#ifdef FFC_START_DELAY
    StartNow(
#else
    Start(
#endif
        DWORD dwIterations = 1,
        DWORD dwFlags = 0
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
        LPCFEELIT_CONDITION pConditionX, 
        LPCFEELIT_CONDITION pConditionY
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

    //
    // INTERNAL DATA
    //

    FEEL_CONDITION m_aCondition[2];
    DWORD m_dwfAxis;
    BOOL m_bUseMousePosAtStart;
    
    protected:
	    BOOL m_bUseDeviceCoordinates;

};


//
// INLINES
//

inline BOOL
CFeelCondition::GetIsCompatibleGUID(GUID &guid)
{
	return  IsEqualGUID(guid, GUID_Feel_Spring) ||
			IsEqualGUID(guid, GUID_Feel_DeviceSpring) ||
			IsEqualGUID(guid, GUID_Feel_Damper) ||
			IsEqualGUID(guid, GUID_Feel_Inertia) ||
			IsEqualGUID(guid, GUID_Feel_Friction) ||
			IsEqualGUID(guid, GUID_Feel_Texture) ||
			IsEqualGUID(guid, GUID_Feel_Grid);
}

#endif // !defined(AFX_FEELCONDITION_H__135B88C4_4175_11D1_B049_0020AF30269A__INCLUDED_)
