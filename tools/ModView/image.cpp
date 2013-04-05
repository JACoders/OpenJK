// Filename:-	image.cpp
//

#include "stdafx.h"
#include "includes.h"
#include "modview.h"
#include "textures.h"
//
#include "image.h"

#ifndef DWORD
#define DWORD unsigned long
#endif


#define iDSTAMP_CELL_PIXDIM			16	// adjust this, higher = more secure but less stamps on picture
#define iDSTAMP_CELL_PIXELS			(iDSTAMP_CELL_PIXDIM * iDSTAMP_CELL_PIXDIM)
#define iDSTAMP_INSTANCE_CELLDIM	8	// leave this alone at all times!
#define iDSTAMP_INSTANCE_PIXDIM		(iDSTAMP_INSTANCE_CELLDIM * iDSTAMP_CELL_PIXDIM)
#define iDSTAMP_CHAR_BITS			7



#define iHIDDENBIT		1//32	// 1,2,4,8,16,32,64, or 128
#define iHIDDENBITSHIFT 0//5	// 0,1,2,3, 4  5  6       7

#pragma pack(push,1)
	typedef	struct
	{
		char	sHDR[3];		// "HDR", no trailing zero
		char	sDDMMYY[6];		// date/month/year, no trailing zero
		char	sText[14];		// text, no end trailing zero, but maybe early one
	} WatermarkData_t;
	typedef struct
	{
		WatermarkData_t Data;
		DWORD	dwCRC;
	} Watermark_t;
	typedef struct
	{
		byte R,G,B,A;
	}Pixel_t;
#pragma pack(pop)

typedef struct
{
	byte *pPixels;
	int iWidth;
	int iHeight;
	int iPlanes;
} DStampImage_t;

typedef struct
{
	DStampImage_t *pImage;
	Watermark_t	*pWatermark;
} DStampData_t;

Watermark_t TheWatermark;

#define PixelSetR(p,r) p.R=r
#define PixelSetG(p,g) p.G=g
#define PixelSetB(p,b) p.B=b
#define PixelSetA(p,a) p.A=a

#define PixelGetR(p) p.R
#define PixelGetG(p) p.G
#define PixelGetB(p) p.B
#define PixelGetA(p) p.A

#define PixelInit(p) memset(&p,0,sizeof(p));	// later on I'll adapt this for initing default alpha etc, but for now



#define PROGRESS_INIT \
	CProgressCtrl *pProgress = NULL; \
	if (((CModViewApp*)AfxGetApp())->m_pMainWnd)	\
	{												\
		pProgress = new CProgressCtrl;				\
		bool bOK = !!pProgress->Create(	WS_CHILD|WS_VISIBLE|PBS_SMOOTH,				\
										CRect(100,100,200,200),						\
										((CModViewApp*)AfxGetApp())->m_pMainWnd,	\
										1											\
										);											\
		if (!bOK)						\
		{								\
			delete pProgress;			\
			pProgress = NULL;			\
		}								\
	}
	

#define PROGRESS_SETRANGE(range)		\
	if (pProgress)						\
	{									\
		pProgress->SetRange(0,range);	\
	}	

#define PROGRESS_SETPOS(position)	\
	if (pProgress)					\
	{								\
		pProgress->SetPos(position);\
	}
					
#define PROGRESS_CLOSE		\
	if (pProgress)			\
	{						\
		delete pProgress;	\
		pProgress = 0;		\
	}













// return NULL = legal, else string saying why not...
//
static LPCSTR DStamp_TextIsLegal(LPCSTR psText)
{
	int iLen = strlen(psText);
	if (iLen > sizeof(TheWatermark.Data.sText))
	{
		return va("\"%s\" is too long by %d chars",psText, sizeof(TheWatermark.Data.sText) - iLen);
	}

	for (int i=0; i<iLen; i++)
	{
		if (psText[i] > 127)
		{
			return va("\"%s\" contains hi-ascii char '%c', only 7-bit ascii allowed",psText, psText[i]);
		}
	}

	return NULL;
}


static void DStamp_EncryptData(void *pvData, int iByteLength)
{	
/*	don't use this method since we need to keep as 7-bit ascii
	byte *pData = (byte*) pvData;

	for (int i=0; i<iByteLength; i++)
	{
		*pData = *pData ^ ('#'+i);
		pData++;
	}
*/
}
// same as encrypt at the moment, because of XOR, but may change...
//
static void DStamp_DecryptData(void *pvData, int iByteLength)
{
	DStamp_EncryptData(pvData,iByteLength);
}

static DWORD DStamp_UpdateCRC(DWORD dwCRC, byte b)
{
	dwCRC += (DWORD) b;

	if (dwCRC & (1<<31))
	{
		dwCRC<<=1;
		dwCRC+=2;	// add 1, plus carry
	}
	else
	{
		dwCRC<<=1;
		dwCRC++;	// add 1, no carry
	}

	return dwCRC;
}

static DWORD DStamp_CalcBlockCRC(void *pvData, int iByteLength)
{
	byte *pData = (byte*) pvData;

	DWORD dwCRC = 0;
	
	for (int i=0; i<iByteLength; i++)
	{
		dwCRC = DStamp_UpdateCRC(dwCRC, pData[i]);
	}

	dwCRC &= 0x7F7F7F7F;	// lose 7th bit from each byte
	return dwCRC;
}



static LPCSTR DStamp_GetYear(void)
{
	static char sTemp[20];	
    time_t ltime;	

    time( &ltime );    
	
    struct tm *today = localtime( &ltime );    
	
	strftime( sTemp, sizeof(sTemp), "%d%m%y", today );

	return &sTemp[0];
}


static Watermark_t *DStamp_InitWatermark(LPCSTR psText)
{
	Watermark_t *pWatermark = &TheWatermark;

	strncpy(pWatermark->Data.sHDR,		"HDR",				sizeof(pWatermark->Data.sHDR));
	strncpy(pWatermark->Data.sText,		psText,				sizeof(pWatermark->Data.sText));
	strncpy(pWatermark->Data.sDDMMYY,	DStamp_GetYear(),	sizeof(pWatermark->Data.sDDMMYY));

	DStamp_EncryptData(&pWatermark->Data, sizeof(pWatermark->Data));
	pWatermark->dwCRC = DStamp_CalcBlockCRC(&pWatermark->Data, sizeof(pWatermark->Data));

	return pWatermark;
}


static Pixel_t *DStamp_GetImagePixel(DStampImage_t *pImage, int iXpos, int iYpos)
{
	int iBPP = (pImage->iPlanes == 24)?3:4;

	byte *pPixels = pImage->pPixels + (iYpos * pImage->iWidth * iBPP) + (iXpos * iBPP);

	static Pixel_t Pixel;
	memcpy(&Pixel, pPixels, iBPP);
	if (iBPP == 3)
	{
		Pixel.A = 255;	// actually, this can be ignored
	}
	
	return &Pixel;
}

static void DStamp_SetImagePixel(DStampImage_t *pImage, int iXpos, int iYpos, Pixel_t *pPixel)
{
	int iBPP = (pImage->iPlanes == 24)?3:4;

	byte *pPixels = pImage->pPixels + (iYpos * pImage->iWidth * iBPP) + (iXpos * iBPP);

	memcpy(pPixels, pPixel, iBPP);
}

// returns either 0 or 1 for bits, or 2 for "finished"
//
static int DStamp_GetWaterMarkBit(int *piSourceIndex_Byte, int *piSourceIndex_Bit, void *pvBytes, int iBytesLen)
{
	byte *pBytes = (byte *) pvBytes;
//	if (*piSourceIndex_Byte==(int)strlen(psWatermarkString)+1)	// trying to get bits from beyond trailing zero?
//		return 2;
	if (*piSourceIndex_Byte==iBytesLen)	// trying to get bits from beyond trailing zero?
		return 2;

	char c = pBytes[*piSourceIndex_Byte];

	int iBitReturn = (c>>*piSourceIndex_Bit)&1;
	*piSourceIndex_Bit+=1;	// *not* ++!
	if (*piSourceIndex_Bit == iDSTAMP_CHAR_BITS)
	{
		*piSourceIndex_Bit = 0;
		*piSourceIndex_Byte+=1;	// *not* ++!
	}

	return iBitReturn;
}


static void DStamp_ReadCell(DStampData_t *pData, int iCellXpos, int iCellYpos, byte *b1, byte *b2, byte *b3)
{
	int iWBitR = 0,
		iWBitG = 0,
		iWBitB = 0;

	for (int iPixelY=0; iPixelY<iDSTAMP_CELL_PIXDIM; iPixelY++)
	{
		for (int iPixelX=0; iPixelX<iDSTAMP_CELL_PIXDIM; iPixelX++)
		{
			Pixel_t Pixel = *DStamp_GetImagePixel(pData->pImage, iPixelX + iCellXpos, iPixelY + iCellYpos);

			iWBitR += (PixelGetR(Pixel)&iHIDDENBIT)?1:0;
			iWBitG += (PixelGetG(Pixel)&iHIDDENBIT)?1:0;
			iWBitB += (PixelGetB(Pixel)&iHIDDENBIT)?1:0;
		}
	}

	// work out pixel consensuses... (consensi?)
	//
	// if > half the pixels are true, accept that as true...
	//
	*b1 = (iWBitR > iDSTAMP_CELL_PIXELS/2)?1:0;
	*b2 = (iWBitG > iDSTAMP_CELL_PIXELS/2)?1:0;
	*b3 = (iWBitB > iDSTAMP_CELL_PIXELS/2)?1:0;
}

static void DStamp_MarkCell(DStampData_t *pData, int iCellXpos, int iCellYpos, int *piTxtByte, int *piTxtBit)
{
	byte R = DStamp_GetWaterMarkBit(piTxtByte, piTxtBit, pData->pWatermark, sizeof(*pData->pWatermark));
	byte G = DStamp_GetWaterMarkBit(piTxtByte, piTxtBit, pData->pWatermark, sizeof(*pData->pWatermark));
	byte B = DStamp_GetWaterMarkBit(piTxtByte, piTxtBit, pData->pWatermark, sizeof(*pData->pWatermark));
	
	for (int iPixelY=0; iPixelY<iDSTAMP_CELL_PIXDIM; iPixelY++)
	{
		for (int iPixelX=0; iPixelX<iDSTAMP_CELL_PIXDIM; iPixelX++)
		{
			Pixel_t Pixel = *DStamp_GetImagePixel(pData->pImage, iPixelX + iCellXpos, iPixelY + iCellYpos);

			PixelSetR(Pixel,(  (PixelGetR(Pixel)&~iHIDDENBIT) | (((R==2)?0:R)<<iHIDDENBITSHIFT) ));
			PixelSetG(Pixel,(  (PixelGetG(Pixel)&~iHIDDENBIT) | (((G==2)?0:G)<<iHIDDENBITSHIFT) ));
			PixelSetB(Pixel,(  (PixelGetB(Pixel)&~iHIDDENBIT) | (((B==2)?0:B)<<iHIDDENBITSHIFT) ));

			DStamp_SetImagePixel(pData->pImage, iPixelX + iCellXpos, iPixelY + iCellYpos, &Pixel);
		}
	}

/*	byte R2,G2,B2;
	DStamp_ReadCell(pData, iCellXpos, iCellYpos, &R2,&G2,&B2);

	assert(	(R2==((R==2)?0:R)) && 
			(G2==((G==2)?0:G)) && 
			(B2==((B==2)?0:B))
			);
*/
}

// apply one watermark-instance to the image...
//
static void DStamp_MarkInstance(DStampData_t *pData, int iInstancePixelX, int iInstancePixelY)
{
	int iTxtByte = 0;
	int iTxtBit  = 0;

	for (int iYCell = 0; iYCell < iDSTAMP_INSTANCE_CELLDIM; iYCell++)
	{
		for (int iXCell = 0; iXCell < iDSTAMP_INSTANCE_CELLDIM; iXCell++)
		{
			int iCellXpos = iInstancePixelX + (iXCell * iDSTAMP_CELL_PIXDIM);
			int iCellYpos = iInstancePixelY + (iYCell * iDSTAMP_CELL_PIXDIM);
			DStamp_MarkCell(pData, iCellXpos, iCellYpos, &iTxtByte, &iTxtBit);
		}
	}
}


// see if we can read some watermark data, else return NULL...
//
static LPCSTR DStamp_ReadInstance(DStampData_t *pData, int iInstancePixelX, int iInstancePixelY)
{
	LPCSTR psMessage = NULL;

	int iTxtByte = 0;
	int iTxtBit  = 0;

	byte *pbOut = (byte*) pData->pWatermark;
	memset(pbOut,0,sizeof(*pData->pWatermark));

	for (int iYCell = 0; iYCell < iDSTAMP_INSTANCE_CELLDIM; iYCell++)
	{
		for (int iXCell = 0; iXCell < iDSTAMP_INSTANCE_CELLDIM; iXCell++)
		{
			int iCellXpos = iInstancePixelX + (iXCell * iDSTAMP_CELL_PIXDIM);
			int iCellYpos = iInstancePixelY + (iYCell * iDSTAMP_CELL_PIXDIM);

			byte b1,b2,b3;						
			DStamp_ReadCell(pData, iCellXpos, iCellYpos, &b1,&b2,&b3);

#define DECODEBIT(DestString,bit) \
				DestString[iTxtByte] |= bit<<iTxtBit++;	\
				if (iTxtBit==iDSTAMP_CHAR_BITS)			\
				{										\
					iTxtBit=0;							\
					iTxtByte++;							\
				}

			DECODEBIT(pbOut,b1);
			DECODEBIT(pbOut,b2);
			DECODEBIT(pbOut,b3);

			if (iTxtByte>=3)	// huge speed opt, check for header, if not found, give up on this cell...
			{
				if (strncmp((char*)pbOut,"HDR",3))
				{
					return NULL;
				}
			}
		}
	}

	if (!strncmp(pData->pWatermark->Data.sHDR,"HDR",3))
	{
		DWORD dwCRC = DStamp_CalcBlockCRC(&pData->pWatermark->Data, sizeof(pData->pWatermark->Data));

		char sString[100];
		char sDate[100];
		strncpy(sString,pData->pWatermark->Data.sText,sizeof(pData->pWatermark->Data.sText));
		sString[sizeof(pData->pWatermark->Data.sText)] = '\0';
		strncpy(sDate,pData->pWatermark->Data.sDDMMYY,sizeof(pData->pWatermark->Data.sDDMMYY));
		sDate[sizeof(pData->pWatermark->Data.sDDMMYY)] = '\0';
		static char sOutput[1024];
		sprintf(sOutput,"SentTo: \"%s\",  Date(DD/MM/YY) = %s",sString,sDate);

		if (dwCRC == pData->pWatermark->dwCRC)
		{
			OutputDebugString(sOutput);
			psMessage = &sOutput[0];
		}
		else
		{
			OutputDebugString(va("Skipping non-CRC HDR-match: %s\n",sOutput));
		}
	}

	return psMessage;
}

// return is NULL for ok, else error string...
//
LPCSTR DStamp_MarkImage(byte *pPixels, int iWidth, int iHeight, int iPlanes, LPCSTR psText)
{
	LPCSTR psError = NULL;

	if (iPlanes == 24 || iPlanes == 32)
	{
		psError = DStamp_TextIsLegal(psText);

		if (!psError)
		{
			DStampImage_t	Image;
							Image.pPixels	= pPixels;
							Image.iWidth	= iWidth;
							Image.iHeight	= iHeight;
							Image.iPlanes	= iPlanes;		

			DStampData_t	Data;
							Data.pImage		= &Image;
							Data.pWatermark = DStamp_InitWatermark(psText);

			int iInstances_Across	= Data.pImage->iWidth / iDSTAMP_INSTANCE_PIXDIM;
			int iInstances_Down		= Data.pImage->iHeight	/ iDSTAMP_INSTANCE_PIXDIM;

			int iInstancesTotal		= iInstances_Across * iInstances_Down;
	#ifdef _DEBUG
	int iDebug_WaterMarkSize  = sizeof(Watermark_t);
	
	OutputDebugString(va("%d stamp instances on screen\n",iInstancesTotal));
	#endif

			if (iInstancesTotal)
			{
				// center the stamp grid within the image...
				//
				int iYStart = (Data.pImage->iHeight - (iInstances_Down   * iDSTAMP_INSTANCE_PIXDIM))/2;
				int iXStart = (Data.pImage->iWidth  - (iInstances_Across * iDSTAMP_INSTANCE_PIXDIM))/2;

				for (int iInstanceY = 0; iInstanceY < iInstances_Down; iInstanceY++)
				{
					for (int iInstanceX = 0; iInstanceX < iInstances_Across; iInstanceX++)
					{
						int iInstancePixelX = (iInstanceX * iDSTAMP_INSTANCE_PIXDIM) + iXStart;
						int iInstancePixelY = (iInstanceY * iDSTAMP_INSTANCE_PIXDIM) + iYStart;

						DStamp_MarkInstance(&Data, iInstancePixelX, iInstancePixelY);
	//					LPCSTR psMessage = DStamp_ReadInstance(&Data, iInstancePixelX, iInstancePixelY);
	//					if (psMessage)
	//					{
	//						OutputDebugString(va("Marked Instance with message \"%s\"\n",psMessage));
	//					}
	//					else
	//					{
	//						int z=1;
	//					}
					}
				}
			}
			else
			{
				psError = va("DStamp_MarkImage(): Unable to fit watermark on screen using iDSTAMP_CELL_PIXDIM of %d!",iDSTAMP_CELL_PIXDIM);
			}
		}
	}
	else
	{
		psError = "DStamp_MarkImage(): Supplied image must be 24 or 32 bit";		
	}

	return psError;
}

bool DStamp_MarkImage(Texture_t *pTexture, LPCSTR psText)
{	
	LPCSTR psError = DStamp_MarkImage(pTexture->pPixels, pTexture->iWidth, pTexture->iHeight, 32, psText);

	if (!psError)
		return true;

	ErrorBox(psError);
	return false;
}

void DStamp_AnalyseImage(byte *pPixels, int iWidth, int iHeight, int iPlanes)
{
	CWaitCursor wait;

	const int iBlockSampleSize = 8;

	PROGRESS_INIT;
	PROGRESS_SETRANGE((iHeight/iBlockSampleSize));

	DStampImage_t	Image;
					Image.pPixels	= pPixels;
					Image.iWidth	= iWidth;
					Image.iHeight	= iHeight;
					Image.iPlanes	= iPlanes;

	DStampImage_t	ImageOut;
					ImageOut.pPixels	= (byte *) malloc (iWidth * iHeight * ((iPlanes == 24)?3:4));
					ImageOut.iWidth		= iWidth;
					ImageOut.iHeight	= iHeight;
					ImageOut.iPlanes	= iPlanes;	
	int y = 0;


/*
	// new line dup code...
	//
	for (int y=0; y<iHeight-3; y+=30)
	{
		for (int x=0; x<iWidth; x++)
		{
			Pixel_t *pPixel = DStamp_GetImagePixel(&Image, x, y);
			DStamp_SetImagePixel(&Image, x, y+1, pPixel);
			DStamp_SetImagePixel(&Image, x, y+2, pPixel);
			DStamp_SetImagePixel(&Image, x, y+3, pPixel);
		}
	}
*/		
#if 1
	for (y = 0; y+iBlockSampleSize < iHeight; y += iBlockSampleSize)
	{
		PROGRESS_SETPOS(y);
		for (int x = 0; x+iBlockSampleSize < iWidth; x += iBlockSampleSize)
		{
			unsigned int r=0,g=0,b=0;
			unsigned int rh=0,gh=0,bh=0;
			unsigned int rl=INT_MAX,gl=INT_MAX,bl=INT_MAX;

			if (x==120 && y==232)
			{
				int z=1;
			}

			int iNoisyPixels = 0;
			int iMassivelyDeviantPixels = 0;
			for (int iBlockPass=0; iBlockPass<3; iBlockPass++)
			{
				// pass 0 = read pixels, average out channels
				// pass 1 = count noisy pixels
				// pass 2 = colour in block as 0 or 255 to show noise if > half blocks noisy
				//				
				for (int by=0; by < iBlockSampleSize; by++)
				{
					for (int bx=0; bx < iBlockSampleSize; bx++)
					{
						int px = x+bx;
						int py = y+by;
						Pixel_t *pPixel = DStamp_GetImagePixel(&Image, px, py);

						switch (iBlockPass)
						{
							case 0:	// read/accumulate pixel values...
							{
								r += pPixel->R;
								g += pPixel->G;
								b += pPixel->B;

								rh = max(rh,pPixel->R);
								gh = max(gh,pPixel->G);
								bh = max(bh,pPixel->B);
								
								rl = min(rl,pPixel->R);
								gl = min(gl,pPixel->G);
								bl = min(bl,pPixel->B);
							}
							break;

							case 1:	// analyse...
							{								
								
								if (rh-rl>90 ||
									gh-gl>90 ||
									bh-bl>90
									)
								{
									iMassivelyDeviantPixels++;
								}								
								
								if (px>0 && px<iWidth-1 && py>0 && py<iHeight-1)
								{
									int this_r = pPixel->R;
									int this_g = pPixel->G;
									int this_b = pPixel->B;

									bool bPixelIsNoisy = false;

									if (px==123 && py==235)
									{
										int z=1;
									}

									for (int y2=py-1; y2<py+2 && !bPixelIsNoisy; y2++)
									{
										for (int x2=px-1; x2<px+2 && !bPixelIsNoisy; x2++)
										{
											if (x2!=px&&y2!=py)
											{
												pPixel = DStamp_GetImagePixel(&Image, x2, y2);

												int rdiff = abs(pPixel->R - this_r);
												int gdiff = abs(pPixel->G - this_g);
												int bdiff = abs(pPixel->B - this_b);

												const int iMinTolerance = 7;
												const int iMaxTolerance = 50;

												if ((rdiff > iMaxTolerance) ||
													(gdiff > iMaxTolerance) ||
													(bdiff > iMaxTolerance) ||													
													abs((int)(pPixel->R - r)) > iMaxTolerance ||
													abs((int)(pPixel->G - g)) > iMaxTolerance ||
													abs((int)(pPixel->B - b)) > iMaxTolerance
													)
												{
													//iMassivelyDeviantPixels++;
													iNoisyPixels-=5000;	// force not to be noisy for blocks with sharp contrasts
												}
												else
												if ((rdiff > iMinTolerance && rdiff < iMaxTolerance) ||
													(gdiff > iMinTolerance && gdiff < iMaxTolerance) ||
													(bdiff > iMinTolerance && bdiff < iMaxTolerance)
													)
												{
													 bPixelIsNoisy = true;
												}
											}
										}
									}
									if (bPixelIsNoisy)
									{
										iNoisyPixels++;
									}
								}
							}
							break;

							case 2:	// paint...
							{
								pPixel->R = pPixel->G = pPixel->B = (iNoisyPixels>((iBlockSampleSize*iBlockSampleSize)/2))?255:0;

								DStamp_SetImagePixel(&ImageOut, x+bx, y+by, pPixel);
							}
							break;
						}
					}
				}

				switch (iBlockPass)
				{
					case 0:
						r /= iBlockSampleSize*iBlockSampleSize;
						g /= iBlockSampleSize*iBlockSampleSize;
						b /= iBlockSampleSize*iBlockSampleSize;
						break;

					case 1:						
						if (iMassivelyDeviantPixels > (iBlockSampleSize*iBlockSampleSize)/8)
						{
							// if > 1/8th of the pixels are massively deviant, then set this block so we can't use it.
							iNoisyPixels-=5000;	// force not to be noisy for blocks with sharp contrasts
						}
						break;
				}

			}
		}
	}
#endif
	#if 1
	// copy analysis results over input picture for saving on return...
	//
	DStampImage_t	ImageTmp;
					ImageTmp.pPixels	= (byte *) malloc (iWidth * iHeight * ((iPlanes == 24)?3:4));
					ImageTmp.iWidth		= iWidth;
					ImageTmp.iHeight	= iHeight;
					ImageTmp.iPlanes	= iPlanes;	

	memcpy(ImageTmp.pPixels,ImageOut.pPixels, iWidth * iHeight * ((iPlanes == 24)?3:4));	

	// now eliminate any single white or black sampleblocks...
	//
//	for (int iEliminate = 0; iEliminate<2; iEliminate++)
	{
		// start one-in from all edges, so we can check 8 surrounding squares safely...
		//
		for (y = 0+iBlockSampleSize; y + (2*iBlockSampleSize) < iHeight; y += iBlockSampleSize)
		{
			for (int x = 0+iBlockSampleSize; x + (2*iBlockSampleSize) < iWidth; x+= iBlockSampleSize)
			{
				unsigned int r=0,g=0,b=0;

				Pixel_t *pPixel = DStamp_GetImagePixel(&ImageTmp, x, y);

				bool bThisPixelWasBlack = !(pPixel->R);	// all channels same, so just checking R is fine

				//if (bThisPixelWasBlack)
				{
					// now check neighbours, all 8 surrounding blocks must be same colour, or bye-bye...
					//
					bool bSurroundedByOpposite = true;
					for (int y2=y-iBlockSampleSize; y2<y+(2*iBlockSampleSize) && bSurroundedByOpposite; y2+=iBlockSampleSize)
					{
						for (int x2=x-iBlockSampleSize; x2<x+(2*iBlockSampleSize) && bSurroundedByOpposite; x2+=iBlockSampleSize)
						{
							if (x2!=x && y2!=y)
							{
								Pixel_t *pPixel = DStamp_GetImagePixel(&ImageTmp, x2, y2);

								bool bScanPixelWasBlack = !(pPixel->R);

								if (bScanPixelWasBlack == bThisPixelWasBlack)
								{
									bSurroundedByOpposite = false;
								}
							}
						}
					}

					if (bSurroundedByOpposite)
					{
						// blot this pixel out by toggling it to the other colour...
						//
						pPixel->R = pPixel->G = pPixel->B = bThisPixelWasBlack?255:0;

						for (int py=y; py<y+iBlockSampleSize; py++)
						{
							for (int px=x; px<x+iBlockSampleSize; px++)
							{
								DStamp_SetImagePixel(&ImageOut, px, py, pPixel);
							}
						}
					}
				}
			}
		}
	}
	memcpy(ImageTmp.pPixels,ImageOut.pPixels, iWidth * iHeight * ((iPlanes == 24)?3:4));	
#endif

	// now highlight original pic with differences...
	//
	for (y=0; y+iBlockSampleSize < iHeight; y+=iBlockSampleSize)
	{
		for (int x=0; x+iBlockSampleSize < iWidth; x+=iBlockSampleSize)
		{
			Pixel_t *pPixel = DStamp_GetImagePixel(&ImageTmp, x, y);

			bool bThisPixelWasBlack = !(pPixel->R);	// all channels same, so just checking R is fine

//			bool bPrevPixelWasBlack = true;
//			if (x>=iBlockSampleSize)
//			{
//				pPixel = DStamp_GetImagePixel(&ImageTmp, x-iBlockSampleSize, y);
//				bPrevPixelWasBlack = !(pPixel->R);	// all channels same, so just checking R is fine
//			}			

			if (!bThisPixelWasBlack )//&& !bPrevPixelWasBlack)
			{
				unsigned int r=0,g=0,b=0;
				for (int iPass=0; iPass<2; iPass++)
				{
					// highlight this sample square of the original pic...
					//
					for (int y2=y; y2<y+iBlockSampleSize; y2++)
					{
						for (int x2=x; x2<x+iBlockSampleSize; x2++)
						{
							if (!iPass)
							{
								pPixel = DStamp_GetImagePixel(&Image, x2, y2);
								r+= pPixel->R;
								g+= pPixel->G;
								b+= pPixel->B;
							}
							else
							{
								#if 1
									// highlight...
									//
									pPixel = DStamp_GetImagePixel(&Image, x2/*-iBlockSampleSize*/, y2);

									pPixel->R |= 128;
									pPixel->G &=~128;
									pPixel->B &=~128;
								#else
									pPixel = DStamp_GetImagePixel(&Image, x2, y2);

									pPixel->R = (r + pPixel->R)/2;
									pPixel->G = (g + pPixel->G)/2;
									pPixel->B = (b + pPixel->B)/2;
								#endif
								DStamp_SetImagePixel(&Image, x2, y2, pPixel);
							}
						}
					}
					if (!iPass)
					{
						pPixel->R = r = r/(iBlockSampleSize*iBlockSampleSize);
						pPixel->G = g = g/(iBlockSampleSize*iBlockSampleSize);
						pPixel->B = b = b/(iBlockSampleSize*iBlockSampleSize);
					}
				}
			}
		}
	}


	PROGRESS_CLOSE;

	free(ImageTmp.pPixels);
	free(ImageOut.pPixels);
}

void DStamp_AnalyseImage(Texture_t *pTexture)
{
	DStamp_AnalyseImage(pTexture->pPixels, pTexture->iWidth, pTexture->iHeight, 32);
}

LPCSTR	DStamp_ReadImage(byte *pPixels, int iWidth, int iHeight, int iPlanes)
{
	CWaitCursor wait;

	LPCSTR psMessage = NULL;

	if (iPlanes == 24 || iPlanes == 32)
	{
		DStampImage_t	Image;
						Image.pPixels	= pPixels;
						Image.iWidth	= iWidth;
						Image.iHeight	= iHeight;
						Image.iPlanes	= iPlanes;		

		DStampData_t	Data;
						Data.pImage		= &Image;
						Data.pWatermark = &TheWatermark;

		if (1)
		{

			int iInstances_Across	= Data.pImage->iWidth / iDSTAMP_INSTANCE_PIXDIM;
			int iInstances_Down		= Data.pImage->iHeight	/ iDSTAMP_INSTANCE_PIXDIM;

//	#ifdef _DEBUG
//	int iDebug_WaterMarkSize  = sizeof(Watermark_t);
//	int iDebug_InstancesTotal = iInstances_Across * iInstances_Down;
//	#endif
/*
#define iDSTAMP_CELL_PIXDIM			16	// adjust this, higher = more secure but less stamps on picture
#define iDSTAMP_CELL_PIXELS			(iDSTAMP_CELL_PIXDIM * iDSTAMP_CELL_PIXDIM)
#define iDSTAMP_INSTANCE_CELLDIM	8	// leave this alone at all times!
#define iDSTAMP_INSTANCE_PIXDIM		(iDSTAMP_INSTANCE_CELLDIM * iDSTAMP_CELL_PIXDIM)
#define iDSTAMP_CHAR_BITS			7

*/
			CProgressCtrl *pProgress = NULL;

			if (((CModViewApp*)AfxGetApp())->m_pMainWnd)
			{				
				pProgress = new CProgressCtrl;
				bool bOK = !!pProgress->Create(	WS_CHILD|WS_VISIBLE|PBS_SMOOTH,		// DWORD dwStyle, 
												CRect(100,100,200,200),				// const RECT& rect, 
												((CModViewApp*)AfxGetApp())->m_pMainWnd,	// CWnd* pParentWnd, 
												1									// UINT nID 
												);
				if (!bOK)
				{
					delete pProgress;
					pProgress = NULL;
				}
			}
	

			// the image may have been cropped, so we need to hunt for the signature at every pixel position...
			//
			int iYScans = 0;
			for (int iLazy = 0; iLazy<2; iLazy++)
			{
				for (int iYStart = 0; iYStart < iDSTAMP_INSTANCE_PIXDIM && !psMessage; iYStart++)
				{
					if (iLazy)
					{
						if (pProgress)
						{			
							pProgress->SetRange(0,iYScans);
						}
						OutputDebugString(va("iYStart %%%d\n",(iYStart*100)/iDSTAMP_INSTANCE_PIXDIM));
						if (pProgress)
						{
							pProgress->SetPos(iYStart);		
		//					wait.Restore();
						}
					
						for (int iXStart = 0; iXStart < iDSTAMP_INSTANCE_PIXDIM && !psMessage; iXStart++)
						{
							// look for some stamps here...
							//
							for (int iInstanceY = 0; iInstanceY < iInstances_Down && !psMessage; iInstanceY++)
							{
								for (int iInstanceX = 0; iInstanceX < iInstances_Across && !psMessage; iInstanceX++)
								{
									int iInstancePixelX = (iInstanceX * iDSTAMP_INSTANCE_PIXDIM) + iXStart;
									int iInstancePixelY = (iInstanceY * iDSTAMP_INSTANCE_PIXDIM) + iYStart;

									if (iInstancePixelX + iDSTAMP_INSTANCE_PIXDIM >= iWidth
										||
										iInstancePixelY + iDSTAMP_INSTANCE_PIXDIM >= iHeight
										)
									{
										// ... then skip this position while hunting for header start
									}
									else
									{					
										psMessage = DStamp_ReadInstance(&Data, iInstancePixelX, iInstancePixelY);
									}
								}
							}
						}
					}
					else
					{
						for (int iInstanceY = 0; iInstanceY < iInstances_Down; iInstanceY++)
						{
							int iInstancePixelY = (iInstanceY * iDSTAMP_INSTANCE_PIXDIM) + iYStart;

							if (iInstancePixelY + iDSTAMP_INSTANCE_PIXDIM >= iHeight)
							{
							}
							else
							{
								iYScans++;
							}
						}
					}
				}
			}

			if (pProgress)
			{
				delete pProgress;
				pProgress = 0;
			}
		}
	}
	else
	{
		ErrorBox("DStamp_ReadImage(): Supplied image must be 24 or 32 bit");
	}	

	return psMessage;
}


////////////////// eof ////////////////




