#pragma once
#error You should not be including this file on this platform

#include <windows.h>

typedef struct glwstate_s {
	WNDPROC		wndproc;

	HDC     hDC;			// handle to device context
	HGLRC   hGLRC;			// handle to GL rendering context

	HINSTANCE hinstOpenGL;	// HINSTANCE for the OpenGL library

	qboolean allowdisplaydepthchange;
	qboolean pixelFormatSet;

	int		 desktopBitsPixel;
	int		 desktopWidth, desktopHeight;

	qboolean	cdsFullscreen;

	FILE *log_fp;
} glwstate_t;

extern glwstate_t glw_state;

bool GL_CheckForExtension(const char *ext);
