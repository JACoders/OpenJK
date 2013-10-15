/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
/*
** QGL.H
*/

#ifndef __QGL_H__
#define __QGL_H__

#ifndef _WIN32
#include "SDL/SDL_opengl.h"
#else
#include <Windows.h>
#include <gl/GL.h>
#include "glext.h"

#define GLsizeiptrARB	size_t *
#define GLintptrARB		int *
#define GLhandleARB		unsigned int
#define GLcharARB		char
#endif

typedef void (APIENTRY *PFNGLCOMBINERPARAMETERFVNV) (GLenum pname,const GLfloat *params);
typedef void (APIENTRY *PFNGLCOMBINERPARAMETERIVNV) (GLenum pname,const GLint *params);
typedef void (APIENTRY *PFNGLCOMBINERPARAMETERFNV) (GLenum pname,GLfloat param);
typedef void (APIENTRY *PFNGLCOMBINERPARAMETERINV) (GLenum pname,GLint param);
typedef void (APIENTRY *PFNGLCOMBINERINPUTNV) (GLenum stage,GLenum portion,GLenum variable,GLenum input,GLenum mapping,
											   GLenum componentUsage);
typedef void (APIENTRY *PFNGLCOMBINEROUTPUTNV) (GLenum stage,GLenum portion,GLenum abOutput,GLenum cdOutput,GLenum sumOutput,
												GLenum scale, GLenum bias,GLboolean abDotProduct,GLboolean cdDotProduct,
												GLboolean muxSum);
typedef void (APIENTRY *PFNGLFINALCOMBINERINPUTNV) (GLenum variable,GLenum input,GLenum mapping,GLenum componentUsage);

typedef void (APIENTRY *PFNGLGETCOMBINERINPUTPARAMETERFVNV) (GLenum stage,GLenum portion,GLenum variable,GLenum pname,GLfloat *params);
typedef void (APIENTRY *PFNGLGETCOMBINERINPUTPARAMETERIVNV) (GLenum stage,GLenum portion,GLenum variable,GLenum pname,GLint *params);
typedef void (APIENTRY *PFNGLGETCOMBINEROUTPUTPARAMETERFVNV) (GLenum stage,GLenum portion,GLenum pname,GLfloat *params);
typedef void (APIENTRY *PFNGLGETCOMBINEROUTPUTPARAMETERIVNV) (GLenum stage,GLenum portion,GLenum pname,GLint *params);
typedef void (APIENTRY *PFNGLGETFINALCOMBINERINPUTPARAMETERFVNV) (GLenum variable,GLenum pname,GLfloat *params);
typedef void (APIENTRY *PFNGLGETFINALCOMBINERINPUTPARAMETERIVNV) (GLenum variable,GLenum pname,GLfloat *params);
/***********************************************************************************************************/

// Declare Register Combiners function pointers.
extern PFNGLCOMBINERPARAMETERFVNV				qglCombinerParameterfvNV;
extern PFNGLCOMBINERPARAMETERIVNV				qglCombinerParameterivNV;
extern PFNGLCOMBINERPARAMETERFNV				qglCombinerParameterfNV;
extern PFNGLCOMBINERPARAMETERINV				qglCombinerParameteriNV;
extern PFNGLCOMBINERINPUTNV						qglCombinerInputNV;
extern PFNGLCOMBINEROUTPUTNV					qglCombinerOutputNV;
extern PFNGLFINALCOMBINERINPUTNV				qglFinalCombinerInputNV;
extern PFNGLGETCOMBINERINPUTPARAMETERFVNV		qglGetCombinerInputParameterfvNV;
extern PFNGLGETCOMBINERINPUTPARAMETERIVNV		qglGetCombinerInputParameterivNV;
extern PFNGLGETCOMBINEROUTPUTPARAMETERFVNV		qglGetCombinerOutputParameterfvNV;
extern PFNGLGETCOMBINEROUTPUTPARAMETERIVNV		qglGetCombinerOutputParameterivNV;
extern PFNGLGETFINALCOMBINERINPUTPARAMETERFVNV	qglGetFinalCombinerInputParameterfvNV;
extern PFNGLGETFINALCOMBINERINPUTPARAMETERIVNV	qglGetFinalCombinerInputParameterivNV;

extern void (APIENTRYP qglActiveTextureARB) (GLenum texture);
extern void (APIENTRYP qglClientActiveTextureARB) (GLenum texture);
extern void (APIENTRYP qglMultiTexCoord2fARB) (GLenum target, GLfloat s, GLfloat t);

extern void (APIENTRYP qglLockArraysEXT) (GLint first, GLsizei count);
extern void (APIENTRYP qglUnlockArraysEXT) (void);


//===========================================================================
#ifdef _WIN32
extern int ( WINAPI * qwglSwapIntervalEXT)( int interval );

extern BOOL  ( WINAPI * qwglCopyContext)(HGLRC, HGLRC, UINT);
extern HGLRC ( WINAPI * qwglCreateContext)(HDC);
extern HGLRC ( WINAPI * qwglCreateLayerContext)(HDC, int);
extern BOOL  ( WINAPI * qwglDeleteContext)(HGLRC);
extern HGLRC ( WINAPI * qwglGetCurrentContext)(VOID);
extern HDC   ( WINAPI * qwglGetCurrentDC)(VOID);
extern PROC  ( WINAPI * qwglGetProcAddress)(LPCSTR);
extern BOOL  ( WINAPI * qwglMakeCurrent)(HDC, HGLRC);
extern BOOL  ( WINAPI * qwglShareLists)(HGLRC, HGLRC);
extern BOOL  ( WINAPI * qwglUseFontBitmaps)(HDC, DWORD, DWORD, DWORD);

extern BOOL  ( WINAPI * qwglUseFontOutlines)(HDC, DWORD, DWORD, DWORD, FLOAT,
                                           FLOAT, int, LPGLYPHMETRICSFLOAT);

extern BOOL ( WINAPI * qwglDescribeLayerPlane)(HDC, int, int, UINT,
                                            LPLAYERPLANEDESCRIPTOR);
extern int  ( WINAPI * qwglSetLayerPaletteEntries)(HDC, int, int, int,
                                                CONST COLORREF *);
extern int  ( WINAPI * qwglGetLayerPaletteEntries)(HDC, int, int, int,
                                                COLORREF *);
extern BOOL ( WINAPI * qwglRealizeLayerPalette)(HDC, int, BOOL);
extern BOOL ( WINAPI * qwglSwapLayerBuffers)(HDC, UINT);

extern void ( APIENTRY * qglAccum )(GLenum op, GLfloat value);
extern void ( APIENTRY * qglAlphaFunc )(GLenum func, GLclampf ref);
extern GLboolean ( APIENTRY * qglAreTexturesResident )(GLsizei n, const GLuint *textures, GLboolean *residences);
extern void ( APIENTRY * qglArrayElement )(GLint i);
extern void ( APIENTRY * qglBegin )(GLenum mode);
extern void ( APIENTRY * qglBindTexture )(GLenum target, GLuint texture);
extern void ( APIENTRY * qglBitmap )(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap);
extern void ( APIENTRY * qglBlendFunc )(GLenum sfactor, GLenum dfactor);
extern void ( APIENTRY * qglCallList )(GLuint list);
extern void ( APIENTRY * qglCallLists )(GLsizei n, GLenum type, const GLvoid *lists);
extern void ( APIENTRY * qglClear )(GLbitfield mask);
extern void ( APIENTRY * qglClearAccum )(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
extern void ( APIENTRY * qglClearColor )(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
extern void ( APIENTRY * qglClearDepth )(GLclampd depth);
extern void ( APIENTRY * qglClearIndex )(GLfloat c);
extern void ( APIENTRY * qglClearStencil )(GLint s);
extern void ( APIENTRY * qglClipPlane )(GLenum plane, const GLdouble *equation);
extern void ( APIENTRY * qglColor3b )(GLbyte red, GLbyte green, GLbyte blue);
extern void ( APIENTRY * qglColor3bv )(const GLbyte *v);
extern void ( APIENTRY * qglColor3d )(GLdouble red, GLdouble green, GLdouble blue);
extern void ( APIENTRY * qglColor3dv )(const GLdouble *v);
extern void ( APIENTRY * qglColor3f )(GLfloat red, GLfloat green, GLfloat blue);
extern void ( APIENTRY * qglColor3fv )(const GLfloat *v);
extern void ( APIENTRY * qglColor3i )(GLint red, GLint green, GLint blue);
extern void ( APIENTRY * qglColor3iv )(const GLint *v);
extern void ( APIENTRY * qglColor3s )(GLshort red, GLshort green, GLshort blue);
extern void ( APIENTRY * qglColor3sv )(const GLshort *v);
extern void ( APIENTRY * qglColor3ub )(GLubyte red, GLubyte green, GLubyte blue);
extern void ( APIENTRY * qglColor3ubv )(const GLubyte *v);
extern void ( APIENTRY * qglColor3ui )(GLuint red, GLuint green, GLuint blue);
extern void ( APIENTRY * qglColor3uiv )(const GLuint *v);
extern void ( APIENTRY * qglColor3us )(GLushort red, GLushort green, GLushort blue);
extern void ( APIENTRY * qglColor3usv )(const GLushort *v);
extern void ( APIENTRY * qglColor4b )(GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha);
extern void ( APIENTRY * qglColor4bv )(const GLbyte *v);
extern void ( APIENTRY * qglColor4d )(GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha);
extern void ( APIENTRY * qglColor4dv )(const GLdouble *v);
extern void ( APIENTRY * qglColor4f )(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
extern void ( APIENTRY * qglColor4fv )(const GLfloat *v);
extern void ( APIENTRY * qglColor4i )(GLint red, GLint green, GLint blue, GLint alpha);
extern void ( APIENTRY * qglColor4iv )(const GLint *v);
extern void ( APIENTRY * qglColor4s )(GLshort red, GLshort green, GLshort blue, GLshort alpha);
extern void ( APIENTRY * qglColor4sv )(const GLshort *v);
extern void ( APIENTRY * qglColor4ub )(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha);
extern void ( APIENTRY * qglColor4ubv )(const GLubyte *v);
extern void ( APIENTRY * qglColor4ui )(GLuint red, GLuint green, GLuint blue, GLuint alpha);
extern void ( APIENTRY * qglColor4uiv )(const GLuint *v);
extern void ( APIENTRY * qglColor4us )(GLushort red, GLushort green, GLushort blue, GLushort alpha);
extern void ( APIENTRY * qglColor4usv )(const GLushort *v);
extern void ( APIENTRY * qglColorMask )(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
extern void ( APIENTRY * qglColorMaterial )(GLenum face, GLenum mode);
extern void ( APIENTRY * qglColorPointer )(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
extern void ( APIENTRY * qglCopyPixels )(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type);
extern void ( APIENTRY * qglCopyTexImage1D )(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLint border);
extern void ( APIENTRY * qglCopyTexImage2D )(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
extern void ( APIENTRY * qglCopyTexSubImage1D )(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width);
extern void ( APIENTRY * qglCopyTexSubImage2D )(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
extern void ( APIENTRY * qglCullFace )(GLenum mode);
extern void ( APIENTRY * qglDeleteLists )(GLuint list, GLsizei range);
extern void ( APIENTRY * qglDeleteTextures )(GLsizei n, const GLuint *textures);
extern void ( APIENTRY * qglDepthFunc )(GLenum func);
extern void ( APIENTRY * qglDepthMask )(GLboolean flag);
extern void ( APIENTRY * qglDepthRange )(GLclampd zNear, GLclampd zFar);
extern void ( APIENTRY * qglDisable )(GLenum cap);
extern void ( APIENTRY * qglDisableClientState )(GLenum array);
extern void ( APIENTRY * qglDrawArrays )(GLenum mode, GLint first, GLsizei count);
extern void ( APIENTRY * qglDrawBuffer )(GLenum mode);
extern void ( APIENTRY * qglDrawElements )(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
extern void ( APIENTRY * qglDrawPixels )(GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
extern void ( APIENTRY * qglEdgeFlag )(GLboolean flag);
extern void ( APIENTRY * qglEdgeFlagPointer )(GLsizei stride, const GLvoid *pointer);
extern void ( APIENTRY * qglEdgeFlagv )(const GLboolean *flag);
extern void ( APIENTRY * qglEnable )(GLenum cap);
extern void ( APIENTRY * qglEnableClientState )(GLenum array);
extern void ( APIENTRY * qglEnd )(void);
extern void ( APIENTRY * qglEndList )(void);
extern void ( APIENTRY * qglEvalCoord1d )(GLdouble u);
extern void ( APIENTRY * qglEvalCoord1dv )(const GLdouble *u);
extern void ( APIENTRY * qglEvalCoord1f )(GLfloat u);
extern void ( APIENTRY * qglEvalCoord1fv )(const GLfloat *u);
extern void ( APIENTRY * qglEvalCoord2d )(GLdouble u, GLdouble v);
extern void ( APIENTRY * qglEvalCoord2dv )(const GLdouble *u);
extern void ( APIENTRY * qglEvalCoord2f )(GLfloat u, GLfloat v);
extern void ( APIENTRY * qglEvalCoord2fv )(const GLfloat *u);
extern void ( APIENTRY * qglEvalMesh1 )(GLenum mode, GLint i1, GLint i2);
extern void ( APIENTRY * qglEvalMesh2 )(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2);
extern void ( APIENTRY * qglEvalPoint1 )(GLint i);
extern void ( APIENTRY * qglEvalPoint2 )(GLint i, GLint j);
extern void ( APIENTRY * qglFeedbackBuffer )(GLsizei size, GLenum type, GLfloat *buffer);
extern void ( APIENTRY * qglFinish )(void);
extern void ( APIENTRY * qglFlush )(void);
extern void ( APIENTRY * qglFogf )(GLenum pname, GLfloat param);
extern void ( APIENTRY * qglFogfv )(GLenum pname, const GLfloat *params);
extern void ( APIENTRY * qglFogi )(GLenum pname, GLint param);
extern void ( APIENTRY * qglFogiv )(GLenum pname, const GLint *params);
extern void ( APIENTRY * qglFrontFace )(GLenum mode);
extern void ( APIENTRY * qglFrustum )(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
extern GLuint ( APIENTRY * qglGenLists )(GLsizei range);
extern void ( APIENTRY * qglGenTextures )(GLsizei n, GLuint *textures);
extern void ( APIENTRY * qglGetBooleanv )(GLenum pname, GLboolean *params);
extern void ( APIENTRY * qglGetClipPlane )(GLenum plane, GLdouble *equation);
extern void ( APIENTRY * qglGetDoublev )(GLenum pname, GLdouble *params);
extern GLenum ( APIENTRY * qglGetError )(void);
extern void ( APIENTRY * qglGetFloatv )(GLenum pname, GLfloat *params);
extern void ( APIENTRY * qglGetIntegerv )(GLenum pname, GLint *params);
extern void ( APIENTRY * qglGetLightfv )(GLenum light, GLenum pname, GLfloat *params);
extern void ( APIENTRY * qglGetLightiv )(GLenum light, GLenum pname, GLint *params);
extern void ( APIENTRY * qglGetMapdv )(GLenum target, GLenum query, GLdouble *v);
extern void ( APIENTRY * qglGetMapfv )(GLenum target, GLenum query, GLfloat *v);
extern void ( APIENTRY * qglGetMapiv )(GLenum target, GLenum query, GLint *v);
extern void ( APIENTRY * qglGetMaterialfv )(GLenum face, GLenum pname, GLfloat *params);
extern void ( APIENTRY * qglGetMaterialiv )(GLenum face, GLenum pname, GLint *params);
extern void ( APIENTRY * qglGetPixelMapfv )(GLenum m, GLfloat *values); //rwwRMG - map->m (avoid map type conflict)
extern void ( APIENTRY * qglGetPixelMapuiv )(GLenum m, GLuint *values); //rwwRMG - map->m (avoid map type conflict)
extern void ( APIENTRY * qglGetPixelMapusv )(GLenum m, GLushort *values); //rwwRMG - map->m (avoid map type conflict)
extern void ( APIENTRY * qglGetPointerv )(GLenum pname, GLvoid* *params);
extern void ( APIENTRY * qglGetPolygonStipple )(GLubyte *mask);
extern const GLubyte * ( APIENTRY * qglGetString )(GLenum name);
extern void ( APIENTRY * qglGetTexEnvfv )(GLenum target, GLenum pname, GLfloat *params);
extern void ( APIENTRY * qglGetTexEnviv )(GLenum target, GLenum pname, GLint *params);
extern void ( APIENTRY * qglGetTexGendv )(GLenum coord, GLenum pname, GLdouble *params);
extern void ( APIENTRY * qglGetTexGenfv )(GLenum coord, GLenum pname, GLfloat *params);
extern void ( APIENTRY * qglGetTexGeniv )(GLenum coord, GLenum pname, GLint *params);
extern void ( APIENTRY * qglGetTexImage )(GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels);
extern void ( APIENTRY * qglGetTexLevelParameterfv )(GLenum target, GLint level, GLenum pname, GLfloat *params);
extern void ( APIENTRY * qglGetTexLevelParameteriv )(GLenum target, GLint level, GLenum pname, GLint *params);
extern void ( APIENTRY * qglGetTexParameterfv )(GLenum target, GLenum pname, GLfloat *params);
extern void ( APIENTRY * qglGetTexParameteriv )(GLenum target, GLenum pname, GLint *params);
extern void ( APIENTRY * qglHint )(GLenum target, GLenum mode);
extern void ( APIENTRY * qglIndexMask )(GLuint mask);
extern void ( APIENTRY * qglIndexPointer )(GLenum type, GLsizei stride, const GLvoid *pointer);
extern void ( APIENTRY * qglIndexd )(GLdouble c);
extern void ( APIENTRY * qglIndexdv )(const GLdouble *c);
extern void ( APIENTRY * qglIndexf )(GLfloat c);
extern void ( APIENTRY * qglIndexfv )(const GLfloat *c);
extern void ( APIENTRY * qglIndexi )(GLint c);
extern void ( APIENTRY * qglIndexiv )(const GLint *c);
extern void ( APIENTRY * qglIndexs )(GLshort c);
extern void ( APIENTRY * qglIndexsv )(const GLshort *c);
extern void ( APIENTRY * qglIndexub )(GLubyte c);
extern void ( APIENTRY * qglIndexubv )(const GLubyte *c);
extern void ( APIENTRY * qglInitNames )(void);
extern void ( APIENTRY * qglInterleavedArrays )(GLenum format, GLsizei stride, const GLvoid *pointer);
extern GLboolean ( APIENTRY * qglIsEnabled )(GLenum cap);
extern GLboolean ( APIENTRY * qglIsList )(GLuint list);
extern GLboolean ( APIENTRY * qglIsTexture )(GLuint texture);
extern void ( APIENTRY * qglLightModelf )(GLenum pname, GLfloat param);
extern void ( APIENTRY * qglLightModelfv )(GLenum pname, const GLfloat *params);
extern void ( APIENTRY * qglLightModeli )(GLenum pname, GLint param);
extern void ( APIENTRY * qglLightModeliv )(GLenum pname, const GLint *params);
extern void ( APIENTRY * qglLightf )(GLenum light, GLenum pname, GLfloat param);
extern void ( APIENTRY * qglLightfv )(GLenum light, GLenum pname, const GLfloat *params);
extern void ( APIENTRY * qglLighti )(GLenum light, GLenum pname, GLint param);
extern void ( APIENTRY * qglLightiv )(GLenum light, GLenum pname, const GLint *params);
extern void ( APIENTRY * qglLineStipple )(GLint factor, GLushort pattern);
extern void ( APIENTRY * qglLineWidth )(GLfloat width);
extern void ( APIENTRY * qglListBase )(GLuint base);
extern void ( APIENTRY * qglLoadIdentity )(void);
extern void ( APIENTRY * qglLoadMatrixd )(const GLdouble *m);
extern void ( APIENTRY * qglLoadMatrixf )(const GLfloat *m);
extern void ( APIENTRY * qglLoadName )(GLuint name);
extern void ( APIENTRY * qglLogicOp )(GLenum opcode);
extern void ( APIENTRY * qglMap1d )(GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points);
extern void ( APIENTRY * qglMap1f )(GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points);
extern void ( APIENTRY * qglMap2d )(GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points);
extern void ( APIENTRY * qglMap2f )(GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points);
extern void ( APIENTRY * qglMapGrid1d )(GLint un, GLdouble u1, GLdouble u2);
extern void ( APIENTRY * qglMapGrid1f )(GLint un, GLfloat u1, GLfloat u2);
extern void ( APIENTRY * qglMapGrid2d )(GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2);
extern void ( APIENTRY * qglMapGrid2f )(GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2);
extern void ( APIENTRY * qglMaterialf )(GLenum face, GLenum pname, GLfloat param);
extern void ( APIENTRY * qglMaterialfv )(GLenum face, GLenum pname, const GLfloat *params);
extern void ( APIENTRY * qglMateriali )(GLenum face, GLenum pname, GLint param);
extern void ( APIENTRY * qglMaterialiv )(GLenum face, GLenum pname, const GLint *params);
extern void ( APIENTRY * qglMatrixMode )(GLenum mode);
extern void ( APIENTRY * qglMultMatrixd )(const GLdouble *m);
extern void ( APIENTRY * qglMultMatrixf )(const GLfloat *m);
extern void ( APIENTRY * qglNewList )(GLuint list, GLenum mode);
extern void ( APIENTRY * qglNormal3b )(GLbyte nx, GLbyte ny, GLbyte nz);
extern void ( APIENTRY * qglNormal3bv )(const GLbyte *v);
extern void ( APIENTRY * qglNormal3d )(GLdouble nx, GLdouble ny, GLdouble nz);
extern void ( APIENTRY * qglNormal3dv )(const GLdouble *v);
extern void ( APIENTRY * qglNormal3f )(GLfloat nx, GLfloat ny, GLfloat nz);
extern void ( APIENTRY * qglNormal3fv )(const GLfloat *v);
extern void ( APIENTRY * qglNormal3i )(GLint nx, GLint ny, GLint nz);
extern void ( APIENTRY * qglNormal3iv )(const GLint *v);
extern void ( APIENTRY * qglNormal3s )(GLshort nx, GLshort ny, GLshort nz);
extern void ( APIENTRY * qglNormal3sv )(const GLshort *v);
extern void ( APIENTRY * qglNormalPointer )(GLenum type, GLsizei stride, const GLvoid *pointer);
extern void ( APIENTRY * qglOrtho )(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
extern void ( APIENTRY * qglPassThrough )(GLfloat token);
extern void ( APIENTRY * qglPixelMapfv )(GLenum m, GLsizei mapsize, const GLfloat *values); //rwwRMG - map->m (avoid map type conflict)
extern void ( APIENTRY * qglPixelMapuiv )(GLenum m, GLsizei mapsize, const GLuint *values); //rwwRMG - map->m (avoid map type conflict)
extern void ( APIENTRY * qglPixelMapusv )(GLenum m, GLsizei mapsize, const GLushort *values); //rwwRMG - map->m (avoid map type conflict)
extern void ( APIENTRY * qglPixelStoref )(GLenum pname, GLfloat param);
extern void ( APIENTRY * qglPixelStorei )(GLenum pname, GLint param);
extern void ( APIENTRY * qglPixelTransferf )(GLenum pname, GLfloat param);
extern void ( APIENTRY * qglPixelTransferi )(GLenum pname, GLint param);
extern void ( APIENTRY * qglPixelZoom )(GLfloat xfactor, GLfloat yfactor);
extern void ( APIENTRY * qglPointSize )(GLfloat size);
extern void ( APIENTRY * qglPolygonMode )(GLenum face, GLenum mode);
extern void ( APIENTRY * qglPolygonOffset )(GLfloat factor, GLfloat units);
extern void ( APIENTRY * qglPolygonStipple )(const GLubyte *mask);
extern void ( APIENTRY * qglPopAttrib )(void);
extern void ( APIENTRY * qglPopClientAttrib )(void);
extern void ( APIENTRY * qglPopMatrix )(void);
extern void ( APIENTRY * qglPopName )(void);
extern void ( APIENTRY * qglPrioritizeTextures )(GLsizei n, const GLuint *textures, const GLclampf *priorities);
extern void ( APIENTRY * qglPushAttrib )(GLbitfield mask);
extern void ( APIENTRY * qglPushClientAttrib )(GLbitfield mask);
extern void ( APIENTRY * qglPushMatrix )(void);
extern void ( APIENTRY * qglPushName )(GLuint name);
extern void ( APIENTRY * qglRasterPos2d )(GLdouble x, GLdouble y);
extern void ( APIENTRY * qglRasterPos2dv )(const GLdouble *v);
extern void ( APIENTRY * qglRasterPos2f )(GLfloat x, GLfloat y);
extern void ( APIENTRY * qglRasterPos2fv )(const GLfloat *v);
extern void ( APIENTRY * qglRasterPos2i )(GLint x, GLint y);
extern void ( APIENTRY * qglRasterPos2iv )(const GLint *v);
extern void ( APIENTRY * qglRasterPos2s )(GLshort x, GLshort y);
extern void ( APIENTRY * qglRasterPos2sv )(const GLshort *v);
extern void ( APIENTRY * qglRasterPos3d )(GLdouble x, GLdouble y, GLdouble z);
extern void ( APIENTRY * qglRasterPos3dv )(const GLdouble *v);
extern void ( APIENTRY * qglRasterPos3f )(GLfloat x, GLfloat y, GLfloat z);
extern void ( APIENTRY * qglRasterPos3fv )(const GLfloat *v);
extern void ( APIENTRY * qglRasterPos3i )(GLint x, GLint y, GLint z);
extern void ( APIENTRY * qglRasterPos3iv )(const GLint *v);
extern void ( APIENTRY * qglRasterPos3s )(GLshort x, GLshort y, GLshort z);
extern void ( APIENTRY * qglRasterPos3sv )(const GLshort *v);
extern void ( APIENTRY * qglRasterPos4d )(GLdouble x, GLdouble y, GLdouble z, GLdouble w);
extern void ( APIENTRY * qglRasterPos4dv )(const GLdouble *v);
extern void ( APIENTRY * qglRasterPos4f )(GLfloat x, GLfloat y, GLfloat z, GLfloat w);
extern void ( APIENTRY * qglRasterPos4fv )(const GLfloat *v);
extern void ( APIENTRY * qglRasterPos4i )(GLint x, GLint y, GLint z, GLint w);
extern void ( APIENTRY * qglRasterPos4iv )(const GLint *v);
extern void ( APIENTRY * qglRasterPos4s )(GLshort x, GLshort y, GLshort z, GLshort w);
extern void ( APIENTRY * qglRasterPos4sv )(const GLshort *v);
extern void ( APIENTRY * qglReadBuffer )(GLenum mode);
extern void ( APIENTRY * qglReadPixels )(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels);
extern void ( APIENTRY * qglRectd )(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2);
extern void ( APIENTRY * qglRectdv )(const GLdouble *v1, const GLdouble *v2);
extern void ( APIENTRY * qglRectf )(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2);
extern void ( APIENTRY * qglRectfv )(const GLfloat *v1, const GLfloat *v2);
extern void ( APIENTRY * qglRecti )(GLint x1, GLint y1, GLint x2, GLint y2);
extern void ( APIENTRY * qglRectiv )(const GLint *v1, const GLint *v2);
extern void ( APIENTRY * qglRects )(GLshort x1, GLshort y1, GLshort x2, GLshort y2);
extern void ( APIENTRY * qglRectsv )(const GLshort *v1, const GLshort *v2);
extern GLint ( APIENTRY * qglRenderMode )(GLenum mode);
extern void ( APIENTRY * qglRotated )(GLdouble angle, GLdouble x, GLdouble y, GLdouble z);
extern void ( APIENTRY * qglRotatef )(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
extern void ( APIENTRY * qglScaled )(GLdouble x, GLdouble y, GLdouble z);
extern void ( APIENTRY * qglScalef )(GLfloat x, GLfloat y, GLfloat z);
extern void ( APIENTRY * qglScissor )(GLint x, GLint y, GLsizei width, GLsizei height);
extern void ( APIENTRY * qglSelectBuffer )(GLsizei size, GLuint *buffer);
extern void ( APIENTRY * qglShadeModel )(GLenum mode);
extern void ( APIENTRY * qglStencilFunc )(GLenum func, GLint ref, GLuint mask);
extern void ( APIENTRY * qglStencilMask )(GLuint mask);
extern void ( APIENTRY * qglStencilOp )(GLenum fail, GLenum zfail, GLenum zpass);
extern void ( APIENTRY * qglTexCoord1d )(GLdouble s);
extern void ( APIENTRY * qglTexCoord1dv )(const GLdouble *v);
extern void ( APIENTRY * qglTexCoord1f )(GLfloat s);
extern void ( APIENTRY * qglTexCoord1fv )(const GLfloat *v);
extern void ( APIENTRY * qglTexCoord1i )(GLint s);
extern void ( APIENTRY * qglTexCoord1iv )(const GLint *v);
extern void ( APIENTRY * qglTexCoord1s )(GLshort s);
extern void ( APIENTRY * qglTexCoord1sv )(const GLshort *v);
extern void ( APIENTRY * qglTexCoord2d )(GLdouble s, GLdouble t);
extern void ( APIENTRY * qglTexCoord2dv )(const GLdouble *v);
extern void ( APIENTRY * qglTexCoord2f )(GLfloat s, GLfloat t);
extern void ( APIENTRY * qglTexCoord2fv )(const GLfloat *v);
extern void ( APIENTRY * qglTexCoord2i )(GLint s, GLint t);
extern void ( APIENTRY * qglTexCoord2iv )(const GLint *v);
extern void ( APIENTRY * qglTexCoord2s )(GLshort s, GLshort t);
extern void ( APIENTRY * qglTexCoord2sv )(const GLshort *v);
extern void ( APIENTRY * qglTexCoord3d )(GLdouble s, GLdouble t, GLdouble r);
extern void ( APIENTRY * qglTexCoord3dv )(const GLdouble *v);
extern void ( APIENTRY * qglTexCoord3f )(GLfloat s, GLfloat t, GLfloat r);
extern void ( APIENTRY * qglTexCoord3fv )(const GLfloat *v);
extern void ( APIENTRY * qglTexCoord3i )(GLint s, GLint t, GLint r);
extern void ( APIENTRY * qglTexCoord3iv )(const GLint *v);
extern void ( APIENTRY * qglTexCoord3s )(GLshort s, GLshort t, GLshort r);
extern void ( APIENTRY * qglTexCoord3sv )(const GLshort *v);
extern void ( APIENTRY * qglTexCoord4d )(GLdouble s, GLdouble t, GLdouble r, GLdouble q);
extern void ( APIENTRY * qglTexCoord4dv )(const GLdouble *v);
extern void ( APIENTRY * qglTexCoord4f )(GLfloat s, GLfloat t, GLfloat r, GLfloat q);
extern void ( APIENTRY * qglTexCoord4fv )(const GLfloat *v);
extern void ( APIENTRY * qglTexCoord4i )(GLint s, GLint t, GLint r, GLint q);
extern void ( APIENTRY * qglTexCoord4iv )(const GLint *v);
extern void ( APIENTRY * qglTexCoord4s )(GLshort s, GLshort t, GLshort r, GLshort q);
extern void ( APIENTRY * qglTexCoord4sv )(const GLshort *v);
extern void ( APIENTRY * qglTexCoordPointer )(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
extern void ( APIENTRY * qglTexEnvf )(GLenum target, GLenum pname, GLfloat param);
extern void ( APIENTRY * qglTexEnvfv )(GLenum target, GLenum pname, const GLfloat *params);
extern void ( APIENTRY * qglTexEnvi )(GLenum target, GLenum pname, GLint param);
extern void ( APIENTRY * qglTexEnviv )(GLenum target, GLenum pname, const GLint *params);
extern void ( APIENTRY * qglTexGend )(GLenum coord, GLenum pname, GLdouble param);
extern void ( APIENTRY * qglTexGendv )(GLenum coord, GLenum pname, const GLdouble *params);
extern void ( APIENTRY * qglTexGenf )(GLenum coord, GLenum pname, GLfloat param);
extern void ( APIENTRY * qglTexGenfv )(GLenum coord, GLenum pname, const GLfloat *params);
extern void ( APIENTRY * qglTexGeni )(GLenum coord, GLenum pname, GLint param);
extern void ( APIENTRY * qglTexGeniv )(GLenum coord, GLenum pname, const GLint *params);
extern void ( APIENTRY * qglTexImage1D )(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
extern void ( APIENTRY * qglTexImage2D )(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
extern void ( APIENTRY * qglTexParameterf )(GLenum target, GLenum pname, GLfloat param);
extern void ( APIENTRY * qglTexParameterfv )(GLenum target, GLenum pname, const GLfloat *params);
extern void ( APIENTRY * qglTexParameteri )(GLenum target, GLenum pname, GLint param);
extern void ( APIENTRY * qglTexParameteriv )(GLenum target, GLenum pname, const GLint *params);
extern void ( APIENTRY * qglTexSubImage1D )(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels);
extern void ( APIENTRY * qglTexSubImage2D )(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
extern void ( APIENTRY * qglTranslated )(GLdouble x, GLdouble y, GLdouble z);
extern void ( APIENTRY * qglTranslatef )(GLfloat x, GLfloat y, GLfloat z);
extern void ( APIENTRY * qglVertex2d )(GLdouble x, GLdouble y);
extern void ( APIENTRY * qglVertex2dv )(const GLdouble *v);
extern void ( APIENTRY * qglVertex2f )(GLfloat x, GLfloat y);
extern void ( APIENTRY * qglVertex2fv )(const GLfloat *v);
extern void ( APIENTRY * qglVertex2i )(GLint x, GLint y);
extern void ( APIENTRY * qglVertex2iv )(const GLint *v);
extern void ( APIENTRY * qglVertex2s )(GLshort x, GLshort y);
extern void ( APIENTRY * qglVertex2sv )(const GLshort *v);
extern void ( APIENTRY * qglVertex3d )(GLdouble x, GLdouble y, GLdouble z);
extern void ( APIENTRY * qglVertex3dv )(const GLdouble *v);
extern void ( APIENTRY * qglVertex3f )(GLfloat x, GLfloat y, GLfloat z);
extern void ( APIENTRY * qglVertex3fv )(const GLfloat *v);
extern void ( APIENTRY * qglVertex3i )(GLint x, GLint y, GLint z);
extern void ( APIENTRY * qglVertex3iv )(const GLint *v);
extern void ( APIENTRY * qglVertex3s )(GLshort x, GLshort y, GLshort z);
extern void ( APIENTRY * qglVertex3sv )(const GLshort *v);
extern void ( APIENTRY * qglVertex4d )(GLdouble x, GLdouble y, GLdouble z, GLdouble w);
extern void ( APIENTRY * qglVertex4dv )(const GLdouble *v);
extern void ( APIENTRY * qglVertex4f )(GLfloat x, GLfloat y, GLfloat z, GLfloat w);
extern void ( APIENTRY * qglVertex4fv )(const GLfloat *v);
extern void ( APIENTRY * qglVertex4i )(GLint x, GLint y, GLint z, GLint w);
extern void ( APIENTRY * qglVertex4iv )(const GLint *v);
extern void ( APIENTRY * qglVertex4s )(GLshort x, GLshort y, GLshort z, GLshort w);
extern void ( APIENTRY * qglVertex4sv )(const GLshort *v);
extern void ( APIENTRY * qglVertexPointer )(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
extern void ( APIENTRY * qglViewport )(GLint x, GLint y, GLsizei width, GLsizei height);
#else
#define qglAccum glAccum
#define qglAlphaFunc glAlphaFunc
#define qglAreTexturesResident glAreTexturesResident
#define qglArrayElement glArrayElement
#define qglBegin glBegin
#define qglBindTexture glBindTexture
#define qglBitmap glBitmap
#define qglBlendFunc glBlendFunc
#define qglCallList glCallList
#define qglCallLists glCallLists
#define qglClear glClear
#define qglClearAccum glClearAccum
#define qglClearColor glClearColor
#define qglClearDepth glClearDepth
#define qglClearIndex glClearIndex
#define qglClearStencil glClearStencil
#define qglClipPlane glClipPlane
#define qglColor3b glColor3b
#define qglColor3bv glColor3bv
#define qglColor3d glColor3d
#define qglColor3dv glColor3dv
#define qglColor3f glColor3f
#define qglColor3fv glColor3fv
#define qglColor3i glColor3i
#define qglColor3iv glColor3iv
#define qglColor3s glColor3s
#define qglColor3sv glColor3sv
#define qglColor3ub glColor3ub
#define qglColor3ubv glColor3ubv
#define qglColor3ui glColor3ui
#define qglColor3uiv glColor3uiv
#define qglColor3us glColor3us
#define qglColor3usv glColor3usv
#define qglColor4b glColor4b
#define qglColor4bv glColor4bv
#define qglColor4d glColor4d
#define qglColor4dv glColor4dv
#define qglColor4f glColor4f
#define qglColor4fv glColor4fv
#define qglColor4i glColor4i
#define qglColor4iv glColor4iv
#define qglColor4s glColor4s
#define qglColor4sv glColor4sv
#define qglColor4ub glColor4ub
#define qglColor4ubv glColor4ubv
#define qglColor4ui glColor4ui
#define qglColor4uiv glColor4uiv
#define qglColor4us glColor4us
#define qglColor4usv glColor4usv
#define qglColorMask glColorMask
#define qglColorMaterial glColorMaterial
#define qglColorPointer glColorPointer
#define qglCopyPixels glCopyPixels
#define qglCopyTexImage1D glCopyTexImage1D
#define qglCopyTexImage2D glCopyTexImage2D
#define qglCopyTexSubImage1D glCopyTexSubImage1D
#define qglCopyTexSubImage2D glCopyTexSubImage2D
#define qglCullFace glCullFace
#define qglDeleteLists glDeleteLists
#define qglDeleteTextures glDeleteTextures
#define qglDepthFunc glDepthFunc
#define qglDepthMask glDepthMask
#define qglDepthRange glDepthRange
#define qglDisable glDisable
#define qglDisableClientState glDisableClientState
#define qglDrawArrays glDrawArrays
#define qglDrawBuffer glDrawBuffer
#define qglDrawElements glDrawElements
#define qglDrawPixels glDrawPixels
#define qglEdgeFlag glEdgeFlag
#define qglEdgeFlagPointer glEdgeFlagPointer
#define qglEdgeFlagv glEdgeFlagv
#define qglEnable glEnable
#define qglEnableClientState glEnableClientState
#define qglEnd glEnd
#define qglEndList glEndList
#define qglEvalCoord1d glEvalCoord1d
#define qglEvalCoord1dv glEvalCoord1dv
#define qglEvalCoord1f glEvalCoord1f
#define qglEvalCoord1fv glEvalCoord1fv
#define qglEvalCoord2d glEvalCoord2d
#define qglEvalCoord2dv glEvalCoord2dv
#define qglEvalCoord2f glEvalCoord2f
#define qglEvalCoord2fv glEvalCoord2fv
#define qglEvalMesh1 glEvalMesh1
#define qglEvalMesh2 glEvalMesh2
#define qglEvalPoint1 glEvalPoint1
#define qglEvalPoint2 glEvalPoint2
#define qglFeedbackBuffer glFeedbackBuffer
#define qglFinish glFinish
#define qglFlush glFlush
#define qglFogf glFogf
#define qglFogfv glFogfv
#define qglFogi glFogi
#define qglFogiv glFogiv
#define qglFrontFace glFrontFace
#define qglFrustum glFrustum
#define qglGenLists glGenLists
#define qglGenTextures glGenTextures
#define qglGetBooleanv glGetBooleanv
#define qglGetClipPlane glGetClipPlane
#define qglGetDoublev glGetDoublev
#define qglGetError glGetError
#define qglGetFloatv glGetFloatv
#define qglGetIntegerv glGetIntegerv
#define qglGetLightfv glGetLightfv
#define qglGetLightiv glGetLightiv
#define qglGetMapdv glGetMapdv
#define qglGetMapfv glGetMapfv
#define qglGetMapiv glGetMapiv
#define qglGetMaterialfv glGetMaterialfv
#define qglGetMaterialiv glGetMaterialiv
#define qglGetPixelMapfv glGetPixelMapfv
#define qglGetPixelMapuiv glGetPixelMapuiv
#define qglGetPixelMapusv glGetPixelMapusv
#define qglGetPointerv glGetPointerv
#define qglGetPolygonStipple glGetPolygonStipple
#define qglGetString glGetString
#define qglGetTexGendv glGetTexGendv
#define qglGetTexGenfv glGetTexGenfv
#define qglGetTexGeniv glGetTexGeniv
#define qglGetTexImage glGetTexImage
#define qglGetTexLevelParameterfv glGetTexLevelParameterfv
#define qglGetTexLevelParameteriv glGetTexLevelParameteriv
#define qglGetTexParameterfv glGetTexParameterfv
#define qglGetTexParameteriv glGetTexParameteriv
#define qglHint glHint
#define qglIndexMask glIndexMask
#define qglIndexPointer glIndexPointer
#define qglIndexd glIndexd
#define qglIndexdv glIndexdv
#define qglIndexf glIndexf
#define qglIndexfv glIndexfv
#define qglIndexi glIndexi
#define qglIndexiv glIndexiv
#define qglIndexs glIndexs
#define qglIndexsv glIndexsv
#define qglIndexub glIndexub
#define qglIndexubv glIndexubv
#define qglInitNames glInitNames
#define qglInterleavedArrays glInterleavedArrays
#define qglIsEnabled glIsEnabled
#define qglIsList glIsList
#define qglIsTexture glIsTexture
#define qglLightModelf glLightModelf
#define qglLightModelfv glLightModelfv
#define qglLightModeli glLightModeli
#define qglLightModeliv glLightModeliv
#define qglLightf glLightf
#define qglLightfv glLightfv
#define qglLighti glLighti
#define qglLightiv glLightiv
#define qglLineStipple glLineStipple
#define qglLineWidth glLineWidth
#define qglListBase glListBase
#define qglLoadIdentity glLoadIdentity
#define qglLoadMatrixd glLoadMatrixd
#define qglLoadMatrixf glLoadMatrixf
#define qglLoadName glLoadName
#define qglLogicOp glLogicOp
#define qglMap1d glMap1d
#define qglMap1f glMap1f
#define qglMap2d glMap2d
#define qglMap2f glMap2f
#define qglMapGrid1d glMapGrid1d
#define qglMapGrid1f glMapGrid1f
#define qglMapGrid2d glMapGrid2d
#define qglMapGrid2f glMapGrid2f
#define qglMaterialf glMaterialf
#define qglMaterialfv glMaterialfv
#define qglMateriali glMateriali
#define qglMaterialiv glMaterialiv
#define qglMatrixMode glMatrixMode
#define qglMultMatrixd glMultMatrixd
#define qglMultMatrixf glMultMatrixf
#define qglNewList glNewList
#define qglNormal3b glNormal3b
#define qglNormal3bv glNormal3bv
#define qglNormal3d glNormal3d
#define qglNormal3dv glNormal3dv
#define qglNormal3f glNormal3f
#define qglNormal3fv glNormal3fv
#define qglNormal3i glNormal3i
#define qglNormal3iv glNormal3iv
#define qglNormal3s glNormal3s
#define qglNormal3sv glNormal3sv
#define qglNormalPointer glNormalPointer
#define qglOrtho glOrtho
#define qglPassThrough glPassThrough
#define qglPixelMapfv glPixelMapfv
#define qglPixelMapuiv glPixelMapuiv
#define qglPixelMapusv glPixelMapusv
#define qglPixelStoref glPixelStoref
#define qglPixelStorei glPixelStorei
#define qglPixelTransferf glPixelTransferf
#define qglPixelTransferi glPixelTransferi
#define qglPixelZoom glPixelZoom
#define qglPointSize glPointSize
#define qglPolygonMode glPolygonMode
#define qglPolygonOffset glPolygonOffset
#define qglPolygonStipple glPolygonStipple
#define qglPopAttrib glPopAttrib
#define qglPopClientAttrib glPopClientAttrib
#define qglPopMatrix glPopMatrix
#define qglPopName glPopName
#define qglPrioritizeTextures glPrioritizeTextures
#define qglPushAttrib glPushAttrib
#define qglPushClientAttrib glPushClientAttrib
#define qglPushMatrix glPushMatrix
#define qglPushName glPushName
#define qglRasterPos2d glRasterPos2d
#define qglRasterPos2dv glRasterPos2dv
#define qglRasterPos2f glRasterPos2f
#define qglRasterPos2fv glRasterPos2fv
#define qglRasterPos2i glRasterPos2i
#define qglRasterPos2iv glRasterPos2iv
#define qglRasterPos2s glRasterPos2s
#define qglRasterPos2sv glRasterPos2sv
#define qglRasterPos3d glRasterPos3d
#define qglRasterPos3dv glRasterPos3dv
#define qglRasterPos3f glRasterPos3f
#define qglRasterPos3fv glRasterPos3fv
#define qglRasterPos3i glRasterPos3i
#define qglRasterPos3iv glRasterPos3iv
#define qglRasterPos3s glRasterPos3s
#define qglRasterPos3sv glRasterPos3sv
#define qglRasterPos4d glRasterPos4d
#define qglRasterPos4dv glRasterPos4dv
#define qglRasterPos4f glRasterPos4f
#define qglRasterPos4fv glRasterPos4fv
#define qglRasterPos4i glRasterPos4i
#define qglRasterPos4iv glRasterPos4iv
#define qglRasterPos4s glRasterPos4s
#define qglRasterPos4sv glRasterPos4sv
#define qglReadBuffer glReadBuffer
#define qglReadPixels glReadPixels
#define qglRectd glRectd
#define qglRectdv glRectdv
#define qglRectf glRectf
#define qglRectfv glRectfv
#define qglRecti glRecti
#define qglRectiv glRectiv
#define qglRects glRects
#define qglRectsv glRectsv
#define qglRenderMode glRenderMode
#define qglRotated glRotated
#define qglRotatef glRotatef
#define qglScaled glScaled
#define qglScalef glScalef
#define qglScissor glScissor
#define qglSelectBuffer glSelectBuffer
#define qglShadeModel glShadeModel
#define qglStencilFunc glStencilFunc
#define qglStencilMask glStencilMask
#define qglStencilOp glStencilOp
#define qglTexCoord1d glTexCoord1d
#define qglTexCoord1dv glTexCoord1dv
#define qglTexCoord1f glTexCoord1f
#define qglTexCoord1fv glTexCoord1fv
#define qglTexCoord1i glTexCoord1i
#define qglTexCoord1iv glTexCoord1iv
#define qglTexCoord1s glTexCoord1s
#define qglTexCoord1sv glTexCoord1sv
#define qglTexCoord2d glTexCoord2d
#define qglTexCoord2dv glTexCoord2dv
#define qglTexCoord2f glTexCoord2f
#define qglTexCoord2fv glTexCoord2fv
#define qglTexCoord2i glTexCoord2i
#define qglTexCoord2iv glTexCoord2iv
#define qglTexCoord2s glTexCoord2s
#define qglTexCoord2sv glTexCoord2sv
#define qglTexCoord3d glTexCoord3d
#define qglTexCoord3dv glTexCoord3dv
#define qglTexCoord3f glTexCoord3f
#define qglTexCoord3fv glTexCoord3fv
#define qglTexCoord3i glTexCoord3i
#define qglTexCoord3iv glTexCoord3iv
#define qglTexCoord3s glTexCoord3s
#define qglTexCoord3sv glTexCoord3sv
#define qglTexCoord4d glTexCoord4d
#define qglTexCoord4dv glTexCoord4dv
#define qglTexCoord4f glTexCoord4f
#define qglTexCoord4fv glTexCoord4fv
#define qglTexCoord4i glTexCoord4i
#define qglTexCoord4iv glTexCoord4iv
#define qglTexCoord4s glTexCoord4s
#define qglTexCoord4sv glTexCoord4sv
#define qglTexCoordPointer glTexCoordPointer
#define qglTexEnvf glTexEnvf
#define qglTexEnvfv glTexEnvfv
#define qglTexEnvi glTexEnvi
#define qglTexEnviv glTexEnviv
#define qglTexGend glTexGend
#define qglTexGendv glTexGendv
#define qglTexGenf glTexGenf
#define qglTexGenfv glTexGenfv
#define qglTexGeni glTexGeni
#define qglTexGeniv glTexGeniv
#define qglTexImage1D glTexImage1D
#define qglTexImage2D glTexImage2D
#define qglTexParameterf glTexParameterf
#define qglTexParameterfv glTexParameterfv
#define qglTexParameteri glTexParameteri
#define qglTexParameteriv glTexParameteriv
#define qglTexSubImage1D glTexSubImage1D
#define qglTexSubImage2D glTexSubImage2D
#define qglTranslated glTranslated
#define qglTranslatef glTranslatef
#define qglVertex2d glVertex2d
#define qglVertex2dv glVertex2dv
#define qglVertex2f glVertex2f
#define qglVertex2fv glVertex2fv
#define qglVertex2i glVertex2i
#define qglVertex2iv glVertex2iv
#define qglVertex2s glVertex2s
#define qglVertex2sv glVertex2sv
#define qglVertex3d glVertex3d
#define qglVertex3dv glVertex3dv
#define qglVertex3f glVertex3f
#define qglVertex3fv glVertex3fv
#define qglVertex3i glVertex3i
#define qglVertex3iv glVertex3iv
#define qglVertex3s glVertex3s
#define qglVertex3sv glVertex3sv
#define qglVertex4d glVertex4d
#define qglVertex4dv glVertex4dv
#define qglVertex4f glVertex4f
#define qglVertex4fv glVertex4fv
#define qglVertex4i glVertex4i
#define qglVertex4iv glVertex4iv
#define qglVertex4s glVertex4s
#define qglVertex4sv glVertex4sv
#define qglVertexPointer glVertexPointer
#define qglViewport glViewport
#endif //_WIN32
// GL_EXT_draw_range_elements
extern void     (APIENTRY * qglDrawRangeElementsEXT) (GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices);

// GL_EXT_multi_draw_arrays
extern void     (APIENTRY * qglMultiDrawArraysEXT) (GLenum, const GLint *, const GLsizei *, GLsizei);
extern void     (APIENTRY * qglMultiDrawElementsEXT) (GLenum, const GLsizei *, GLenum, const GLvoid **, GLsizei);

// GL_ARB_shading_language_100
#ifndef GL_ARB_shading_language_100
#define GL_ARB_shading_language_100
#define GL_SHADING_LANGUAGE_VERSION_ARB 0x8B8C
#endif

// GL_ARB_vertex_program
extern void     (APIENTRY * qglVertexAttrib4fARB) (GLuint, GLfloat, GLfloat, GLfloat, GLfloat);
extern void     (APIENTRY * qglVertexAttrib4fvARB) (GLuint, const GLfloat *);
extern void     (APIENTRY * qglVertexAttribPointerARB) (GLuint index, GLint size, GLenum type, GLboolean normalized,
														GLsizei stride, const GLvoid * pointer);
extern void     (APIENTRY * qglEnableVertexAttribArrayARB) (GLuint index);
extern void     (APIENTRY * qglDisableVertexAttribArrayARB) (GLuint index);

// GL_ARB_vertex_buffer_object
extern void     (APIENTRY * qglBindBufferARB) (GLenum target, GLuint buffer);
extern void     (APIENTRY * qglDeleteBuffersARB) (GLsizei n, const GLuint * buffers);
extern void     (APIENTRY * qglGenBuffersARB) (GLsizei n, GLuint * buffers);
extern          GLboolean(APIENTRY * qglIsBufferARB) (GLuint buffer);
extern void     (APIENTRY * qglBufferDataARB) (GLenum target, GLint size, const GLvoid * data, GLenum usage);
extern void     (APIENTRY * qglBufferSubDataARB) (GLenum target, GLint offset, GLint size, const GLvoid * data);
extern void     (APIENTRY * qglGetBufferSubDataARB) (GLenum target, GLintptrARB offset, GLsizeiptrARB size, GLvoid * data);
extern void     (APIENTRY * qglGetBufferParameterivARB) (GLenum target, GLenum pname, GLint * params);
extern void     (APIENTRY * qglGetBufferPointervARB) (GLenum target, GLenum pname, GLvoid * *params);

// GL_ARB_shader_objects
extern void     (APIENTRY * qglDeleteObjectARB) (GLhandleARB obj);
extern          GLhandleARB(APIENTRY * qglGetHandleARB) (GLenum pname);
extern void     (APIENTRY * qglDetachObjectARB) (GLhandleARB containerObj, GLhandleARB attachedObj);
extern          GLhandleARB(APIENTRY * qglCreateShaderObjectARB) (GLenum shaderType);
extern void     (APIENTRY * qglShaderSourceARB) (GLhandleARB shaderObj, GLsizei count, const GLcharARB * *string,
												 const GLint * length);
extern void     (APIENTRY * qglCompileShaderARB) (GLhandleARB shaderObj);
extern          GLhandleARB(APIENTRY * qglCreateProgramObjectARB) (void);
extern void     (APIENTRY * qglAttachObjectARB) (GLhandleARB containerObj, GLhandleARB obj);
extern void     (APIENTRY * qglLinkProgramARB) (GLhandleARB programObj);
extern void     (APIENTRY * qglUseProgramObjectARB) (GLhandleARB programObj);
extern void     (APIENTRY * qglValidateProgramARB) (GLhandleARB programObj);
extern void     (APIENTRY * qglUniform1fARB) (GLint location, GLfloat v0);
extern void     (APIENTRY * qglUniform2fARB) (GLint location, GLfloat v0, GLfloat v1);
extern void     (APIENTRY * qglUniform3fARB) (GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
extern void     (APIENTRY * qglUniform4fARB) (GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
extern void     (APIENTRY * qglUniform1iARB) (GLint location, GLint v0);
extern void     (APIENTRY * qglUniform2iARB) (GLint location, GLint v0, GLint v1);
extern void     (APIENTRY * qglUniform3iARB) (GLint location, GLint v0, GLint v1, GLint v2);
extern void     (APIENTRY * qglUniform4iARB) (GLint location, GLint v0, GLint v1, GLint v2, GLint v3);
extern void     (APIENTRY * qglUniform1fvARB) (GLint location, GLsizei count, const GLfloat * value);
extern void     (APIENTRY * qglUniform2fvARB) (GLint location, GLsizei count, const GLfloat * value);
extern void     (APIENTRY * qglUniform3fvARB) (GLint location, GLsizei count, const GLfloat * value);
extern void     (APIENTRY * qglUniform4fvARB) (GLint location, GLsizei count, const GLfloat * value);
extern void     (APIENTRY * qglUniform2ivARB) (GLint location, GLsizei count, const GLint * value);
extern void     (APIENTRY * qglUniform3ivARB) (GLint location, GLsizei count, const GLint * value);
extern void     (APIENTRY * qglUniform4ivARB) (GLint location, GLsizei count, const GLint * value);
extern void     (APIENTRY * qglUniformMatrix2fvARB) (GLint location, GLsizei count, GLboolean transpose, const GLfloat * value);
extern void     (APIENTRY * qglUniformMatrix3fvARB) (GLint location, GLsizei count, GLboolean transpose, const GLfloat * value);
extern void     (APIENTRY * qglUniformMatrix4fvARB) (GLint location, GLsizei count, GLboolean transpose, const GLfloat * value);
extern void     (APIENTRY * qglGetObjectParameterfvARB) (GLhandleARB obj, GLenum pname, GLfloat * params);
extern void     (APIENTRY * qglGetObjectParameterivARB) (GLhandleARB obj, GLenum pname, GLint * params);
extern void     (APIENTRY * qglGetInfoLogARB) (GLhandleARB obj, GLsizei maxLength, GLsizei * length, GLcharARB * infoLog);
extern void     (APIENTRY * qglGetAttachedObjectsARB) (GLhandleARB containerObj, GLsizei maxCount, GLsizei * count,
													   GLhandleARB * obj);
extern          GLint(APIENTRY * qglGetUniformLocationARB) (GLhandleARB programObj, const GLcharARB * name);
extern void     (APIENTRY * qglGetActiveUniformARB) (GLhandleARB programObj, GLuint index, GLsizei maxIndex, GLsizei * length,
													 GLint * size, GLenum * type, GLcharARB * name);
extern void     (APIENTRY * qglGetUniformfvARB) (GLhandleARB programObj, GLint location, GLfloat * params);
extern void     (APIENTRY * qglGetUniformivARB) (GLhandleARB programObj, GLint location, GLint * params);
extern void     (APIENTRY * qglGetShaderSourceARB) (GLhandleARB obj, GLsizei maxLength, GLsizei * length, GLcharARB * source);

// GL_ARB_vertex_shader
extern void     (APIENTRY * qglBindAttribLocationARB) (GLhandleARB programObj, GLuint index, const GLcharARB * name);
extern void     (APIENTRY * qglGetActiveAttribARB) (GLhandleARB programObj, GLuint index, GLsizei maxLength, GLsizei * length,
													GLint * size, GLenum * type, GLcharARB * name);
extern          GLint(APIENTRY * qglGetAttribLocationARB) (GLhandleARB programObj, const GLcharARB * name);

// GL_ARB_texture_compression
extern void (APIENTRY * qglCompressedTexImage3DARB)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, 
	GLsizei depth, GLint border, GLsizei imageSize, const GLvoid *data);
extern void (APIENTRY * qglCompressedTexImage2DARB)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height,
	GLint border, GLsizei imageSize, const GLvoid *data);
extern void (APIENTRY * qglCompressedTexImage1DARB)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border,
	GLsizei imageSize, const GLvoid *data);
extern void (APIENTRY * qglCompressedTexSubImage3DARB)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset,
	GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const GLvoid *data);
extern void (APIENTRY * qglCompressedTexSubImage2DARB)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width,
	GLsizei height, GLenum format, GLsizei imageSize, const GLvoid *data);
extern void (APIENTRY * qglCompressedTexSubImage1DARB)(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, 
	GLsizei imageSize, const GLvoid *data);
extern void (APIENTRY * qglGetCompressedTexImageARB)(GLenum target, GLint lod,
	GLvoid *img);

// GL_NVX_gpu_memory_info
#ifndef GL_NVX_gpu_memory_info
#define GL_NVX_gpu_memory_info
#define GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX          0x9047
#define GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX    0x9048
#define GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX  0x9049
#define GL_GPU_MEMORY_INFO_EVICTION_COUNT_NVX            0x904A
#define GL_GPU_MEMORY_INFO_EVICTED_MEMORY_NVX            0x904B
#endif

// GL_ATI_meminfo
#ifndef GL_ATI_meminfo
#define GL_ATI_meminfo
#define GL_VBO_FREE_MEMORY_ATI                    0x87FB
#define GL_TEXTURE_FREE_MEMORY_ATI                0x87FC
#define GL_RENDERBUFFER_FREE_MEMORY_ATI           0x87FD
#endif

// GL_ARB_texture_float
#ifndef GL_ARB_texture_float
#define GL_ARB_texture_float
#define GL_TEXTURE_RED_TYPE_ARB             0x8C10
#define GL_TEXTURE_GREEN_TYPE_ARB           0x8C11
#define GL_TEXTURE_BLUE_TYPE_ARB            0x8C12
#define GL_TEXTURE_ALPHA_TYPE_ARB           0x8C13
#define GL_TEXTURE_LUMINANCE_TYPE_ARB       0x8C14
#define GL_TEXTURE_INTENSITY_TYPE_ARB       0x8C15
#define GL_TEXTURE_DEPTH_TYPE_ARB           0x8C16
#define GL_UNSIGNED_NORMALIZED_ARB          0x8C17
#define GL_RGBA32F_ARB                      0x8814
#define GL_RGB32F_ARB                       0x8815
#define GL_ALPHA32F_ARB                     0x8816
#define GL_INTENSITY32F_ARB                 0x8817
#define GL_LUMINANCE32F_ARB                 0x8818
#define GL_LUMINANCE_ALPHA32F_ARB           0x8819
#define GL_RGBA16F_ARB                      0x881A
#define GL_RGB16F_ARB                       0x881B
#define GL_ALPHA16F_ARB                     0x881C
#define GL_INTENSITY16F_ARB                 0x881D
#define GL_LUMINANCE16F_ARB                 0x881E
#define GL_LUMINANCE_ALPHA16F_ARB           0x881F
#endif

#ifndef GL_ARB_half_float_pixel
#define GL_ARB_half_float_pixel
#define GL_HALF_FLOAT_ARB                   0x140B
#endif

// GL_EXT_framebuffer_object
extern GLboolean (APIENTRY * qglIsRenderbufferEXT)(GLuint renderbuffer);
extern void (APIENTRY * qglBindRenderbufferEXT)(GLenum target, GLuint renderbuffer);
extern void (APIENTRY * qglDeleteRenderbuffersEXT)(GLsizei n, const GLuint *renderbuffers);
extern void (APIENTRY * qglGenRenderbuffersEXT)(GLsizei n, GLuint *renderbuffers);
extern void (APIENTRY * qglRenderbufferStorageEXT)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
extern void (APIENTRY * qglGetRenderbufferParameterivEXT)(GLenum target, GLenum pname, GLint *params);
extern GLboolean (APIENTRY * qglIsFramebufferEXT)(GLuint framebuffer);
extern void (APIENTRY * qglBindFramebufferEXT)(GLenum target, GLuint framebuffer);
extern void (APIENTRY * qglDeleteFramebuffersEXT)(GLsizei n, const GLuint *framebuffers);
extern void (APIENTRY * qglGenFramebuffersEXT)(GLsizei n, GLuint *framebuffers);
extern GLenum (APIENTRY * qglCheckFramebufferStatusEXT)(GLenum target);
extern void (APIENTRY * qglFramebufferTexture1DEXT)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture,
	GLint level);
extern void (APIENTRY * qglFramebufferTexture2DEXT)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture,
	GLint level);
extern void (APIENTRY * qglFramebufferTexture3DEXT)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture,
	GLint level, GLint zoffset);
extern void (APIENTRY * qglFramebufferRenderbufferEXT)(GLenum target, GLenum attachment, GLenum renderbuffertarget,
	GLuint renderbuffer);
extern void (APIENTRY * qglGetFramebufferAttachmentParameterivEXT)(GLenum target, GLenum attachment, GLenum pname, GLint *params);
extern void (APIENTRY * qglGenerateMipmapEXT)(GLenum target);

#ifndef GL_EXT_framebuffer_object
#define GL_EXT_framebuffer_object
#define GL_FRAMEBUFFER_EXT                     0x8D40
#define GL_RENDERBUFFER_EXT                    0x8D41
#define GL_STENCIL_INDEX1_EXT                  0x8D46
#define GL_STENCIL_INDEX4_EXT                  0x8D47
#define GL_STENCIL_INDEX8_EXT                  0x8D48
#define GL_STENCIL_INDEX16_EXT                 0x8D49
#define GL_RENDERBUFFER_WIDTH_EXT              0x8D42
#define GL_RENDERBUFFER_HEIGHT_EXT             0x8D43
#define GL_RENDERBUFFER_INTERNAL_FORMAT_EXT    0x8D44
#define GL_RENDERBUFFER_RED_SIZE_EXT           0x8D50
#define GL_RENDERBUFFER_GREEN_SIZE_EXT         0x8D51
#define GL_RENDERBUFFER_BLUE_SIZE_EXT          0x8D52
#define GL_RENDERBUFFER_ALPHA_SIZE_EXT         0x8D53
#define GL_RENDERBUFFER_DEPTH_SIZE_EXT         0x8D54
#define GL_RENDERBUFFER_STENCIL_SIZE_EXT       0x8D55
#define GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE_EXT            0x8CD0
#define GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME_EXT            0x8CD1
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL_EXT          0x8CD2
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE_EXT  0x8CD3
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_3D_ZOFFSET_EXT     0x8CD4
#define GL_COLOR_ATTACHMENT0_EXT                0x8CE0
#define GL_COLOR_ATTACHMENT1_EXT                0x8CE1
#define GL_COLOR_ATTACHMENT2_EXT                0x8CE2
#define GL_COLOR_ATTACHMENT3_EXT                0x8CE3
#define GL_COLOR_ATTACHMENT4_EXT                0x8CE4
#define GL_COLOR_ATTACHMENT5_EXT                0x8CE5
#define GL_COLOR_ATTACHMENT6_EXT                0x8CE6
#define GL_COLOR_ATTACHMENT7_EXT                0x8CE7
#define GL_COLOR_ATTACHMENT8_EXT                0x8CE8
#define GL_COLOR_ATTACHMENT9_EXT                0x8CE9
#define GL_COLOR_ATTACHMENT10_EXT               0x8CEA
#define GL_COLOR_ATTACHMENT11_EXT               0x8CEB
#define GL_COLOR_ATTACHMENT12_EXT               0x8CEC
#define GL_COLOR_ATTACHMENT13_EXT               0x8CED
#define GL_COLOR_ATTACHMENT14_EXT               0x8CEE
#define GL_COLOR_ATTACHMENT15_EXT               0x8CEF
#define GL_DEPTH_ATTACHMENT_EXT                 0x8D00
#define GL_STENCIL_ATTACHMENT_EXT               0x8D20
#define GL_FRAMEBUFFER_COMPLETE_EXT                          0x8CD5
#define GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT             0x8CD6
#define GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT     0x8CD7
#define GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT             0x8CD9
#define GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT                0x8CDA
#define GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT            0x8CDB
#define GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT            0x8CDC
#define GL_FRAMEBUFFER_UNSUPPORTED_EXT                       0x8CDD
#define GL_FRAMEBUFFER_BINDING_EXT             0x8CA6
#define GL_RENDERBUFFER_BINDING_EXT            0x8CA7
#define GL_MAX_COLOR_ATTACHMENTS_EXT           0x8CDF
#define GL_MAX_RENDERBUFFER_SIZE_EXT           0x84E8
#define GL_INVALID_FRAMEBUFFER_OPERATION_EXT   0x0506
#endif

// GL_EXT_packed_depth_stencil
#ifndef GL_EXT_packed_depth_stencil
#define GL_EXT_packed_depth_stencil
#define GL_DEPTH_STENCIL_EXT                              0x84F9
#define GL_UNSIGNED_INT_24_8_EXT                          0x84FA
#define GL_DEPTH24_STENCIL8_EXT                           0x88F0
#define GL_TEXTURE_STENCIL_SIZE_EXT                       0x88F1
#endif

// GL_ARB_occlusion_query
extern void (APIENTRY * qglGenQueriesARB)(GLsizei n, GLuint *ids);
extern void (APIENTRY * qglDeleteQueriesARB)(GLsizei n, const GLuint *ids);
extern GLboolean (APIENTRY * qglIsQueryARB)(GLuint id);
extern void (APIENTRY * qglBeginQueryARB)(GLenum target, GLuint id);
extern void (APIENTRY * qglEndQueryARB)(GLenum target);
extern void (APIENTRY * qglGetQueryivARB)(GLenum target, GLenum pname, GLint *params);
extern void (APIENTRY * qglGetQueryObjectivARB)(GLuint id, GLenum pname, GLint *params);
extern void (APIENTRY * qglGetQueryObjectuivARB)(GLuint id, GLenum pname, GLuint *params);

#ifndef GL_ARB_occlusion_query
#define GL_ARB_occlusion_query
#define GL_SAMPLES_PASSED_ARB                             0x8914
#define GL_QUERY_COUNTER_BITS_ARB                         0x8864
#define GL_CURRENT_QUERY_ARB                              0x8865
#define GL_QUERY_RESULT_ARB                               0x8866
#define GL_QUERY_RESULT_AVAILABLE_ARB                     0x8867
#endif

// GL_EXT_framebuffer_blit
extern void (APIENTRY * qglBlitFramebufferEXT)(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
                            GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
                            GLbitfield mask, GLenum filter);

#ifndef GL_EXT_framebuffer_blit
#define GL_EXT_framebuffer_blit
#define GL_READ_FRAMEBUFFER_EXT                0x8CA8
#define GL_DRAW_FRAMEBUFFER_EXT                0x8CA9
#define GL_DRAW_FRAMEBUFFER_BINDING_EXT        0x8CA6
#define GL_READ_FRAMEBUFFER_BINDING_EXT        0x8CAA
#endif

// GL_EXT_framebuffer_multisample
extern void (APIENTRY * qglRenderbufferStorageMultisampleEXT)(GLenum target, GLsizei samples,
	GLenum internalformat, GLsizei width, GLsizei height);

#ifndef GL_EXT_framebuffer_multisample
#define GL_EXT_framebuffer_multisample
#define GL_RENDERBUFFER_SAMPLES_EXT                0x8CAB
#define GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_EXT  0x8D56
#define GL_MAX_SAMPLES_EXT                         0x8D57
#endif

#ifndef GL_EXT_texture_sRGB
#define GL_EXT_texture_sRGB
#define GL_SRGB_EXT                                       0x8C40
#define GL_SRGB8_EXT                                      0x8C41
#define GL_SRGB_ALPHA_EXT                                 0x8C42
#define GL_SRGB8_ALPHA8_EXT                               0x8C43
#define GL_SLUMINANCE_ALPHA_EXT                           0x8C44
#define GL_SLUMINANCE8_ALPHA8_EXT                         0x8C45
#define GL_SLUMINANCE_EXT                                 0x8C46
#define GL_SLUMINANCE8_EXT                                0x8C47
#define GL_COMPRESSED_SRGB_EXT                            0x8C48
#define GL_COMPRESSED_SRGB_ALPHA_EXT                      0x8C49
#define GL_COMPRESSED_SLUMINANCE_EXT                      0x8C4A
#define GL_COMPRESSED_SLUMINANCE_ALPHA_EXT                0x8C4B
#define GL_COMPRESSED_SRGB_S3TC_DXT1_EXT                  0x8C4C
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT            0x8C4D
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT            0x8C4E
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT            0x8C4F
#endif

#ifndef GL_EXT_framebuffer_sRGB
#define GL_EXT_framebuffer_sRGB
#define GL_FRAMEBUFFER_SRGB_EXT                         0x8DB9
#endif

#ifndef GL_EXT_texture_compression_latc
#define GL_EXT_texture_compression_latc
#define GL_COMPRESSED_LUMINANCE_LATC1_EXT                 0x8C70
#define GL_COMPRESSED_SIGNED_LUMINANCE_LATC1_EXT          0x8C71
#define GL_COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT           0x8C72
#define GL_COMPRESSED_SIGNED_LUMINANCE_ALPHA_LATC2_EXT    0x8C73
#endif

#ifndef GL_ARB_texture_compression_bptc
#define GL_ARB_texture_compression_bptc
#define GL_COMPRESSED_RGBA_BPTC_UNORM_ARB                 0x8E8C
#define GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB           0x8E8D
#define GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT_ARB           0x8E8E
#define GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT_ARB         0x8E8F
#endif

// GL_ARB_draw_buffers
extern void (APIENTRY * qglDrawBuffersARB)(GLsizei n, const GLenum *bufs);
#ifndef GL_ARB_draw_buffers
#define GL_ARB_draw_buffers
#define GL_MAX_DRAW_BUFFERS_ARB                    0x8824
#define GL_DRAW_BUFFER0_ARB                        0x8825
#define GL_DRAW_BUFFER1_ARB                        0x8826
#define GL_DRAW_BUFFER2_ARB                        0x8827
#define GL_DRAW_BUFFER3_ARB                        0x8828
#define GL_DRAW_BUFFER4_ARB                        0x8829
#define GL_DRAW_BUFFER5_ARB                        0x882A
#define GL_DRAW_BUFFER6_ARB                        0x882B
#define GL_DRAW_BUFFER7_ARB                        0x882C
#define GL_DRAW_BUFFER8_ARB                        0x882D
#define GL_DRAW_BUFFER9_ARB                        0x882E
#define GL_DRAW_BUFFER10_ARB                       0x882F
#define GL_DRAW_BUFFER11_ARB                       0x8830
#define GL_DRAW_BUFFER12_ARB                       0x8831
#define GL_DRAW_BUFFER13_ARB                       0x8832
#define GL_DRAW_BUFFER14_ARB                       0x8833
#define GL_DRAW_BUFFER15_ARB                       0x8834
#endif

#ifndef GL_ARB_depth_clamp
#define GL_ARB_depth_clamp
#define GL_DEPTH_CLAMP				      0x864F
#endif

#ifndef GL_ARB_seamless_cube_map
#define GL_ARB_seamless_cube_map
#define GL_TEXTURE_CUBE_MAP_SEAMLESS               0x884F
#endif

#if defined(WIN32)
// WGL_ARB_create_context
#ifndef WGL_ARB_create_context
#define WGL_CONTEXT_MAJOR_VERSION_ARB             0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB             0x2092
#define WGL_CONTEXT_LAYER_PLANE_ARB               0x2093
#define WGL_CONTEXT_FLAGS_ARB                     0x2094
#define WGL_CONTEXT_PROFILE_MASK_ARB              0x9126
#define WGL_CONTEXT_DEBUG_BIT_ARB                 0x0001
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB    0x0002
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB          0x00000001
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x00000002
#define ERROR_INVALID_VERSION_ARB                 0x2095
#define ERROR_INVALID_PROFILE_ARB                 0x2096
#endif

extern          HGLRC(APIENTRY * qwglCreateContextAttribsARB) (HDC hdC, HGLRC hShareContext, const int *attribList);
#endif

#if 0 //defined(__linux__)
// GLX_ARB_create_context
#ifndef GLX_ARB_create_context
#define GLX_CONTEXT_DEBUG_BIT_ARB          0x00000001
#define GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB 0x00000002
#define GLX_CONTEXT_MAJOR_VERSION_ARB      0x2091
#define GLX_CONTEXT_MINOR_VERSION_ARB      0x2092
#define GLX_CONTEXT_FLAGS_ARB              0x2094
#endif

extern GLXContext	(APIENTRY * qglXCreateContextAttribsARB) (Display *dpy, GLXFBConfig config, GLXContext share_context, Bool direct, const int *attrib_list);
#endif

#endif

#ifdef _WIN32

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Pixel Format extension definitions. - AReis
/***********************************************************************************************************/
#define WGL_COLOR_BITS_ARB             0x2014
#define WGL_ALPHA_BITS_ARB             0x201B
#define WGL_DEPTH_BITS_ARB             0x2022
#define WGL_STENCIL_BITS_ARB           0x2023

typedef BOOL (WINAPI * PFNWGLGETPIXELFORMATATTRIBIVARBPROC) (HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, const int *piAttributes, int *piValues);
typedef BOOL (WINAPI * PFNWGLGETPIXELFORMATATTRIBFVARBPROC) (HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, const int *piAttributes, FLOAT *pfValues);
typedef BOOL (WINAPI * PFNWGLCHOOSEPIXELFORMATARBPROC) (HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats);
/***********************************************************************************************************/

// Declare Pixel Format function pointers.
extern PFNWGLGETPIXELFORMATATTRIBIVARBPROC		qwglGetPixelFormatAttribivARB;
extern PFNWGLGETPIXELFORMATATTRIBFVARBPROC		qwglGetPixelFormatAttribfvARB;
extern PFNWGLCHOOSEPIXELFORMATARBPROC			qwglChoosePixelFormatARB;


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Pixel Buffer extension definitions. - AReis
/***********************************************************************************************************/
DECLARE_HANDLE(HPBUFFERARB);

#define WGL_SUPPORT_OPENGL_ARB         0x2010
#define WGL_DOUBLE_BUFFER_ARB          0x2011
#define WGL_DRAW_TO_PBUFFER_ARB        0x202D
#define WGL_PBUFFER_WIDTH_ARB          0x2034
#define WGL_PBUFFER_HEIGHT_ARB         0x2035
#define WGL_RED_BITS_ARB               0x2015
#define WGL_GREEN_BITS_ARB             0x2017
#define WGL_BLUE_BITS_ARB              0x2019

typedef HPBUFFERARB (WINAPI * PFNWGLCREATEPBUFFERARBPROC) (HDC hDC, int iPixelFormat, int iWidth, int iHeight, const int *piAttribList);
typedef HDC (WINAPI * PFNWGLGETPBUFFERDCARBPROC) (HPBUFFERARB hPbuffer);
typedef int (WINAPI * PFNWGLRELEASEPBUFFERDCARBPROC) (HPBUFFERARB hPbuffer, HDC hDC);
typedef BOOL (WINAPI * PFNWGLDESTROYPBUFFERARBPROC) (HPBUFFERARB hPbuffer);
typedef BOOL (WINAPI * PFNWGLQUERYPBUFFERARBPROC) (HPBUFFERARB hPbuffer, int iAttribute, int *piValue);
/***********************************************************************************************************/

// Declare Pixel Buffer function pointers.
extern PFNWGLCREATEPBUFFERARBPROC				qwglCreatePbufferARB;
extern PFNWGLGETPBUFFERDCARBPROC				qwglGetPbufferDCARB;
extern PFNWGLRELEASEPBUFFERDCARBPROC			qwglReleasePbufferDCARB;
extern PFNWGLDESTROYPBUFFERARBPROC				qwglDestroyPbufferARB;
extern PFNWGLQUERYPBUFFERARBPROC				qwglQueryPbufferARB;


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Render-Texture extension definitions. - AReis
/***********************************************************************************************************/
#define WGL_BIND_TO_TEXTURE_RGBA_ARB       0x2071
#define WGL_TEXTURE_FORMAT_ARB             0x2072
#define WGL_TEXTURE_TARGET_ARB             0x2073
#define WGL_TEXTURE_RGB_ARB                0x2075
#define WGL_TEXTURE_RGBA_ARB               0x2076
#define WGL_TEXTURE_2D_ARB                 0x207A
#define WGL_FRONT_LEFT_ARB                 0x2083

typedef BOOL (WINAPI * PFNWGLBINDTEXIMAGEARBPROC) (HPBUFFERARB hPbuffer, int iBuffer);
typedef BOOL (WINAPI * PFNWGLRELEASETEXIMAGEARBPROC) (HPBUFFERARB hPbuffer, int iBuffer);
typedef BOOL (WINAPI * PFNWGLSETPBUFFERATTRIBARBPROC) (HPBUFFERARB hPbuffer, const int * piAttribList);
/***********************************************************************************************************/

// Declare Render-Texture function pointers.
extern PFNWGLBINDTEXIMAGEARBPROC			qwglBindTexImageARB;
extern PFNWGLRELEASETEXIMAGEARBPROC			qwglReleaseTexImageARB;
extern PFNWGLSETPBUFFERATTRIBARBPROC		qwglSetPbufferAttribARB;

#endif //_WIN32

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Vertex and Fragment Program extension definitions. - AReis
/***********************************************************************************************************/
#ifndef GL_ARB_fragment_program
#define GL_FRAGMENT_PROGRAM_ARB           0x8804
#define GL_PROGRAM_ALU_INSTRUCTIONS_ARB   0x8805
#define GL_PROGRAM_TEX_INSTRUCTIONS_ARB   0x8806
#define GL_PROGRAM_TEX_INDIRECTIONS_ARB   0x8807
#define GL_PROGRAM_NATIVE_ALU_INSTRUCTIONS_ARB 0x8808
#define GL_PROGRAM_NATIVE_TEX_INSTRUCTIONS_ARB 0x8809
#define GL_PROGRAM_NATIVE_TEX_INDIRECTIONS_ARB 0x880A
#define GL_MAX_PROGRAM_ALU_INSTRUCTIONS_ARB 0x880B
#define GL_MAX_PROGRAM_TEX_INSTRUCTIONS_ARB 0x880C
#define GL_MAX_PROGRAM_TEX_INDIRECTIONS_ARB 0x880D
#define GL_MAX_PROGRAM_NATIVE_ALU_INSTRUCTIONS_ARB 0x880E
#define GL_MAX_PROGRAM_NATIVE_TEX_INSTRUCTIONS_ARB 0x880F
#define GL_MAX_PROGRAM_NATIVE_TEX_INDIRECTIONS_ARB 0x8810
#define GL_MAX_TEXTURE_COORDS_ARB         0x8871
#define GL_MAX_TEXTURE_IMAGE_UNITS_ARB    0x8872
#endif

// NOTE: These are obviously not all the vertex program flags (have you seen how many there actually are!). I'm
// only including the ones I use (to reduce code clutter), so if you need any of the other flags, just add them.
#define GL_VERTEX_PROGRAM_ARB                       0x8620
#define GL_PROGRAM_FORMAT_ASCII_ARB                 0x8875

typedef void (APIENTRY * PFNGLPROGRAMSTRINGARBPROC) (GLenum target, GLenum format, GLsizei len, const GLvoid *string); 
typedef void (APIENTRY * PFNGLBINDPROGRAMARBPROC) (GLenum target, GLuint program);
typedef void (APIENTRY * PFNGLDELETEPROGRAMSARBPROC) (GLsizei n, const GLuint *programs);
typedef void (APIENTRY * PFNGLGENPROGRAMSARBPROC) (GLsizei n, GLuint *programs);
typedef void (APIENTRY * PFNGLPROGRAMENVPARAMETER4DARBPROC) (GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
typedef void (APIENTRY * PFNGLPROGRAMENVPARAMETER4DVARBPROC) (GLenum target, GLuint index, const GLdouble *params);
typedef void (APIENTRY * PFNGLPROGRAMENVPARAMETER4FARBPROC) (GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
typedef void (APIENTRY * PFNGLPROGRAMENVPARAMETER4FVARBPROC) (GLenum target, GLuint index, const GLfloat *params);
typedef void (APIENTRY * PFNGLPROGRAMLOCALPARAMETER4DARBPROC) (GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
typedef void (APIENTRY * PFNGLPROGRAMLOCALPARAMETER4DVARBPROC) (GLenum target, GLuint index, const GLdouble *params);
typedef void (APIENTRY * PFNGLPROGRAMLOCALPARAMETER4FARBPROC) (GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
typedef void (APIENTRY * PFNGLPROGRAMLOCALPARAMETER4FVARBPROC) (GLenum target, GLuint index, const GLfloat *params);
typedef void (APIENTRY * PFNGLGETPROGRAMENVPARAMETERDVARBPROC) (GLenum target, GLuint index, GLdouble *params);
typedef void (APIENTRY * PFNGLGETPROGRAMENVPARAMETERFVARBPROC) (GLenum target, GLuint index, GLfloat *params);
typedef void (APIENTRY * PFNGLGETPROGRAMLOCALPARAMETERDVARBPROC) (GLenum target, GLuint index, GLdouble *params);
typedef void (APIENTRY * PFNGLGETPROGRAMLOCALPARAMETERFVARBPROC) (GLenum target, GLuint index, GLfloat *params);
typedef void (APIENTRY * PFNGLGETPROGRAMIVARBPROC) (GLenum target, GLenum pname, GLint *params);
typedef void (APIENTRY * PFNGLGETPROGRAMSTRINGARBPROC) (GLenum target, GLenum pname, GLvoid *string);
typedef GLboolean (APIENTRY * PFNGLISPROGRAMARBPROC) (GLuint program);
/***********************************************************************************************************/

// Declare Vertex and Fragment Program function pointers.
extern PFNGLPROGRAMSTRINGARBPROC qglProgramStringARB;
extern PFNGLBINDPROGRAMARBPROC qglBindProgramARB;
extern PFNGLDELETEPROGRAMSARBPROC qglDeleteProgramsARB;
extern PFNGLGENPROGRAMSARBPROC qglGenProgramsARB;
extern PFNGLPROGRAMENVPARAMETER4DARBPROC qglProgramEnvParameter4dARB;
extern PFNGLPROGRAMENVPARAMETER4DVARBPROC qglProgramEnvParameter4dvARB;
extern PFNGLPROGRAMENVPARAMETER4FARBPROC qglProgramEnvParameter4fARB;
extern PFNGLPROGRAMENVPARAMETER4FVARBPROC qglProgramEnvParameter4fvARB;
extern PFNGLPROGRAMLOCALPARAMETER4DARBPROC qglProgramLocalParameter4dARB;
extern PFNGLPROGRAMLOCALPARAMETER4DVARBPROC qglProgramLocalParameter4dvARB;
extern PFNGLPROGRAMLOCALPARAMETER4FARBPROC qglProgramLocalParameter4fARB;
extern PFNGLPROGRAMLOCALPARAMETER4FVARBPROC qglProgramLocalParameter4fvARB;
extern PFNGLGETPROGRAMENVPARAMETERDVARBPROC qglGetProgramEnvParameterdvARB;
extern PFNGLGETPROGRAMENVPARAMETERFVARBPROC qglGetProgramEnvParameterfvARB;
extern PFNGLGETPROGRAMLOCALPARAMETERDVARBPROC qglGetProgramLocalParameterdvARB;
extern PFNGLGETPROGRAMLOCALPARAMETERFVARBPROC qglGetProgramLocalParameterfvARB;
extern PFNGLGETPROGRAMIVARBPROC qglGetProgramivARB;
extern PFNGLGETPROGRAMSTRINGARBPROC qglGetProgramStringARB;
extern PFNGLISPROGRAMARBPROC qglIsProgramARB;


/*
** extension constants
*/


// S3TC compression constants
#define GL_RGB_S3TC							0x83A0
#define GL_RGB4_S3TC						0x83A1


// extensions will be function pointers on all platforms

extern	void ( APIENTRY * qglMultiTexCoord2fARB )( GLenum texture, GLfloat s, GLfloat t );
extern	void ( APIENTRY * qglActiveTextureARB )( GLenum texture );
extern	void ( APIENTRY * qglClientActiveTextureARB )( GLenum texture );

extern	void ( APIENTRY * qglLockArraysEXT) (GLint, GLint);
extern	void ( APIENTRY * qglUnlockArraysEXT) (void);

extern	void ( APIENTRY * qglPointParameterfEXT)( GLenum, GLfloat);
extern	void ( APIENTRY * qglPointParameterfvEXT)( GLenum, GLfloat *);

//3d textures -rww
extern	void ( APIENTRY * qglTexImage3DEXT) (GLenum, GLint, GLenum, GLsizei, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid *);
extern	void ( APIENTRY * qglTexSubImage3DEXT) (GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *);

#define GL_MAX_ACTIVE_TEXTURES_ARB          0x84E2