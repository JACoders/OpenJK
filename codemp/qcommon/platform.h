// Simple header file to dispatch to the relevant platform API headers
#ifndef _PLATFORM_H
#define _PLATFORM_H

#if defined(_XBOX)
#include <xtl.h>
#endif

#if defined(_WINDOWS)
#include <windows.h>
#endif

#if defined (__linux__)
typedef const char *LPCTSTR;
typedef const char *LPCSTR;
typedef unsigned long DWORD;
typedef unsigned int UINT;
typedef void* HANDLE;
typedef DWORD COLORREF;
typedef unsigned char BYTE;
#endif 
#endif
