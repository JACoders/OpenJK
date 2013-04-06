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

  FILE:		ImmTexture.h

  PURPOSE:	Texture Class for Feelit API Foundation Classes

  STARTED:	2/27/98

  NOTES/REVISIONS:
     3/2/99 jrm (Jeff Mallett): Force-->Feel renaming
	 3/2/99 jrm: Added GetIsCompatibleGUID
	 3/15/99 jrm: __declspec(dllimport/dllexport) the whole class

**********************************************************************/


#if !defined(AFX_ImmTexture_H__135B88C4_4175_11D1_B049_0020AF30269A__INCLUDED_)
#define AFX_ImmTexture_H__135B88C4_4175_11D1_B049_0020AF30269A__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef _IFCDLL_
#define DLLIFC __declspec(dllimport)
#else
#define DLLIFC __declspec(dllexport)
#endif

#include <windows.h>
#include "ImmBaseTypes.h"
#include "ImmEffect.h"



//================================================================
// Constants
//================================================================

const POINT IMM_TEXTURE_PT_NULL = { 0, 0 };
const POINT IMM_TEXTURE_DEFAULT_OFFSET_POINT = { 0, 0};

#define     IMM_TEXTURE_DEFAULT_MAGNITUDE        5000
#define     IMM_TEXTURE_DEFAULT_WIDTH		       10
#define     IMM_TEXTURE_DEFAULT_SPACING		   20


//================================================================
// CImmTexture
//================================================================

//
// ------ PUBLIC INTERFACE ------ 
//
        
class DLLIFC CImmTexture : public CImmEffect
{
    //
    // CONSTRUCTOR/DESTRUCTOR
    //

    public:
    
    // Constructor
	CImmTexture();

	// Destructor
	virtual
	~CImmTexture();


    //
    // ATTRIBUTES
    //

    public:

	virtual BOOL
	GetIsCompatibleGUID(
		GUID &guid
		);

	virtual DWORD GetEffectType()
		{ return IMM_EFFECTTYPE_TEXTURE; }

    // Use this form for single-axis and dual-axis effects
    BOOL
    ChangeTextureParams( 
        LPCIMM_TEXTURE pTextureX,
        LPCIMM_TEXTURE pTextureY
        );

    // Use this form for directional effects
    BOOL
    ChangeTextureParams( 
        LPCIMM_TEXTURE pTexture,
        LONG lDirectionX,
        LONG lDirectionY
        );

    // Use this form for directional effects
	BOOL
    ChangeTextureParamsPolar( 
        LPCIMM_TEXTURE pTexture,
        LONG lAngle
        );

    // Use this form for single-axis, dual-axis symetrical, or directional effects
    BOOL
    ChangeTextureParams( 
        LONG lPosBumpMag = IMM_EFFECT_DONT_CHANGE,
        DWORD dwPosBumpWidth = IMM_EFFECT_DONT_CHANGE,
        DWORD dwPosBumpSpacing = IMM_EFFECT_DONT_CHANGE,
        LONG lNegBumpMag = IMM_EFFECT_DONT_CHANGE,
        DWORD dwNegBumpWidth = IMM_EFFECT_DONT_CHANGE,
        DWORD dwNegBumpSpacing = IMM_EFFECT_DONT_CHANGE,
        POINT pntOffset = IMM_EFFECT_DONT_CHANGE_POINT,
        LONG lDirectionX = IMM_EFFECT_DONT_CHANGE,
        LONG lDirectionY = IMM_EFFECT_DONT_CHANGE
        );

    // Use this form for single-axis, dual-axis, or directional effects
    BOOL
    ChangeTextureParamsX( 
        LONG lPosBumpMag = IMM_EFFECT_DONT_CHANGE,
        DWORD dwPosBumpWidth = IMM_EFFECT_DONT_CHANGE,
        DWORD dwPosBumpSpacing = IMM_EFFECT_DONT_CHANGE,
        LONG lNegBumpMag = IMM_EFFECT_DONT_CHANGE,
        DWORD dwNegBumpWidth = IMM_EFFECT_DONT_CHANGE,
        DWORD dwNegBumpSpacing = IMM_EFFECT_DONT_CHANGE,
        POINT pntOffset = IMM_EFFECT_DONT_CHANGE_POINT,
        LONG lDirectionX = IMM_EFFECT_DONT_CHANGE,
        LONG lDirectionY = IMM_EFFECT_DONT_CHANGE
        );

    BOOL
    ChangeTextureParamsY( 
        LONG lPosBumpMag = IMM_EFFECT_DONT_CHANGE,
        DWORD dwPosBumpWidth = IMM_EFFECT_DONT_CHANGE,
        DWORD dwPosBumpSpacing = IMM_EFFECT_DONT_CHANGE,
        LONG lNegBumpMag = IMM_EFFECT_DONT_CHANGE,
        DWORD dwNegBumpWidth = IMM_EFFECT_DONT_CHANGE,
        DWORD dwNegBumpSpacing = IMM_EFFECT_DONT_CHANGE,
        POINT pntOffset = IMM_EFFECT_DONT_CHANGE_POINT,
        LONG lDirectionX = IMM_EFFECT_DONT_CHANGE,
        LONG lDirectionY = IMM_EFFECT_DONT_CHANGE
        );

    // Use this form for single-axis, dual-axis symetrical, or directional effects
    BOOL
    ChangeTextureParamsPolar( 
        LONG lPosBumpMag = IMM_EFFECT_DONT_CHANGE,
        DWORD dwPosBumpWidth = IMM_EFFECT_DONT_CHANGE,
        DWORD dwPosBumpSpacing = IMM_EFFECT_DONT_CHANGE,
        LONG lNegBumpMag = IMM_EFFECT_DONT_CHANGE,
        DWORD dwNegBumpWidth = IMM_EFFECT_DONT_CHANGE,
        DWORD dwNegBumpSpacing = IMM_EFFECT_DONT_CHANGE,
        POINT pntOffset = IMM_EFFECT_DONT_CHANGE_POINT,
        LONG lAngle = IMM_EFFECT_DONT_CHANGE
        );

    // Use this form for single-axis, dual-axis, or directional effects
    BOOL
    ChangeTextureParamsPolarX( 
        LONG lPosBumpMag = IMM_EFFECT_DONT_CHANGE,
        DWORD dwPosBumpWidth = IMM_EFFECT_DONT_CHANGE,
        DWORD dwPosBumpSpacing = IMM_EFFECT_DONT_CHANGE,
        LONG lNegBumpMag = IMM_EFFECT_DONT_CHANGE,
        DWORD dwNegBumpWidth = IMM_EFFECT_DONT_CHANGE,
        DWORD dwNegBumpSpacing = IMM_EFFECT_DONT_CHANGE,
        POINT pntOffset = IMM_EFFECT_DONT_CHANGE_POINT,
        LONG lAngle = IMM_EFFECT_DONT_CHANGE
        );

    BOOL
    ChangeTextureParamsPolarY( 
        LONG lPosBumpMag = IMM_EFFECT_DONT_CHANGE,
        DWORD dwPosBumpWidth = IMM_EFFECT_DONT_CHANGE,
        DWORD dwPosBumpSpacing = IMM_EFFECT_DONT_CHANGE,
        LONG lNegBumpMag = IMM_EFFECT_DONT_CHANGE,
        DWORD dwNegBumpWidth = IMM_EFFECT_DONT_CHANGE,
        DWORD dwNegBumpSpacing = IMM_EFFECT_DONT_CHANGE,
        POINT pntOffset = IMM_EFFECT_DONT_CHANGE_POINT,
        LONG lAngle = IMM_EFFECT_DONT_CHANGE
        );

    // Use these to change the the X Axis parameters for a dual-axis effect 
	BOOL ChangePositiveBumpMagX( LONG lPosBumpMag );
	BOOL ChangeNegativeBumpMagX( LONG lNegBumpMag );
	BOOL ChangePositiveBumpSpacingX( DWORD dwPosBumpSpacing );
	BOOL ChangeNegativeBumpSpacingX( DWORD dwNegBumpSpacing );
	BOOL ChangePositiveBumpWidthX( DWORD dwPosBumpWidth );
	BOOL ChangeNegativeBumpWidthX( DWORD dwNegBumpWidth );

    // Use these to change the the Y Axis parameters for a dual-axis effect 
	BOOL ChangePositiveBumpMagY( LONG lPosBumpMag );
	BOOL ChangeNegativeBumpMagY( LONG lNegBumpMag );
	BOOL ChangePositiveBumpSpacingY( DWORD dwPosBumpSpacing );
	BOOL ChangeNegativeBumpSpacingY( DWORD dwNegBumpSpacing );
	BOOL ChangePositiveBumpWidthY( DWORD dwPosBumpWidth );
	BOOL ChangeNegativeBumpWidthY( DWORD dwNegBumpWidth );

    // Use these to change the the parameters for a single-axis or 
	// dual-axis symetrical effect 
	BOOL ChangePositiveBumpMag( LONG lPosBumpMag );
	BOOL ChangeNegativeBumpMag( LONG lNegBumpMag );
	BOOL ChangePositiveBumpSpacing( DWORD dwPosBumpSpacing );
	BOOL ChangeNegativeBumpSpacing( DWORD dwNegBumpSpacing );
	BOOL ChangePositiveBumpWidth( DWORD dwPosBumpWidth );
	BOOL ChangeNegativeBumpWidth( DWORD dwNegBumpWidth );

	BOOL ChangeOffset( POINT pntOffset );

	BOOL GetPositiveBumpMagX( LONG &lPosBumpMag );
	BOOL GetNegativeBumpMagX( LONG &lNegBumpMag );
	BOOL GetPositiveBumpSpacingX( DWORD &dwPosBumpSpacing );
	BOOL GetNegativeBumpSpacingX( DWORD &dwNegBumpSpacing );
	BOOL GetPositiveBumpWidthX( DWORD &dwPosBumpWidth );
	BOOL GetNegativeBumpWidthX( DWORD &dwNegBumpWidth );
	BOOL GetPositiveBumpMagY( LONG &lPosBumpMag );
	BOOL GetNegativeBumpMagY( LONG &lNegBumpMag );
	BOOL GetPositiveBumpSpacingY( DWORD &dwPosBumpSpacing );
	BOOL GetNegativeBumpSpacingY( DWORD &dwNegBumpSpacing );
	BOOL GetPositiveBumpWidthY( DWORD &dwPosBumpWidth );
	BOOL GetNegativeBumpWidthY( DWORD &dwNegBumpWidth );
	BOOL GetOffset( POINT &pntOffset );

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
    InitTexture( 
        CImmDevice* pDevice, 
        LPCIMM_TEXTURE pTextureX,
        LPCIMM_TEXTURE pTextureY,
		DWORD dwNoDownload = 0
        );


    // Use this form for directional effects
    BOOL 
    InitTexture( 
        CImmDevice* pDevice, 
        LPCIMM_TEXTURE pTexture,
        LONG lDirectionX,
        LONG lDirectionY,
		DWORD dwNoDownload = 0
        );


    // Use this form for directional effects
    BOOL 
    InitTexturePolar( 
        CImmDevice* pDevice, 
        LPCIMM_TEXTURE pTexture,
        LONG lAngle,
		DWORD dwNoDownload = 0
        );


    // Use this form for single-axis, dual-axis symetrical, or directional effects
    BOOL 
    InitTexture( 
        CImmDevice* pDevice, 
        LONG lPosBumpMag = IMM_TEXTURE_DEFAULT_MAGNITUDE,
        DWORD dwPosBumpWidth = IMM_TEXTURE_DEFAULT_WIDTH,
        DWORD dwPosBumpSpacing = IMM_TEXTURE_DEFAULT_SPACING,
        LONG lNegBumpMag = IMM_TEXTURE_DEFAULT_MAGNITUDE,
        DWORD dwNegBumpWidth = IMM_TEXTURE_DEFAULT_WIDTH,
        DWORD dwNegBumpSpacing = IMM_TEXTURE_DEFAULT_SPACING,
        DWORD dwfAxis = IMM_EFFECT_AXIS_BOTH,
        POINT pntOffset = IMM_TEXTURE_DEFAULT_OFFSET_POINT,
        LONG lDirectionX = IMM_EFFECT_DEFAULT_DIRECTION_X,
        LONG lDirectionY = IMM_EFFECT_DEFAULT_DIRECTION_Y,
		DWORD dwNoDownload = 0
        );

    // Use this form for directional effects
    BOOL 
    InitTexturePolar( 
        CImmDevice* pDevice, 
        LONG lPosBumpMag = IMM_TEXTURE_DEFAULT_MAGNITUDE,
        DWORD dwPosBumpWidth = IMM_TEXTURE_DEFAULT_WIDTH,
        DWORD dwPosBumpSpacing = IMM_TEXTURE_DEFAULT_SPACING,
        LONG lNegBumpMag = IMM_TEXTURE_DEFAULT_MAGNITUDE,
        DWORD dwNegBumpWidth = IMM_TEXTURE_DEFAULT_WIDTH,
        DWORD dwNegBumpSpacing = IMM_TEXTURE_DEFAULT_SPACING,
        POINT pntOffset = IMM_TEXTURE_DEFAULT_OFFSET_POINT,
        LONG lAngle = IMM_EFFECT_DEFAULT_ANGLE,
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
        DWORD dwfAxis,
        DWORD dwfCoordinates,
        LONG lDirection0,
        LONG lDirection1,
        LPCIMM_TEXTURE pTextureX,
        LPCIMM_TEXTURE pTextureY
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

	DWORD
	change_parameters( 
		LONG lDirection0,
		LONG lDirection1,
		LPCIMM_TEXTURE pTextureX, 
		LPCIMM_TEXTURE pTextureY
	    );

	DWORD
	change_parameters( 
		LONG lDirection0,
		LONG lDirection1,
		LONG lPosBumpMag,
		DWORD dwPosBumpWidth,
		DWORD dwPosBumpSpacing,
		LONG lNegBumpMag,
		DWORD dwNegBumpWidth,
		DWORD dwNegBumpSpacing,
		POINT pntOffset,
		int fAxis
		);
	
	int
	buffer_ifr_data(
		TCHAR* pData
		);

    //
    // INTERNAL DATA
    //

    IMM_TEXTURE m_aTexture[2];
    DWORD m_dwfAxis;
    
    protected:

};



//
// INLINES
//

inline BOOL
CImmTexture::GetIsCompatibleGUID(GUID &guid)
{
	return IsEqualGUID(guid, GUID_Imm_Texture);
}

#endif // !defined(AFX_ImmTexture_H__135B88C4_4175_11D1_B049_0020AF30269A__INCLUDED_)
