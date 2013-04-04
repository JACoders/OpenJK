// Filename:-	clipboard.h
//

#ifndef CLIPBOARD_H
#define CLIPBOARD_H


BOOL Clipboard_SendString(LPCSTR psString);
BOOL ClipBoard_SendDIB(LPVOID pvData, int iBytes);

// other stuff that's not actually clipboard, but is called only in conjunction with it anyway...
//
bool ScreenShot(LPCSTR psFilename = NULL, LPCSTR psCopyrightMessage = NULL, int iWidth = g_iScreenWidth, int iHeight = g_iScreenHeight);
bool BMP_GetMemDIB(void *&pvAddress, int &iBytes);
void BMP_Free(void);

#endif

/////////////////// eof //////////////////

