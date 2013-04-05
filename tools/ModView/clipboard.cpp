// Filename:-	clipboard.cpp
//
#include "stdafx.h"
#include "includes.h"
//#include <windows.h>
//#include <stdio.h>
//#include <assert.h>
//#include "oddbits.h"
#include "text.h"
//
#include "clipboard.h"



#define APP_HWND AppVars.hWnd



BOOL ClipBoard_SendDIB(LPVOID pvData, int iBytes)
{
	HGLOBAL hXferBuffer = GlobalAlloc((UINT)GMEM_MOVEABLE|GMEM_DDESHARE,(DWORD)iBytes);	
	if (hXferBuffer)
	{
   		char *psLockedDest = (char*) GlobalLock(hXferBuffer);	
		memcpy(psLockedDest,pvData,iBytes);
		GlobalUnlock(psLockedDest);	
	
		if (OpenClipboard(APP_HWND))
		{
			EmptyClipboard();												// empty it (all handles to NULL);
			if((SetClipboardData((UINT)CF_DIB,hXferBuffer))==NULL)
			{
				CloseClipboard();												
				ErrorBox("ClipBoard_SendDIB(): Dammit, some sort of problem writing to the clipboard...");
				return FALSE;	// hmmmm... Oh well.
			}
			CloseClipboard();
			return TRUE;
		}
	}

	ErrorBox(va("ClipBoard_SendDIB(): Dammit, I can't allocate %d bytes for some strange reason (reboot, then try again, else tell me - Ste)",iBytes));
	return FALSE;			
}


BOOL Clipboard_SendString(LPCSTR psString)
{
	HGLOBAL hXferBuffer = GlobalAlloc((UINT)GMEM_MOVEABLE|GMEM_DDESHARE,(DWORD)strlen(psString)+1);	
	if (hXferBuffer)
	{
   		char *psLockedDest = (char*) GlobalLock(hXferBuffer);	
		memcpy(psLockedDest,psString,strlen(psString)+1);
		GlobalUnlock(psLockedDest);	
	
		if (OpenClipboard(APP_HWND))
		{
			EmptyClipboard();												// empty it (all handles to NULL);
			if((SetClipboardData((UINT)CF_TEXT,hXferBuffer))==NULL)
			{
				CloseClipboard();												
				ErrorBox("Clipboard_SendString(): Dammit, some sort of problem writing to the clipboard...");
				return FALSE;	// hmmmm... Oh well.
			}
			CloseClipboard();
			return TRUE;
		}
	}

	ErrorBox(va("Clipboard_SendString(): Dammit, I can't allocate %d bytes for some strange reason (reboot, then try again, else tell me - Ste)",strlen(psString)+1));
	return FALSE;			
}








////////////////////////////////////////////////////////////////////////
//
// from this point on is a bunch of crap that's not strictly clipboard stuff, 
//	but is used only in conjunction with it and therefore may as well go here.
//
//	This also includes another some more BMP code. This is here purely because
//	we need to be able to write BMPs, not just read them as the other code does...
//


#include <pshpack1.h>
typedef struct
{
	BYTE r,g,b;	// R&B different order to windoze's RGBTRIPLE struct
} GLRGBBYTES,*LPGLRGBBYTES;
#include <poppack.h>

typedef struct
{		
	BITMAPINFOHEADER BMPInfoHeader;
	RGBTRIPLE RGBData[1];	// a label just for addressing purposes, the actual array size depends on screen dims
} MEMORYBMP, *PMEMORYBMP;


static bool BMP_FlipTrueColour(LPCSTR psFilename);


static int iBMP_PixelWriteOffset;
static PMEMORYBMP pBMP = NULL;
static int iBMP_MallocSize;
//
static FILE *fhBMP = 0;


bool BMP_GetMemDIB(void *&pvAddress, int &iBytes)
{
	if (pBMP)
	{
		pvAddress = pBMP;
		iBytes = iBMP_MallocSize;
		return true;
	}

	return false;
}


void BMP_Free(void)
{
	if (pBMP)
	{
		free(pBMP);
		pBMP = NULL;
	}	
}



// open 24-bit RGB file either to disk or memory
//
// if psFileName == NULL, open memory file instead
//
static bool BMP_Open(LPCSTR psFilename, int iWidth, int iHeight)
{
	BITMAPFILEHEADER BMPFileHeader;
	BITMAPINFOHEADER BMPInfoHeader;

	int iPadBytes	= (4-((iWidth * sizeof(RGBTRIPLE))%4))&3;
	int iWidthBytes =     (iWidth * sizeof(RGBTRIPLE))+iPadBytes;
		
	BMP_Free();
	fhBMP = NULL;
	
	if (psFilename)
	{
		extern void CreatePath (const char *path);
		CreatePath(psFilename);

		fhBMP = fopen(psFilename,"wb");
		if (!(int)fhBMP)
			return false;
	}
	else
	{
		iBMP_MallocSize = sizeof(BITMAPINFOHEADER) + (iWidthBytes * iHeight);
		pBMP = (PMEMORYBMP) malloc ( iBMP_MallocSize );
		if (!pBMP)
			return false;
	}

	memset(&BMPFileHeader, 0, sizeof(BITMAPFILEHEADER));
	BMPFileHeader.bfType=(WORD)('B'+256*'M');
//	int iPad= ((sizeof(RGBTRIPLE)*iWidth)%3)*iHeight;	
//	BMPFileHeader.bfSize=sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER)+(sizeof(RGBTRIPLE)*iWidth*iHeight);//+iPad;
	BMPFileHeader.bfSize=sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER)+(iWidthBytes * iHeight);

	BMPFileHeader.bfOffBits=sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER);	// No palette
	
	if (fhBMP)
	{
		fwrite	(&BMPFileHeader,sizeof(BMPFileHeader),1,fhBMP);
	}
	else
	{
		// memory version doesn't use the BITMAPFILEHEADER structure
	}

	memset(&BMPInfoHeader, 0, sizeof(BITMAPINFOHEADER));
	BMPInfoHeader.biSize=sizeof(BITMAPINFOHEADER);
	BMPInfoHeader.biWidth=iWidth;
	BMPInfoHeader.biHeight=iHeight;
	BMPInfoHeader.biPlanes=1;
	BMPInfoHeader.biBitCount=24;
	BMPInfoHeader.biCompression=BI_RGB; 
	BMPInfoHeader.biSizeImage=0;// BMPFileHeader.bfSize - (sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER));	// allowed for BI_RGB bitmaps
	BMPInfoHeader.biXPelsPerMeter=0;	// don't know about these
	BMPInfoHeader.biYPelsPerMeter=0;
	BMPInfoHeader.biClrUsed=0;
	BMPInfoHeader.biClrImportant=0;

	if (fhBMP)
	{
		fwrite	(&BMPInfoHeader,sizeof(BMPInfoHeader),1,fhBMP);
	}
	else
	{
		pBMP->BMPInfoHeader = BMPInfoHeader;	// struct copy
		iBMP_PixelWriteOffset = 0;
	}

	return true;
}

static bool BMP_WritePixel(byte Red, byte Green, byte Blue)
{
	RGBTRIPLE Trip = {0,0,0};

	Trip.rgbtRed	= Red;
	Trip.rgbtGreen	= Green;
	Trip.rgbtBlue	= Blue;

	if (fhBMP)
	{
		fwrite(&Trip, sizeof(RGBTRIPLE), 1, fhBMP);
	}
	else
	{
		RGBTRIPLE *pDest = (RGBTRIPLE *) ((byte *)pBMP->RGBData + iBMP_PixelWriteOffset);

		*pDest = Trip;

		iBMP_PixelWriteOffset += sizeof(RGBTRIPLE);
	}

	return true;
}

// BMP files need padding to 4-byte boundarys after writing each scan line... (which sucks, and messes up pixel indexing)
//
static bool BMP_WriteLinePadding(int iPixelsPerLine)
{
	static char cPad[4]={0};

	int iPadBytes = (4-((iPixelsPerLine * sizeof(RGBTRIPLE))%4))&3;

	if (iPadBytes)
	{
		if (fhBMP)
		{
			fwrite( &cPad, iPadBytes, 1, fhBMP);
		}
		else
		{
			iBMP_PixelWriteOffset += iPadBytes;	// <g>, can't be bothered padding with zeroes
		}
	}
	return true;
}

// BMP files are stored upside down, but if we're writing this out as a result of doing an OpenGL pixel read, then
//	the src buffer will be upside down anyway, so I added this flip-bool -Ste
//
// (psFilename can be NULL for mem files)
//
static bool BMP_Close(LPCSTR psFilename, bool bFlipFinal)
{
	if (fhBMP)
	{
		fclose (fhBMP);
		fhBMP = NULL;
	}
	else
	{
		#if 1

		int iPadBytes	= (4-((pBMP->BMPInfoHeader.biWidth * sizeof(RGBTRIPLE))%4))&3;
		int iWidthBytes =     (pBMP->BMPInfoHeader.biWidth * sizeof(RGBTRIPLE))+iPadBytes;

		assert(iBMP_PixelWriteOffset == iWidthBytes * pBMP->BMPInfoHeader.biHeight);
		assert((iBMP_PixelWriteOffset + (int)sizeof(BITMAPINFOHEADER)) == iBMP_MallocSize);

		#endif
	}

	if (bFlipFinal)
	{
		if (psFilename)
		{
			if (!BMP_FlipTrueColour(psFilename))
				return false;
		}
	}

	return true;
}


static bool BMP_FlipTrueColour(LPCSTR psFilename)
{
	BITMAPFILEHEADER BMPFileHeader;
	BITMAPINFOHEADER BMPInfoHeader;
	RGBTRIPLE *RGBTriples, *tTopLine, *tBottomLine;//, *AfterLastLine;
	BYTE  *byTopLine, *byBottomLine, *byAfterLastLine;
	RGBTRIPLE Trip;
	int x,y;
	int iPadBytes,iRealWidth;


// reopen it to flip it
	fhBMP=fopen(psFilename,"rb");	// checked fopen
	if (!(int)fhBMP)
		return false;

	fread	(&BMPFileHeader,sizeof(BMPFileHeader),1,fhBMP);
	fread	(&BMPInfoHeader,sizeof(BMPInfoHeader),1,fhBMP);
	iPadBytes = (4-((BMPInfoHeader.biWidth * sizeof(RGBTRIPLE))%4))&3;
	iRealWidth=(sizeof(RGBTRIPLE)*BMPInfoHeader.biWidth)+iPadBytes;

	RGBTriples=(RGBTRIPLE *)malloc(iRealWidth*BMPInfoHeader.biHeight);
	fread	(RGBTriples,iRealWidth*BMPInfoHeader.biHeight,1,fhBMP);

	fclose (fhBMP);

	byTopLine=(BYTE *)RGBTriples;
	byAfterLastLine=byTopLine+iRealWidth*BMPInfoHeader.biHeight;

// actually flip it
	for (y=0;	y<BMPInfoHeader.biHeight/2;	y++)
	{
		byBottomLine=byAfterLastLine-((y+1)*iRealWidth);

		tTopLine=(RGBTRIPLE *)byTopLine;
		tBottomLine=(RGBTRIPLE *)byBottomLine;

		for (x=0;	x<BMPInfoHeader.biWidth;	x++)
		{
			Trip=tTopLine[x];
			tTopLine[x]=tBottomLine[x];
			tBottomLine[x]=Trip;
		}

		byTopLine+=iRealWidth;
	}

	
// rewrite it flipped
	fhBMP=fopen(psFilename,"wb");	// checked fopen
	if (!(int)fhBMP)
		return false;
	fwrite	(&BMPFileHeader,sizeof(BMPFileHeader),1,fhBMP);
	fwrite	(&BMPInfoHeader,sizeof(BMPInfoHeader),1,fhBMP);
	fwrite	(RGBTriples,(iRealWidth)*BMPInfoHeader.biHeight,1,fhBMP);
	fclose (fhBMP);
	free(RGBTriples);

	return true;
}












// if psFilename == NULL, takes a memory screenshot in DIB format (for copying to clipboard)
//
bool ScreenShot(LPCSTR psFilename,			// else NULL = take memory snapshot (for clipboard)
				LPCSTR psCopyrightMessage,	// /* = NULL */
				int iWidth,					// /* = <screenwidth>  */
				int iHeight					// /* = <screenheight> */
				)
{
	bool bReturn = false;

	int iOldPack;	
	glGetIntegerv(GL_PACK_ALIGNMENT,&iOldPack);
	glPixelStorei(GL_PACK_ALIGNMENT,1);
	
	void *pvGLPixels = malloc (iWidth * iHeight * 3);	// 3 = R,G,B

	if (pvGLPixels)
	{
		if (psCopyrightMessage)
		{
			bool bOldInhibit = gbTextInhibit;
			gbTextInhibit = false;
			Text_DisplayFlat(psCopyrightMessage, 0, (iHeight-TEXT_DEPTH)-1,255,255,255);	// y-1 = aesthetic only
			gbTextInhibit = bOldInhibit;
		}

		glReadPixels(	0,					// x
						0,					// y (from bottom left)
						iWidth,				// width
						iHeight,			// height
						GL_RGB,				// format
						GL_UNSIGNED_BYTE,	// type
						pvGLPixels			// buffer ptr 			
						);

		// save area is valid size...
		//
		if (BMP_Open(psFilename, iWidth, iHeight))
		{
			for (int y=0; y<iHeight; y++)				
			{
				LPGLRGBBYTES 
				lpGLRGBBytes = (LPGLRGBBYTES) pvGLPixels;
				lpGLRGBBytes+= y * iWidth;

				for (int x=0; x<iWidth; x++, lpGLRGBBytes++)
				{
					BMP_WritePixel(lpGLRGBBytes->r,lpGLRGBBytes->g,lpGLRGBBytes->b);
				}

				BMP_WriteLinePadding(iWidth);	// arg is # pixels per row	
			}

			BMP_Close(psFilename,false);	// false = bFlipFinal			

			bReturn = true;
		}

		free(pvGLPixels);
		pvGLPixels = NULL;	// yeah...yeah
	}	

	glPixelStorei(GL_PACK_ALIGNMENT,iOldPack);

	return bReturn;
}


//////////////// eof ////////////////

