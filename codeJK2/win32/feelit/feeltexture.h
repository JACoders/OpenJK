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

  FILE:		FeelTexture.h

  PURPOSE:	Texture Class for Feelit API Foundation Classes

  STARTED:	2/27/98

  NOTES/REVISIONS:
     3/2/99 jrm (Jeff Mallett): Force-->Feel renaming
	 3/2/99 jrm: Added GetIsCompatibleGUID
	 3/15/99 jrm: __declspec(dllimport/dllexport) the whole class

**********************************************************************/


#if !defined(AFX_FeelTexture_H__135B88C4_4175_11D1_B049_0020AF30269A__INCLUDED_)
#define AFX_FeelTexture_H__135B88C4_4175_11D1_B049_0020AF30269A__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef _FFCDLL_
#define DLLFFC __declspec(dllimport)
#else
#define DLLFFC __declspec(dllexport)
#endif

#include <windows.h>
#include "FeelBaseTypes.h"
#include "FeelEffect.h"



//================================================================
// Constants
//================================================================

const POINT FEEL_TEXTURE_PT_NULL = { 0, 0 };
const POINT FEEL_TEXTURE_DEFAULT_OFFSET_POINT = { 0, 0};

#define     FEEL_TEXTURE_DEFAULT_MAGNITUDE        5000
#define     FEEL_TEXTURE_DEFAULT_WIDTH		       10
#define     FEEL_TEXTURE_DEFAULT_SPACING		   20

//
// FORCE --> FEEL Wrappers
//
#define     FORCE_TEXTURE_PT_NULL				FEEL_TEXTURE_PT_NULL
#define     FORCE_TEXTURE_DEFAULT_OFFSET_POINT	FEEL_TEXTURE_DEFAULT_OFFSET_POINT

#define     FORCE_TEXTURE_DEFAULT_MAGNITUDE		FEEL_TEXTURE_DEFAULT_MAGNITUDE
#define     FORCE_TEXTURE_DEFAULT_WIDTH			FEEL_TEXTURE_DEFAULT_WIDTH
#define     FORCE_TEXTURE_DEFAULT_SPACING		FEEL_TEXTURE_DEFAULT_SPACING


//================================================================
// CFeelTexture
//================================================================

//
// ------ PUBLIC INTERFACE ------ 
//
        
class DLLFFC CFeelTexture : public CFeelEffect
{
    //
    // CONSTRUCTOR/DESTRUCTOR
    //

    public:
    
    // Constructor
	CFeelTexture();

	// Destructor
	virtual
	~CFeelTexture();


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
    ChangeTextureParams( 
        LPCFEELIT_TEXTURE pTextureX,
        LPCFEELIT_TEXTURE pTextureY
        );

    // Use this form for directional effects
    BOOL
    ChangeTextureParams( 
        LPCFEELIT_TEXTURE pTexture,
        LONG lDirectionX,
        LONG lDirectionY
        );

    // Use this form for directional effects
	BOOL
    ChangeTextureParamsPolar( 
        LPCFEELIT_TEXTURE pTexture,
        LONG lAngle
        );

    // Use this form for single-axis, dual-axis symetrical, or directional effects
    BOOL
    ChangeTextureParams( 
        LONG lPosBumpMag = FEEL_EFFECT_DONT_CHANGE,
        DWORD dwPosBumpWidth = FEEL_EFFECT_DONT_CHANGE,
        DWORD dwPosBumpSpacing = FEEL_EFFECT_DONT_CHANGE,
        LONG lNegBumpMag = FEEL_EFFECT_DONT_CHANGE,
        DWORD dwNegBumpWidth = FEEL_EFFECT_DONT_CHANGE,
        DWORD dwNegBumpSpacing = FEEL_EFFECT_DONT_CHANGE,
        POINT pntOffset = FEEL_EFFECT_DONT_CHANGE_POINT,
        LONG lDirectionX = FEEL_EFFECT_DONT_CHANGE,
        LONG lDirectionY = FEEL_EFFECT_DONT_CHANGE
        );

    // Use this form for single-axis, dual-axis symetrical, or directional effects
    BOOL
    ChangeTextureParamsPolar( 
        LONG lPosBumpMag = FEEL_EFFECT_DONT_CHANGE,
        DWORD dwPosBumpWidth = FEEL_EFFECT_DONT_CHANGE,
        DWORD dwPosBumpSpacing = FEEL_EFFECT_DONT_CHANGE,
        LONG lNegBumpMag = FEEL_EFFECT_DONT_CHANGE,
        DWORD dwNegBumpWidth = FEEL_EFFECT_DONT_CHANGE,
        DWORD dwNegBumpSpacing = FEEL_EFFECT_DONT_CHANGE,
        POINT pntOffset = FEEL_EFFECT_DONT_CHANGE_POINT,
        LONG lAngle = FEEL_EFFECT_DONT_CHANGE
        );

    BOOL
    SetOffset( 
        POINT pntOffset 
        );

    //
    // OPERATIONS
    //

    public:

    // Use this form for single-axis and dual-axis effects
    BOOL 
    InitTexture( 
        CFeelDevice* pDevice, 
        LPCFEELIT_TEXTURE pTextureX,
        LPCFEELIT_TEXTURE pTextureY
        );


    // Use this form for directional effects
    BOOL 
    InitTexture( 
        CFeelDevice* pDevice, 
        LPCFEELIT_TEXTURE pTexture,
        LONG lDirectionX,
        LONG lDirectionY
        );


    // Use this form for directional effects
    BOOL 
    InitTexturePolar( 
        CFeelDevice* pDevice, 
        LPCFEELIT_TEXTURE pTexture,
        LONG lAngle
        );


    // Use this form for single-axis, dual-axis symetrical, or directional effects
    BOOL 
    InitTexture( 
        CFeelDevice* pDevice, 
        LONG lPosBumpMag = FEEL_TEXTURE_DEFAULT_MAGNITUDE,
        DWORD dwPosBumpWidth = FEEL_TEXTURE_DEFAULT_WIDTH,
        DWORD dwPosBumpSpacing = FEEL_TEXTURE_DEFAULT_SPACING,
        LONG lNegBumpMag = FEEL_TEXTURE_DEFAULT_MAGNITUDE,
        DWORD dwNegBumpWidth = FEEL_TEXTURE_DEFAULT_WIDTH,
        DWORD dwNegBumpSpacing = FEEL_TEXTURE_DEFAULT_SPACING,
        DWORD dwfAxis = FEEL_EFFECT_AXIS_BOTH,
        POINT pntOffset = FEEL_TEXTURE_DEFAULT_OFFSET_POINT,
        LONG lDirectionX = FEEL_EFFECT_DEFAULT_DIRECTION_X,
        LONG lDirectionY = FEEL_EFFECT_DEFAULT_DIRECTION_Y
        );

    // Use this form for directional effects
    BOOL 
    InitTexturePolar( 
        CFeelDevice* pDevice, 
        LONG lPosBumpMag = FEEL_TEXTURE_DEFAULT_MAGNITUDE,
        DWORD dwPosBumpWidth = FEEL_TEXTURE_DEFAULT_WIDTH,
        DWORD dwPosBumpSpacing = FEEL_TEXTURE_DEFAULT_SPACING,
        LONG lNegBumpMag = FEEL_TEXTURE_DEFAULT_MAGNITUDE,
        DWORD dwNegBumpWidth = FEEL_TEXTURE_DEFAULT_WIDTH,
        DWORD dwNegBumpSpacing = FEEL_TEXTURE_DEFAULT_SPACING,
        POINT pntOffset = FEEL_TEXTURE_DEFAULT_OFFSET_POINT,
        LONG lAngle = FEEL_EFFECT_DEFAULT_ANGLE
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
        DWORD dwfAxis,
        DWORD dwfCoordinates,
        LONG lDirection0,
        LONG lDirection1,
        LPCFEELIT_TEXTURE pTextureX,
        LPCFEELIT_TEXTURE pTextureY
        );

    BOOL
    set_parameters( 
        DWORD dwfAxis,
        DWORD dwfCoordinates,
        LONG lDirection0,
        LONG lDirection1,
        LONG lPosBumpMag,
        DWORD dwPosBumpWidth,
        DWORD dwPosBumpSpacing,
        LONG lNegBumpMag,
        DWORD dwNegBumpWidth,
        DWORD dwNegBumpSpacing,
        POINT pntOffset
        );

    //
    // INTERNAL DATA
    //

    FEEL_TEXTURE m_aTexture[2];
    DWORD m_dwfAxis;
    
    protected:

};



//
// INLINES
//

inline BOOL
CFeelTexture::GetIsCompatibleGUID(GUID &guid)
{
	return IsEqualGUID(guid, GUID_Feel_Texture);
}

#endif // !defined(AFX_FeelTexture_H__135B88C4_4175_11D1_B049_0020AF30269A__INCLUDED_)
