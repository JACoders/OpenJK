/**********************************************************************
	Copyright (c) 1997-2000 Immersion Corporation

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

  FILE:		FeelitAPI.h

  PURPOSE:	Feelit API

  STARTED:	09/08/97

  NOTES/REVISIONS:

  23-May-2000   MPR     Added support for querying the number of effects that can be 
                        downloaded to a device (FEELIT_PROPNUMEFFECTS and FEELIT_PROP_NUMEFFECTS).  
                        Added a new device subtype, FEELIT_DEVICETYPEMOUSE_VIBRATION_FF.
                        Removed FEELIT_DEVICETYPE_HID.

  06-Jun-2000   MPR     Added FEELIT_PROP_DEVICEID.

  08-Jun-2000   MPR     Added MAKE_FEELIT_DEVICE_TYPE_DWORD.

**********************************************************************/

#ifndef __FEELITAPI_INCLUDED__
#define __FEELITAPI_INCLUDED__

#ifndef FEELIT_VERSION
#define FEELIT_VERSION         0x0103
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#define COM_NO_WINDOWS_H
#include <objbase.h>
#endif

/****************************************************************************
 *
 *      Class IDs
 *
 ****************************************************************************/

DEFINE_GUID(CLSID_Feelit,		0x5959df60,0x2911,0x11d1,0xb0,0x49,0x00,0x20,0xaf,0x30,0x26,0x9a);
DEFINE_GUID(CLSID_FeelitDevice,	0x5959df61,0x2911,0x11d1,0xb0,0x49,0x00,0x20,0xaf,0x30,0x26,0x9a);


/****************************************************************************
 *
 *      Interfaces
 *
 ****************************************************************************/

DEFINE_GUID(IID_IFeelit,		0x5959df62,0x2911,0x11d1,0xb0,0x49,0x00,0x20,0xaf,0x30,0x26,0x9a);
DEFINE_GUID(IID_IFeelitDevice,	0x5959df63,0x2911,0x11d1,0xb0,0x49,0x00,0x20,0xaf,0x30,0x26,0x9a);
DEFINE_GUID(IID_IFeelitEffect,	0x5959df64,0x2911,0x11d1,0xb0,0x49,0x00,0x20,0xaf,0x30,0x26,0x9a);
DEFINE_GUID(IID_IFeelitConfig,  0x900c39e0,0xcc5c,0x11d2,0x8c,0x5d,0x00,0x10,0x5a,0x17,0x8a,0xd1);


/****************************************************************************
 *
 *      Predefined object types
 *
 ****************************************************************************/

DEFINE_GUID(GUID_Feelit_XAxis,   0x5959df65,0x2911,0x11d1,0xb0,0x49,0x00,0x20,0xaf,0x30,0x26,0x9a);
DEFINE_GUID(GUID_Feelit_YAxis,   0x5959df66,0x2911,0x11d1,0xb0,0x49,0x00,0x20,0xaf,0x30,0x26,0x9a);
DEFINE_GUID(GUID_Feelit_ZAxis,   0x5959df67,0x2911,0x11d1,0xb0,0x49,0x00,0x20,0xaf,0x30,0x26,0x9a);
DEFINE_GUID(GUID_Feelit_RxAxis,  0x5959df68,0x2911,0x11d1,0xb0,0x49,0x00,0x20,0xaf,0x30,0x26,0x9a);
DEFINE_GUID(GUID_Feelit_RyAxis,  0x5959df69,0x2911,0x11d1,0xb0,0x49,0x00,0x20,0xaf,0x30,0x26,0x9a);
DEFINE_GUID(GUID_Feelit_RzAxis,  0x5959df6a,0x2911,0x11d1,0xb0,0x49,0x00,0x20,0xaf,0x30,0x26,0x9a);
DEFINE_GUID(GUID_Feelit_Slider,  0x5959df6b,0x2911,0x11d1,0xb0,0x49,0x00,0x20,0xaf,0x30,0x26,0x9a);

DEFINE_GUID(GUID_Feelit_Button,  0x5959df6c,0x2911,0x11d1,0xb0,0x49,0x00,0x20,0xaf,0x30,0x26,0x9a);
DEFINE_GUID(GUID_Feelit_Key,     0x5959df6d,0x2911,0x11d1,0xb0,0x49,0x00,0x20,0xaf,0x30,0x26,0x9a);

DEFINE_GUID(GUID_Feelit_POV,     0x5959df6e,0x2911,0x11d1,0xb0,0x49,0x00,0x20,0xaf,0x30,0x26,0x9a);

DEFINE_GUID(GUID_Feelit_Unknown, 0x5959df6f,0x2911,0x11d1,0xb0,0x49,0x00,0x20,0xaf,0x30,0x26,0x9a);


/****************************************************************************
 *
 *      Predefined Product GUIDs
 *
 ****************************************************************************/

DEFINE_GUID(GUID_Feelit_Mouse,   0x99bb5400,0x2b94,0x11d1,0xb0,0x49,0x00,0x20,0xaf,0x30,0x26,0x9a);

/****************************************************************************
 *
 *      Force feedback effects
 *
 ****************************************************************************/


/* Constant Force */
DEFINE_GUID(GUID_Feelit_ConstantForce,0x5959df71,0x2911,0x11d1,0xb0,0x49,0x00,0x20,0xaf,0x30,0x26,0x9a);

/* Ramp Force */
DEFINE_GUID(GUID_Feelit_RampForce,	0x5959df72,0x2911,0x11d1,0xb0,0x49,0x00,0x20,0xaf,0x30,0x26,0x9a);

/* Periodic Effects */
DEFINE_GUID(GUID_Feelit_Square,      0x5959df73,0x2911,0x11d1,0xb0,0x49,0x00,0x20,0xaf,0x30,0x26,0x9a);
DEFINE_GUID(GUID_Feelit_Sine,        0x5959df74,0x2911,0x11d1,0xb0,0x49,0x00,0x20,0xaf,0x30,0x26,0x9a);
DEFINE_GUID(GUID_Feelit_Triangle,    0x5959df75,0x2911,0x11d1,0xb0,0x49,0x00,0x20,0xaf,0x30,0x26,0x9a);
DEFINE_GUID(GUID_Feelit_SawtoothUp,	0x5959df76,0x2911,0x11d1,0xb0,0x49,0x00,0x20,0xaf,0x30,0x26,0x9a);
DEFINE_GUID(GUID_Feelit_SawtoothDown,0x5959df77,0x2911,0x11d1,0xb0,0x49,0x00,0x20,0xaf,0x30,0x26,0x9a);


/* Conditions */
DEFINE_GUID(GUID_Feelit_Spring,      0x5959df78,0x2911,0x11d1,0xb0,0x49,0x00,0x20,0xaf,0x30,0x26,0x9a);
DEFINE_GUID(GUID_Feelit_DeviceSpring,0x5959df83,0x2911,0x11d1,0xb0,0x49,0x00,0x20,0xaf,0x30,0x26,0x9a);
DEFINE_GUID(GUID_Feelit_Damper,      0x5959df79,0x2911,0x11d1,0xb0,0x49,0x00,0x20,0xaf,0x30,0x26,0x9a);
DEFINE_GUID(GUID_Feelit_Inertia,     0x5959df7a,0x2911,0x11d1,0xb0,0x49,0x00,0x20,0xaf,0x30,0x26,0x9a);
DEFINE_GUID(GUID_Feelit_Friction,    0x5959df7b,0x2911,0x11d1,0xb0,0x49,0x00,0x20,0xaf,0x30,0x26,0x9a);
DEFINE_GUID(GUID_Feelit_Texture,		0x5959df7c,0x2911,0x11d1,0xb0,0x49,0x00,0x20,0xaf,0x30,0x26,0x9a);
DEFINE_GUID(GUID_Feelit_Grid,		0x5959df7d,0x2911,0x11d1,0xb0,0x49,0x00,0x20,0xaf,0x30,0x26,0x9a);

/* Enclosures */
DEFINE_GUID(GUID_Feelit_Enclosure,	0x5959df7f,0x2911,0x11d1,0xb0,0x49,0x00,0x20,0xaf,0x30,0x26,0x9a);
DEFINE_GUID(GUID_Feelit_Ellipse,	    0x5959df82,0x2911,0x11d1,0xb0,0x49,0x00,0x20,0xaf,0x30,0x26,0x9a);

/* Custom Force */
DEFINE_GUID(GUID_Feelit_CustomForce, 0x5959df7e,0x2911,0x11d1,0xb0,0x49,0x00,0x20,0xaf,0x30,0x26,0x9a);

/****************************************************************************
 *
 *      Interfaces and Structures...
 *
 ****************************************************************************/


/****************************************************************************
 *
 *      IFeelitEffect
 *
 ****************************************************************************/

#define FEELIT_FEFFECTTYPE_ALL					0x00000000

#define FEELIT_FEFFECTTYPE_CONSTANTFORCE		0x00000001
#define FEELIT_FEFFECTTYPE_RAMPFORCE			0x00000002
#define FEELIT_FEFFECTTYPE_PERIODIC				0x00000003
#define FEELIT_FEFFECTTYPE_CONDITION			0x00000004
#define FEELIT_FEFFECTTYPE_ENCLOSURE			0x00000005
#define FEELIT_FEFFECTTYPE_ELLIPSE				0x00000006
#define FEELIT_FEFFECTTYPE_TEXTURE				0x00000007
#define FEELIT_FEFFECTTYPE_CUSTOMFORCE			0x000000F0
#define FEELIT_FEFFECTTYPE_HARDWARE				0x000000FF

#define FEELIT_FEFFECTTYPE_FFATTACK				0x00000200
#define FEELIT_FEFFECTTYPE_FFFADE				0x00000400
#define FEELIT_FEFFECTTYPE_SATURATION			0x00000800
#define FEELIT_FEFFECTTYPE_POSNEGCOEFFICIENTS	0x00001000
#define FEELIT_FEFFECTTYPE_POSNEGSATURATION		0x00002000
#define FEELIT_FEFFECTTYPE_DEADBAND				0x00004000

#define FEELIT_EFFECTTYPE_GETTYPE(n)             LOBYTE(n)
#define FEELIT_EFFECTTYPE_GETFLAGS(n)            ( n & 0xFFFFFF00 )

#define FEELIT_DEGREES                  100
#define FEELIT_FFNOMINALMAX             10000
#define FEELIT_SECONDS                  1000000

typedef struct FEELIT_CONSTANTFORCE {
    LONG  lMagnitude;	/* Magnitude of the effect, in the range -10000 to 10000 */
} FEELIT_CONSTANTFORCE, *LPFEELIT_CONSTANTFORCE;
typedef const FEELIT_CONSTANTFORCE *LPCFEELIT_CONSTANTFORCE;

typedef struct FEELIT_RAMPFORCE {
    LONG  lStart;	/* Magnitude at start of effect. Range -10000 to 10000 */
    LONG  lEnd;		/* Magnitude at end of effect. Range -10000 to 10000 */
} FEELIT_RAMPFORCE, *LPFEELIT_RAMPFORCE;
typedef const FEELIT_RAMPFORCE *LPCFEELIT_RAMPFORCE;

typedef struct FEELIT_PERIODIC {
    DWORD dwMagnitude;  /* Magnitude of the effect, in the range 0 to 10000 */
    LONG  lOffset;		/* Force will be gen'd in range lOffset - dwMagnitude to lOffset + dwMagnitude */
    DWORD dwPhase;		/* Position in cycle at wich playback begins. Range 0 - 35,999 */
    DWORD dwPeriod;		/* Period (length of one cycle) of the effect in microseconds */
} FEELIT_PERIODIC, *LPFEELIT_PERIODIC;
typedef const FEELIT_PERIODIC *LPCFEELIT_PERIODIC;

typedef struct FEELIT_CONDITION {
    LONG  lCenter;				/* Center-point in screen coords. Axis depends on that in FEELIT_EFFECT */
    LONG  lPositiveCoefficient;	/* Coef. on pos. side of the offset. Range -10000 to 10000 */
    LONG  lNegativeCoefficient;	/* Coef. on neg. side of the offset. Range -10000 to 10000 */
    DWORD dwPositiveSaturation; /* Max force output on pos. side of offset. Range 0 to 10000 */
    DWORD dwNegativeSaturation;	/* Max force output on neg. side of offset. Range 0 to 10000 */
    LONG  lDeadBand;			/* Region around lOffset where condition is not active. Range 0 to 10000 */
} FEELIT_CONDITION, *LPFEELIT_CONDITION;
typedef const FEELIT_CONDITION *LPCFEELIT_CONDITION;

typedef struct FEELIT_TEXTURE {
    DWORD dwSize;			    /* sizeof(FEELIT_TEXTURE)   */
    LONG lOffset;				/* Offset in screen coords of first texture from left or top edge */
    LONG lPosBumpMag;			/* Magnitude of bumps felt when mouse travels in positive direction */
    DWORD dwPosBumpWidth;		/* Width of bumps felt when mouse travels in positive direction */
    DWORD dwPosBumpSpacing;		/* Center-to-Center spacing of bumps felt when mouse travels in positive direction */
    LONG lNegBumpMag;	        /* Magnitude of bumps felt when mouse travels in negative direction */
    DWORD dwNegBumpWidth;		/* Width of bumps felt when mouse travels in negative direction */
    DWORD dwNegBumpSpacing;		/* Center-to-Center spacing of bumps felt when mouse travels in negative direction */
} FEELIT_TEXTURE, *LPFEELIT_TEXTURE;
typedef const FEELIT_TEXTURE *LPCFEELIT_TEXTURE;

typedef struct FEELIT_CUSTOMFORCE {
    DWORD cChannels;		/* No. of channels (axes) affected by this force */
    DWORD dwSamplePeriod;	/* Sample period in microseconds */
    DWORD cSamples;			/* Total number of samples in the rglForceData */
    LPLONG rglForceData;	/* long[cSamples]. Array of force values. Channels are interleaved */
} FEELIT_CUSTOMFORCE, *LPFEELIT_CUSTOMFORCE;
typedef const FEELIT_CUSTOMFORCE *LPCFEELIT_CUSTOMFORCE;

typedef struct FEELIT_ENVELOPE {
    DWORD dwSize;			/* sizeof(FEELIT_ENVELOPE)   */
    DWORD dwAttackLevel;	/* Ampl. for start of env., rel. to baseline. Range 0 to 10000 */
    DWORD dwAttackTime;     /* Time, in microseconds, to reach sustain level */
    DWORD dwFadeLevel;		/* Ampl. for end of env., rel. to baseline. Range 0 to 10000 */
    DWORD dwFadeTime;       /* Time, in microseconds, to reach fade level */
} FEELIT_ENVELOPE, *LPFEELIT_ENVELOPE;
typedef const FEELIT_ENVELOPE *LPCFEELIT_ENVELOPE;

typedef struct FEELIT_EFFECT {
    DWORD dwSize;                   /* sizeof(FEELIT_EFFECT) */
	GUID guidEffect;			    /* Effect Identifier    */
	DWORD dwFlags;                  /* FEELIT_FEFFECT_*      */
    DWORD dwDuration;               /* Microseconds         */
    DWORD dwSamplePeriod;           /* RESERVED             */
    DWORD dwGain;					/* RESERVED             */
    DWORD dwTriggerButton;          /* RESERVED             */
    DWORD dwTriggerRepeatInterval;  /* RESERVED             */
    DWORD cAxes;                    /* Number of axes       */
    LPDWORD rgdwAxes;               /* Array of axes        */
    LPLONG rglDirection;            /* Array of directions  */
    LPFEELIT_ENVELOPE lpEnvelope;   /* Optional             */
    DWORD cbTypeSpecificParams;     /* Size of params       */
    LPVOID lpvTypeSpecificParams;   /* Pointer to params    */
	 DWORD dwStartDelay;             /* Microseconds delay    */
} FEELIT_EFFECT, *LPFEELIT_EFFECT;
typedef const FEELIT_EFFECT *LPCFEELIT_EFFECT;


/* Effect Flags */
#define FEELIT_FEFFECT_OBJECTIDS             0x00000001
#define FEELIT_FEFFECT_OBJECTOFFSETS         0x00000002
#define FEELIT_FEFFECT_CARTESIAN             0x00000010
#define FEELIT_FEFFECT_POLAR                 0x00000020
#define FEELIT_FEFFECT_SPHERICAL             0x00000040

/* Parameter Flags */
#define FEELIT_FPARAM_DURATION               0x00000001
#define FEELIT_FPARAM_SAMPLEPERIOD           0x00000002
#define FEELIT_FPARAM_GAIN                   0x00000004
#define FEELIT_FPARAM_TRIGGERBUTTON          0x00000008
#define FEELIT_FPARAM_TRIGGERREPEATINTERVAL  0x00000010
#define FEELIT_FPARAM_AXES                   0x00000020
#define FEELIT_FPARAM_DIRECTION              0x00000040
#define FEELIT_FPARAM_ENVELOPE               0x00000080
#define FEELIT_FPARAM_TYPESPECIFICPARAMS     0x00000100
#define FEELIT_FPARAM_STARTDELAY             0x00000200
#define FEELIT_FPARAM_ALLPARAMS              0x000003FF
#define FEELIT_FPARAM_START                  0x20000000
#define FEELIT_FPARAM_NORESTART              0x40000000
#define FEELIT_FPARAM_NODOWNLOAD             0x80000000

#define FEELIT_PARAM_NOTRIGGER               0xFFFFFFFF

/* Start Flags */
#define FEELIT_FSTART_SOLO                   0x00000001
#define FEELIT_FSTART_NODOWNLOAD             0x80000000

/* Status Flags */
#define FEELIT_FSTATUS_PLAYING               0x00000001
#define FEELIT_FSTATUS_EMULATED              0x00000002

/* Stiffness Mask Flags */
#define FEELIT_FSTIFF_NONE				0x00000000
#define FEELIT_FSTIFF_OUTERLEFTWALL		0x00000001
#define FEELIT_FSTIFF_INNERLEFTWALL		0x00000002
#define FEELIT_FSTIFF_INNERRIGHTWALL	0x00000004
#define FEELIT_FSTIFF_OUTERRIGHTWALL	0x00000008
#define FEELIT_FSTIFF_OUTERTOPWALL		0x00000010
#define FEELIT_FSTIFF_INNERTOPWALL		0x00000020
#define FEELIT_FSTIFF_INNERBOTTOMWALL	0x00000040
#define FEELIT_FSTIFF_OUTERBOTTOMWALL	0x00000080
#define FEELIT_FSTIFF_OUTERANYWALL		( FEELIT_FSTIFF_OUTERTOPWALL | FEELIT_FSTIFF_OUTERBOTTOMWALL | FEELIT_FSTIFF_OUTERLEFTWALL | FEELIT_FSTIFF_OUTERRIGHTWALL )
#define FEELIT_FSTIFF_INBOUNDANYWALL	FEELIT_FSTIFF_OUTERANYWALL
#define FEELIT_FSTIFF_INNERANYWALL		( FEELIT_FSTIFF_INNERTOPWALL | FEELIT_FSTIFF_INNERBOTTOMWALL | FEELIT_FSTIFF_INNERLEFTWALL | FEELIT_FSTIFF_INNERRIGHTWALL )
#define FEELIT_FSTIFF_OUTBOUNDANYWALL	FEELIT_FSTIFF_INNERANYWALL
#define FEELIT_FSTIFF_ANYWALL			( FEELIT_FSTIFF_OUTERANYWALL | FEELIT_FSTIFF_INNERANYWALL )

/* Clipping Mask Flags */
#define FEELIT_FCLIP_NONE				0x00000000
#define FEELIT_FCLIP_OUTERLEFTWALL		0x00000001
#define FEELIT_FCLIP_INNERLEFTWALL		0x00000002
#define FEELIT_FCLIP_INNERRIGHTWALL		0x00000004
#define FEELIT_FCLIP_OUTERRIGHTWALL		0x00000008
#define FEELIT_FCLIP_OUTERTOPWALL		0x00000010
#define FEELIT_FCLIP_INNERTOPWALL		0x00000020
#define FEELIT_FCLIP_INNERBOTTOMWALL	0x00000040
#define FEELIT_FCLIP_OUTERBOTTOMWALL	0x00000080
#define FEELIT_FCLIP_OUTERANYWALL		( FEELIT_FCLIP_OUTERTOPWALL | FEELIT_FCLIP_OUTERBOTTOMWALL | FEELIT_FCLIP_OUTERLEFTWALL | FEELIT_FCLIP_OUTERRIGHTWALL )
#define FEELIT_FCLIP_INNERANYWALL		( FEELIT_FCLIP_INNERTOPWALL | FEELIT_FCLIP_INNERBOTTOMWALL | FEELIT_FCLIP_INNERLEFTWALL | FEELIT_FCLIP_INNERRIGHTWALL )
#define FEELIT_FCLIP_ANYWALL			( FEELIT_FCLIP_OUTERANYWALL | FEELIT_FCLIP_INNERANYWALL )

typedef struct FEELIT_EFFESCAPE {
    DWORD   dwSize;			/* sizeof( FEELIT_EFFESCAPE ) */
    DWORD   dwCommand;		/* Driver-specific command number */
    LPVOID  lpvInBuffer;	/* Buffer containing data required to perform the operation */
    DWORD   cbInBuffer;		/* Size, in bytes, of lpvInBuffer */
    LPVOID  lpvOutBuffer;	/* Buffer in which the operation's output data is returned */
    DWORD   cbOutBuffer;	/* Size, in bytes, of lpvOutBuffer */
} FEELIT_EFFESCAPE, *LPFEELIT_EFFESCAPE;


#undef INTERFACE
#define INTERFACE IFeelitEffect

DECLARE_INTERFACE_(IFeelitEffect, IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;

    /*** IFeelitEffect methods ***/
    STDMETHOD(GetEffectGuid)(THIS_ LPGUID) PURE;
    STDMETHOD(GetParameters)(THIS_ LPFEELIT_EFFECT,DWORD) PURE;
    STDMETHOD(SetParameters)(THIS_ LPCFEELIT_EFFECT,DWORD) PURE;
    STDMETHOD(Start)(THIS_ DWORD,DWORD) PURE;
    STDMETHOD(Stop)(THIS) PURE;
    STDMETHOD(GetEffectStatus)(THIS_ LPDWORD) PURE;
    STDMETHOD(Download)(THIS) PURE;
    STDMETHOD(Unload)(THIS) PURE;
    STDMETHOD(Escape)(THIS_ LPFEELIT_EFFESCAPE) PURE;
};

typedef struct IFeelitEffect *LPIFEELIT_EFFECT;


#if !defined(__cplusplus) || defined(CINTERFACE)
#define IFeelitEffect_QueryInterface(p,a,b) (p)->lpVtbl->QueryInterface(p,a,b)
#define IFeelitEffect_AddRef(p) (p)->lpVtbl->AddRef(p)
#define IFeelitEffect_Release(p) (p)->lpVtbl->Release(p)
#define IFeelitEffect_GetEffectGuid(p,a) (p)->lpVtbl->GetEffectGuid(p,a)
#define IFeelitEffect_GetParameters(p,a,b) (p)->lpVtbl->GetParameters(p,a,b)
#define IFeelitEffect_SetParameters(p,a,b) (p)->lpVtbl->SetParameters(p,a,b)
#define IFeelitEffect_Start(p,a,b) (p)->lpVtbl->Start(p,a,b)
#define IFeelitEffect_Stop(p) (p)->lpVtbl->Stop(p)
#define IFeelitEffect_GetEffectStatus(p,a) (p)->lpVtbl->GetEffectStatus(p,a)
#define IFeelitEffect_Download(p) (p)->lpVtbl->Download(p)
#define IFeelitEffect_Unload(p) (p)->lpVtbl->Unload(p)
#define IFeelitEffect_Escape(p,a) (p)->lpVtbl->Escape(p,a)
#else
#define IFeelitEffect_QueryInterface(p,a,b) (p)->QueryInterface(a,b)
#define IFeelitEffect_AddRef(p) (p)->AddRef()
#define IFeelitEffect_Release(p) (p)->Release()
#define IFeelitEffect_GetEffectGuid(p,a) (p)->GetEffectGuid(a)
#define IFeelitEffect_GetParameters(p,a,b) (p)->GetParameters(a,b)
#define IFeelitEffect_SetParameters(p,a,b) (p)->SetParameters(a,b)
#define IFeelitEffect_Start(p,a,b) (p)->Start(a,b)
#define IFeelitEffect_Stop(p) (p)->Stop()
#define IFeelitEffect_GetEffectStatus(p,a) (p)->GetEffectStatus(a)
#define IFeelitEffect_Download(p) (p)->Download()
#define IFeelitEffect_Unload(p) (p)->Unload()
#define IFeelitEffect_Escape(p,a) (p)->Escape(a)
#endif


typedef struct FEELIT_ENCLOSURE {
    DWORD dwSize;                       /* sizeof(FEELIT_ENCLOSURE) */
    RECT rectBoundary;                  /* Rectangle defining the boundaries of the effect, in screen coords */
    DWORD dwTopAndBottomWallThickness;  /* Thickness (pixels) of top/bottom walls. Must be < rectOutside.Height()/2 */
    DWORD dwLeftAndRightWallThickness;  /* Thickness (pixels) of left/right walls. Must be < rectOutside.Width()/2 */
    LONG lTopAndBottomWallStiffness;    /* Stiffness of horizontal borders */
    LONG lLeftAndRightWallStiffness;    /* Stiffness of vertical borders */
    DWORD dwStiffnessMask;              /* Borders where stiffness is turned on (FEELIT_FSTIFF*) */
    DWORD dwClippingMask;               /* Borders where clipping is turned on  (FEELIT_FCLIP*) */
    DWORD dwTopAndBottomWallSaturation; /* Saturation level of spring effect for top/bottom borders */
    DWORD dwLeftAndRightWallSaturation; /* Saturation level of spring effect for left/right borders */
    LPIFEELIT_EFFECT piInsideEffect;    /* Interface pointer to effect active in inner area of the enclosure */
} FEELIT_ENCLOSURE, *LPFEELIT_ENCLOSURE;
typedef const FEELIT_ENCLOSURE *LPCFEELIT_ENCLOSURE;


typedef struct FEELIT_ELLIPSE {
    DWORD dwSize;                       /* sizeof(FEELIT_ELLIPSE) */
    RECT  rectBoundary;                 /* Rectangle which circumscribes the ellipse (screen coords) */
    DWORD dwWallThickness;              /* Thickness (pixels) of ellipse wall at its thickest point */
    LONG lStiffness;                    /* Stiffness of ellipse borders */
    DWORD dwStiffnessMask;              /* Borders where stiffness is turned on (FEELIT_FSTIFF*) */
    DWORD dwClippingMask;               /* Borders where clipping is turned on  (FEELIT_FCLIP*) */
    DWORD dwSaturation;                 /* Saturation level of spring effect for ellipse borders */
    LPIFEELIT_EFFECT piInsideEffect;    /* Interface pointer to effect active in inner area of the ellipse */
} FEELIT_ELLIPSE, *LPFEELIT_ELLIPSE;
typedef const FEELIT_ELLIPSE *LPCFEELIT_ELLIPSE;


/****************************************************************************
 *
 *      IFeelitDevice
 *
 ****************************************************************************/

/* Device types */
#define FEELIT_DEVICETYPE_DEVICE    1
#define FEELIT_DEVICETYPE_MOUSE     2

/* Device subtypes */
#define FEELIT_DEVICETYPEMOUSE_UNKNOWN          1
#define FEELIT_DEVICETYPEMOUSE_TRADITIONAL_FF   2
#define FEELIT_DEVICETYPEMOUSE_VIBRATION_FF     3

/* Device type macros */
#define GET_FEELIT_DEVICE_TYPE(dwDevType)    LOBYTE(dwDevType)
#define GET_FEELIT_DEVICE_SUBTYPE(dwDevType) HIBYTE(dwDevType)
#define MAKE_FEELIT_DEVICE_TYPE_DWORD(ucDevType, ucDevSubtype)  (((DWORD) ((BYTE) ucDevSubtype << 8)) | (BYTE) ucDevType)

typedef struct FEELIT_DEVCAPS {
    DWORD   dwSize;					/* sizeof( FEELIT_DEVCAPS ) */
    DWORD   dwFlags;				/* FEELIT_FDEVCAPS_* */
    DWORD   dwDevType;				/* FEELIT_DEVICETYPE* */
    DWORD   dwAxes;					/* No. of axes available on the device */
    DWORD   dwButtons;				/* No. of buttons available on the device */
    DWORD   dwPOVs;					/* No. of point-of-view controllers on the device */
    DWORD   dwFFSamplePeriod;		/* Min. time btwn playback of consec. raw force commands */
    DWORD   dwFFMinTimeResolution;	/* Min. time, in microseconds, the device can resolve */
    DWORD   dwFirmwareRevision;		/* Firmware revision number of the device */
    DWORD   dwHardwareRevision;		/* Hardware revision number of the device */
    DWORD   dwFFDriverVersion;		/* Version number of the device driver */
} FEELIT_DEVCAPS, *LPFEELIT_DEVCAPS;
typedef const FEELIT_DEVCAPS *LPCFEELIT_DEVCAPS;

/* Device capabilities flags */
#define FEELIT_FDEVCAPS_ATTACHED           0x00000001
#define FEELIT_FDEVCAPS_POLLEDDEVICE       0x00000002
#define FEELIT_FDEVCAPS_EMULATED           0x00000004
#define FEELIT_FDEVCAPS_POLLEDDATAFORMAT   0x00000008
#define FEELIT_FDEVCAPS_FORCEFEEDBACK      0x00000100
#define FEELIT_FDEVCAPS_FFATTACK           0x00000200
#define FEELIT_FDEVCAPS_FFFADE             0x00000400
#define FEELIT_FDEVCAPS_SATURATION         0x00000800
#define FEELIT_FDEVCAPS_POSNEGCOEFFICIENTS 0x00001000
#define FEELIT_FDEVCAPS_POSNEGSATURATION   0x00002000
#define FEELIT_FDEVCAPS_DEADBAND           0x00004000


/* Data Format Type Flags */

#define FEELIT_FOBJDATAFMT_ALL               0x00000000

#define FEELIT_FOBJDATAFMT_RELAXIS           0x00000001
#define FEELIT_FOBJDATAFMT_ABSAXIS           0x00000002
#define FEELIT_FOBJDATAFMT_AXIS              0x00000003

#define FEELIT_FOBJDATAFMT_PSHBUTTON         0x00000004
#define FEELIT_FOBJDATAFMT_TGLBUTTON         0x00000008
#define FEELIT_FOBJDATAFMT_BUTTON            0x0000000C

#define FEELIT_FOBJDATAFMT_POV               0x00000010

#define FEELIT_FOBJDATAFMT_COLLECTION        0x00000040
#define FEELIT_FOBJDATAFMT_NODATA            0x00000080
#define FEELIT_FOBJDATAFMT_FFACTUATOR        0x01000000
#define FEELIT_FOBJDATAFMT_FFEFFECTTRIGGER   0x02000000
#define FEELIT_FOBJDATAFMT_NOCOLLECTION      0x00FFFF00

#define FEELIT_FOBJDATAFMT_ANYINSTANCE       0x00FFFF00
#define FEELIT_FOBJDATAFMT_INSTANCEMASK      FEELIT_FOBJDATAFMT_ANYINSTANCE


/* Data Format Type Macros */
#define FEELIT_OBJDATAFMT_MAKEINSTANCE(n)	((WORD)(n) << 8)
#define FEELIT_OBJDATAFMT_GETTYPE(n)			LOBYTE(n)
#define FEELIT_OBJDATAFMT_GETINSTANCE(n)		LOWORD((n) >> 8)
#define FEELIT_OBJDATAFMT_ENUMCOLLECTION(n)	((WORD)(n) << 8)


typedef struct _FEELIT_OBJECTDATAFORMAT {
    const GUID *pguid;	/* Unique ID for the axis, button, or other input source. */
    DWORD   dwOfs;		/* Offset in data packet where input source data is stored */
    DWORD   dwType;		/* Device type describing the object. (FEELIT_FOBJDATAFMT_*) */
    DWORD   dwFlags;	/* Aspect flags.  Zero or more of FEELIT_FDEVOBJINST_ASPECT* */
} FEELIT_OBJECTDATAFORMAT, *LPFEELIT_OBJECTDATAFORMAT;
typedef const FEELIT_OBJECTDATAFORMAT *LPCFEELIT_OBJECTDATAFORMAT;

typedef struct _FEELIT_DATAFORMAT {
    DWORD   dwSize;		/* sizeof( FEELIT_DATAFORMAT ) */
    DWORD   dwObjSize;	/* sizeof( FEELIT_OBJECTDATAFORMAT ) */
    DWORD   dwFlags;	/* One of FEELIT_FDATAFORMAT_* */
    DWORD   dwDataSize;	/* Size, in bytes, of data packet returned by the device */
    DWORD   dwNumObjs;	/* Number of object in the rgodf array */
    LPFEELIT_OBJECTDATAFORMAT rgodf;	/* Ptr to array of FEELIT_OBJECTDATAFORMAT */
} FEELIT_DATAFORMAT, *LPFEELIT_DATAFORMAT;
typedef const FEELIT_DATAFORMAT *LPCFEELIT_DATAFORMAT;

/* Data Format Flags */
#define FEELIT_FDATAFORMAT_ABSAXIS            0x00000001
#define FEELIT_FDATAFORMAT_RELAXIS            0x00000002

/* Predefined Data Formats */
extern const FEELIT_DATAFORMAT c_dfFeelitMouse;

typedef struct FEELIT_DEVICEOBJECTINSTANCE {
    DWORD   dwSize;					/* sizeof( FEELIT_DEVICEOBJECTINSTANCE ) */
    GUID    guidType;				/* Optional unique ID indicating object type */
    DWORD   dwOfs;					/* Offset within data format for data from this object */
    DWORD   dwType;					/* Device type describing the object. (FEELIT_FOBJDATAFMT_*) */
    DWORD   dwFlags;				/* Zero or more of FEELIT_FDEVOBJINST_* */
    CHAR    tszName[MAX_PATH];		/* Name of object (e.g. "X-Axis") */
    DWORD   dwFFMaxForce;			/* Mag. of max force created by actuator for this object */
    DWORD   dwFFForceResolution;	/* Force resolution of the actuator for this object */
    WORD    wCollectionNumber;		/* RESERVED */
    WORD    wDesignatorIndex;		/* RESERVED */
    WORD    wUsagePage;				/* HID usage page associated with the object */
    WORD    wUsage;					/* HID usage associated with the object */
    DWORD   dwDimension;			/* Dimensional units in which object's value is reported */
    WORD    wExponent;				/* Exponent to associate with the demension */
    WORD    wReserved;
} FEELIT_DEVICEOBJECTINSTANCE, *LPFEELIT_DEVICEOBJECTINSTANCE;
typedef const FEELIT_DEVICEOBJECTINSTANCE  *LPCFEELIT_DEVICEOBJECTINSTANCE;

typedef BOOL (FAR PASCAL * LPFEELIT_ENUMDEVICEOBJECTSCALLBACK)(LPCFEELIT_DEVICEOBJECTINSTANCE, LPVOID);

/* Device Object Instance Flags */
#define FEELIT_FDEVOBJINST_FFACTUATOR        0x00000001
#define FEELIT_FDEVOBJINST_FFEFFECTTRIGGER   0x00000002
#define FEELIT_FDEVOBJINST_POLLED            0x00008000
#define FEELIT_FDEVOBJINST_ASPECTPOSITION    0x00000100
#define FEELIT_FDEVOBJINST_ASPECTVELOCITY    0x00000200
#define FEELIT_FDEVOBJINST_ASPECTACCEL       0x00000300
#define FEELIT_FDEVOBJINST_ASPECTFORCE       0x00000400
#define FEELIT_FDEVOBJINST_ASPECTMASK        0x00000F00

typedef struct FEELIT_PROPHEADER {
    DWORD   dwSize;			/* Size of enclosing struct, to which this struct is header */
    DWORD   dwHeaderSize;	/* sizeof ( FEELIT_PROPHEADER ) */
    DWORD   dwObj;			/* Object for which the property is to be accessed */
    DWORD   dwHow;			/* Specifies how dwObj is interpreted. ( FEELIT_FPROPHEADER_* ) */
} FEELIT_PROPHEADER, *LPFEELIT_PROPHEADER;
typedef const FEELIT_PROPHEADER *LPCFEELIT_PROPHEADER;

/* Prop header flags */
#define FEELIT_FPROPHEADER_DEVICE             0
#define FEELIT_FPROPHEADER_BYOFFSET           1
#define FEELIT_FPROPHEADER_BYID               2

typedef struct FEELIT_PROPDWORD {
    FEELIT_PROPHEADER feelitph;	/* Feelit property header struct */
    DWORD   dwData;				/* Property-specific value being retrieved */
} FEELIT_PROPDWORD, *LPFEELIT_PROPDWORD;
typedef const FEELIT_PROPDWORD *LPCFEELIT_PROPDWORD;

typedef struct FEELIT_PROPRANGE {
    FEELIT_PROPHEADER feelitph;	/* Feelit property header struct */
    LONG    lMin;				/* Lower limit of range, inclusive */
    LONG    lMax;				/* Upper limit of range, inclusive */
} FEELIT_PROPRANGE, *LPFEELIT_PROPRANGE;
typedef const FEELIT_PROPRANGE *LPCFEELIT_PROPRANGE;

#define FEELIT_PROPRANGE_NOMIN       ((LONG)0x80000000)
#define FEELIT_PROPRANGE_NOMAX       ((LONG)0x7FFFFFFF)


#ifdef __cplusplus
#define MAKE_FEELIT_PROP(prop)    (*(const GUID *)(prop))
#else
#define MAKE_FEELIT_PROP(prop)    ((REFGUID)(prop))
#endif

#define FEELIT_PROP_BUFFERSIZE       MAKE_FEELIT_PROP(1)
#define FEELIT_PROP_AXISMODE         MAKE_FEELIT_PROP(2)
#define FEELIT_PROP_GRANULARITY      MAKE_FEELIT_PROP(3)
#define FEELIT_PROP_RANGE            MAKE_FEELIT_PROP(4)
#define FEELIT_PROP_DEADZONE         MAKE_FEELIT_PROP(5)
#define FEELIT_PROP_SATURATION       MAKE_FEELIT_PROP(6)
#define FEELIT_PROP_FFGAIN           MAKE_FEELIT_PROP(7)
#define FEELIT_PROP_FFLOAD           MAKE_FEELIT_PROP(8)
#define FEELIT_PROP_AUTOCENTER       MAKE_FEELIT_PROP(9)
#define FEELIT_PROP_CALIBRATIONMODE  MAKE_FEELIT_PROP(10)
#define FEELIT_PROP_DEVICEGAIN       MAKE_FEELIT_PROP(11)
#define FEELIT_PROP_BALLISTICS       MAKE_FEELIT_PROP(12)
#define FEELIT_PROP_SCREENSIZE       MAKE_FEELIT_PROP(13)
#define FEELIT_PROP_ABSOLUTEMODE     MAKE_FEELIT_PROP(14)
#define FEELIT_PROP_DEVICEMODE       MAKE_FEELIT_PROP(15)
#define FEELIT_PROP_NUMEFFECTS       MAKE_FEELIT_PROP(16)
#define FEELIT_PROP_DEVICEID         MAKE_FEELIT_PROP(17)

#define FEELIT_PROPAXISMODE_ABS      0
#define FEELIT_PROPAXISMODE_REL      1

#define FEELIT_PROPAUTOCENTER_OFF    0
#define FEELIT_PROPAUTOCENTER_ON     1

#define FEELIT_PROPCALIBRATIONMODE_COOKED    0
#define FEELIT_PROPCALIBRATIONMODE_RAW       1

// Device configuration/control, for use by control panels
typedef struct FEELIT_PROPBALLISTICS {
    FEELIT_PROPHEADER feelitph;	/* Feelit property header struct */
    INT   Sensitivity;			/* Property-specific value */
    INT   LowThreshhold;		/* Property-specific value */
    INT   HighThreshhold;		/* Property-specific value */
} FEELIT_PROPBALLISTICS, *LPFEELIT_PROPBALLISTICS;
typedef const FEELIT_PROPBALLISTICS *LPCFEELIT_PROPBALLISTICS;

typedef struct FEELIT_PROPSCREENSIZE {
    FEELIT_PROPHEADER feelitph;	/* Feelit property header struct */
    DWORD   dwXScreenSize;		/* Max X screen coord value */
    DWORD   dwYScreenSize;		/* Max Y screen coord value */
} FEELIT_PROPSCREENSIZE, *LPFEELIT_PROPSCREENSIZE;
typedef const FEELIT_PROPSCREENSIZE *LPCFEELIT_PROPSCREENSIZE;

typedef struct FEELIT_PROPABSOLUTEMODE {
	FEELIT_PROPHEADER feelitph;	/* Feelit property header struct */
	BOOL bAbsMode;				/* TRUE for Absolute mode FALSE for Relative mode */
} FEELIT_PROPABSOLUTEMODE, *LPFEELIT_PROPABSOLUTEMODE;
typedef const FEELIT_PROPABSOLUTEMODE *LPCFEELIT_PROPABSOLUTEMODE;

#define FEELIT_PROPDEVICEMODE_MOUSE       1			
#define FEELIT_PROPDEVICEMODE_JOYSTICK    2

// Number of effects.  This is the number of effects (not the number of effect types)
// that can be downloaded to a device.  dwTotalEffects includes any caching cababilities
// of the driver.  dwHardwareEffects is strictly the number of effects that can be
// stored in the device hardware.  Pay attention to dwHardwareEffects only if you're
// worried about optimizing for speed.
typedef struct FEELIT_PROPNUMEFFECTS {
    FEELIT_PROPHEADER feelitph;	/* Feelit property header struct */
    DWORD   dwTotalEffects;	    /* Total number of effects a device driver can support */
    DWORD   dwHardwareEffects;	/* Number of effects the device hardware can hold */
} FEELIT_PROPNUMEFFECTS, *LPFEELIT_PROPNUMEFFECTS;
typedef const FEELIT_PROPNUMEFFECTS *LPCFEELIT_PROPNUMEFFECTS;


typedef struct FEELIT_DEVICEOBJECTDATA {
    DWORD   dwOfs;			/* Offset into current data format of this data's object */
    DWORD   dwData;			/* Data obtained from the device */
    DWORD   dwTimeStamp;	/* Tick count, in milliseconds, at which event was generated */
    DWORD   dwSequence;		/* Sequence number for this event */
} FEELIT_DEVICEOBJECTDATA, *LPFEELIT_DEVICEOBJECTDATA;
typedef const FEELIT_DEVICEOBJECTDATA *LPCFEELIT_DEVICEOBJECTDATA;

#define FEELIT_SEQUENCE_COMPARE(dwSequence1, cmp, dwSequence2) \
                        ((int)((dwSequence1) - (dwSequence2)) cmp 0)

/* GetDeviceData Flags */
#define FEELIT_FGETDEVDATA_PEEK          0x00000001

/* Cooperative Level Flags */
#define FEELIT_FCOOPLEVEL_EXCLUSIVE			    0x00000001
#define FEELIT_FCOOPLEVEL_NONEXCLUSIVE		    0x00000002
#define FEELIT_FCOOPLEVEL_FOREGROUND			0x00000004
#define FEELIT_FCOOPLEVEL_BACKGROUND			0x00000008

typedef struct FEELIT_DEVICEINSTANCE {
    DWORD   dwSize;						/* sizeof ( FEELIT_DEVICEINSTANCE ) */
    GUID    guidInstance;				/* Unique id for instance of device */
    GUID    guidProduct;				/* Unique id for the product */
    DWORD   dwDevType;					/* Device type (FEELIT_DEVICETYPE*) */
    CHAR    tszInstanceName[MAX_PATH];	/* Friendly name for the instance (e.g. "Feelit Mouse 1") */
    CHAR    tszProductName[MAX_PATH];	/* Friendly name for the product (e.g. "Feelit Mouse") */
    GUID    guidFFDriver;				/* Unique id for the driver being used for force feedback */
    WORD    wUsagePage;					/* HID usage page code (if the device driver is a HID device) */
    WORD    wUsage;						/* HID usage code (if the device driver is a HID device) */
} FEELIT_DEVICEINSTANCE, *LPFEELIT_DEVICEINSTANCE;
typedef const FEELIT_DEVICEINSTANCE  *LPCFEELIT_DEVICEINSTANCE;

#define FEELIT_FCOMMAND_RESET            0x00000001
#define FEELIT_FCOMMAND_STOPALL          0x00000002
#define FEELIT_FCOMMAND_PAUSE            0x00000004
#define FEELIT_FCOMMAND_CONTINUE         0x00000008
#define FEELIT_FCOMMAND_SETACTUATORSON   0x00000010
#define FEELIT_FCOMMAND_SETACTUATORSOFF  0x00000020

#define FEELIT_FDEVICESTATE_EMPTY            0x00000001
#define FEELIT_FDEVICESTATE_STOPPED          0x00000002
#define FEELIT_FDEVICESTATE_PAUSED           0x00000004
#define FEELIT_FDEVICESTATE_ACTUATORSON      0x00000010
#define FEELIT_FDEVICESTATE_ACTUATORSOFF     0x00000020
#define FEELIT_FDEVICESTATE_POWERON          0x00000040
#define FEELIT_FDEVICESTATE_POWEROFF         0x00000080
#define FEELIT_FDEVICESTATE_SAFETYSWITCHON   0x00000100
#define FEELIT_FDEVICESTATE_SAFETYSWITCHOFF  0x00000200
#define FEELIT_FDEVICESTATE_USERFFSWITCHON   0x00000400
#define FEELIT_FDEVICESTATE_USERFFSWITCHOFF  0x00000800
#define FEELIT_FDEVICESTATE_DEVICELOST       0x80000000

typedef struct FEELIT_EFFECTINFO {
    DWORD   dwSize;				/* sizeof( FEELIT_EFFECTINFO ) */
    GUID    guid;				/* Unique ID of the effect */
    DWORD   dwEffType;			/* Zero or more of FEELIT_FEFFECTTYPE_* */
    DWORD   dwStaticParams;		/* All params supported. Zero or more of FEELIT_FPARAM_* */
    DWORD   dwDynamicParams;	/* Params modifiable while effect playing. (FEELIT_FPARAM_*) */
    CHAR    tszName[MAX_PATH];	/* Name of effect (e.g. "Enclosure" ) */
} FEELIT_EFFECTINFO, *LPFEELIT_EFFECTINFO;
typedef const FEELIT_EFFECTINFO  *LPCFEELIT_EFFECTINFO;

typedef BOOL (FAR PASCAL * LPFEELIT_ENUMEFFECTSCALLBACK)(LPCFEELIT_EFFECTINFO, LPVOID);
typedef BOOL (FAR PASCAL * LPFEELIT_ENUMCREATEDEFFECTOBJECTSCALLBACK)(LPFEELIT_EFFECT, LPVOID);


/*
				Feelit Events

Feelit events are defined using a FEELIT_EVENT struct.  They are created by 
passing the struct to CreateFeelitEvent, which returns an HFEELITEVENT handle.
Feelit notifies clients that Feelit Event has triggered by sending a message to
the window handle associated with the event.  Window handles are associated with
events using the hWndEventHandler param. of the FEELIT_EVENT struct. The window 
message that Feelit sends to notify of an event, contains information in the
WPARAM and LPARAM as described below.

DURING INITIALIZATION:
const UINT g_wmFeelitEvent = RegisterWindowMessage( FEELIT_EVENT_MSG_STRING );

IN MESSAGE LOOP:
if ( msgID == g_wmFeelitEvent )
{
    WORD wRef = LOWORD(wParam);				// 16-bit app-defined event id
    WORD wfTriggers = HIWORD(wParam);		// Trigger Flags
    short xForce = (short) LOWORD(lParam);	// Force applied along X-axis
    short yForce = (short) HIWORD(lParam);	// Force applied along Y-axis
}

*/

#define FEELIT_EVENT_MSG_STRING  "FEELIT_EVENT_MSG"

typedef HANDLE HFEELITEVENT, *LPHFEELITEVENT;		/* Handle type used to manage Feelit Events */

typedef struct FEELIT_EVENT {
   DWORD dwSize;                   /* sizeof(FEELIT_EVENT) */
   HWND hWndEventHandler;			/* Handle of window to which event msgs are sent */
   WORD wRef;						/* 16-bit app-defined value to identify the event to the app */
   DWORD dwEventTriggerMask;		/* Mask specifying events which trigger the callback  (FEELIT_FTRIG*) */
   LPIFEELIT_EFFECT	piEffect;		/* Effect, if any, that this event is associated with */
} FEELIT_EVENT, *LPFEELIT_EVENT;

typedef const FEELIT_EVENT *LPCFEELIT_EVENT;


/* Event Trigger Flags */

#define FEELIT_FTRIG_NONE			0x00000000
#define FEELIT_FTRIG_ENTER 		    0x00000001
#define FEELIT_FTRIG_EXIT      	    0x00000002
#define FEELIT_FTRIG_OUTER		    0x00000004
#define FEELIT_FTRIG_INBOUND		FEELIT_FTRIG_OUTER
#define FEELIT_FTRIG_INNER  	    0x00000008
#define FEELIT_FTRIG_OUTBOUND  	    FEELIT_FTRIG_INNER
#define FEELIT_FTRIG_TOPWALL		0x00000010
#define FEELIT_FTRIG_BOTTOMWALL	    0x00000020
#define FEELIT_FTRIG_LEFTWALL		0x00000040
#define FEELIT_FTRIG_RIGHTWALL	    0x00000080
#define FEELIT_FTRIG_ANYWALL		( FEELIT_FTRIG_TOPWALL | FEELIT_FTRIG_BOTTOMWALL | FEELIT_FTRIG_LEFTWALL | FEELIT_FTRIG_RIGHTWALL )
#define FEELIT_FTRIG_ONENTERANY		( FEELIT_FTRIG_ENTER | FEELIT_FTRIG_ANYWALL )
#define FEELIT_FTRIG_ONEXITANY		( FEELIT_FTRIG_EXIT | FEELIT_FTRIG_ANYWALL )
#define FEELIT_FTRIG_ONOUTERANY		( FEELIT_FTRIG_OUTER | FEELIT_FTRIG_ANYWALL )
#define FEELIT_FTRIG_ONINBOUNDANY	FEELIT_FTRIG_ONOUTERANY
#define FEELIT_FTRIG_ONINNERANY		( FEELIT_FTRIG_INNER | FEELIT_FTRIG_ANYWALL )
#define FEELIT_FTRIG_ONOUTBOUNDANY	FEELIT_FTRIG_ONINNERANY
#define FEELIT_FTRIG_ONANYENCLOSURE ( FEELIT_FTRIG_ONENTERANY | FEELIT_FTRIG_ONEXITANY | FEELIT_FTRIG_ONOUTERANY | FEELIT_FTRIG_ONINNERANY )

#define FEELIT_FTRIG_ONSCROLL			0x0000100
#define FEELIT_FTRIG_ONEFFECTCOMPLETION	0x0000200

#undef INTERFACE
#define INTERFACE IFeelitDevice

DECLARE_INTERFACE_(IFeelitDevice, IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;

    /*** IFeelitDevice methods ***/
    STDMETHOD(GetCapabilities)(THIS_ LPFEELIT_DEVCAPS) PURE;
    STDMETHOD(EnumObjects)(THIS_ LPFEELIT_ENUMDEVICEOBJECTSCALLBACK,LPVOID,DWORD) PURE;
    STDMETHOD(GetProperty)(THIS_ REFGUID,LPFEELIT_PROPHEADER) PURE;
    STDMETHOD(SetProperty)(THIS_ REFGUID,LPCFEELIT_PROPHEADER) PURE;
    STDMETHOD(Acquire)(THIS) PURE;
    STDMETHOD(Unacquire)(THIS) PURE;
    STDMETHOD(GetDeviceState)(THIS_ DWORD,LPVOID) PURE;
    STDMETHOD(GetDeviceData)(THIS_ DWORD,LPFEELIT_DEVICEOBJECTDATA,LPDWORD,DWORD) PURE;
    STDMETHOD(SetDataFormat)(THIS_ LPCFEELIT_DATAFORMAT) PURE;
    STDMETHOD(SetEventNotification)(THIS_ HANDLE) PURE;
    STDMETHOD(SetCooperativeLevel)(THIS_ HWND,DWORD) PURE;
    STDMETHOD(GetObjectInfo)(THIS_ LPFEELIT_DEVICEOBJECTINSTANCE,DWORD,DWORD) PURE;
    STDMETHOD(GetDeviceInfo)(THIS_ LPFEELIT_DEVICEINSTANCE) PURE;
    STDMETHOD(RunControlPanel)(THIS_ HWND,DWORD) PURE;
    STDMETHOD(CreateEffect)(THIS_ LPCFEELIT_EFFECT,LPIFEELIT_EFFECT *,LPUNKNOWN) PURE;
    STDMETHOD(EnumEffects)(THIS_ LPFEELIT_ENUMEFFECTSCALLBACK,LPVOID,DWORD) PURE;
    STDMETHOD(GetEffectInfo)(THIS_ LPFEELIT_EFFECTINFO,REFGUID) PURE;
    STDMETHOD(GetForceFeedbackState)(THIS_ LPDWORD) PURE;
    STDMETHOD(SendForceFeedbackCommand)(THIS_ DWORD) PURE;
    STDMETHOD(EnumCreatedEffectObjects)(THIS_ LPFEELIT_ENUMCREATEDEFFECTOBJECTSCALLBACK,LPVOID,DWORD) PURE;
    STDMETHOD(Escape)(THIS_ LPFEELIT_EFFESCAPE) PURE;
    STDMETHOD(Poll)(THIS) PURE;
    STDMETHOD(SendDeviceData)(THIS_ DWORD,LPFEELIT_DEVICEOBJECTDATA,LPDWORD,DWORD) PURE;
    STDMETHOD(CreateFeelitEvent)(THIS_ LPCFEELIT_EVENT,LPHFEELITEVENT) PURE;
    STDMETHOD(DestroyFeelitEvent)(THIS_ HFEELITEVENT) PURE;
    STDMETHOD(EnableFeelitEvent)(THIS_ HFEELITEVENT,BOOL) PURE;
    STDMETHOD(SetEventNotificationPeriodicity)(THIS_ DWORD) PURE;
};

typedef struct IFeelitDevice *LPIFEELIT_DEVICE;

#if !defined(__cplusplus) || defined(CINTERFACE)
#define IFeelitDevice_QueryInterface(p,a,b) (p)->lpVtbl->QueryInterface(p,a,b)
#define IFeelitDevice_AddRef(p) (p)->lpVtbl->AddRef(p)
#define IFeelitDevice_Release(p) (p)->lpVtbl->Release(p)
#define IFeelitDevice_GetCapabilities(p,a) (p)->lpVtbl->GetCapabilities(p,a)
#define IFeelitDevice_EnumObjects(p,a,b,c) (p)->lpVtbl->EnumObjects(p,a,b,c)
#define IFeelitDevice_GetProperty(p,a,b) (p)->lpVtbl->GetProperty(p,a,b)
#define IFeelitDevice_SetProperty(p,a,b) (p)->lpVtbl->SetProperty(p,a,b)
#define IFeelitDevice_Acquire(p) (p)->lpVtbl->Acquire(p)
#define IFeelitDevice_Unacquire(p) (p)->lpVtbl->Unacquire(p)
#define IFeelitDevice_GetDeviceState(p,a,b) (p)->lpVtbl->GetDeviceState(p,a,b)
#define IFeelitDevice_GetDeviceData(p,a,b,c,d) (p)->lpVtbl->GetDeviceData(p,a,b,c,d)
#define IFeelitDevice_SetDataFormat(p,a) (p)->lpVtbl->SetDataFormat(p,a)
#define IFeelitDevice_SetEventNotification(p,a) (p)->lpVtbl->SetEventNotification(p,a)
#define IFeelitDevice_SetCooperativeLevel(p,a,b) (p)->lpVtbl->SetCooperativeLevel(p,a,b)
#define IFeelitDevice_GetObjectInfo(p,a,b,c) (p)->lpVtbl->GetObjectInfo(p,a,b,c)
#define IFeelitDevice_GetDeviceInfo(p,a) (p)->lpVtbl->GetDeviceInfo(p,a)
#define IFeelitDevice_RunControlPanel(p,a,b) (p)->lpVtbl->RunControlPanel(p,a,b)
#define IFeelitDevice_CreateEffect(p,a,b,c,d) (p)->lpVtbl->CreateEffect(p,a,b,c,d)
#define IFeelitDevice_EnumEffects(p,a,b,c) (p)->lpVtbl->EnumEffects(p,a,b,c)
#define IFeelitDevice_GetEffectInfo(p,a,b) (p)->lpVtbl->GetEffectInfo(p,a,b)
#define IFeelitDevice_GetForceFeedbackState(p,a) (p)->lpVtbl->GetForceFeedbackState(p,a)
#define IFeelitDevice_SendForceFeedbackCommand(p,a) (p)->lpVtbl->SendForceFeedbackCommand(p,a)
#define IFeelitDevice_EnumCreatedEffectObjects(p,a,b,c) (p)->lpVtbl->EnumCreatedEffectObjects(p,a,b,c)
#define IFeelitDevice_Escape(p,a) (p)->lpVtbl->Escape(p,a)
#define IFeelitDevice_Poll(p) (p)->lpVtbl->Poll(p)
#define IFeelitDevice_SendDeviceData(p,a,b,c,d) (p)->lpVtbl->SendDeviceData(p,a,b,c,d)
#define IFeelitDevice_CreateFeelitEvent(p,a,b) (p)->lpVtbl->CreateFeelitEvent(p,a,b)
#define IFeelitDevice_DestroyFeelitEvent(p,a) (p)->lpVtbl->DestroyFeelitEvent(p,a)
#define IFeelitDevice_EnableFeelitEvent(p,a,b) (p)->lpVtbl->EnableFeelitEvent(p,a,b)
#define IFeelitDevice_SetEventNotificationPeriodicity(p,a) (p)->lpVtbl->SetEventNotificationPeriodicity(p,a)
#else
#define IFeelitDevice_QueryInterface(p,a,b) (p)->QueryInterface(a,b)
#define IFeelitDevice_AddRef(p) (p)->AddRef()
#define IFeelitDevice_Release(p) (p)->Release()
#define IFeelitDevice_GetCapabilities(p,a) (p)->GetCapabilities(a)
#define IFeelitDevice_EnumObjects(p,a,b,c) (p)->EnumObjects(a,b,c)
#define IFeelitDevice_GetProperty(p,a,b) (p)->GetProperty(a,b)
#define IFeelitDevice_SetProperty(p,a,b) (p)->SetProperty(a,b)
#define IFeelitDevice_Acquire(p) (p)->Acquire()
#define IFeelitDevice_Unacquire(p) (p)->Unacquire()
#define IFeelitDevice_GetDeviceState(p,a,b) (p)->GetDeviceState(a,b)
#define IFeelitDevice_GetDeviceData(p,a,b,c,d) (p)->GetDeviceData(a,b,c,d)
#define IFeelitDevice_SetDataFormat(p,a) (p)->SetDataFormat(a)
#define IFeelitDevice_SetEventNotification(p,a) (p)->SetEventNotification(a)
#define IFeelitDevice_SetCooperativeLevel(p,a,b) (p)->SetCooperativeLevel(a,b)
#define IFeelitDevice_GetObjectInfo(p,a,b,c) (p)->GetObjectInfo(a,b,c)
#define IFeelitDevice_GetDeviceInfo(p,a) (p)->GetDeviceInfo(a)
#define IFeelitDevice_RunControlPanel(p,a,b) (p)->RunControlPanel(a,b)
#define IFeelitDevice_CreateEffect(p,a,b,c,d) (p)->CreateEffect(a,b,c,d)
#define IFeelitDevice_EnumEffects(p,a,b,c) (p)->EnumEffects(a,b,c)
#define IFeelitDevice_GetEffectInfo(p,a,b) (p)->GetEffectInfo(a,b)
#define IFeelitDevice_GetForceFeedbackState(p,a) (p)->GetForceFeedbackState(a)
#define IFeelitDevice_SendForceFeedbackCommand(p,a) (p)->SendForceFeedbackCommand(a)
#define IFeelitDevice_EnumCreatedEffectObjects(p,a,b,c) (p)->EnumCreatedEffectObjects(a,b,c)
#define IFeelitDevice_Escape(p,a) (p)->Escape(a)
#define IFeelitDevice_Poll(p) (p)->Poll()
#define IFeelitDevice_SendDeviceData(p,a,b,c,d) (p)->SendDeviceData(a,b,c,d)
#define IFeelitDevice_CreateFeelitEvent(p,a,b) (p)->CreateFeelitEvent(a,b)
#define IFeelitDevice_DestroyFeelitEvent(p,a) (p)->DestroyFeelitEvent(a)
#define IFeelitDevice_EnableFeelitEvent(p,a,b) (p)->EnableFeelitEvent(a,b)
#define IFeelitDevice_SetEventNotificationPeriodicity(p,a) (p)->SetEventNotificationPeriodicity(a)
#endif

/****************************************************************************
 *
 *      Mouse State
 *
 ****************************************************************************/

typedef struct _FEELIT_MOUSESTATE {
    LONG    lXpos;
    LONG    lYpos;
    LONG    lZpos;
    LONG    lXforce;
    LONG    lYforce;
    LONG    lZforce;
    BYTE    rgbButtons[4];
} FEELIT_MOUSESTATE, *LPFEELIT_MOUSESTATE;

#define FEELIT_MOUSEOFFSET_XAXIS		FIELD_OFFSET(FEELIT_MOUSESTATE, lXpos)
#define FEELIT_MOUSEOFFSET_YAXIS     FIELD_OFFSET(FEELIT_MOUSESTATE, lYpos)
#define FEELIT_MOUSEOFFSET_ZAXIS     FIELD_OFFSET(FEELIT_MOUSESTATE, lZpos)
#define FEELIT_MOUSEOFFSET_XFORCE    FIELD_OFFSET(FEELIT_MOUSESTATE, lXforce)
#define FEELIT_MOUSEOFFSET_YFORCE    FIELD_OFFSET(FEELIT_MOUSESTATE, lYforce)
#define FEELIT_MOUSEOFFSET_ZFORCE    FIELD_OFFSET(FEELIT_MOUSESTATE, lZforce)
#define FEELIT_MOUSEOFFSET_BUTTON0	(FIELD_OFFSET(FEELIT_MOUSESTATE, rgbButtons) + 0)
#define FEELIT_MOUSEOFFSET_BUTTON1	(FIELD_OFFSET(FEELIT_MOUSESTATE, rgbButtons) + 1)
#define FEELIT_MOUSEOFFSET_BUTTON2	(FIELD_OFFSET(FEELIT_MOUSESTATE, rgbButtons) + 2)
#define FEELIT_MOUSEOFFSET_BUTTON3	(FIELD_OFFSET(FEELIT_MOUSESTATE, rgbButtons) + 3)


/****************************************************************************
 *
 *  IFeelit
 *
 ****************************************************************************/

#define FEELIT_ENUM_STOP             0
#define FEELIT_ENUM_CONTINUE         1

typedef BOOL (FAR PASCAL * LPFEELIT_ENUMDEVICESCALLBACK)(LPCFEELIT_DEVICEINSTANCE, LPVOID);

#define FEELIT_FENUMDEV_ALLDEVICES       0x00000000
#define FEELIT_FENUMDEV_ATTACHEDONLY     0x00000001
#define FEELIT_FENUMDEV_FORCEFEEDBACK    0x00000100

#undef INTERFACE
#define INTERFACE IFeelit

DECLARE_INTERFACE_(IFeelit, IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;

    /*** IFeelit methods ***/
    STDMETHOD(CreateDevice)(THIS_ REFGUID,LPIFEELIT_DEVICE *,LPUNKNOWN) PURE;
    STDMETHOD(EnumDevices)(THIS_ DWORD,LPFEELIT_ENUMDEVICESCALLBACK,LPVOID,DWORD) PURE;
    STDMETHOD(GetDeviceStatus)(THIS_ REFGUID) PURE;
    STDMETHOD(RunControlPanel)(THIS_ HWND,DWORD) PURE;
    STDMETHOD(Initialize)(THIS_ HINSTANCE,DWORD) PURE;
};

typedef struct IFeelit *LPIFEELIT;

#if !defined(__cplusplus) || defined(CINTERFACE)
#define IFeelit_QueryInterface(p,a,b) (p)->lpVtbl->QueryInterface(p,a,b)
#define IFeelit_AddRef(p) (p)->lpVtbl->AddRef(p)
#define IFeelit_Release(p) (p)->lpVtbl->Release(p)
#define IFeelit_CreateDevice(p,a,b,c) (p)->lpVtbl->CreateDevice(p,a,b,c)
#define IFeelit_EnumDevices(p,a,b,c,d) (p)->lpVtbl->EnumDevices(p,a,b,c,d)
#define IFeelit_GetDeviceStatus(p,a) (p)->lpVtbl->GetDeviceStatus(p,a)
#define IFeelit_RunControlPanel(p,a,b) (p)->lpVtbl->RunControlPanel(p,a,b)
#define IFeelit_Initialize(p,a,b) (p)->lpVtbl->Initialize(p,a,b)
#define IFeelit_FindDevice(p,a,b,c) (p)->lpVtbl->FindDevice(p,a,b,c)
#endif

extern HRESULT WINAPI FeelitCreateA(HINSTANCE hinst, DWORD dwVersion, LPIFEELIT *ppFeelit, LPUNKNOWN punkOuter);
#define FeelitCreate FeelitCreateA


/****************************************************************************
 *
 *  Return Codes
 *
 ****************************************************************************/

/*
 *  The operation completed successfully.
 */
#define FEELIT_RESULT_OK                           S_OK

/*
 *  The device exists but is not currently attached.
 */
#define FEELIT_RESULT_NOTATTACHED                  S_FALSE

/*
 *  The device buffer overflowed.  Some input was lost.
 */
#define FEELIT_RESULT_BUFFEROVERFLOW               S_FALSE

/*
 *  The change in device properties had no effect.
 */
#define FEELIT_RESULT_PROPNOEFFECT                 S_FALSE

/*
 *  The operation had no effect.
 */
#define FEELIT_RESULT_NOEFFECT                     S_FALSE

/*
 *  The device is a polled device.  As a result, device buffering
 *  will not collect any data and event notifications will not be
 *  signalled until GetDeviceState is called.
 */
#define FEELIT_RESULT_POLLEDDEVICE                 ((HRESULT)0x00000002L)

/*
 *  The parameters of the effect were successfully updated by
 *  IFeelitEffect::SetParameters, but the effect was not
 *  downloaded because the device is not exclusively acquired
 *  or because the FEELIT_FPARAM_NODOWNLOAD flag was passed.
 */
#define FEELIT_RESULT_DOWNLOADSKIPPED              ((HRESULT)0x00000003L)

/*
 *  The parameters of the effect were successfully updated by
 *  IFeelitEffect::SetParameters, but in order to change
 *  the parameters, the effect needed to be restarted.
 */
#define FEELIT_RESULT_EFFECTRESTARTED              ((HRESULT)0x00000004L)

/*
 *  The parameters of the effect were successfully updated by
 *  IFeelitEffect::SetParameters, but some of them were
 *  beyond the capabilities of the device and were truncated.
 */
#define FEELIT_RESULT_TRUNCATED                    ((HRESULT)0x00000008L)

/*
 *  Equal to FEELIT_RESULT_EFFECTRESTARTED | FEELIT_RESULT_TRUNCATED.
 */
#define FEELIT_RESULT_TRUNCATEDANDRESTARTED        ((HRESULT)0x0000000CL)

/*
 *  The application requires a newer version of Feelit.
 */
#define FEELIT_ERROR_OLDFEELITVERSION     \
    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_OLD_WIN_VERSION)

/*
 *  The application was written for an unsupported prerelease version
 *  of Feelit.
 */
#define FEELIT_ERROR_BETAFEELITVERSION    \
    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_RMODE_APP)

/*
 *  The object could not be created due to an incompatible driver version
 *  or mismatched or incomplete driver components.
 */
#define FEELIT_ERROR_BADDRIVERVER              \
    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_BAD_DRIVER_LEVEL)

/*
 * The device or device instance or effect is not registered with Feelit.
 */
#define FEELIT_ERROR_DEVICENOTREG              REGDB_E_CLASSNOTREG

/*
 * The requested object does not exist.
 */
#define FEELIT_ERROR_NOTFOUND                  \
    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_FILE_NOT_FOUND)

/*
 * The requested object does not exist.
 */
#define FEELIT_ERROR_OBJECTNOTFOUND            \
    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_FILE_NOT_FOUND)

/*
 * An invalid parameter was passed to the returning function,
 * or the object was not in a state that admitted the function
 * to be called.
 */
#define FEELIT_ERROR_INVALIDPARAM              E_INVALIDARG

/*
 * The specified interface is not supported by the object
 */
#define FEELIT_ERROR_NOINTERFACE               E_NOINTERFACE

/*
 * An undetermined error occured inside the Feelit subsystem
 */
#define FEELIT_ERROR_GENERIC                   E_FAIL

/*
 * The Feelit subsystem couldn't allocate sufficient memory to complete the
 * caller's request.
 */
#define FEELIT_ERROR_OUTOFMEMORY               E_OUTOFMEMORY

/*
 * The function called is not supported at this time
 */
#define FEELIT_ERROR_UNSUPPORTED               E_NOTIMPL

/*
 * This object has not been initialized
 */
#define FEELIT_ERROR_NOTINITIALIZED            \
    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_NOT_READY)

/*
 * This object is already initialized
 */
#define FEELIT_ERROR_ALREADYINITIALIZED        \
    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_ALREADY_INITIALIZED)

/*
 * This object does not support aggregation
 */
#define FEELIT_ERROR_NOAGGREGATION             CLASS_E_NOAGGREGATION

/*
 * Another app has a higher priority level, preventing this call from
 * succeeding.
 */
#define FEELIT_ERROR_OTHERAPPHASPRIO           E_ACCESSDENIED

/*
 * Access to the device has been lost.  It must be re-acquired.
 */
#define FEELIT_ERROR_INPUTLOST                 \
    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_READ_FAULT)

/*
 * The operation cannot be performed while the device is acquired.
 */
#define FEELIT_ERROR_ACQUIRED                  \
    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_BUSY)

/*
 * The operation cannot be performed unless the device is acquired.
 */
#define FEELIT_ERROR_NOTACQUIRED               \
    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_INVALID_ACCESS)

/*
 * The specified property cannot be changed.
 */
#define FEELIT_ERROR_READONLY                  E_ACCESSDENIED

/*
 * The device already has an event notification associated with it.
 */
#define FEELIT_ERROR_HANDLEEXISTS              E_ACCESSDENIED

/*
 * Data is not yet available.
 */
#ifndef E_PENDING
#define E_PENDING                       0x80070007L
#endif

/*
 * Unable to perform the requested operation because the user
 * does not have sufficient privileges.
 */
#define FEELIT_ERROR_INSUFFICIENTPRIVS         0x80040200L

/*
 * The device is full.
 */
#define FEELIT_ERROR_DEVICEFULL                0x80040201L

/*
 * Not all the requested information fit into the buffer.
 */
#define FEELIT_ERROR_MOREDATA                  0x80040202L

/*
 * The effect is not downloaded.
 */
#define FEELIT_ERROR_NOTDOWNLOADED             0x80040203L

/*
 *  The device cannot be reinitialized because there are still effects
 *  attached to it.
 */
#define FEELIT_ERROR_HASEFFECTS                0x80040204L

/*
 *  The operation cannot be performed unless the device is acquired
 *  in FEELIT_FCOOPLEVEL_EXCLUSIVE mode.
 */
#define FEELIT_ERROR_NOTEXCLUSIVEACQUIRED      0x80040205L

/*
 *  The effect could not be downloaded because essential information
 *  is missing.  For example, no axes have been associated with the
 *  effect, or no type-specific information has been created.
 */
#define FEELIT_ERROR_INCOMPLETEEFFECT          0x80040206L

/*
 *  Attempted to read buffered device data from a device that is
 *  not buffered.
 */
#define FEELIT_ERROR_NOTBUFFERED               0x80040207L

/*
 *  An attempt was made to modify parameters of an effect while it is
 *  playing.  Not all hardware devices support altering the parameters
 *  of an effect while it is playing.
 */
#define FEELIT_ERROR_EFFECTPLAYING             0x80040208L

/*
 *  An internal error occurred (inside the API or the driver)
 */
#define FEELIT_ERROR_INTERNAL             0x80040209L

/*
 *  Effect set referenced by a command is not the active set
 */
#define FEELIT_ERROR_INACTIVE             0x8004020AL

#ifdef __cplusplus
};
#endif


#endif  /* __FEELITAPI_INCLUDED__ */

