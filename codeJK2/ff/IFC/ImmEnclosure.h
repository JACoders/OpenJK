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

  FILE:		ImmEnclosure.h

  PURPOSE:	Base Enclosure Class for Immersion Foundation Classes

  STARTED:	10/29/97

  NOTES/REVISIONS:
     3/2/99 jrm (Jeff Mallett): Force-->Feel renaming
	 3/2/99 jrm: Added GetIsCompatibleGUID
	 3/15/99 jrm: __declspec(dllimport/dllexport) the whole class
	 11/15/99 sdr (Steve Rank): Converted to IFC

**********************************************************************/


#if !defined(AFX_IMMENCLOSURE_H__135B88C4_4175_11D1_B049_0020AF30269A__INCLUDED_)
#define AFX_IMMENCLOSURE_H__135B88C4_4175_11D1_B049_0020AF30269A__INCLUDED_

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


#define     IMM_ENCLOSURE_DEFAULT_STIFFNESS      5000
#define     IMM_ENCLOSURE_DEFAULT_SATURATION     10000
#define     IMM_ENCLOSURE_DEFAULT_WIDTH          10
#define     IMM_ENCLOSURE_HEIGHT_AUTO            MAXDWORD
#define     IMM_ENCLOSURE_DEFAULT_HEIGHT         IMM_ENCLOSURE_HEIGHT_AUTO
#define     IMM_ENCLOSURE_WALL_WIDTH_AUTO        MAXDWORD
#define     IMM_ENCLOSURE_DEFAULT_WALL_WIDTH     IMM_ENCLOSURE_WALL_WIDTH_AUTO
#define     IMM_ENCLOSURE_DEFAULT_STIFFNESS_MASK IMM_STIFF_ANYWALL
#define     IMM_ENCLOSURE_DEFAULT_CLIPPING_MASK  IMM_CLIP_NONE

#define     IMM_ENCLOSURE_DEFAULT_CENTER_POINT   IMM_EFFECT_MOUSE_POS_AT_START





//================================================================
// CImmEnclosure
//================================================================

//
// ------ PUBLIC INTERFACE ------ 
//

class DLLIFC CImmEnclosure : public CImmEffect
{
    //
    // CONSTRUCTOR/DESCTRUCTOR
    //

    public:
    
    // Constructor
	CImmEnclosure();

	// Destructor
	virtual
	~CImmEnclosure();


    //
    // ATTRIBUTES
    //

    public:

	virtual BOOL
	GetIsCompatibleGUID(
		GUID &guid
		);

	virtual DWORD GetEffectType()
		{ return IMM_EFFECTTYPE_ENCLOSURE; }

    BOOL
    ChangeParameters( 
        POINT pntCenter,
        DWORD dwWidth = IMM_EFFECT_DONT_CHANGE,
        DWORD dwHeight = IMM_EFFECT_DONT_CHANGE,
        LONG lTopAndBottomWallStiffness = IMM_EFFECT_DONT_CHANGE,
        LONG lLeftAndRightWallStiffness = IMM_EFFECT_DONT_CHANGE,
        DWORD dwTopAndBottomWallWallWidth = IMM_EFFECT_DONT_CHANGE,
        DWORD dwLeftAndRightWallWallWidth = IMM_EFFECT_DONT_CHANGE,
        DWORD dwTopAndBottomWallSaturation = IMM_EFFECT_DONT_CHANGE,
        DWORD dwLeftAndRightWallSaturation = IMM_EFFECT_DONT_CHANGE,
        DWORD dwStiffnessMask = IMM_EFFECT_DONT_CHANGE,
        DWORD dwClippingMask = IMM_EFFECT_DONT_CHANGE,
        CImmEffect* pInsideEffect = (CImmEffect*) IMM_EFFECT_DONT_CHANGE,  
		LONG lAngle = IMM_EFFECT_DONT_CHANGE
        );

    BOOL
    ChangeParameters( 
        LPCRECT pRectOutside,
        LONG lTopAndBottomWallStiffness = IMM_EFFECT_DONT_CHANGE,
        LONG lLeftAndRightWallStiffness = IMM_EFFECT_DONT_CHANGE,
        DWORD dwTopAndBottomWallWallThickness = IMM_EFFECT_DONT_CHANGE,
        DWORD dwLeftAndRightWallWallThickness = IMM_EFFECT_DONT_CHANGE,
        DWORD dwTopAndBottomWallSaturation = IMM_EFFECT_DONT_CHANGE,
        DWORD dwLeftAndRightWallSaturation = IMM_EFFECT_DONT_CHANGE,
        DWORD dwStiffnessMask = IMM_EFFECT_DONT_CHANGE,
        DWORD dwClippingMask = IMM_EFFECT_DONT_CHANGE,
        CImmEffect* pInsideEffect = (CImmEffect*) IMM_EFFECT_DONT_CHANGE,  
		LONG lAngle = IMM_EFFECT_DONT_CHANGE
        );

	BOOL ChangeTopAndBottomWallStiffness ( LONG lTopAndBottomWallStiffness );
	BOOL ChangeLeftAndRightWallStiffness ( LONG lLeftAndRightWallStiffness );
	BOOL ChangeTopAndBottomWallThickness ( DWORD dwTopAndBottomWallThickness );
	BOOL ChangeLeftAndRightWallThickness ( DWORD dwLeftAndRightWallThickness );
	BOOL ChangeTopAndBottomWallSaturation( DWORD dwTopAndBottomWallSaturation );
	BOOL ChangeLeftAndRightWallSaturation( DWORD dwLeftAndRightWallSaturation );
	BOOL ChangeStiffnessMask			 ( DWORD dwStiffnessMask );
	BOOL ChangeClippingMask				 ( DWORD dwClippingMask );
	BOOL ChangeInsideEffect				 ( CImmEffect* pInsideEffect );

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

	BOOL
	ShowRect(
		BOOL bRectOn
		);

	BOOL GetTopAndBottomWallStiffness ( LONG &lTopAndBottomWallStiffness );
	BOOL GetLeftAndRightWallStiffness ( LONG &lLeftAndRightWallStiffness );
	BOOL GetTopAndBottomWallThickness ( DWORD &dwTopAndBottomWallThickness );
	BOOL GetLeftAndRightWallThickness ( DWORD &dwLeftAndRightWallThickness );
	BOOL GetTopAndBottomWallSaturation ( DWORD &dwTopAndBottomWallSaturation );
	BOOL GetLeftAndRightWallSaturation ( DWORD &dwLeftAndRightWallSaturation );
	BOOL GetStiffnessMask ( DWORD &dwStiffnessMask );
	BOOL GetClippingMask ( DWORD &dwClippingMask );
	
	BOOL GetRect( RECT* pRect );
	BOOL GetCenter( POINT &pntCenter );
	BOOL GetCenter( LONG &x, LONG &y);

	CImmEffect* GetInsideEffect ();

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
        DWORD dwWidth = IMM_ENCLOSURE_DEFAULT_WIDTH,
        DWORD dwHeight = IMM_ENCLOSURE_DEFAULT_HEIGHT,
        LONG lTopAndBottomWallStiffness = IMM_ENCLOSURE_DEFAULT_STIFFNESS,
        LONG lLeftAndRightWallStiffness = IMM_ENCLOSURE_DEFAULT_STIFFNESS,
        DWORD dwTopAndBottomWallWallWidth = IMM_ENCLOSURE_DEFAULT_WALL_WIDTH,
        DWORD dwLeftAndRightWallWallWidth = IMM_ENCLOSURE_DEFAULT_WALL_WIDTH,
        DWORD dwTopAndBottomWallSaturation = IMM_ENCLOSURE_DEFAULT_SATURATION,
        DWORD dwLeftAndRightWallSaturation = IMM_ENCLOSURE_DEFAULT_SATURATION,
        DWORD dwStiffnessMask = IMM_ENCLOSURE_DEFAULT_STIFFNESS_MASK,
        DWORD dwClippingMask = IMM_ENCLOSURE_DEFAULT_CLIPPING_MASK,
        POINT pntCenter = IMM_ENCLOSURE_DEFAULT_CENTER_POINT,
        CImmEffect* pInsideEffect = NULL,  
		LONG lAngle = IMM_EFFECT_DEFAULT_ANGLE,
		DWORD dwNoDownload = 0
        );


    BOOL 
    Initialize( 
        CImmDevice* pDevice, 
        LPCRECT pRectOutside,
        LONG lTopAndBottomWallStiffness = IMM_ENCLOSURE_DEFAULT_STIFFNESS,
        LONG lLeftAndRightWallStiffness = IMM_ENCLOSURE_DEFAULT_STIFFNESS,
        DWORD dwTopAndBottomWallWallWidth = IMM_ENCLOSURE_DEFAULT_WALL_WIDTH,
        DWORD dwLeftAndRightWallWallWidth = IMM_ENCLOSURE_DEFAULT_WALL_WIDTH,
        DWORD dwTopAndBottomWallSaturation = IMM_ENCLOSURE_DEFAULT_SATURATION,
        DWORD dwLeftAndRightWallSaturation = IMM_ENCLOSURE_DEFAULT_SATURATION,
        DWORD dwStiffnessMask = IMM_ENCLOSURE_DEFAULT_STIFFNESS_MASK,
        DWORD dwClippingMask = IMM_ENCLOSURE_DEFAULT_CLIPPING_MASK,
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

	virtual BOOL
	Stop();
        
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
        LONG lTopAndBottomWallStiffness,
        LONG lLeftAndRightWallStiffness,
        DWORD dwTopAndBottomWallWallWidth,
        DWORD dwLeftAndRightWallWallWidth,
        DWORD dwTopAndBottomWallSaturation,
        DWORD dwLeftAndRightWallSaturation,
        DWORD dwStiffnessMask,
        DWORD dwClippingMask,
        CImmEffect* pInsideEffect,  
		LONG lAngle
        );

	DWORD
	change_parameters( 
		LPCRECT prectBoundary,
		LONG lTopAndBottomWallStiffness,
		LONG lLeftAndRightWallStiffness,
		DWORD dwTopAndBottomWallThickness,
		DWORD dwLeftAndRightWallThickness,
		DWORD dwTopAndBottomWallSaturation,
		DWORD dwLeftAndRightWallSaturation,
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

    IMM_ENCLOSURE m_enclosure;
    BOOL m_bUseMousePosAtStart;

	// Needed for co-ordinating events for Enclosures/Ellipes and the inside effects.
	CImmEffect*	m_pInsideEffect;

};


//
// INLINES
//

inline BOOL
CImmEnclosure::GetIsCompatibleGUID(GUID &guid)
{
	return IsEqualGUID(guid, GUID_Imm_Enclosure);
}

#endif // !defined(AFX_IMMENCLOSURE_H__135B88C4_4175_11D1_B049_0020AF30269A__INCLUDED_)
