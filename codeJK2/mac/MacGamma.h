/*
	File:		MacGamma.h

	Contains:	Functions to enable Mac OS device gamma adjustments using Windows common 3 channel 256 element 8 bit gamma ramps

	Written by:	Geoff Stahl

	Copyright:	Copyright © 1999 Apple Computer, Inc., All Rights Reserved

	Change History (most recent first):

         <4>     5/20/99    GGS     Updated function names.
         <3>     5/20/99    GGS     Cleaned up and commented
         <2>     5/20/99    GGS     Added system wide get and restore gamma functions to enable
                                    restoration of original for all devices.  Modified functionality
                                    to return pointers vice squirreling away the memory.
         <1>     5/20/99    GGS     Initial Add
*/



// include control --------------------------------------------------

#ifndef MacGamma_h
#define MacGamma_h



// includes ---------------------------------------------------------

#include <Quickdraw.h>
#include <QDOffscreen.h>



// structures/classes -----------------------------------------------
	
typedef struct 											// storage for device handle and gamma table
{
	GDHandle hGD;												// handle to device
	GammaTblPtr pDeviceGamma;									// pointer to device gamma table
} recDeviceGamma;
typedef recDeviceGamma * precDeviceGamma;

typedef struct											// storage for system devices and gamma tables
{
	short numDevices;											// number of devices
	precDeviceGamma * devGamma;									// array of pointers to device gamma records
} recSystemGamma;
typedef recSystemGamma * precSystemGamma;



// function declarations --------------------------------------------

// 5/20/99: (GGS) changed functional specification
OSErr GetGammaTable(GDHandle gd, GammaTblPtr * ppTableGammaOut);		// Returns the device gamma table pointer in ppDeviceTable
Ptr CreateEmptyGammaTable (short channels, short entries, short bits);	// creates an empty gamma table of a given size, assume no formula data will be used
Ptr CopyGammaTable (GammaTblPtr pTableGammaIn);							// given a pointer toa device gamma table properly iterates and copies
void DisposeGammaTable (Ptr pGamma);									// disposes gamma table returned from GetGammaTable, GetDeviceGamma, or CopyGammaTable 

Ptr GetDeviceGamma (GDHandle hGD);								// returns pointer to copy of orginal device gamma table in native format
void RestoreDeviceGamma (GDHandle hGD, Ptr pGammaTable);				// sets device to saved table

// 5/20/99: (GGS) added system wide gamma get and restore
Ptr GetSystemGammas (void);										// returns a pointer to a set of all current device gammas in native format
																	// (returns NULL on failure, which means reseting gamma will not be possible)
void RestoreSystemGammas (Ptr pSystemGammas);					// restores all system devices to saved gamma setting
void DisposeSystemGammas (Ptr* ppSystemGammas);					// iterates through and deletes stored gamma settings

Boolean GetDeviceGammaRampGD (GDHandle hGD, Ptr pRamp);			// retrieves the gamma ramp from a graphics device (pRamp: 3 arrays of 256 elements each)
Boolean GetDeviceGammaRampGW (GWorldPtr pGW, Ptr pRamp);		// retrieves the gamma ramp from a graphics device associated with a GWorld pointer (pRamp: 3 arrays of 256 elements each)
Boolean GetDeviceGammaRampCGP (CGrafPtr pGraf, Ptr pRamp);		// retrieves the gamma ramp from a graphics device associated with a CGraf pointer (pRamp: 3 arrays of 256 elements each)


Boolean SetDeviceGammaRampGD (GDHandle hGD, Ptr pRamp);			// sets the gamma ramp for a graphics device (pRamp: 3 arrays of 256 elements each (R,G,B))
Boolean SetDeviceGammaRampGW (GWorldPtr pGW, Ptr pRamp);		// sets the gamma ramp for a graphics device associated with a GWorld pointer (pRamp: 3 arrays of 256 elements each (R,G,B))
Boolean SetDeviceGammaRampCGP (CGrafPtr pGraf, Ptr pRamp);		// sets the gamma ramp for a graphics device associated with a CGraf pointer (pRamp: 3 arrays of 256 elements each (R,G,B))



#endif // MacGamma_h