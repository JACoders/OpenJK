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

  FILE:		ImmGrid.h

  PURPOSE:	Immersion Foundation Classes Grid Effect

  STARTED:	Dec.11.97

  NOTES/REVISIONS:
     Mar.02.99 jrm (Jeff Mallett): Force-->Feel renaming
	 Mar.02.99 jrm: Added GetIsCompatibleGUID
	 Mar.02.99 jrm: __declspec(dllimport/dllexport) the whole class
	 Nov.15.99 efw (Evan Wies): Converted to IFC

**********************************************************************/


#if !defined(AFX_IMMGrid_H__135B88C4_4175_11D1_B049_0020AF30269A__INCLUDED_)
#define AFX_IMMGrid_H__135B88C4_4175_11D1_B049_0020AF30269A__INCLUDED_

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

#define     IMM_GRID_DEFAULT_HORIZ_OFFSET     0  
#define     IMM_GRID_DEFAULT_VERT_OFFSET      0
#define     IMM_GRID_DEFAULT_HORIZ_SPACING    100
#define     IMM_GRID_DEFAULT_VERT_SPACING     100
#define     IMM_GRID_DEFAULT_NODE_STRENGTH    5000
#define     IMM_GRID_DEFAULT_NODE_SATURATION  10000   



//================================================================
// CImmGrid
//================================================================

//
// ------ PUBLIC INTERFACE ------ 
//

class DLLIFC CImmGrid : public CImmCondition
{
    //
    // CONSTRUCTOR/DESTRUCTOR
    //

    public:
    
    // Constructor
	CImmGrid();

	// Destructor
	virtual
	~CImmGrid();


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
        DWORD dwHorizSpacing,
        DWORD dwVertSpacing = IMM_EFFECT_DONT_CHANGE,
        LONG  lHorizNodeStrength = IMM_EFFECT_DONT_CHANGE,
        LONG  lVertNodeStrength = IMM_EFFECT_DONT_CHANGE,
        LONG  lHorizOffset = IMM_EFFECT_DONT_CHANGE,
        LONG  lVertOffset = IMM_EFFECT_DONT_CHANGE,
        DWORD dwHorizNodeSaturation = IMM_EFFECT_DONT_CHANGE,
        DWORD dwVertNodeSaturation = IMM_EFFECT_DONT_CHANGE
        );

	BOOL ChangeHSpacing( DWORD dwHorizSpacing );
	BOOL ChangeVSpacing( DWORD dwVertSpacing );
	BOOL ChangeHNodeStrength( LONG lHorizNodeStrength );
	BOOL ChangeVNodeStrength( LONG lVertNodeStrength );
	BOOL ChangeOffset( POINT pntOffset );
	BOOL ChangeHNodeSaturation( DWORD dwHorizNodeSaturation );
	BOOL ChangeVNodeSaturation( DWORD dwVertNodeSaturation );
	
	BOOL GetHSpacing( DWORD &dwHorizSpacing );
	BOOL GetVSpacing( DWORD &dwVertSpacing );
	BOOL GetHNodeStrength( LONG &lHorizNodeStrength );
	BOOL GetVNodeStrength( LONG &lVertNodeStrength );
	BOOL GetOffset( POINT &pntOffset );
	BOOL GetHNodeSaturation( DWORD &dwHorizNodeSaturation );
	BOOL GetVNodeSaturation( DWORD &dwVertNodeSaturation );

    //
    // OPERATIONS
    //

    public:

    virtual BOOL 
    Initialize( 
        CImmDevice* pDevice, 
        DWORD dwHorizSpacing = IMM_GRID_DEFAULT_HORIZ_SPACING,
        DWORD dwVertSpacing = IMM_GRID_DEFAULT_VERT_SPACING,
        LONG  lHorizNodeStrength = IMM_GRID_DEFAULT_NODE_STRENGTH,
        LONG  lVertNodeStrength = IMM_GRID_DEFAULT_NODE_STRENGTH,
        DWORD dwHorizOffset = IMM_GRID_DEFAULT_HORIZ_OFFSET,
        DWORD dwVertOffset = IMM_GRID_DEFAULT_VERT_OFFSET,
        DWORD dwHorizNodeSaturation = IMM_GRID_DEFAULT_NODE_SATURATION,
        DWORD dwVertNodeSaturation = IMM_GRID_DEFAULT_NODE_SATURATION,
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
CImmGrid::GetIsCompatibleGUID(GUID &guid)
{
	return IsEqualGUID(guid, GUID_Imm_Grid);
}
		
#endif // !defined(AFX_IMMGrid_H__135B88C4_4175_11D1_B049_0020AF30269A__INCLUDED_)
