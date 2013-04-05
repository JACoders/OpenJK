/*
	File:		MacGamma.cpp

	Contains:	Functions to enable Mac OS device gamma adjustments using Windows common 3 channel 256 element 8 bit gamma ramps

	Written by:	Geoff Stahl

	Copyright:	Copyright © 1999 Apple Computer, Inc., All Rights Reserved

	Change History (most recent first):

         <4>     5/20/99    GGS     Added handling for gamma tables with different data widths,
                                    number of entries, and channels.  Forced updates to 3 channels
                                    (poss. could break on rare card, but very unlikely).  Added
                                    quick update with BlockMove for 3x256x8 tables. Updated function
                                    names.
         <3>     5/20/99    GGS     Cleaned up and commented
         <2>     5/20/99    GGS     Added system wide get and restore gamma functions to enable
                                    restoration of original for all devices.  Modified functionality
                                    to return pointers vice squirreling away the memory.
         <1>     5/20/99    GGS     Initial Add
*/



// system includes ----------------------------------------------------------

#include <Devices.h>
#include <Files.h>
#include <MacTypes.h>
#include <QDOffscreen.h>
#include <Quickdraw.h>
#include <video.h>



// project includes ---------------------------------------------------------

#include "MacGamma.h"



// functions (external/public) ----------------------------------------------

// GetRawDeviceGamma

// Returns the device gamma table pointer in ppDeviceTable

OSErr GetGammaTable (GDHandle hGD, GammaTblPtr * ppTableGammaOut)
{
	VDGammaRecord   DeviceGammaRec;
	CntrlParam		cParam;
	OSErr			err;
	
	cParam.ioCompletion = NULL;										// set up control params
	cParam.ioNamePtr = NULL;
	cParam.ioVRefNum = 0;
	cParam.ioCRefNum = (**hGD).gdRefNum;
	cParam.csCode = cscGetGamma;									// Get Gamma commnd to device
	*(Ptr *)cParam.csParam = (Ptr) &DeviceGammaRec;					// record for gamma

	err = PBStatus( (ParmBlkPtr)&cParam, 0 );						// get gamma
	
	*ppTableGammaOut = (GammaTblPtr)(DeviceGammaRec.csGTable);		// pull table out of record
	
	return err;	
}

// --------------------------------------------------------------------------

// CreateEmptyGammaTable

// creates an empty gamma table of a given size, assume no formula data will be used

Ptr CreateEmptyGammaTable (short channels, short entries, short bits)
{
	GammaTblPtr		pTableGammaOut = NULL;
	short			tableSize, dataWidth;

	dataWidth = (bits + 7) / 8;										// number of bytes per entry
	tableSize = sizeof (GammaTbl) + (channels * entries * dataWidth);
	pTableGammaOut = (GammaTblPtr) NewPtrClear (tableSize);			// allocate new tabel

	if (pTableGammaOut)												// if we successfully allocated
	{
		pTableGammaOut->gVersion = 0;								// set parameters based on input
		pTableGammaOut->gType = 0;
		pTableGammaOut->gFormulaSize = 0;
		pTableGammaOut->gChanCnt = channels;
		pTableGammaOut->gDataCnt = entries;
		pTableGammaOut->gDataWidth = bits;
	}
	return (Ptr)pTableGammaOut;										// return whatever we allocated
}

// --------------------------------------------------------------------------

// CopyGammaTable

// given a pointer toa device gamma table properly iterates and copies

Ptr CopyGammaTable (GammaTblPtr pTableGammaIn)
{
	GammaTblPtr		pTableGammaOut = NULL;
	short			tableSize, dataWidth;

	if (pTableGammaIn)												// if there is a table to copy 
	{
		dataWidth = (pTableGammaIn->gDataWidth + 7) / 8;			// number of bytes per entry
		tableSize = sizeof (GammaTbl) + pTableGammaIn->gFormulaSize +
					(pTableGammaIn->gChanCnt * pTableGammaIn->gDataCnt * dataWidth);
		pTableGammaOut = (GammaTblPtr) NewPtr (tableSize);			// allocate new table
		if (pTableGammaOut)											
			BlockMove( (Ptr)pTableGammaIn, (Ptr)pTableGammaOut, tableSize);	// move everything
	}
	return (Ptr)pTableGammaOut;										// return whatever we allocated, could be NULL
}

// --------------------------------------------------------------------------

// DisposeGammaTable

// disposes gamma table returned from GetGammaTable, GetDeviceGamma, or CopyGammaTable 
// 5/20/99: (GGS) added

void DisposeGammaTable (Ptr pGamma)
{
	if (pGamma)
		DisposePtr((Ptr) pGamma);									// get rid of it
}

// --------------------------------------------------------------------------

// GetDeviceGamma

// returns pointer to copy of orginal device gamma table in native format (allocates memory for gamma table, call DisposeDeviceGamma to delete)
// 5/20/99: (GGS) change spec to return the allocated pointer vice storing internally

Ptr GetDeviceGamma (GDHandle hGD)
{
	GammaTblPtr		pTableGammaDevice = NULL;
	GammaTblPtr		pTableGammaReturn = NULL;	
	OSErr			err;
	
	err = GetGammaTable (hGD, &pTableGammaDevice);					// get a pointer to the devices table
	if ((err == noErr) && pTableGammaDevice)						// if succesful
		pTableGammaReturn = (GammaTblPtr) CopyGammaTable (pTableGammaDevice); // copy to global

	return (Ptr) pTableGammaReturn;
}

// --------------------------------------------------------------------------

// RestoreDeviceGamma

// sets device to saved table
// 5/20/99: (GGS) now does not delete table, avoids confusion

void RestoreDeviceGamma (GDHandle hGD, Ptr pGammaTable)
{
	VDSetEntryRecord setEntriesRec;
	VDGammaRecord	gameRecRestore;
	CTabHandle      hCTabDeviceColors;
	Ptr				csPtr;
	OSErr			err = noErr;
	
	if (pGammaTable)												// if we have a table to restore								
	{
		gameRecRestore.csGTable = pGammaTable;						// setup restore record
		csPtr = (Ptr) &gameRecRestore;
		err = Control((**hGD).gdRefNum, cscSetGamma, (Ptr) &csPtr);	// restore gamma
		
		if ((err == noErr) && ((**(**hGD).gdPMap).pixelSize == 8))	// if successful and on an 8 bit device
		{
			hCTabDeviceColors = (**(**hGD).gdPMap).pmTable;			// do SetEntries to force CLUT update
			setEntriesRec.csTable = (ColorSpec *) &(**hCTabDeviceColors).ctTable;
			setEntriesRec.csStart = 0;
			setEntriesRec.csCount = (**hCTabDeviceColors).ctSize;
			csPtr = (Ptr) &setEntriesRec;
			
			err = Control((**hGD).gdRefNum, cscSetEntries, (Ptr) &csPtr); // SetEntries in CLUT
		}
	}
}

// --------------------------------------------------------------------------

// GetSystemGammas

// returns a pointer to a set of all current device gammas in native format (returns NULL on failure, which means reseting gamma will not be possible)
// 5/20/99: (GGS) added

Ptr GetSystemGammas (void)										
{
	precSystemGamma pSysGammaOut;									// return pointer to system device gamma info
	short devCount = 0;												// number of devices attached
	Boolean fail = false;

	pSysGammaOut = (precSystemGamma) NewPtr (sizeof (recSystemGamma)); // allocate for structure
	
	GDHandle hGDevice = GetDeviceList ();							// top of device list
	do																// iterate
	{
		devCount++;													// count devices					
		hGDevice = GetNextDevice (hGDevice);						// next device
	} while (hGDevice);
	
	pSysGammaOut->devGamma = (precDeviceGamma *) NewPtr (sizeof (precDeviceGamma) * devCount); // allocate for array of pointers to device records
	if (pSysGammaOut)
	{
		pSysGammaOut->numDevices = devCount;						// stuff count
		
		devCount = 0;												// reset iteration
		hGDevice = GetDeviceList ();
		do
		{
			pSysGammaOut->devGamma [devCount] = (precDeviceGamma) NewPtr (sizeof (recDeviceGamma));	  // new device record
			if (pSysGammaOut->devGamma [devCount])					// if we actually allocated memory
			{
				pSysGammaOut->devGamma [devCount]->hGD = hGDevice;										  // stuff handle
				pSysGammaOut->devGamma [devCount]->pDeviceGamma = (GammaTblPtr)GetDeviceGamma (hGDevice); // copy gamma table
			}
			else													// otherwise dump record on exit
			 fail = true;
			devCount++;												// next device
			hGDevice = GetNextDevice (hGDevice);						
		} while (hGDevice);
	}
	if (!fail)														// if we did not fail
		return (Ptr) pSysGammaOut;									// return pointer to structure
	else
	{
		DisposeSystemGammas (&(Ptr)pSysGammaOut);					// otherwise dump the current structures (dispose does error checking)
		return NULL;												// could not complete
	}
}

// --------------------------------------------------------------------------

// RestoreSystemGammas

// restores all system devices to saved gamma setting
// 5/20/99: (GGS) added

void RestoreSystemGammas (Ptr pSystemGammas)
{
	precSystemGamma pSysGammaIn = (precSystemGamma) pSystemGammas;
	if (pSysGammaIn)
		for (short i = 0; i < pSysGammaIn->numDevices; i++)			// for all devices
			RestoreDeviceGamma (pSysGammaIn->devGamma [i]->hGD, (Ptr) pSysGammaIn->devGamma [i]->pDeviceGamma);	// restore gamma
}

// --------------------------------------------------------------------------

// DisposeSystemGammas

// iterates through and deletes stored gamma settings
// 5/20/99: (GGS) added

void DisposeSystemGammas (Ptr* ppSystemGammas)
{
	precSystemGamma pSysGammaIn;
	if (ppSystemGammas)
	{
		pSysGammaIn = (precSystemGamma) *ppSystemGammas;
		if (pSysGammaIn)
		{
			for (short i = 0; i < pSysGammaIn->numDevices; i++)		// for all devices
				if (pSysGammaIn->devGamma [i])						// if pointer is valid
				{
					DisposeGammaTable ((Ptr) pSysGammaIn->devGamma [i]->pDeviceGamma); // dump gamma table
					DisposePtr ((Ptr) pSysGammaIn->devGamma [i]);						 // dump device info
				}
			DisposePtr ((Ptr) pSysGammaIn->devGamma);				// dump device pointer array		
			DisposePtr ((Ptr) pSysGammaIn);							// dump system structure
			*ppSystemGammas = NULL;
		}	
	}
}

// --------------------------------------------------------------------------

// GetDeviceGammaRampGD

// retrieves the gamma ramp from a graphics device (pRamp: 3 arrays of 256 elements each)

Boolean GetDeviceGammaRampGD (GDHandle hGD, Ptr pRamp)
{
	GammaTblPtr		pTableGammaTemp = NULL;
	long 			indexChan, indexEntry;
	OSErr			err;
	
	if (pRamp)															// ensure pRamp is allocated
	{
		err = GetGammaTable (hGD, &pTableGammaTemp);					// get a pointer to the current gamma
		if ((err == noErr) && pTableGammaTemp)							// if successful
		{															
			// fill ramp
			unsigned char * pEntry = (unsigned char *)&pTableGammaTemp->gFormulaData + pTableGammaTemp->gFormulaSize; // base of table
			short bytesPerEntry = (pTableGammaTemp->gDataWidth + 7) / 8; // size, in bytes, of the device table entries
			short shiftRightValue = pTableGammaTemp->gDataWidth - 8; 	 // number of right shifts device -> ramp
			short channels = pTableGammaTemp->gChanCnt;	
			short entries = pTableGammaTemp->gDataCnt;									
			if (channels == 3)											// RGB format
			{															// note, this will create runs of entries if dest. is bigger (not linear interpolate)
				for (indexChan = 0; indexChan < channels; indexChan++)
					for (indexEntry = 0; indexEntry < 256; indexEntry++)
						*((unsigned char *)pRamp + (indexChan << 8) + indexEntry) = 
						  *(pEntry + (indexChan * entries * bytesPerEntry) + indexEntry * ((entries * bytesPerEntry) >> 8)) >> shiftRightValue;
			}
			else														// single channel format
			{
				for (indexEntry = 0; indexEntry < 256; indexEntry++)	// for all entries set vramp value
					for (indexChan = 0; indexChan < channels; indexChan++)	// repeat for all channels
						*((unsigned char *)pRamp + (indexChan << 8) + indexEntry) = 
						  *(pEntry + ((indexEntry * entries * bytesPerEntry) >> 8)) >> shiftRightValue;
			}
			return true;
		}
	}
	return false;
}

// --------------------------------------------------------------------------

// GetDeviceGammaRampGW

// retrieves the gamma ramp from a graphics device associated with a GWorld pointer (pRamp: 3 arrays of 256 elements each)

Boolean GetDeviceGammaRampGW (GWorldPtr pGW, Ptr pRamp)
{
	GDHandle hGD = GetGWorldDevice (pGW);
	return GetDeviceGammaRampGD (hGD, pRamp);
}

// --------------------------------------------------------------------------

// GetDeviceGammaRampCGP

// retrieves the gamma ramp from a graphics device associated with a CGraf pointer (pRamp: 3 arrays of 256 elements each)

Boolean GetDeviceGammaRampCGP (CGrafPtr pGraf, Ptr pRamp)
{
	CGrafPtr pGrafSave;
	GDHandle hGDSave;
	GetGWorld (&pGrafSave, &hGDSave);
	SetGWorld (pGraf, NULL);
	GDHandle hGD = GetGDevice ();
	Boolean fResult = GetDeviceGammaRampGD (hGD, pRamp);
	SetGWorld (pGrafSave, hGDSave);
	return fResult;
}

// --------------------------------------------------------------------------

// SetDeviceGammaRampGD

// sets the gamma ramp for a graphics device (pRamp: 3 arrays of 256 elements each (R,G,B))

Boolean SetDeviceGammaRampGD (GDHandle hGD, Ptr pRamp)
{
	VDSetEntryRecord setEntriesRec;
	VDGammaRecord	gameRecRestore;
	GammaTblPtr		pTableGammaNew;
	GammaTblPtr		pTableGammaCurrent = NULL;
	CTabHandle      hCTabDeviceColors;
	Ptr				csPtr;
	OSErr			err;
	short 			dataBits, entries, channels = 3;						// force three channels in the gamma table
	
	if (pRamp)																// ensure pRamp is allocated
	{
		err= GetGammaTable (hGD, &pTableGammaCurrent);						// get pointer to current table
		if ((err == noErr) && pTableGammaCurrent)
		{
			dataBits = pTableGammaCurrent->gDataWidth;						// table must have same data width
			entries = pTableGammaCurrent->gDataCnt;							// table must be same size
			pTableGammaNew = (GammaTblPtr) CreateEmptyGammaTable (channels, entries, dataBits); // our new table
			if (pTableGammaNew)												// if successful fill table
			{	
				unsigned char * pGammaBase = (unsigned char *)&pTableGammaNew->gFormulaData + pTableGammaNew->gFormulaSize; // base of table
				if (entries == 256 && dataBits == 8)						// simple case: direct mapping
					BlockMove ((Ptr)pRamp, (Ptr)pGammaBase, channels * entries); // move everything
				else														// tough case handle entry, channel and data size disparities
				{
					short bytesPerEntry = (dataBits + 7) / 8; 				// size, in bytes, of the device table entries
					short shiftRightValue = 8 - dataBits;					// number of right shifts ramp -> device
					shiftRightValue += ((bytesPerEntry - 1) * 8);  			// multibyte entries and the need to map a byte at a time most sig. to least sig.
					for (short indexChan = 0; indexChan < channels; indexChan++) // for all the channels
						for (short indexEntry = 0; indexEntry < entries; indexEntry++) // for all the entries
						{
							short currentShift = shiftRightValue;			// reset current bit shift
							long temp = *((unsigned char *)pRamp + (indexChan << 8) + (indexEntry << 8) / entries); // get data from ramp
							for (short indexByte = 0; indexByte < bytesPerEntry; indexByte++) // for all bytes
							{
								if (currentShift < 0)						// shift data correctly for current byte
									*(pGammaBase++) = temp << -currentShift;
								else
									*(pGammaBase++) = temp >> currentShift;
								currentShift -= 8;							// increment shift to align to next less sig. byte
							}
						}
				}
				
				// set gamma
				gameRecRestore.csGTable = (Ptr) pTableGammaNew;				// setup restore record
				csPtr = (Ptr) &gameRecRestore;
				err = Control((**hGD).gdRefNum, cscSetGamma, (Ptr) &csPtr);	// restore gamma
				
				if (((**(**hGD).gdPMap).pixelSize == 8) && (err == noErr))	// if successful and on an 8 bit device
				{
					hCTabDeviceColors = (**(**hGD).gdPMap).pmTable;			// do SetEntries to force CLUT update
					setEntriesRec.csTable = (ColorSpec *) &(**hCTabDeviceColors).ctTable;
					setEntriesRec.csStart = 0;
					setEntriesRec.csCount = (**hCTabDeviceColors).ctSize;
					csPtr = (Ptr) &setEntriesRec;
					err = Control((**hGD).gdRefNum, cscSetEntries, (Ptr) &csPtr);	// SetEntries in CLUT
				}
				DisposeGammaTable ((Ptr) pTableGammaNew);					// dump table
				if (err == noErr)
					return true;
			}
		}
	}
	else																	// set NULL gamma -> results in linear map
	{
		gameRecRestore.csGTable = (Ptr) NULL;								// setup restore record
		csPtr = (Ptr) &gameRecRestore;
		err = Control((**hGD).gdRefNum, cscSetGamma, (Ptr) &csPtr);			// restore gamma
		
		if (((**(**hGD).gdPMap).pixelSize == 8) && (err == noErr))			// if successful and on an 8 bit device
		{
			hCTabDeviceColors = (**(**hGD).gdPMap).pmTable;					// do SetEntries to force CLUT update
			setEntriesRec.csTable = (ColorSpec *) &(**hCTabDeviceColors).ctTable;
			setEntriesRec.csStart = 0;
			setEntriesRec.csCount = (**hCTabDeviceColors).ctSize;
			csPtr = (Ptr) &setEntriesRec;
			err = Control((**hGD).gdRefNum, cscSetEntries, (Ptr) &csPtr);	// SetEntries in CLUT
		}
		if (err == noErr)
			return true;
	}
	return false;															// memory allocation or device control failed if we get here
}

// --------------------------------------------------------------------------

// SetDeviceGammaRampGW

// sets the gamma ramp for a graphics device associated with a GWorld pointer (pRamp: 3 arrays of 256 elements each (R,G,B))

Boolean SetDeviceGammaRampGW (GWorldPtr pGW, Ptr pRamp)
{
	GDHandle hGD = GetGWorldDevice (pGW);
	return SetDeviceGammaRampGD (hGD, pRamp);
}

// --------------------------------------------------------------------------

// SetDeviceGammaRampCGP

// sets the gamma ramp for a graphics device associated with a CGraf pointer (pRamp: 3 arrays of 256 elements each (R,G,B))

Boolean SetDeviceGammaRampCGP (CGrafPtr pGraf, Ptr pRamp)
{
	CGrafPtr pGrafSave;
	GDHandle hGDSave;
	GetGWorld (&pGrafSave, &hGDSave);
	SetGWorld (pGraf, NULL);
	GDHandle hGD = GetGDevice ();
	Boolean fResult = SetDeviceGammaRampGD (hGD, pRamp);
	SetGWorld (pGrafSave, hGDSave);
	return fResult;
}