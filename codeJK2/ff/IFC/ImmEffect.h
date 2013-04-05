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

  FILE:		ImmEffect.h

  PURPOSE:	Immersion Foundation Classes Base Effect
  
  STARTED:	Oct.10.97

  NOTES/REVISIONS:
     Mar.02.99 jrm (Jeff Mallett): Force-->Feel renaming
	 Mar.02.99 jrm: Added GetIsCompatibleGUID and feel_to_DI_GUID
	 Mar.15.99 jrm: __declspec(dllimport/dllexport) the whole class
	 Nov.15.99 efw (Evan Wies): Converted to IFC

**********************************************************************/


#if !defined(AFX_IMMEffect_H__135B88C4_4175_11D1_B049_0020AF30269A__INCLUDED_)
#define AFX_IMMEffect_H__135B88C4_4175_11D1_B049_0020AF30269A__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef _IFCDLL_
#define DLLIFC __declspec(dllimport)
#else
#define DLLIFC __declspec(dllexport)
#endif

#include "ImmBaseTypes.h"
#include "ImmDevice.h"
class CImmProject;

//================================================================
// Constants
//================================================================

#define     IMM_EFFECT_AXIS_X             1
#define     IMM_EFFECT_AXIS_Y             2
#define     IMM_EFFECT_AXIS_BOTH          3
#define     IMM_EFFECT_AXIS_DIRECTIONAL   4
#define     IMM_EFFECT_DONT_CHANGE        MINLONG
#define     IMM_EFFECT_DONT_CHANGE_PTR    MAXDWORD
const POINT IMM_EFFECT_DONT_CHANGE_POINT	= { 0xFFFFFFFF, 0xFFFFFFFF };
const POINT IMM_EFFECT_MOUSE_POS_AT_START	= { MAXLONG, MAXLONG };

#define     IMM_EFFECT_DEFAULT_ENVELOPE   	NULL
#define     IMM_EFFECT_DEFAULT_DIRECTION_X	1
#define     IMM_EFFECT_DEFAULT_DIRECTION_Y	1
#define     IMM_EFFECT_DEFAULT_ANGLE		0

// GENERIC_EFFECT_PTR
// This is really a pointer to a child of CImmEffect.
typedef class CImmEffect * 	GENERIC_EFFECT_PTR;


//================================================================
// CImmEffect
//================================================================

//
// ------ PUBLIC INTERFACE ------ 
//

class DLLIFC CImmEffect  
{
    //
    // CONSTRUCTOR/DESTRUCTOR
    //

    public:
    
    // Constructor
	CImmEffect(
		const GUID& rguidEffect
		);

	// Destructor
	virtual
	~CImmEffect();

    //
    // ATTRIBUTES
    //

    public:

	LPIIMM_EFFECT
    GetEffect() 
		{ return m_piImmEffect; }

	CImmDevice* 
	GetDevice() 
		{ return m_pImmDevice; }

    BOOL 
    GetStatus(
		DWORD* pdwStatus
		);

	void
	GetParameters(IMM_EFFECT &Effect);
	BOOL GetEnvelope( LPIMM_ENVELOPE pEnvelope );

	BOOL GetDuration( DWORD &dwDuration );
	BOOL GetGain( DWORD &dwGain );
	BOOL GetStartDelay(	DWORD &dwStartDelay );
	BOOL GetTriggerButton( DWORD &dwTriggerButton );
	BOOL GetTriggerRepeatInterval( DWORD &dwTriggerRepeatInterval );
	BOOL GetDirection( LONG &lDirectionX, LONG &lDirectionY );
	BOOL GetDirection( LONG &lAngle );

	BOOL GetIterations( DWORD &dwIterations );

    GUID
	GetGUID()
		{ return m_guidEffect; }

	virtual BOOL
	GetIsCompatibleGUID(
		GUID & /* guid */
		)
		{ return true; }
 
	virtual DWORD GetEffectType()
	{ return 0;	}
	
	LPCSTR
	GetName()
		{ return m_lpszName; }

	// Allocates an object of the correct IFC class from the given GUID
	static GENERIC_EFFECT_PTR
	NewObjectFromGUID(
		GUID &guid
		);

    BOOL
    ChangeBaseParams( 
        LONG lDirectionX,
        LONG lDirectionY,
        DWORD dwDuration = IMM_EFFECT_DONT_CHANGE,
        LPIMM_ENVELOPE pEnvelope = (LPIMM_ENVELOPE) IMM_EFFECT_DONT_CHANGE_PTR,
        DWORD dwSamplePeriod = IMM_EFFECT_DONT_CHANGE,
        DWORD dwGain = IMM_EFFECT_DONT_CHANGE,
        DWORD dwTriggerButton = IMM_EFFECT_DONT_CHANGE,
        DWORD dwTriggerRepeatInterval = IMM_EFFECT_DONT_CHANGE
#ifdef IFC_START_DELAY
		,DWORD dwStartDelay = IMM_EFFECT_DONT_CHANGE // milliseconds
#endif
		);

    BOOL
    ChangeBaseParamsPolar( 
        LONG lAngle,
        DWORD dwDuration = IMM_EFFECT_DONT_CHANGE, // milliseconds
        LPIMM_ENVELOPE pEnvelope = (LPIMM_ENVELOPE) IMM_EFFECT_DONT_CHANGE_PTR,
        DWORD dwSamplePeriod = IMM_EFFECT_DONT_CHANGE,
        DWORD dwGain = IMM_EFFECT_DONT_CHANGE,
        DWORD dwTriggerButton = IMM_EFFECT_DONT_CHANGE,
        DWORD dwTriggerRepeatInterval = IMM_EFFECT_DONT_CHANGE
#ifdef IFC_START_DELAY
		,DWORD dwStartDelay = IMM_EFFECT_DONT_CHANGE // milliseconds
#endif
		);

    BOOL
    ChangeDirection( 
        LONG lDirectionX,
        LONG lDirectionY 
		);

    BOOL
    ChangeDirection( 
        LONG lAngle
        );

	BOOL
	ChangeDuration(
		DWORD dwDuration
		);

	BOOL
	ChangeGain(
		DWORD dwGain
		);

	BOOL 
	ChangeStartDelay(
		DWORD dwStartDelay
		);

	BOOL 
	ChangeTriggerButton(
		DWORD dwTriggerButton
		);

	BOOL 
	ChangeTriggerRepeatInterval(
		DWORD dwTriggerRepeatInterval
		);

	BOOL 
	ChangeIterations(
		DWORD dwIterations
		);

    BOOL
    ChangeEnvelope(
        DWORD dwAttackLevel,
        DWORD dwAttackTime, // microseconds
        DWORD dwFadeLevel,
        DWORD dwFadeTime // microseconds
        );

    BOOL
    ChangeEnvelope(
        LPIMM_ENVELOPE pEnvelope
        );


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
	InitializeFromProject(
		CImmProject &project,
		LPCSTR lpszEffectName,
		CImmDevice* pDevice = NULL,
		DWORD dwNoDownload = 0
	);

	virtual BOOL 
	Start(
        DWORD dwIterations = IMM_EFFECT_DONT_CHANGE,
        DWORD dwFlags = 0
#ifdef IFC_START_DELAY
		, BOOL bAllowStartDelayEmulation = true
#endif
        );

    virtual BOOL 
    Stop();
   

//
// ------ PRIVATE INTERFACE ------ 
//

	//
    // CACHING
    //

#ifdef IFC_EFFECT_CACHING

	public:

	friend class CEffectList;
	friend class CImmCompoundEffect;

	BOOL GetIsPlaying();
	BOOL GetIsTriggered() const;
	short GetPriority() const { return m_Priority; }
	void SetPriority(short priority) { m_Priority = priority; }
	virtual HRESULT Unload();
	virtual void Reload();

	//Althought public, this should only be used internally.
	BOOL
	set_outside_effect( 
		CImmEffect* pImmOutsideEffect 
		);

	BOOL
	get_is_inside_effect()
		{ return m_bIsInsideEffect; }

	public:

	ECacheState m_CacheState; // effect's status in the cache
	BOOL m_bInCurrentSuite; // is the effect in the currently loaded suite?
	short m_Priority; // Priority within suite: higher number is higher priority
	DWORD m_dwLastStarted; // when last started (0 = never) or when param change made on device
	DWORD m_dwLastStopped; // when last stopped (0 = not since last start)
	DWORD m_dwLastLoaded; // when last loaded with CImmEffectSuite::Load or Create

    protected:

	CImmDevice *m_pImmDevice; // ### Use instead of m_piImmDevice
#endif

	//
    // HELPERS
    //
    protected:

#ifdef IFC_START_DELAY
	void EmulateStartDelay(
		DWORD dwIterations,
		DWORD dwNoDownload
		);
#endif

#ifdef IFC_EFFECT_CACHING
	public: // initalize needs to be called by CImmDevice
#endif
   BOOL 
    initialize( 
        CImmDevice* pDevice,
		DWORD dwNoDownload
        );
#ifdef IFC_EFFECT_CACHING
	protected:
#endif

	HRESULT
	set_parameters_on_device(
		DWORD dwFlags
		);

	BOOL
	set_name(
		const char *lpszName
		);

	void 
	imm_to_DI_GUID( 
		GUID &guid
		);

	void 
	DI_to_imm_GUID( 
		GUID &guid
		);

    void
    reset();

    void
    reset_effect_struct();

	void
	reset_device();

	void
	buffer_direction(
		TCHAR** pData
		);

	void
	buffer_long_param(
		TCHAR** pData, 
		LPCSTR lpszKey, 
		long lDefault, 
		long lValue
		);

	void
	buffer_dword_param(
		TCHAR** pData, 
		LPCSTR lpszKey, 
		DWORD dwDefault, 
		DWORD dwValue
		);

	virtual int
	buffer_ifr_data(
		TCHAR* pData
		);

	virtual BOOL
	get_ffe_data(
		LPDIEFFECT pdiEffect
		);


    //
    // INTERNAL DATA
    //

    protected:

    IMM_EFFECT m_Effect;
    DWORD m_dwaAxes[2];
    LONG m_laDirections[2];
	IMM_ENVELOPE m_Envelope;

    GUID m_guidEffect;
    BOOL m_bIsPlaying;
	DWORD m_dwDeviceType;
    LPIIMM_DEVICE m_piImmDevice; // Might also be holding LPDIRECTINPUTDEVICE2
    LPIIMM_EFFECT m_piImmEffect;
	DWORD m_cAxes; // Number of axes
	DWORD m_dwNoDownload;
	DWORD m_dwIterations;
	char *m_lpszName; // Name of this effect primative

	// Needed for co-ordinating events for Enclosures/Ellipes and the inside effects.
	BOOL m_bIsInsideEffect;
	CImmEffect* m_pOutsideEffect;

#ifdef IFC_START_DELAY
	public:
	// Prevents access to dangling pointer when this is deleted
	// All relevent code may be removed when all hardware and drivers support start delay
	CImmEffect **m_ppTimerRef; // pointer to pointer to this.
#endif
};


#endif // !defined(AFX_ImmEffect_H__135B88C4_4175_11D1_B049_0020AF30269A__INCLUDED_)

