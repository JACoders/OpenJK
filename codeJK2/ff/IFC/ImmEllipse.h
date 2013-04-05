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

  FILE:		ImmEllipse.h

  PURPOSE:	Base Ellipse Class for Immersion Foundation Classes

  STARTED:	10/29/97

  NOTES/REVISIONS:
     3/2/99 jrm (Jeff Mallett): Force-->Feel renaming
	 3/2/99 jrm: Added GetIsCompatibleGUID
	 3/15/99 jrm: __declspec(dllimport/dllexport) the whole class
	 11/15/99 sdr (Steve Rank): Converted to IFC

**********************************************************************/


#if !defined(AFX_IMMELLIPSE_H__135B88C4_4175_11D1_B049_0020AF30269A__INCLUDED_)
#define AFX_IMMELLIPSE_H__135B88C4_4175_11D1_B049_0020AF30269A__INCLUDED_

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

#define     IMM_ELLIPSE_DEFAULT_STIFFNESS      5000
#define     IMM_ELLIPSE_DEFAULT_SATURATION     10000
#define     IMM_ELLIPSE_DEFAULT_WIDTH          10
#define     IMM_ELLIPSE_HEIGHT_AUTO            MAXDWORD
#define     IMM_ELLIPSE_DEFAULT_HEIGHT         IMM_ELLIPSE_HEIGHT_AUTO
#define     IMM_ELLIPSE_WALL_WIDTH_AUTO        MAXDWORD
#define     IMM_ELLIPSE_DEFAULT_WALL_WIDTH     IMM_ELLIPSE_WALL_WIDTH_AUTO
#define     IMM_ELLIPSE_DEFAULT_STIFFNESS_MASK IMM_STIFF_ANYWALL
#define     IMM_ELLIPSE_DEFAULT_CLIPPING_MASK  IMM_CLIP_NONE

#define     IMM_ELLIPSE_DEFAULT_CENTER_POINT   IMM_EFFECT_MOUSE_POS_AT_START





//================================================================
// CImmEllipse
//================================================================

//
// ------ PUBLIC INTERFACE ------ 
//

class DLLIFC CImmEllipse : public CImmEffect
{
    //
    // CONSTRUCTOR/DESCTRUCTOR
    //

    public:
    
    // Constructor
	CImmEllipse();

	// Destructor
	virtual
	~CImmEllipse();


    //
    // ATTRIBUTES
    //

    public:

	virtual BOOL
	GetIsCompatibleGUID(
		GUID &guid
		);

	virtual DWORD GetEffectType()
		{ return IMM_EFFECTTYPE_ELLIPSE; }

    BOOL
    ChangeParameters( 
        POINT pntCenter,
        DWORD dwWidth = IMM_EFFECT_DONT_CHANGE,
        DWORD dwHeight = IMM_EFFECT_DONT_CHANGE,
        LONG lStiffness = IMM_EFFECT_DONT_CHANGE,
        DWORD dwWallThickness = IMM_EFFECT_DONT_CHANGE,
        DWORD dwSaturation = IMM_EFFECT_DONT_CHANGE,
        DWORD dwStiffnessMask = IMM_EFFECT_DONT_CHANGE,
        DWORD dwClippingMask = IMM_EFFECT_DONT_CHANGE,
        CImmEffect* pInsideEffect = (CImmEffect*) IMM_EFFECT_DONT_CHANGE,
		LONG lAngle = IMM_EFFECT_DONT_CHANGE
        );

    BOOL
    ChangeParameters( 
        LPCRECT pRectOutside,
        LONG lStiffness = IMM_EFFECT_DONT_CHANGE,
        DWORD dwWallThickness = IMM_EFFECT_DONT_CHANGE,
        DWORD dwSaturation = IMM_EFFECT_DONT_CHANGE,
        DWORD dwStiffnessMask = IMM_EFFECT_DONT_CHANGE,
        DWORD dwClippingMask = IMM_EFFECT_DONT_CHANGE,
        CImmEffect* pInsideEffect = (CImmEffect*) IMM_EFFECT_DONT_CHANGE,  
		LONG lAngle = IMM_EFFECT_DONT_CHANGE
        );

	
	BOOL ChangeStiffness	( LONG lStiffness );
	BOOL ChangeWallThickness( DWORD dwThickness	);
	BOOL ChangeSaturation	( DWORD dwSaturation );
	BOOL ChangeStiffnessMask( DWORD dwStiffnessMask );
	BOOL ChangeClippingMask	( DWORD dwClippingMask );
	BOOL ChangeInsideEffect	( CImmEffect* pInsideEffect );

    BOOL
    ChangeRect( 
        LPCRECT pRect 
        );


    BOOL
    ChangeCenter( 
        POINT pntCenter 
        );


    BOOL
    ChangeCenter( 
        LONG x, 
        LONG y 
        );

	BOOL GetStiffness	( LONG &lStiffness );
	BOOL GetWallThickness( DWORD &dwThickness	);
	BOOL GetSaturation	( DWORD &dwSaturation );
	BOOL GetStiffnessMask( DWORD &dwStiffnessMask );
	BOOL GetClippingMask	( DWORD &dwClippingMask );

	BOOL GetRect( RECT* pRect );
	BOOL GetCenter( POINT &pntCenter );
	BOOL GetCenter( LONG &x, LONG &y);

	CImmEffect* GetInsideEffect();

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

    BOOL 
    Initialize( 
        CImmDevice* pDevice, 
        DWORD dwWidth = IMM_ELLIPSE_DEFAULT_WIDTH,
        DWORD dwHeight = IMM_ELLIPSE_DEFAULT_HEIGHT,
        LONG lStiffness = IMM_ELLIPSE_DEFAULT_STIFFNESS,
        DWORD dwWallWidth = IMM_ELLIPSE_DEFAULT_WALL_WIDTH,
        DWORD dwSaturation = IMM_ELLIPSE_DEFAULT_SATURATION,
        DWORD dwStiffnessMask = IMM_ELLIPSE_DEFAULT_STIFFNESS_MASK,
        DWORD dwClippingMask = IMM_ELLIPSE_DEFAULT_CLIPPING_MASK,
        POINT pntCenter = IMM_ELLIPSE_DEFAULT_CENTER_POINT,
        CImmEffect* pInsideEffect = NULL,  
		LONG lAngle = IMM_EFFECT_DEFAULT_ANGLE,
		DWORD dwNoDownload = 0
        );


    BOOL 
    Initialize( 
        CImmDevice* pDevice, 
        LPCRECT pRectOutside,
        LONG lStiffness = IMM_ELLIPSE_DEFAULT_STIFFNESS,
        DWORD dwWallWidth = IMM_ELLIPSE_DEFAULT_WALL_WIDTH,
        DWORD dwSaturation = IMM_ELLIPSE_DEFAULT_SATURATION,
        DWORD dwStiffnessMask = IMM_ELLIPSE_DEFAULT_STIFFNESS_MASK,
        DWORD dwClippingMask = IMM_ELLIPSE_DEFAULT_CLIPPING_MASK,
        CImmEffect* pInsideEffect = NULL,  
 		LONG lAngle = IMM_EFFECT_DEFAULT_ANGLE,
		DWORD dwNoDownload = 0
        );


    virtual BOOL 
	Start(
        DWORD dwIterations = 1,
        DWORD dwFlags = 0, 
		BOOL bAllowStartDelayEmulation = true
		);

	HRESULT Unload();
	void Reload();

        
//
// ------ PRIVATE INTERFACE ------ 
//

    //
    // HELPERS
    //

    protected:

    BOOL
        set_parameters( 
        LPCRECT pRectOutside,
        LONG lStiffness,
        DWORD dwWallWidth,
        DWORD dwSaturation,
        DWORD dwStiffnessMask,
        DWORD dwClippingMask,
        CImmEffect* pInsideEffect,
		LONG lAngle
        );
	
	DWORD
	change_parameters( 
		LPCRECT prectBoundary,
		LONG lStiffness,
		DWORD dwWallThickness,
		DWORD dwSaturation,
		DWORD dwStiffnessMask,
		DWORD dwClippingMask,
		CImmEffect* pInsideEffect,
		LONG lAngle
		);
	
	int
	buffer_ifr_data(
		TCHAR* pData
		);

    //
    // INTERNAL DATA
    //
    
    protected:

    IMM_ELLIPSE m_ellipse;
    BOOL m_bUseMousePosAtStart;

	// Needed for co-ordinating events for Enclosures/Ellipes and the inside effects.
	CImmEffect*	m_pInsideEffect;
};



//
// INLINES
//

inline BOOL
CImmEllipse::GetIsCompatibleGUID(GUID &guid)
{
	return IsEqualGUID(guid, GUID_Imm_Ellipse);
}

#endif // !defined(AFX_IMMELLIPSE_H__135B88C4_4175_11D1_B049_0020AF30269A__INCLUDED_)
