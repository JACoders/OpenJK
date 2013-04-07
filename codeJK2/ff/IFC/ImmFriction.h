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

  FILE:		ImmFriction.h

  PURPOSE:	Immersion Foundation Classes Friction Effect
  
  STARTED:	Dec.29.97

  NOTES/REVISIONS:
     Mar.02.99 jrm (Jeff Mallett): Force-->Feel renaming
	 Mar.02.99 jrm: Added GetIsCompatibleGUID
	 Mar.15.99 jrm: __declspec(dllimport/dllexport) the whole class
	 Nov.15.99 efw (Evan Wies): Converted to IFC

**********************************************************************/


#if !defined(AFX_ImmFriction_H__135B88C4_4175_11D1_B049_0020AF30269A__INCLUDED_)
#define AFX_ImmFriction_H__135B88C4_4175_11D1_B049_0020AF30269A__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef _IFCDLL_
#define DLLIFC __declspec(dllimport)
#else
#define DLLIFC __declspec(dllexport)
#endif

#include <windows.h>
#include "ImmCondition.h"


//================================================================
// Constants
//================================================================

#define     IMM_FRICTION_DEFAULT_COEFFICIENT    2500
#define     IMM_FRICTION_DEFAULT_MIN_VELOCITY   0


//================================================================
// CImmFriction
//================================================================

//
// ------ PUBLIC INTERFACE ------ 
//

class DLLIFC CImmFriction : public CImmCondition
{
    //
    // CONSTRUCTOR/DESTRUCTOR
    //

    public:
    
    // Constructor
	CImmFriction();

	// Destructor
	virtual
	~CImmFriction();


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
        DWORD dwCoefficient,
        DWORD dwMinVelocity = IMM_EFFECT_DONT_CHANGE,
        LONG lDirectionX = IMM_EFFECT_DONT_CHANGE,
        LONG lDirectionY = IMM_EFFECT_DONT_CHANGE
        );

    BOOL
    ChangeParametersPolar( 
        DWORD dwCoefficient,
        DWORD dwMinVelocity,
        LONG lAngle
        );

	BOOL ChangeMinVelocityX( DWORD dwMinVelocity );
	BOOL ChangeMinVelocityY( DWORD dwMinVelocity );
	//For setting both axes to the same value
	BOOL ChangeMinVelocity( DWORD dwMinVelocity );

	BOOL GetMinVelocityX( DWORD &dwMinVelocity );
	BOOL GetMinVelocityY( DWORD &dwMinVelocity );

    //
    // OPERATIONS
    //

    public:

    virtual BOOL 
    Initialize( 
        CImmDevice* pDevice, 
        DWORD dwCoefficient = IMM_FRICTION_DEFAULT_COEFFICIENT,
        DWORD dwMinVelocity = IMM_FRICTION_DEFAULT_MIN_VELOCITY,
        DWORD dwfAxis = IMM_EFFECT_AXIS_BOTH,
        LONG lDirectionX = IMM_EFFECT_DEFAULT_DIRECTION_X,
        LONG lDirectionY = IMM_EFFECT_DEFAULT_DIRECTION_Y,
		DWORD dwNoDownload = 0
        );

    virtual BOOL 
    InitializePolar( 
        CImmDevice* pDevice, 
        DWORD dwCoefficient,
        DWORD dwMinVelocity,
        LONG lAngle,
		DWORD dwNoDownload = 0
        );

        
//
// ------ PRIVATE INTERFACE ------ 
//

    //
    // HELPERS
    //

    protected:

    //
    // INTERNAL DATA
    //
    
    protected:

};



//
// INLINES
//

inline BOOL
CImmFriction::GetIsCompatibleGUID(GUID &guid)
{
	return IsEqualGUID(guid, GUID_Imm_Friction);
}


#endif // !defined(AFX_ImmFriction_H__135B88C4_4175_11D1_B049_0020AF30269A__INCLUDED_)
