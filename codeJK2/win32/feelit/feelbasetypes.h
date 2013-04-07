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

  FILE:		FeelBaseTypes.h

  PURPOSE:	Base Types for Feelit API Foundation Classes

  STARTED:	10/29/97

  NOTES/REVISIONS:
     3/2/99 jrm (Jeff Mallett): Force-->Feel renaming

**********************************************************************/


#if !defined(AFX_FEELBASETYPES_H__135B88C4_4175_11D1_B049_0020AF30269A__INCLUDED_)
#define AFX_FEELBASETYPES_H__135B88C4_4175_11D1_B049_0020AF30269A__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

//#include <windows.h>
#include "FeelitApi.h"

#ifndef FFC_VERSION
 #define FFC_VERSION         0x0100
#endif

#if (FFC_VERSION >= 0x0110)
 #define FFC_START_DELAY
 #define FFC_EFFECT_CACHING
#endif

//	These are defined in FEELitAPI.h
//
//	#define FEELIT_DEVICETYPE_DEVICE    1
//	#define FEELIT_DEVICETYPE_MOUSE     2
//	#define FEELIT_DEVICETYPE_HID       0x00010000
//
//	Add define for DirectInput Device emulating FEELit Device
#define FEELIT_DEVICETYPE_DIRECTINPUT	3


//================================================================
// TYPE WRAPPERS
//================================================================

//
// FEEL --> FEELIT Wrappers
//
#define FEEL_DEVICETYPE_DEVICE	FEELIT_DEVICETYPE_DEVICE
#define FEEL_DEVICETYPE_MOUSE	FEELIT_DEVICETYPE_MOUSE
#define FEEL_DEVICETYPE_DIRECTINPUT	FEELIT_DEVICETYPE_DIRECTINPUT

#define FEEL_EFFECT		FEELIT_EFFECT
#define LPFEEL_EFFECT	LPFEELIT_EFFECT
#define LPCFEEL_EFFECT	LPCFEELIT_EFFECT

#define FEEL_CONDITION     FEELIT_CONDITION
#define LPFEEL_CONDITION   LPFEELIT_CONDITION
#define LPCFEEL_CONDITION  LPCFEELIT_CONDITION

#define FEEL_TEXTURE       FEELIT_TEXTURE
#define LPFEEL_TEXTURE     LPFEELIT_TEXTURE
#define LPCFEEL_TEXTURE    LPCFEELIT_TEXTURE

#define FEEL_PERIODIC      FEELIT_PERIODIC
#define LPFEEL_PERIODIC    LPFEELIT_PERIODIC
#define LPCFEEL_PERIODIC   LPCFEELIT_PERIODIC

#define FEEL_CONSTANTFORCE     FEELIT_CONSTANTFORCE
#define LPFEEL_CONSTANTFORCE   LPFEELIT_CONSTANTFORCE
#define LPCFEEL_CONSTANTFORCE  LPCFEELIT_CONSTANTFORCE

#define FEEL_RAMPFORCE     FEELIT_RAMPFORCE
#define LPFEEL_RAMPFORCE   LPFEELIT_RAMPFORCE
#define LPCFEEL_RAMPFORCE  LPCFEELIT_RAMPFORCE

#define FEEL_ENVELOPE      FEELIT_ENVELOPE
#define LPFEEL_ENVELOPE    LPFEELIT_ENVELOPE
#define LPCFEEL_ENVELOPE   LPCFEELIT_ENVELOPE

#define LPIFEEL_API        LPIFEELIT
#define LPIFEEL_EFFECT     LPIFEELIT_EFFECT
#define LPIFEEL_DEVICE     LPIFEELIT_DEVICE

#define LPFEEL_DEVICEINSTANCE              LPFEELIT_DEVICEINSTANCE       
#define LPCFEEL_DEVICEOBJECTINSTANCE       LPCFEELIT_DEVICEOBJECTINSTANCE
#define LPCFEEL_EFFECTINFO                 LPCFEELIT_EFFECTINFO          


#define FEEL_FPARAM_DURATION               FEELIT_FPARAM_DURATION             
#define FEEL_FPARAM_SAMPLEPERIOD           FEELIT_FPARAM_SAMPLEPERIOD         
#define FEEL_FPARAM_GAIN                   FEELIT_FPARAM_GAIN                 
#define FEEL_FPARAM_TRIGGERBUTTON          FEELIT_FPARAM_TRIGGERBUTTON        
#define FEEL_FPARAM_TRIGGERREPEATINTERVAL  FEELIT_FPARAM_TRIGGERREPEATINTERVAL
#define FEEL_FPARAM_AXES                   FEELIT_FPARAM_AXES                 
#define FEEL_FPARAM_DIRECTION              FEELIT_FPARAM_DIRECTION            
#define FEEL_FPARAM_ENVELOPE               FEELIT_FPARAM_ENVELOPE             
#define FEEL_FPARAM_TYPESPECIFICPARAMS     FEELIT_FPARAM_TYPESPECIFICPARAMS   
#define FEEL_FPARAM_ALLPARAMS              FEELIT_FPARAM_ALLPARAMS            
#define FEEL_FPARAM_START                  FEELIT_FPARAM_START                
#define FEEL_FPARAM_NORESTART              FEELIT_FPARAM_NORESTART            
#define FEEL_FPARAM_NODOWNLOAD             FEELIT_FPARAM_NODOWNLOAD           

#define FEEL_FEFFECT_OBJECTIDS             FEELIT_FEFFECT_OBJECTIDS      
#define FEEL_FEFFECT_OBJECTOFFSETS         FEELIT_FEFFECT_OBJECTOFFSETS  
#define FEEL_FEFFECT_CARTESIAN             FEELIT_FEFFECT_CARTESIAN      
#define FEEL_FEFFECT_POLAR                 FEELIT_FEFFECT_POLAR          
#define FEEL_FEFFECT_SPHERICAL             FEELIT_FEFFECT_SPHERICAL      

#define FEEL_PARAM_NOTRIGGER               FEELIT_PARAM_NOTRIGGER

#define FEEL_MOUSEOFFSET_XAXIS             FEELIT_MOUSEOFFSET_XAXIS
#define FEEL_MOUSEOFFSET_YAXIS             FEELIT_MOUSEOFFSET_YAXIS
#define FEEL_MOUSEOFFSET_ZAXIS             FEELIT_MOUSEOFFSET_ZAXIS

//
// FORCE --> FEELIT Wrappers
//
#define FORCE_EFFECT FEELIT_EFFECT
#define LPFORCE_EFFECT LPFEELIT_EFFECT
#define LPCFORCE_EFFECT LPCFEELIT_EFFECT

#define FORCE_CONDITION     FEELIT_CONDITION
#define LPFORCE_CONDITION   LPFEELIT_CONDITION
#define LPCFORCE_CONDITION  LPCFEELIT_CONDITION

#define FORCE_TEXTURE       FEELIT_TEXTURE
#define LPFORCE_TEXTURE     LPFEELIT_TEXTURE
#define LPCFORCE_TEXTURE    LPCFEELIT_TEXTURE

#define FORCE_PERIODIC      FEELIT_PERIODIC
#define LPFORCE_PERIODIC    LPFEELIT_PERIODIC
#define LPCFORCE_PERIODIC   LPCFEELIT_PERIODIC

#define FORCE_CONSTANTFORCE     FEELIT_CONSTANTFORCE
#define LPFORCE_CONSTANTFORCE   LPFEELIT_CONSTANTFORCE
#define LPCFORCE_CONSTANTFORCE  LPCFEELIT_CONSTANTFORCE

#define FORCE_RAMPFORCE     FEELIT_RAMPFORCE
#define LPFORCE_RAMPFORCE   LPFEELIT_RAMPFORCE
#define LPCFORCE_RAMPFORCE  LPCFEELIT_RAMPFORCE

#define FORCE_ENVELOPE      FEELIT_ENVELOPE
#define LPFORCE_ENVELOPE    LPFEELIT_ENVELOPE
#define LPCFORCE_ENVELOPE   LPCFEELIT_ENVELOPE

#define LPIFORCE_API        LPIFEELIT
#define LPIFORCE_EFFECT     LPIFEELIT_EFFECT
#define LPIFORCE_DEVICE     LPIFEELIT_DEVICE

#define LPFORCE_DEVICEINSTANCE              LPFEELIT_DEVICEINSTANCE       
#define LPCFORCE_DEVICEOBJECTINSTANCE       LPCFEELIT_DEVICEOBJECTINSTANCE
#define LPCFORCE_EFFECTINFO                 LPCFEELIT_EFFECTINFO          


#define FORCE_FPARAM_DURATION               FEELIT_FPARAM_DURATION             
#define FORCE_FPARAM_SAMPLEPERIOD           FEELIT_FPARAM_SAMPLEPERIOD         
#define FORCE_FPARAM_GAIN                   FEELIT_FPARAM_GAIN                 
#define FORCE_FPARAM_TRIGGERBUTTON          FEELIT_FPARAM_TRIGGERBUTTON        
#define FORCE_FPARAM_TRIGGERREPEATINTERVAL  FEELIT_FPARAM_TRIGGERREPEATINTERVAL
#define FORCE_FPARAM_AXES                   FEELIT_FPARAM_AXES                 
#define FORCE_FPARAM_DIRECTION              FEELIT_FPARAM_DIRECTION            
#define FORCE_FPARAM_ENVELOPE               FEELIT_FPARAM_ENVELOPE             
#define FORCE_FPARAM_TYPESPECIFICPARAMS     FEELIT_FPARAM_TYPESPECIFICPARAMS   
#define FORCE_FPARAM_ALLPARAMS              FEELIT_FPARAM_ALLPARAMS            
#define FORCE_FPARAM_START                  FEELIT_FPARAM_START                
#define FORCE_FPARAM_NORESTART              FEELIT_FPARAM_NORESTART            
#define FORCE_FPARAM_NODOWNLOAD             FEELIT_FPARAM_NODOWNLOAD           

#define FORCE_FEFFECT_OBJECTIDS             FEELIT_FEFFECT_OBJECTIDS      
#define FORCE_FEFFECT_OBJECTOFFSETS         FEELIT_FEFFECT_OBJECTOFFSETS  
#define FORCE_FEFFECT_CARTESIAN             FEELIT_FEFFECT_CARTESIAN      
#define FORCE_FEFFECT_POLAR                 FEELIT_FEFFECT_POLAR          
#define FORCE_FEFFECT_SPHERICAL             FEELIT_FEFFECT_SPHERICAL      

#define FORCE_PARAM_NOTRIGGER               FEELIT_PARAM_NOTRIGGER

#define FORCE_MOUSEOFFSET_XAXIS             FEELIT_MOUSEOFFSET_XAXIS
#define FORCE_MOUSEOFFSET_YAXIS             FEELIT_MOUSEOFFSET_YAXIS
#define FORCE_MOUSEOFFSET_ZAXIS             FEELIT_MOUSEOFFSET_ZAXIS


//================================================================
// GUID WRAPPERS
//================================================================

//
// Feel --> Feelit Wrappers
//
#define GUID_Feel_ConstantForce	GUID_Feelit_ConstantForce
#define GUID_Feel_RampForce		GUID_Feelit_RampForce
#define GUID_Feel_Square		GUID_Feelit_Square
#define GUID_Feel_Sine			GUID_Feelit_Sine
#define GUID_Feel_Triangle		GUID_Feelit_Triangle
#define GUID_Feel_SawtoothUp	GUID_Feelit_SawtoothUp
#define GUID_Feel_SawtoothDown	GUID_Feelit_SawtoothDown
#define GUID_Feel_Spring		GUID_Feelit_Spring
#define GUID_Feel_DeviceSpring	GUID_Feelit_DeviceSpring
#define GUID_Feel_Damper		GUID_Feelit_Damper
#define GUID_Feel_Inertia		GUID_Feelit_Inertia
#define GUID_Feel_Friction		GUID_Feelit_Friction
#define GUID_Feel_Texture		GUID_Feelit_Texture
#define GUID_Feel_Grid			GUID_Feelit_Grid
#define GUID_Feel_Enclosure		GUID_Feelit_Enclosure
#define GUID_Feel_Ellipse		GUID_Feelit_Ellipse
#define GUID_Feel_CustomForce	GUID_Feelit_CustomForce

//
// Force --> Feelit Wrappers
//
#define GUID_Force_ConstantForce   GUID_Feelit_ConstantForce
#define GUID_Force_RampForce       GUID_Feelit_RampForce
#define GUID_Force_Square          GUID_Feelit_Square
#define GUID_Force_Sine            GUID_Feelit_Sine
#define GUID_Force_Triangle        GUID_Feelit_Triangle
#define GUID_Force_SawtoothUp      GUID_Feelit_SawtoothUp
#define GUID_Force_SawtoothDown    GUID_Feelit_SawtoothDown
#define GUID_Force_Spring          GUID_Feelit_Spring
#define GUID_Force_Damper          GUID_Feelit_Damper
#define GUID_Force_Inertia         GUID_Feelit_Inertia
#define GUID_Force_Friction        GUID_Feelit_Friction
#define GUID_Force_Texture         GUID_Feelit_Texture
#define GUID_Force_Grid            GUID_Feelit_Grid
#define GUID_Force_Enclosure       GUID_Feelit_Enclosure
#define GUID_Force_Ellipse         GUID_Feelit_Ellipse
#define GUID_Force_CustomForce     GUID_Feelit_CustomForce


#endif // !defined(AFX_FEELBASETYPES_H__135B88C4_4175_11D1_B049_0020AF30269A__INCLUDED_)

















