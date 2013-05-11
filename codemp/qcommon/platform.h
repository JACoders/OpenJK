#pragma once

// Simple header file to dispatch to the relevant platform API headers
#if defined(_WINDOWS)
#include <windows.h>
#endif

#if defined (__linux__)
typedef const char *LPCTSTR;
typedef const char *LPCSTR;
typedef unsigned long DWORD;
typedef unsigned int UINT;
typedef void* HANDLE;
typedef HANDLE HINSTANCE;
typedef void *PVOID;
typedef DWORD COLORREF;
typedef unsigned char BYTE;
typedef unsigned char byte;
typedef long LONG;
typedef struct tagPOINT {
  LONG x;
  LONG y;
} POINT;
typedef unsigned short USHORT;
typedef unsigned short WORD;
typedef struct _GUID {
  DWORD Data1;
  WORD  Data2;
  WORD  Data3;
  BYTE  Data4[8];
} GUID;
#define strnicmp Q_stricmpn
#define strcmpi Q_stricmp
#define stricmp Q_stricmp
#define RGB(r,g,b)          ((COLORREF)((r) | ((g) << 8) | ((b) << 16)))

#define _isnan isnan
#endif

#if defined (MACOS_X)
typedef const char *LPCTSTR;
typedef const char *LPCSTR;
typedef unsigned long DWORD;
typedef unsigned int UINT;
typedef void* HANDLE;
typedef HANDLE HINSTANCE;
typedef void *PVOID;
typedef DWORD COLORREF;
typedef unsigned char BYTE;
typedef unsigned char byte;
typedef long LONG;
typedef struct tagPOINT {
    LONG x;
    LONG y;
} POINT;
typedef unsigned short USHORT;
typedef unsigned short WORD;
typedef struct _GUID {
    DWORD Data1;
    WORD  Data2;
    WORD  Data3;
    BYTE  Data4[8];
} GUID;
#define strnicmp Q_stricmpn
#define strcmpi Q_stricmp
#define stricmp Q_stricmp
#define RGB(r,g,b)          ((COLORREF)((r) | ((g) << 8) | ((b) << 16)))

#define _isnan isnan
#endif
