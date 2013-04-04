
/*
 * UNPUBLISHED -- Rights  reserved  under  the  copyright  laws  of the 
 * United States.  Use  of a copyright notice is precautionary only and 
 * does not imply publication or disclosure.                            
 *                                                                      
 * THIS DOCUMENTATION CONTAINS CONFIDENTIAL AND PROPRIETARY INFORMATION 
 * OF    VICARIOUS   VISIONS,  INC.    ANY  DUPLICATION,  MODIFICATION, 
 * DISTRIBUTION, OR DISCLOSURE IS STRICTLY PROHIBITED WITHOUT THE PRIOR 
 * EXPRESS WRITTEN PERMISSION OF VICARIOUS VISIONS, INC.
 */

// leave this as first line for PCH reasons...
//
#include "../server/exe_headers.h"


/*
** QGL_WIN.C
**
** This file implements the operating system binding of GL to QGL function
** pointers.  When doing a port of Quake3 you must implement the following
** two functions:
**
** QGL_Init() - loads libraries, assigns function pointers, etc.
** QGL_Shutdown() - unloads libraries, NULLs function pointers
*/
#include <float.h>
#include "../renderer/tr_local.h"
#include "glw_win_dx8.h"
#include "win_local.h"

#include "xbox_texture_man.h"

#ifdef _XBOX
#include <xgraphics.h>
//#include "win_flareeffect.h"
#include "win_lighteffects.h"
#include "win_highdynamicrange.h"
#include "win_stencilshadow.h"

#ifndef FINAL_BUILD
#include <d3d8perf.h>
#endif

#endif

// Global texture allocator (we only use one in SP):
//SwappingTextureAllocator	gTextures;
StaticTextureAllocator		gTextures;

#include <vector>

extern void Z_SetNewDeleteTemporary(bool);

#define GLW_USE_TRI_STRIPS 1

#ifdef _XBOX
#define GLW_MAX_DRAW_PACKET_SIZE 2040
#else
#define GLW_MAX_DRAW_PACKET_SIZE (SHADER_MAX_VERTEXES*12)
#endif

#define MEMORY_PROFILE 1

int texMemSize = 0;

#if MEMORY_PROFILE
 
static int getTexMemSize(IDirect3DTexture9* mipmap)
{
	int levels = mipmap->GetLevelCount();
	int size = 0;
	while (levels--)
	{ 
		D3DSURFACE_DESC desc;
		mipmap->GetLevelDesc(levels, &desc);
		size += desc.Size;
	}
	return size;
}
#endif

void QGL_EnableLogging( qboolean enable );

void ( * qglAccum )(GLenum op, GLfloat value);
void ( * qglAlphaFunc )(GLenum func, GLclampf ref);
GLboolean ( * qglAreTexturesResident )(GLsizei n, const GLuint *textures, GLboolean *residences);
void ( * qglArrayElement )(GLint i);
void ( * qglBegin )(GLenum mode);
void ( * qglBeginEXT )(GLenum mode, GLint verts, GLint colors, GLint normals, GLint tex0, GLint tex1);//, GLint tex2, GLint tex3);
GLboolean ( * qglBeginFrame )(void);
void ( * qglBeginShadow )(void);
void ( * qglBindTexture )(GLenum target, GLuint texture);
void ( * qglBitmap )(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap);
void ( * qglBlendFunc )(GLenum sfactor, GLenum dfactor);
void ( * qglCallList )(GLuint lnum);
void ( * qglCallLists )(GLsizei n, GLenum type, const GLvoid *lists);
void ( * qglClear )(GLbitfield mask);
void ( * qglClearAccum )(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
void ( * qglClearColor )(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
void ( * qglClearDepth )(GLclampd depth);
void ( * qglClearIndex )(GLfloat c);
void ( * qglClearStencil )(GLint s);
void ( * qglClipPlane )(GLenum plane, const GLdouble *equation);
void ( * qglColor3b )(GLbyte red, GLbyte green, GLbyte blue);
void ( * qglColor3bv )(const GLbyte *v);
void ( * qglColor3d )(GLdouble red, GLdouble green, GLdouble blue);
void ( * qglColor3dv )(const GLdouble *v);
void ( * qglColor3f )(GLfloat red, GLfloat green, GLfloat blue);
void ( * qglColor3fv )(const GLfloat *v);
void ( * qglColor3i )(GLint red, GLint green, GLint blue);
void ( * qglColor3iv )(const GLint *v);
void ( * qglColor3s )(GLshort red, GLshort green, GLshort blue);
void ( * qglColor3sv )(const GLshort *v);
void ( * qglColor3ub )(GLubyte red, GLubyte green, GLubyte blue);
void ( * qglColor3ubv )(const GLubyte *v);
void ( * qglColor3ui )(GLuint red, GLuint green, GLuint blue);
void ( * qglColor3uiv )(const GLuint *v);
void ( * qglColor3us )(GLushort red, GLushort green, GLushort blue);
void ( * qglColor3usv )(const GLushort *v);
void ( * qglColor4b )(GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha);
void ( * qglColor4bv )(const GLbyte *v);
void ( * qglColor4d )(GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha);
void ( * qglColor4dv )(const GLdouble *v);
void ( * qglColor4f )(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
void ( * qglColor4fv )(const GLfloat *v);
void ( * qglColor4i )(GLint red, GLint green, GLint blue, GLint alpha);
void ( * qglColor4iv )(const GLint *v);
void ( * qglColor4s )(GLshort red, GLshort green, GLshort blue, GLshort alpha);
void ( * qglColor4sv )(const GLshort *v);
void ( * qglColor4ub )(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha);
void ( * qglColor4ubv )(const GLubyte *v);
void ( * qglColor4ui )(GLuint red, GLuint green, GLuint blue, GLuint alpha);
void ( * qglColor4uiv )(const GLuint *v);
void ( * qglColor4us )(GLushort red, GLushort green, GLushort blue, GLushort alpha);
void ( * qglColor4usv )(const GLushort *v);
void ( * qglColorMask )(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
void ( * qglColorMaterial )(GLenum face, GLenum mode);
void ( * qglColorPointer )(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
void ( * qglCopyPixels )(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type);
void ( * qglCopyTexImage1D )(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLint border);
void ( * qglCopyTexImage2D )(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
void ( * qglCopyTexSubImage1D )(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width);
void ( * qglCopyTexSubImage2D )(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
void ( * qglCullFace )(GLenum mode);
void ( * qglDeleteLists )(GLuint lnum, GLsizei range);
void ( * qglDeleteTextures )(GLsizei n, const GLuint *textures);
void ( * qglDepthFunc )(GLenum func);
void ( * qglDepthMask )(GLboolean flag);
void ( * qglDepthRange )(GLclampd zNear, GLclampd zFar);
void ( * qglDisable )(GLenum cap);
void ( * qglDisableClientState )(GLenum array);
void ( * qglDrawArrays )(GLenum mode, GLint first, GLsizei count);
void ( * qglDrawBuffer )(GLenum mode);
void ( * qglDrawElements )(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
void ( * qglDrawPixels )(GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
void ( * qglEdgeFlag )(GLboolean flag);
void ( * qglEdgeFlagPointer )(GLsizei stride, const GLvoid *pointer);
void ( * qglEdgeFlagv )(const GLboolean *flag);
void ( * qglEnable )(GLenum cap);
void ( * qglEnableClientState )(GLenum array);
void ( * qglEnd )(void);
void ( * qglEndFrame )(void);
void ( * qglEndShadow )(void);
void ( * qglEndList )(void);
void ( * qglEvalCoord1d )(GLdouble u);
void ( * qglEvalCoord1dv )(const GLdouble *u);
void ( * qglEvalCoord1f )(GLfloat u);
void ( * qglEvalCoord1fv )(const GLfloat *u);
void ( * qglEvalCoord2d )(GLdouble u, GLdouble v);
void ( * qglEvalCoord2dv )(const GLdouble *u);
void ( * qglEvalCoord2f )(GLfloat u, GLfloat v);
void ( * qglEvalCoord2fv )(const GLfloat *u);
void ( * qglEvalMesh1 )(GLenum mode, GLint i1, GLint i2);
void ( * qglEvalMesh2 )(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2);
void ( * qglEvalPoint1 )(GLint i);
void ( * qglEvalPoint2 )(GLint i, GLint j);
void ( * qglFeedbackBuffer )(GLsizei size, GLenum type, GLfloat *buffer);
void ( * qglFinish )(void);
void ( * qglFlush )(void);
void ( * qglFlushShadow )(void);
void ( * qglFogf )(GLenum pname, GLfloat param);
void ( * qglFogfv )(GLenum pname, const GLfloat *params);
void ( * qglFogi )(GLenum pname, GLint param);
void ( * qglFogiv )(GLenum pname, const GLint *params);
void ( * qglFrontFace )(GLenum mode);
void ( * qglFrustum )(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
GLuint ( * qglGenLists )(GLsizei range);
void ( * qglGenTextures )(GLsizei n, GLuint *textures);
void ( * qglGetBooleanv )(GLenum pname, GLboolean *params);
void ( * qglGetClipPlane )(GLenum plane, GLdouble *equation);
void ( * qglGetDoublev )(GLenum pname, GLdouble *params);
GLenum ( * qglGetError )(void);
void ( * qglGetFloatv )(GLenum pname, GLfloat *params);
void ( * qglGetIntegerv )(GLenum pname, GLint *params);
void ( * qglGetLightfv )(GLenum light, GLenum pname, GLfloat *params);
void ( * qglGetLightiv )(GLenum light, GLenum pname, GLint *params);
void ( * qglGetMapdv )(GLenum target, GLenum query, GLdouble *v);
void ( * qglGetMapfv )(GLenum target, GLenum query, GLfloat *v);
void ( * qglGetMapiv )(GLenum target, GLenum query, GLint *v);
void ( * qglGetMaterialfv )(GLenum face, GLenum pname, GLfloat *params);
void ( * qglGetMaterialiv )(GLenum face, GLenum pname, GLint *params);
void ( * qglGetPixelMapfv )(GLenum gmap, GLfloat *values);
void ( * qglGetPixelMapuiv )(GLenum gmap, GLuint *values);
void ( * qglGetPixelMapusv )(GLenum gmap, GLushort *values);
void ( * qglGetPointerv )(GLenum pname, GLvoid* *params);
void ( * qglGetPolygonStipple )(GLubyte *mask);
const GLubyte * ( * qglGetString )(GLenum name);
void ( * qglGetTexEnvfv )(GLenum target, GLenum pname, GLfloat *params);
void ( * qglGetTexEnviv )(GLenum target, GLenum pname, GLint *params);
void ( * qglGetTexGendv )(GLenum coord, GLenum pname, GLdouble *params);
void ( * qglGetTexGenfv )(GLenum coord, GLenum pname, GLfloat *params);
void ( * qglGetTexGeniv )(GLenum coord, GLenum pname, GLint *params);
void ( * qglGetTexImage )(GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels);
void ( * qglGetTexLevelParameterfv )(GLenum target, GLint level, GLenum pname, GLfloat *params);
void ( * qglGetTexLevelParameteriv )(GLenum target, GLint level, GLenum pname, GLint *params);
void ( * qglGetTexParameterfv )(GLenum target, GLenum pname, GLfloat *params);
void ( * qglGetTexParameteriv )(GLenum target, GLenum pname, GLint *params);
void ( * qglHint )(GLenum target, GLenum mode);
void ( * qglIndexedTriToStrip )(GLsizei count, const GLushort *indices);
void ( * qglIndexMask )(GLuint mask);
void ( * qglIndexPointer )(GLenum type, GLsizei stride, const GLvoid *pointer);
void ( * qglIndexd )(GLdouble c);
void ( * qglIndexdv )(const GLdouble *c);
void ( * qglIndexf )(GLfloat c);
void ( * qglIndexfv )(const GLfloat *c);
void ( * qglIndexi )(GLint c);
void ( * qglIndexiv )(const GLint *c);
void ( * qglIndexs )(GLshort c);
void ( * qglIndexsv )(const GLshort *c);
void ( * qglIndexub )(GLubyte c);
void ( * qglIndexubv )(const GLubyte *c);
void ( * qglInitNames )(void);
void ( * qglInterleavedArrays )(GLenum format, GLsizei stride, const GLvoid *pointer);
GLboolean ( * qglIsEnabled )(GLenum cap);
GLboolean ( * qglIsList )(GLuint lnum);
GLboolean ( * qglIsTexture )(GLuint texture);
void ( * qglLightModelf )(GLenum pname, GLfloat param);
void ( * qglLightModelfv )(GLenum pname, const GLfloat *params);
void ( * qglLightModeli )(GLenum pname, GLint param);
void ( * qglLightModeliv )(GLenum pname, const GLint *params);
void ( * qglLightf )(GLenum light, GLenum pname, GLfloat param);
void ( * qglLightfv )(GLenum light, GLenum pname, const GLfloat *params);
void ( * qglLighti )(GLenum light, GLenum pname, GLint param);
void ( * qglLightiv )(GLenum light, GLenum pname, const GLint *params);
void ( * qglLineStipple )(GLint factor, GLushort pattern);
void ( * qglLineWidth )(GLfloat width);
void ( * qglListBase )(GLuint base);
void ( * qglLoadIdentity )(void);
void ( * qglLoadMatrixd )(const GLdouble *m);
void ( * qglLoadMatrixf )(const GLfloat *m);
void ( * qglLoadName )(GLuint name);
void ( * qglLogicOp )(GLenum opcode);
void ( * qglMap1d )(GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points);
void ( * qglMap1f )(GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points);
void ( * qglMap2d )(GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points);
void ( * qglMap2f )(GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points);
void ( * qglMapGrid1d )(GLint un, GLdouble u1, GLdouble u2);
void ( * qglMapGrid1f )(GLint un, GLfloat u1, GLfloat u2);
void ( * qglMapGrid2d )(GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2);
void ( * qglMapGrid2f )(GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2);
void ( * qglMaterialf )(GLenum face, GLenum pname, GLfloat param);
void ( * qglMaterialfv )(GLenum face, GLenum pname, const GLfloat *params);
void ( * qglMateriali )(GLenum face, GLenum pname, GLint param);
void ( * qglMaterialiv )(GLenum face, GLenum pname, const GLint *params);
void ( * qglMatrixMode )(GLenum mode);
void ( * qglMultMatrixd )(const GLdouble *m);
void ( * qglMultMatrixf )(const GLfloat *m);
void ( * qglNewList )(GLuint lnum, GLenum mode);
void ( * qglNormal3b )(GLbyte nx, GLbyte ny, GLbyte nz);
void ( * qglNormal3bv )(const GLbyte *v);
void ( * qglNormal3d )(GLdouble nx, GLdouble ny, GLdouble nz);
void ( * qglNormal3dv )(const GLdouble *v);
void ( * qglNormal3f )(GLfloat nx, GLfloat ny, GLfloat nz);
void ( * qglNormal3fv )(const GLfloat *v);
void ( * qglNormal3i )(GLint nx, GLint ny, GLint nz);
void ( * qglNormal3iv )(const GLint *v);
void ( * qglNormal3s )(GLshort nx, GLshort ny, GLshort nz);
void ( * qglNormal3sv )(const GLshort *v);
void ( * qglNormalPointer )(GLenum type, GLsizei stride, const GLvoid *pointer);
void ( * qglOrtho )(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
void ( * qglPassThrough )(GLfloat token);
void ( * qglPixelMapfv )(GLenum gmap, GLsizei mapsize, const GLfloat *values);
void ( * qglPixelMapuiv )(GLenum gmap, GLsizei mapsize, const GLuint *values);
void ( * qglPixelMapusv )(GLenum gmap, GLsizei mapsize, const GLushort *values);
void ( * qglPixelStoref )(GLenum pname, GLfloat param);
void ( * qglPixelStorei )(GLenum pname, GLint param);
void ( * qglPixelTransferf )(GLenum pname, GLfloat param);
void ( * qglPixelTransferi )(GLenum pname, GLint param);
void ( * qglPixelZoom )(GLfloat xfactor, GLfloat yfactor);
void ( * qglPointSize )(GLfloat size);
void ( * qglPolygonMode )(GLenum face, GLenum mode);
void ( * qglPolygonOffset )(GLfloat factor, GLfloat units);
void ( * qglPolygonStipple )(const GLubyte *mask);
void ( * qglPopAttrib )(void);
void ( * qglPopClientAttrib )(void);
void ( * qglPopMatrix )(void);
void ( * qglPopName )(void);
void ( * qglPrioritizeTextures )(GLsizei n, const GLuint *textures, const GLclampf *priorities);
void ( * qglPushAttrib )(GLbitfield mask);
void ( * qglPushClientAttrib )(GLbitfield mask);
void ( * qglPushMatrix )(void);
void ( * qglPushName )(GLuint name);
void ( * qglRasterPos2d )(GLdouble x, GLdouble y);
void ( * qglRasterPos2dv )(const GLdouble *v);
void ( * qglRasterPos2f )(GLfloat x, GLfloat y);
void ( * qglRasterPos2fv )(const GLfloat *v);
void ( * qglRasterPos2i )(GLint x, GLint y);
void ( * qglRasterPos2iv )(const GLint *v);
void ( * qglRasterPos2s )(GLshort x, GLshort y);
void ( * qglRasterPos2sv )(const GLshort *v);
void ( * qglRasterPos3d )(GLdouble x, GLdouble y, GLdouble z);
void ( * qglRasterPos3dv )(const GLdouble *v);
void ( * qglRasterPos3f )(GLfloat x, GLfloat y, GLfloat z);
void ( * qglRasterPos3fv )(const GLfloat *v);
void ( * qglRasterPos3i )(GLint x, GLint y, GLint z);
void ( * qglRasterPos3iv )(const GLint *v);
void ( * qglRasterPos3s )(GLshort x, GLshort y, GLshort z);
void ( * qglRasterPos3sv )(const GLshort *v);
void ( * qglRasterPos4d )(GLdouble x, GLdouble y, GLdouble z, GLdouble w);
void ( * qglRasterPos4dv )(const GLdouble *v);
void ( * qglRasterPos4f )(GLfloat x, GLfloat y, GLfloat z, GLfloat w);
void ( * qglRasterPos4fv )(const GLfloat *v);
void ( * qglRasterPos4i )(GLint x, GLint y, GLint z, GLint w);
void ( * qglRasterPos4iv )(const GLint *v);
void ( * qglRasterPos4s )(GLshort x, GLshort y, GLshort z, GLshort w);
void ( * qglRasterPos4sv )(const GLshort *v);
void ( * qglReadBuffer )(GLenum mode);
//void ( * qglReadPixels )(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLsizei twidth, GLsizei theight, GLvoid *pixels);
void ( * qglReadPixels )(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels);
void ( * qglCopyBackBufferToTexEXT ) (float width, float height, float u1, float v1, float u2, float v2);
void ( * qglCopyBackBufferToTex ) (void);
void ( * qglRectd )(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2);
void ( * qglRectdv )(const GLdouble *v1, const GLdouble *v2);
void ( * qglRectf )(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2);
void ( * qglRectfv )(const GLfloat *v1, const GLfloat *v2);
void ( * qglRecti )(GLint x1, GLint y1, GLint x2, GLint y2);
void ( * qglRectiv )(const GLint *v1, const GLint *v2);
void ( * qglRects )(GLshort x1, GLshort y1, GLshort x2, GLshort y2);
void ( * qglRectsv )(const GLshort *v1, const GLshort *v2);
GLint ( * qglRenderMode )(GLenum mode);
void ( * qglRotated )(GLdouble angle, GLdouble x, GLdouble y, GLdouble z);
void ( * qglRotatef )(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
void ( * qglScaled )(GLdouble x, GLdouble y, GLdouble z);
void ( * qglScalef )(GLfloat x, GLfloat y, GLfloat z);
void ( * qglScissor )(GLint x, GLint y, GLsizei width, GLsizei height);
void ( * qglSelectBuffer )(GLsizei size, GLuint *buffer);
void ( * qglShadeModel )(GLenum mode);
void ( * qglStencilFunc )(GLenum func, GLint ref, GLuint mask);
void ( * qglStencilMask )(GLuint mask);
void ( * qglStencilOp )(GLenum fail, GLenum zfail, GLenum zpass);
void ( * qglTexCoord1d )(GLdouble s);
void ( * qglTexCoord1dv )(const GLdouble *v);
void ( * qglTexCoord1f )(GLfloat s);
void ( * qglTexCoord1fv )(const GLfloat *v);
void ( * qglTexCoord1i )(GLint s);
void ( * qglTexCoord1iv )(const GLint *v);
void ( * qglTexCoord1s )(GLshort s);
void ( * qglTexCoord1sv )(const GLshort *v);
void ( * qglTexCoord2d )(GLdouble s, GLdouble t);
void ( * qglTexCoord2dv )(const GLdouble *v);
void ( * qglTexCoord2f )(GLfloat s, GLfloat t);
void ( * qglTexCoord2fv )(const GLfloat *v);
void ( * qglTexCoord2i )(GLint s, GLint t);
void ( * qglTexCoord2iv )(const GLint *v);
void ( * qglTexCoord2s )(GLshort s, GLshort t);
void ( * qglTexCoord2sv )(const GLshort *v);
void ( * qglTexCoord3d )(GLdouble s, GLdouble t, GLdouble r);
void ( * qglTexCoord3dv )(const GLdouble *v);
void ( * qglTexCoord3f )(GLfloat s, GLfloat t, GLfloat r);
void ( * qglTexCoord3fv )(const GLfloat *v);
void ( * qglTexCoord3i )(GLint s, GLint t, GLint r);
void ( * qglTexCoord3iv )(const GLint *v);
void ( * qglTexCoord3s )(GLshort s, GLshort t, GLshort r);
void ( * qglTexCoord3sv )(const GLshort *v);
void ( * qglTexCoord4d )(GLdouble s, GLdouble t, GLdouble r, GLdouble q);
void ( * qglTexCoord4dv )(const GLdouble *v);
void ( * qglTexCoord4f )(GLfloat s, GLfloat t, GLfloat r, GLfloat q);
void ( * qglTexCoord4fv )(const GLfloat *v);
void ( * qglTexCoord4i )(GLint s, GLint t, GLint r, GLint q);
void ( * qglTexCoord4iv )(const GLint *v);
void ( * qglTexCoord4s )(GLshort s, GLshort t, GLshort r, GLshort q);
void ( * qglTexCoord4sv )(const GLshort *v);
void ( * qglTexCoordPointer )(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
void ( * qglTexEnvf )(GLenum target, GLenum pname, GLfloat param);
void ( * qglTexEnvfv )(GLenum target, GLenum pname, const GLfloat *params);
void ( * qglTexEnvi )(GLenum target, GLenum pname, GLint param);
void ( * qglTexEnviv )(GLenum target, GLenum pname, const GLint *params);
void ( * qglTexGend )(GLenum coord, GLenum pname, GLdouble param);
void ( * qglTexGendv )(GLenum coord, GLenum pname, const GLdouble *params);
void ( * qglTexGenf )(GLenum coord, GLenum pname, GLfloat param);
void ( * qglTexGenfv )(GLenum coord, GLenum pname, const GLfloat *params);
void ( * qglTexGeni )(GLenum coord, GLenum pname, GLint param);
void ( * qglTexGeniv )(GLenum coord, GLenum pname, const GLint *params);
void ( * qglTexImage1D )(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
void ( * qglTexImage2D )(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
void ( * qglTexImage2DEXT )(GLenum target, GLint level, GLint numlevels, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
void ( * qglTexParameterf )(GLenum target, GLenum pname, GLfloat param);
void ( * qglTexParameterfv )(GLenum target, GLenum pname, const GLfloat *params);
void ( * qglTexParameteri )(GLenum target, GLenum pname, GLint param);
void ( * qglTexParameteriv )(GLenum target, GLenum pname, const GLint *params);
void ( * qglTexSubImage1D )(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels);
void ( * qglTexSubImage2D )(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
void ( * qglTranslated )(GLdouble x, GLdouble y, GLdouble z);
void ( * qglTranslatef )(GLfloat x, GLfloat y, GLfloat z);
void ( * qglVertex2d )(GLdouble x, GLdouble y);
void ( * qglVertex2dv )(const GLdouble *v);
void ( * qglVertex2f )(GLfloat x, GLfloat y);
void ( * qglVertex2fv )(const GLfloat *v);
void ( * qglVertex2i )(GLint x, GLint y);
void ( * qglVertex2iv )(const GLint *v);
void ( * qglVertex2s )(GLshort x, GLshort y);
void ( * qglVertex2sv )(const GLshort *v);
void ( * qglVertex3d )(GLdouble x, GLdouble y, GLdouble z);
void ( * qglVertex3dv )(const GLdouble *v);
void ( * qglVertex3f )(GLfloat x, GLfloat y, GLfloat z);
void ( * qglVertex3fv )(const GLfloat *v);
void ( * qglVertex3i )(GLint x, GLint y, GLint z);
void ( * qglVertex3iv )(const GLint *v);
void ( * qglVertex3s )(GLshort x, GLshort y, GLshort z);
void ( * qglVertex3sv )(const GLshort *v);
void ( * qglVertex4d )(GLdouble x, GLdouble y, GLdouble z, GLdouble w);
void ( * qglVertex4dv )(const GLdouble *v);
void ( * qglVertex4f )(GLfloat x, GLfloat y, GLfloat z, GLfloat w);
void ( * qglVertex4fv )(const GLfloat *v);
void ( * qglVertex4i )(GLint x, GLint y, GLint z, GLint w);
void ( * qglVertex4iv )(const GLint *v);
void ( * qglVertex4s )(GLshort x, GLshort y, GLshort z, GLshort w);
void ( * qglVertex4sv )(const GLshort *v);
void ( * qglVertexPointer )(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
void ( * qglViewport )(GLint x, GLint y, GLsizei width, GLsizei height);

#if 0
void ( * qglMultiTexCoord2fARB )( GLenum texture, GLfloat s, GLfloat t );
void ( * qglActiveTextureARB )( GLenum texture );
void ( * qglClientActiveTextureARB )( GLenum texture );
#endif

static void _d3d_check(HRESULT err, const char* func)
{
	if (err != D3D_OK)
	{
		MEMORYSTATUS status;
		GlobalMemoryStatus(&status);
		Sys_Print(va("%s returned %d!  Memfree=%d\n", func, err, status.dwAvailPhys));
	}
}

#ifdef _WINDOWS
static bool surfaceToBMP(LPDIRECT3DDEVICE8 pd3dDevice, LPDIRECT3DSURFACE8 lpSurface, const char *fname)
{
	DWORD outpixel;
	BITMAPFILEHEADER fh;
	BITMAPINFOHEADER bi;
	int outbyte, BufferIndex, width, height, pitch;
	char *WriteBuffer;
	FILE *file;
	HRESULT Error;
	IDirect3DSurface8 *pTempSurf = NULL;

	// Get the surface description first
	D3DSURFACE_DESC ddsd;
	D3DLOCKED_RECT lrSurf;
	
	Error = lpSurface->GetDesc(&ddsd);
	// This writes out 32 bit values, so whatever surface format we were passed in,
	// copy it into a 32 bit surface
	Error = pd3dDevice->CreateImageSurface(ddsd.Width, ddsd.Height, D3DFMT_A8R8G8B8, &pTempSurf);

	Error = D3DXLoadSurfaceFromSurface(pTempSurf, NULL, NULL, lpSurface, NULL, NULL, D3DX_DEFAULT, 0);

	file = fopen(fname, "wb");
	if(!file)
		return FALSE;

	Error = pTempSurf->LockRect(&lrSurf, NULL, 0);

	BufferIndex = 0;
	width = ddsd.Width;
	height = ddsd.Height;
	pitch = lrSurf.Pitch;
	WriteBuffer = new char[width * height * 3];

	// Setup the file headers
	((char*)&(fh.bfType))[0] = 'B';
	((char*)&(fh.bfType))[1] = 'M';
	fh.bfSize = (long)(sizeof(BITMAPINFOHEADER) + sizeof(BITMAPFILEHEADER) + width * height * 3);
	fh.bfReserved1 = 0;
	fh.bfReserved2 = 0;
	fh.bfOffBits = sizeof(BITMAPINFOHEADER) + sizeof(BITMAPFILEHEADER);
	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = width;
	bi.biHeight = height;
	bi.biPlanes = 1;
	bi.biBitCount = 24;
	bi.biCompression = BI_RGB;
	bi.biSizeImage = 0;
	bi.biXPelsPerMeter = 10000;
	bi.biYPelsPerMeter = 10000;
	bi.biClrUsed = 0;
	bi.biClrImportant = 0;

	fwrite(&fh, sizeof(BITMAPFILEHEADER), 1, file);
	fwrite(&bi, sizeof(BITMAPINFOHEADER), 1, file);

	char *Bitmap_in = (char*)lrSurf.pBits;

	for(int y = height - 1; y >= 0; y--) 
	{
		for(int x = 0; x < width; x++)
		{
			outpixel = *((DWORD *)(Bitmap_in + x * 4 + y * pitch)); //Load a word

			//Load up the Blue component and output it
			outbyte = (((outpixel)&0x000000ff));//blue
			WriteBuffer [BufferIndex++] = outbyte;

			//Load up the green component and output it 
			outbyte = (((outpixel>>8)&0x000000ff)); 
			WriteBuffer [BufferIndex++] = outbyte;

			//Load up the red component and output it 
			outbyte = (((outpixel>>16)&0x000000ff));
			WriteBuffer [BufferIndex++] = outbyte;
		}
	}

	//At this point the buffer should be full, so just write it out
	fwrite(WriteBuffer, BufferIndex, 1, file);

	//Now unlock the surface and we're done
	pTempSurf->UnlockRect();
	pTempSurf->Release();

	fclose(file);

	delete [] WriteBuffer;
	return true;
}
#endif

/*
=================
_fixupScreenCoords

Clamp coords to screen dimensions and fix Y direction.
=================
*/
static void _fixupScreenCoords(GLint& x, GLint& y, GLsizei& width, GLsizei& height)
{
	if (x < 0) x = 0;
	else if (x > glConfig.vidWidth) x = glConfig.vidWidth;
	if (y < 0)
	{
		
#ifdef _XBOX
		height += y;
#endif
		y = 0;
	}
	else if (y > glConfig.vidHeight) y = glConfig.vidHeight;
	
	if (width < 0) width = 0;
#ifdef _XBOX
	else if (x + width > glConfig.vidWidth) width = glConfig.vidWidth - x;
#endif
//	else if (x + width > glConfig.vidWidth) width = glConfig.vidWidth - x;
	if (height < 0) height = 0;
	else if (y + height > glConfig.vidHeight) height = glConfig.vidHeight - y;

	// GL and DX disagree on the direction of Y
	y = glConfig.vidHeight - (y + height);
}


/*
=================
_convertCompare

Convert GL compare function to DX function.
=================
*/
static D3DCMPFUNC _convertCompare(GLenum func)
{
	switch (func)
	{
	case GL_NEVER: return D3DCMP_NEVER;
	case GL_LESS: return D3DCMP_LESS;
	case GL_EQUAL: return D3DCMP_EQUAL;
	case GL_LEQUAL: return D3DCMP_LESSEQUAL;
	case GL_GREATER: return D3DCMP_GREATER;
	case GL_NOTEQUAL: return D3DCMP_NOTEQUAL;
	case GL_GEQUAL: return D3DCMP_GREATEREQUAL;
	default: case GL_ALWAYS: return D3DCMP_ALWAYS;
	}
}


/*
=================
_convertBlendFactor

Convert GL blend mode to DX blend mode.
=================
*/
static D3DBLEND _convertBlendFactor(GLenum factor)
{
	switch (factor)
	{
	case GL_ZERO: return D3DBLEND_ZERO;
	default: case GL_ONE: return D3DBLEND_ONE;
	case GL_SRC_COLOR: return D3DBLEND_SRCCOLOR;
	case GL_ONE_MINUS_SRC_COLOR: return D3DBLEND_INVSRCCOLOR;
	case GL_SRC_ALPHA: return D3DBLEND_SRCALPHA;
	case GL_ONE_MINUS_SRC_ALPHA: return D3DBLEND_INVSRCALPHA;
	case GL_DST_COLOR: return D3DBLEND_DESTCOLOR;
	case GL_ONE_MINUS_DST_COLOR: return D3DBLEND_INVDESTCOLOR;
	case GL_DST_ALPHA: return D3DBLEND_DESTALPHA;
	case GL_ONE_MINUS_DST_ALPHA: return D3DBLEND_INVDESTALPHA;
	case GL_SRC_ALPHA_SATURATE: return D3DBLEND_SRCALPHASAT;
	}
}


/*
=================
_convertPrimMode

Convert GL primitive mode to DX primitive mode.
=================
*/
static D3DPRIMITIVETYPE _convertPrimMode(GLenum mode)
{
	switch (mode)
	{
	case GL_POINTS: return D3DPT_POINTLIST;
	case GL_LINES: return D3DPT_LINELIST;
	case GL_LINE_STRIP: return D3DPT_LINESTRIP;
	case GL_TRIANGLES: return D3DPT_TRIANGLELIST;
	case GL_TRIANGLE_STRIP: return D3DPT_TRIANGLESTRIP;
	case GL_TRIANGLE_FAN: return D3DPT_TRIANGLEFAN;
#ifdef _XBOX
	case GL_QUADS: return D3DPT_QUADLIST;
	case GL_QUAD_STRIP: return D3DPT_QUADSTRIP;
#else
	case GL_QUADS: return D3DPT_TRIANGLELIST;
	case GL_QUAD_STRIP: return D3DPT_TRIANGLESTRIP;
#endif
	case GL_POLYGON: return D3DPT_TRIANGLEFAN;
	default: assert(0); return D3DPT_TRIANGLEFAN;
	}
}


/*
=================
_updateDrawStride

Update the stride of the draw array based on
the number of vertex attributes.  The stride
is in DWORDs.
=================
*/
static void _updateDrawStride(GLint normal, GLint tex0, GLint tex1)
{
	glw_state->drawStride = 4;
	if (normal) glw_state->drawStride += 3;
	if (tex0) glw_state->drawStride += 2;
	if (tex1) glw_state->drawStride += 2;
}


/*
=================
_updateShader

Set the vertex shader based on the number
of texture coordinates.
=================
*/
static void _updateShader(bool normal, bool tex0, bool tex1)//, bool tex2, bool tex3)
{
	DWORD mask = D3DFVF_XYZ;
	if (normal) mask |= D3DFVF_NORMAL;
	mask |= D3DFVF_DIFFUSE;
	if (tex0 && !tex1) mask |= D3DFVF_TEX1;
	else if (tex1) mask |= D3DFVF_TEX2;

//	if (mask != glw_state->shaderMask)
//	{
		glw_state->device->SetVertexShader(mask);
		glw_state->shaderMask = mask;
//	}
}


/*
=================
_getCurrentTexture

Get the texture information for the currently
bound texture at a stage.
=================
*/
static glwstate_t::TextureInfo* _getCurrentTexture(int stage)
{
	glwstate_t::texturexlat_t::iterator i = glw_state->textureXlat.find(
		glw_state->currentTexture[stage]);

	if (i == glw_state->textureXlat.end()) return NULL;

	return &i->second;
}


/*
=================
_updateTextures

Setup texture stages with color operations, filters
and wrapping modes as needed.
=================
*/
static void _updateTextures(void)
{
	for (int t = 0; t < GLW_MAX_TEXTURE_STAGES; ++t)
	{
		if (glw_state->textureStageDirty[t])
		{
			glw_state->textureStageDirty[t] = false;

			if (glw_state->textureStageEnable[t] && glw_state->currentTexture[t])
			{
				glwstate_t::TextureInfo* info = _getCurrentTexture(t);
				if (!info) continue;

				glw_state->device->SetTexture(t, info->mipmap);
				glw_state->device->SetTextureStageState(t, D3DTSS_COLOROP, glw_state->textureEnv[t]);

				glw_state->device->SetTextureStageState(t, D3DTSS_COLORARG1, 
					D3DTA_TEXTURE);
				glw_state->device->SetTextureStageState(t, D3DTSS_COLORARG2, 
					D3DTA_CURRENT);
				glw_state->device->SetTextureStageState(t, D3DTSS_ALPHAOP, 
					glw_state->textureEnv[t]);
				glw_state->device->SetTextureStageState(t, D3DTSS_ALPHAARG1, 
					D3DTA_TEXTURE);
				glw_state->device->SetTextureStageState(t, D3DTSS_ALPHAARG2, 
					D3DTA_CURRENT);

				glw_state->device->SetTextureStageState(t, D3DTSS_MAXANISOTROPY, 
					info->anisotropy);
				glw_state->device->SetTextureStageState(t, D3DTSS_MINFILTER, 
					info->minFilter);
				glw_state->device->SetTextureStageState(t, D3DTSS_MIPFILTER, 
					info->mipFilter);
				glw_state->device->SetTextureStageState(t, D3DTSS_MAGFILTER, 
					info->magFilter);

				glw_state->device->SetTextureStageState(t, D3DTSS_ADDRESSU, 
					info->wrapU);
				glw_state->device->SetTextureStageState(t, D3DTSS_ADDRESSV, 
					info->wrapV);

				glw_state->device->SetTextureStageState(t, D3DTSS_TEXCOORDINDEX, t);

				glw_state->device->SetTextureStageState( t, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2 );

				/*if(tess.shader)
				{
					if(tess.currentPass < tess.shader->numUnfoggedPasses)
					{ 
						if(tess.shader->stages[tess.currentPass].isEnvironment)
						{
							glw_state->device->SetTextureStageState(t, D3DTSS_TEXCOORDINDEX, t | D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR);
						}
					}
				}*/				
			}
			else
			{
				glw_state->device->SetTexture(t, NULL);
				glw_state->device->SetTextureStageState(t, D3DTSS_COLOROP, D3DTOP_DISABLE);
				glw_state->device->SetTextureStageState(t, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
			}
		}
		//else
		//{
		//	// Hard-wired check for turning on hardware environment mapping
		//	if( glw_state->textureStageEnable[t] &&
		//		glw_state->currentTexture[t] &&
		//		tess.shader &&
		//		tess.currentPass < tess.shader->numUnfoggedPasses &&
		//		tess.shader->stages[tess.currentPass].isEnvironment)
		//	{
		//			glw_state->device->SetTextureStageState(t, D3DTSS_TEXCOORDINDEX, t | D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR);
		//	}
		//}
	}
}


/*
=================
_updateMatrices

Set the current projection and view transforms to
the matrices at the top of the relevant stacks.
=================
*/
static void _updateMatrices(void)
{
	if(glw_state->matricesDirty[glwstate_t::MatrixMode_Projection])
	{
		glw_state->device->SetTransform(D3DTS_PROJECTION, 
			glw_state->matrixStack[glwstate_t::MatrixMode_Projection]->GetTop());

		glw_state->matricesDirty[glwstate_t::MatrixMode_Projection] = false;
	}

	if(glw_state->matricesDirty[glwstate_t::MatrixMode_Model])
	{
		glw_state->device->SetTransform(D3DTS_VIEW, 
			glw_state->matrixStack[glwstate_t::MatrixMode_Model]->GetTop());

		glw_state->matricesDirty[glwstate_t::MatrixMode_Model] = false;
	}

	if(glw_state->matricesDirty[glwstate_t::MatrixMode_Texture0])
	{
		glw_state->device->SetTransform(D3DTS_TEXTURE0,
			glw_state->matrixStack[glwstate_t::MatrixMode_Texture0]->GetTop());

		glw_state->matricesDirty[glwstate_t::MatrixMode_Texture0] = false;
	}

	if(glw_state->matricesDirty[glwstate_t::MatrixMode_Texture1])
	{
		glw_state->device->SetTransform(D3DTS_TEXTURE1,
			glw_state->matrixStack[glwstate_t::MatrixMode_Texture1]->GetTop());

		glw_state->matricesDirty[glwstate_t::MatrixMode_Texture1] = false;
	}
}


/*
=================
_getMaxVerts

Calculate the maximum number of verts to draw
given a total number to draw, stride and max
packet size.
=================
*/
static int _getMaxVerts(void)
{
	int max = glw_state->totalVertices;
	if (max > GLW_MAX_DRAW_PACKET_SIZE / glw_state->drawStride)
	{
		max = GLW_MAX_DRAW_PACKET_SIZE / glw_state->drawStride;
	}
	return max;
}

static int _getMaxIndices(void)
{
	int max = glw_state->totalIndices;
	if(max > 1022)
		max = 1022;

	return max;
}

#ifdef _XBOX
/*
=================
_restartDrawPacket

Encode a new draw packet header into the draw array.
=================
*/
inline static DWORD* _restartDrawPacket(DWORD* packet, int verts)
{
	packet[0] = D3DPUSH_ENCODE(D3DPUSH_SET_BEGIN_END, 1);
	packet[1] = glw_state->primitiveMode;
	packet[2] = D3DPUSH_ENCODE(
		D3DPUSH_NOINCREMENT_FLAG|D3DPUSH_INLINE_ARRAY, 
		glw_state->drawStride * verts);
	return packet + 3;
}

/*
=================
_terminateDrawPacket

Finish up the last draw packet.
=================
*/
inline static DWORD* _terminateDrawPacket(DWORD* packet)
{
	packet[0] = D3DPUSH_ENCODE(D3DPUSH_SET_BEGIN_END, 1);
	packet[1] = 0;
	return packet + 2;
}

#define CMD_DRAW_INDEX_BATCH 0x1800
inline static DWORD* _restartIndexPacket(DWORD* packet, int numIndices)
{
	packet[0] = D3DPUSH_ENCODE(D3DPUSH_SET_BEGIN_END, 1);
	packet[1] = glw_state->primitiveMode;
	packet[2] = D3DPUSH_ENCODE(	D3DPUSH_NOINCREMENT_FLAG | CMD_DRAW_INDEX_BATCH, numIndices / 2 );
	return packet + 3;
}

inline static DWORD* _terminateIndexPacket(DWORD* packet)
{
	packet[0] = D3DPUSH_ENCODE(D3DPUSH_SET_BEGIN_END, 1);
	packet[1] = 0;
	return packet + 2;
}


/*
=================
_handleDrawOverflow

Prevent a draw packet from getting too
big for the hardware by restarting it as needed.
=================
*/
static void _handleDrawOverflow(void)
{
	if (glw_state->numVertices >= glw_state->maxVertices)
	{
		glw_state->drawArray += glw_state->numVertices *
			glw_state->drawStride;
		
		glw_state->totalVertices -= glw_state->numVertices;
		glw_state->maxVertices = _getMaxVerts();
		glw_state->numVertices = 0;

		glw_state->drawArray = _restartDrawPacket(
			glw_state->drawArray, glw_state->maxVertices);
	}
}
#else _XBOX
inline static DWORD* _restartDrawPacket(DWORD* packet, int verts)
{
	return packet;
}

inline static DWORD* _terminateDrawPacket(DWORD* packet)
{
	return packet;
}

static void _handleDrawOverflow(void)
{
}
#endif _XBOX


/*
=================
_vertexElement

Copy position information from the source vertex
array into a draw array.
=================
*/
#define _vertexElement(push, i)									\
{																\
	DWORD* vert = (DWORD*)((BYTE*)glw_state->vertexPointer +	\
		(i) * glw_state->vertexStride);							\
	(push)[0] = vert[0];										\
	(push)[1] = vert[1];										\
	(push)[2] = vert[2];										\
}

/*
=================
_colorElement

Copy color information from the source color
array into a draw array.
=================
*/
//#define _colorElement(push, i)									\
//{																\
//	DWORD col = *(DWORD*)((BYTE*)glw_state->colorPointer +		\
//		(i) * glw_state->colorStride);							\
//	(push)[0] =													\
//		((col & 0xFF000000) >> 0) |								\
//		((col & 0x00FF0000) >> 16) |							\
//		((col & 0x0000FF00) << 0) |								\
//		((col & 0x000000FF) << 16);								\
//}
#define _colorElement(push, i)									\
{																\
	DWORD col = *(DWORD*)((BYTE*)glw_state->colorPointer +		\
		(i) * glw_state->colorStride);							\
	(push)[0] =	col;												\
}

/*
=================
_texCoordElement

Copy tex coord information from the source tex coord
array into a draw array.
=================
*/
#define _texCoordElement(push, i, t)							\
{																\
	DWORD* tc = (DWORD*)((BYTE*)glw_state->texCoordPointer[t] +	\
		(i) * glw_state->texCoordStride[t]);					\
	(push)[0] = tc[0];											\
	(push)[1] = tc[1];											\
}

/*
=================
_normalElement

  Copy normal information from the source normal
  array into a draw array
=================
*/
#define _normalElement(push, i)									\
{																\
	DWORD* norm = (DWORD*)((BYTE*)glw_state->normalPointer +	\
		(i) * glw_state->normalStride);							\
	(push)[0] = norm[0];										\
	(push)[1] = norm[1];										\
	(push)[2] = norm[2];										\
}


#define _tangentElement(push, i)								\
{																\
	DWORD* tang = (DWORD*)((BYTE*)&tess.tangent[i]);				\
	(push)[0] = tang[0];										\
	(push)[1] = tang[1];										\
	(push)[2] = tang[2];										\
}



/*
=========================================================
FAST INDEXED GEOMETRY DRAW LOOPS

Used by core draw routines to quickly copy
geometry from various source arrays to main
draw array.
=========================================================
*/
static void _drawElementsV(GLsizei count, const GLushort* indices)
{
	DWORD* push = glw_state->drawArray;
	for (int i = 0; i < count; ++i)
	{
		_vertexElement(&push[0], indices[i]);
		push[3] = glw_state->currentColor;
		push += 4;
	}
}

static void _drawElementsVN(GLsizei count, const GLushort* indices)
{
	DWORD* push = glw_state->drawArray;
	for (int i = 0; i < count; ++i)
	{
		_vertexElement(&push[0], indices[i]);
		_normalElement(&push[3], indices[i]);
		push[6] = glw_state->currentColor;
		push += 7;
	}
}

static void _drawElementsVC(GLsizei count, const GLushort* indices)
{
	DWORD* push = glw_state->drawArray;
	for (int i = 0; i < count; ++i)
	{
		_vertexElement(&push[0], indices[i]);
		_colorElement(&push[3], indices[i]);
		push += 4;
	}
}

static void _drawElementsVCN(GLsizei count, const GLushort* indices)
{
	DWORD* push = glw_state->drawArray;
	for (int i = 0; i < count; ++i)
	{
		_vertexElement(&push[0], indices[i]);
		_normalElement(&push[3], indices[i]);
		_colorElement(&push[6], indices[i]);
		push += 7;
	}
}

static void _drawElementsVCT(GLsizei count, const GLushort* indices)
{
	DWORD* push = glw_state->drawArray;
	for (int i = 0; i < count; ++i)
	{
		_vertexElement(&push[0], indices[i]);
		_colorElement(&push[3], indices[i]);
		_texCoordElement(&push[4], indices[i], 0);
		push += 6;
	}
}

static void _drawElementsVCNT(GLsizei count, const GLushort* indices)
{
	DWORD* push = glw_state->drawArray;
	for (int i = 0; i < count; ++i)
	{
		_vertexElement(&push[0], indices[i]);
		_normalElement(&push[3], indices[i]);
		_colorElement(&push[6], indices[i]);
		_texCoordElement(&push[7], indices[i], 0);
		push += 9;
	}
}

static void _drawElementsVCTT(GLsizei count, const GLushort* indices)
{
	DWORD* push = glw_state->drawArray;
	for (int i = 0; i < count; ++i)
	{
		_vertexElement(&push[0], indices[i]);
		_colorElement(&push[3], indices[i]);
		_texCoordElement(&push[4], indices[i], 0);
		_texCoordElement(&push[6], indices[i], 1);
		push += 8;
	}
}

static void _drawElementsVCNTT(GLsizei count, const GLushort* indices)
{
	DWORD* push = glw_state->drawArray;
	for (int i = 0; i < count; ++i)
	{
		_vertexElement(&push[0], indices[i]);
		_normalElement(&push[3], indices[i]);
		_colorElement(&push[6], indices[i]);
		_texCoordElement(&push[7], indices[i], 0);
		_texCoordElement(&push[9], indices[i], 1);
		push += 11;
	}
}

static void _drawElementsVT(GLsizei count, const GLushort* indices)
{
	DWORD* push = glw_state->drawArray;
	for (int i = 0; i < count; ++i)
	{
		_vertexElement(&push[0], indices[i]);
		push[3] = glw_state->currentColor;
		_texCoordElement(&push[4], indices[i], 0);
		push += 6;
	}
}

static void _drawElementsVNT(GLsizei count, const GLushort* indices)
{
	DWORD* push = glw_state->drawArray;
	for (int i = 0; i < count; ++i)
	{
		_vertexElement(&push[0], indices[i]);
		_normalElement(&push[3], indices[i]);
		push[6] = glw_state->currentColor;
		_texCoordElement(&push[7], indices[i], 0);
		push += 9;
	}
}

static void _drawElementsVTT(GLsizei count, const GLushort* indices)
{
	DWORD* push = glw_state->drawArray;
	for (int i = 0; i < count; ++i)
	{
		_vertexElement(&push[0], indices[i]);
		push[3] = glw_state->currentColor;
		_texCoordElement(&push[4], indices[i], 0);
		_texCoordElement(&push[6], indices[i], 1);
		push += 8;
	}
}

static void _drawElementsVNTT(GLsizei count, const GLushort* indices)
{
	DWORD* push = glw_state->drawArray;
	for (int i = 0; i < count; ++i)
	{
		_vertexElement(&push[0], indices[i]);
		_normalElement(&push[3], indices[i]);
		push[6] = glw_state->currentColor;
		_texCoordElement(&push[7], indices[i], 0);
		_texCoordElement(&push[9], indices[i], 1);
		push += 11;
	}
}


static void _drawElementsLightShader(GLsizei count, const GLushort* indices)
{
	DWORD* push = glw_state->drawArray;
	for(int i = 0; i < count; ++i)
	{
		_vertexElement(&push[0], indices[i]);
		_normalElement(&push[3], indices[i]);
		_texCoordElement(&push[6], indices[i], 0);
		_tangentElement(&push[8], indices[i]);
		push += 11;
	}
}

static void _drawElementsBumpShader(GLsizei count, const GLushort* indices)
{
	DWORD* push = glw_state->drawArray;
	for(int i = 0; i < count; ++i)
	{
		_vertexElement(&push[0], indices[i]);
		_normalElement(&push[3], indices[i]);
		_texCoordElement(&push[6], indices[i], 0);
		_texCoordElement(&push[8], indices[i], 1);
		_tangentElement(&push[10], indices[i]);
		push += 13;
	}
}

static void _drawElementsEnvShader(GLsizei count, const GLushort* indices)
{
	DWORD* push = glw_state->drawArray;
	for(int i = 0; i < count; ++i)
	{
		_vertexElement(&push[0], indices[i]);
		_normalElement(&push[3], indices[i]);
		push += 6;
	}
}

typedef void(*drawelemfunc_t)(GLsizei, const GLushort*);
static drawelemfunc_t _drawElementFuncTable[12] =
{
	_drawElementsV,
	_drawElementsVN,
	_drawElementsVT,
	_drawElementsVNT,
	_drawElementsVTT,
	_drawElementsVNTT,
	_drawElementsVC,
	_drawElementsVCN,
	_drawElementsVCT,
	_drawElementsVCNT,
	_drawElementsVCTT,
	_drawElementsVCNTT,
};



/*
=========================================================
FAST GEOMETRY DRAW LOOPS

Used by core draw routines to quickly copy
geometry from various source arrays to main
draw array.
=========================================================
*/
static void _drawArraysV(GLsizei first, GLsizei last)
{
	DWORD* push = glw_state->drawArray;
	for (int i = first; i < last; ++i)
	{
		_vertexElement(&push[0], i);
		push[3] = glw_state->currentColor;
		push += 4;
	}
}

static void _drawArraysVN(GLsizei first, GLsizei last)
{
	DWORD* push = glw_state->drawArray;
	for (int i = first; i < last; ++i)
	{
		_vertexElement(&push[0], i);
		_normalElement(&push[3], i);
		push[6] = glw_state->currentColor;
		push += 7;
	}
}

static void _drawArraysVC(GLsizei first, GLsizei last)
{
	DWORD* push = glw_state->drawArray;
	for (int i = first; i < last; ++i)
	{
		_vertexElement(&push[0], i);
		_colorElement(&push[3], i);
		push += 4;
	}
}

static void _drawArraysVCN(GLsizei first, GLsizei last)
{
	DWORD* push = glw_state->drawArray;
	for (int i = first; i < last; ++i)
	{
		_vertexElement(&push[0], i);
		_normalElement(&push[3], i);
		_colorElement(&push[6], i);
		push += 7;
	}
}

static void _drawArraysVCT(GLsizei first, GLsizei last)
{
	DWORD* push = glw_state->drawArray;
	for (int i = first; i < last; ++i)
	{
		_vertexElement(&push[0], i);
		_colorElement(&push[3], i);
		_texCoordElement(&push[4], i, 0);
		push += 6;
	}
}

static void _drawArraysVCNT(GLsizei first, GLsizei last)
{
	DWORD* push = glw_state->drawArray;
	for (int i = first; i < last; ++i)
	{
		_vertexElement(&push[0], i);
		_normalElement(&push[3], i);
		_colorElement(&push[6], i);
		_texCoordElement(&push[7], i, 0);
		push += 9;
	}
}

static void _drawArraysVCTT(GLsizei first, GLsizei last)
{
	DWORD* push = glw_state->drawArray;
	for (int i = first; i < last; ++i)
	{
		_vertexElement(&push[0], i);
		_colorElement(&push[3], i);
		_texCoordElement(&push[4], i, 0);
		_texCoordElement(&push[6], i, 1);
		push += 8;
	}
}

static void _drawArraysVCNTT(GLsizei first, GLsizei last)
{
	DWORD* push = glw_state->drawArray;
	for (int i = first; i < last; ++i)
	{
		_vertexElement(&push[0], i);
		_normalElement(&push[3], i);
		_colorElement(&push[6], i);
		_texCoordElement(&push[7], i, 0);
		_texCoordElement(&push[9], i, 1);
		push += 11;
	}
}

static void _drawArraysVT(GLsizei first, GLsizei last)
{
	DWORD* push = glw_state->drawArray;
	for (int i = first; i < last; ++i)
	{
		_vertexElement(&push[0], i);
		push[3] = glw_state->currentColor;
		_texCoordElement(&push[4], i, 0);
		push += 6;
	}
}

static void _drawArraysVNT(GLsizei first, GLsizei last)
{
	DWORD* push = glw_state->drawArray;
	for (int i = first; i < last; ++i)
	{
		_vertexElement(&push[0], i);
		_normalElement(&push[3], i);
		push[6] = glw_state->currentColor;
		_texCoordElement(&push[7], i, 0);
		push += 9;
	}
}

static void _drawArraysVTT(GLsizei first, GLsizei last)
{
	DWORD* push = glw_state->drawArray;
	for (int i = first; i < last; ++i)
	{
		_vertexElement(&push[0], i);
		push[3] = glw_state->currentColor;
		_texCoordElement(&push[4], i, 0);
		_texCoordElement(&push[6], i, 1);
		push += 8;
	}
}

static void _drawArraysVNTT(GLsizei first, GLsizei last)
{
	DWORD* push = glw_state->drawArray;
	for (int i = first; i < last; ++i)
	{
		_vertexElement(&push[0], i);
		_normalElement(&push[3], i);
		push[6] = glw_state->currentColor;
		_texCoordElement(&push[7], i, 0);
		_texCoordElement(&push[9], i, 1);
		push += 11;
	}
}

typedef void(*drawarrayfunc_t)(GLsizei, GLsizei);
static drawarrayfunc_t _drawArrayFuncTable[12] =
{
	_drawArraysV,
	_drawArraysVN,
	_drawArraysVT,
	_drawArraysVNT,
	_drawArraysVTT,
	_drawArraysVNTT,
	_drawArraysVC,
	_drawArraysVCN,
	_drawArraysVCT,
	_drawArraysVCNT,
	_drawArraysVCTT,
	_drawArraysVCNTT,
};


/*
=================
_getDrawFunc

Figure which drawing function we need based on
what vertex components we have.  Use the returned
integer to index the draw function tables.
=================
*/
static int _getDrawFunc(void)
{
	int func = 0;
	if (glw_state->colorArrayState) func += 6;
	if (glw_state->texCoordArrayState[0]) func += 2;
	if (glw_state->texCoordArrayState[1]) func += 2;
	if (glw_state->normalArrayState) ++func;
	return func;
}


static void dllAccum(GLenum op, GLfloat value)
{
	assert(false);
}

static void dllAlphaFunc(GLenum func, GLclampf ref)
{
	D3DCMPFUNC f = _convertCompare(func);
	glw_state->device->SetRenderState(D3DRS_ALPHAFUNC, f);
	glw_state->device->SetRenderState(D3DRS_ALPHAREF, (DWORD)(ref * 255.));
}

GLboolean dllAreTexturesResident(GLsizei n, const GLuint *textures, GLboolean *residences)
{
	assert(false);
	return 1;
}

static void dllArrayElement(GLint i)
{
	assert(glw_state->inDrawBlock);
	
	_handleDrawOverflow();

	DWORD* push = &glw_state->drawArray[glw_state->numVertices * 
		glw_state->drawStride];

	_vertexElement(push, i);
	push += 3;
	
	if (glw_state->colorArrayState)
	{
		_colorElement(push, i);
		++push;
	}
	else
	{
		*push++ = glw_state->currentColor;
	}
	
	for (int t = 0; t < GLW_MAX_TEXTURE_STAGES; ++t)
	{
		if (glw_state->texCoordArrayState[t])
		{
			_texCoordElement(push, i, t);
			push += 2;
		}
	}

	++glw_state->numVertices;
}

// EXTENSION: Begin a drawing block with at verts vertices
static void dllBeginEXT(GLenum mode, GLint verts, GLint colors, GLint normals, GLint tex0, GLint tex1)//, GLint tex2, GLint tex3)
{
	assert(!glw_state->inDrawBlock);

	// start the draw block
	glw_state->inDrawBlock = true;
	glw_state->primitiveMode = _convertPrimMode(mode);

	// update DX with any pending state changes
	_updateDrawStride(normals, tex0, tex1);//, tex2, tex3);
	_updateShader(normals, tex0, tex1);//, tex2, tex3);
	_updateTextures();
	_updateMatrices();

	// set vertex counters
	glw_state->numVertices = 0;
	glw_state->totalVertices = verts;
	glw_state->maxVertices = _getMaxVerts();	

#ifdef _XBOX
	// open a draw packet
	//int num_packets = ((verts * glw_state->drawStride) / GLW_MAX_DRAW_PACKET_SIZE) + 1;
	int num_packets;
	if(glw_state->maxVertices == 0) {
		num_packets = 1;
	} else {
		num_packets = (verts / glw_state->maxVertices) + (!!(verts % glw_state->maxVertices));
	}
	int cmd_size = num_packets * 3;
	int vert_size = glw_state->drawStride * verts;
	
	glw_state->device->BeginPush(vert_size + cmd_size + 2, 
		&glw_state->drawArray);
	
	glw_state->drawArray = _restartDrawPacket(
		glw_state->drawArray, glw_state->maxVertices);
#endif
}

static void dllBegin(GLenum mode)
{
	assert(0);
}

// EXTENSION: Start a new drawing frame
GLboolean dllBeginFrame(void)
{
	GLboolean result = glw_state->device->BeginScene() == D3D_OK;
	return result;
}

// EXTENSION: Begin shadow draw mode
static void dllBeginShadow(void)
{
	//Intentionally left blank
}

static void dllBindTexture(GLenum target, GLuint texture)
{
	assert(target == GL_TEXTURE_2D);

	if (glw_state->currentTexture[glw_state->serverTU] != texture)
	{
		glw_state->currentTexture[glw_state->serverTU] = texture;
		glw_state->textureStageDirty[glw_state->serverTU] = true;
	}
}

static void dllBitmap(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap)
{
	assert(false);
}

static void dllBlendFunc(GLenum sfactor, GLenum dfactor)
{
	D3DBLEND s = _convertBlendFactor(sfactor);
	D3DBLEND d = _convertBlendFactor(dfactor);
	
	glw_state->device->SetRenderState(D3DRS_SRCBLEND, s);
	glw_state->device->SetRenderState(D3DRS_DESTBLEND, d);
}

static void dllCallList(GLuint lnum)
{
	assert(0);
}

static void dllCallLists(GLsizei n, GLenum type, const GLvoid *lists)
{
	assert(0);
}

static void dllClear(GLbitfield mask)
{
	DWORD m = 0;

	if (mask & GL_COLOR_BUFFER_BIT) m |= D3DCLEAR_TARGET;
	if (mask & GL_STENCIL_BUFFER_BIT) m |= D3DCLEAR_STENCIL;

#ifdef _XBOX
	// Clearing stencil when clearing depth buffer
	// is faster on Xbox than just clearing depth alone.
	if (mask & GL_DEPTH_BUFFER_BIT) m |= D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL;
#else
	if (mask & GL_DEPTH_BUFFER_BIT) m |= D3DCLEAR_ZBUFFER;
#endif
	
	glw_state->device->Clear(0, NULL, m, glw_state->clearColor, 
		glw_state->clearDepth, glw_state->clearStencil);
}

static void dllClearAccum(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
	assert(0);
}

static void dllClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
	glw_state->clearColor = D3DCOLOR_COLORVALUE(red, green, blue, alpha);
}

static void dllClearDepth(GLclampd depth)
{
	glw_state->clearDepth = depth;
}

static void dllClearIndex(GLfloat c)
{
	assert(0);
}

static void dllClearStencil(GLint s)
{
	glw_state->clearStencil = s;
}

static void dllClipPlane(GLenum plane, const GLdouble *equation)
{
	//FIXME
}

static void setIntColor(GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha)
{
	glw_state->currentColor = D3DCOLOR_RGBA(red, green, blue, alpha);
}

static void setFloatColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
	glw_state->currentColor = D3DCOLOR_COLORVALUE(red, green, blue, alpha);
}

static void dllColor3b(GLbyte red, GLbyte green, GLbyte blue)
{
	setIntColor(red, green, blue, 127);
}

static void dllColor3bv(const GLbyte *v)
{
	setIntColor(v[0], v[1], v[2], 127);
}

static void dllColor3d(GLdouble red, GLdouble green, GLdouble blue)
{
	setFloatColor(red, green, blue, 1.f);
}

static void dllColor3dv(const GLdouble *v)
{
	setFloatColor(v[0], v[1], v[2], 1.f);
}

static void dllColor3f(GLfloat red, GLfloat green, GLfloat blue)
{
	setFloatColor(red, green, blue, 1.f);
}

static void dllColor3fv(const GLfloat *v)
{
	setFloatColor(v[0], v[1], v[2], 1.f);
}

static void dllColor3i(GLint red, GLint green, GLint blue)
{
	setIntColor(red, green, blue, 127);
}

static void dllColor3iv(const GLint *v)
{
	setIntColor(v[0], v[1], v[2], 127);
}

static void dllColor3s(GLshort red, GLshort green, GLshort blue)
{
	setIntColor(red, green, blue, 127);
}

static void dllColor3sv(const GLshort *v)
{
	setIntColor(v[0], v[1], v[2], 127);
}

static void dllColor3ub(GLubyte red, GLubyte green, GLubyte blue)
{
	setIntColor(red, green, blue, 127);
}

static void dllColor3ubv(const GLubyte *v)
{
	setIntColor(v[0], v[1], v[2], 127);
}

static void dllColor3ui(GLuint red, GLuint green, GLuint blue)
{
	setIntColor(red, green, blue, 127);
}

static void dllColor3uiv(const GLuint *v)
{
	setIntColor(v[0], v[1], v[2], 127);
}

static void dllColor3us(GLushort red, GLushort green, GLushort blue)
{
	setIntColor(red, green, blue, 127);
}

static void dllColor3usv(const GLushort *v)
{
	setIntColor(v[0], v[1], v[2], 127);
}

static void dllColor4b(GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha)
{
	setIntColor(red, green, blue, alpha);
}

static void dllColor4bv(const GLbyte *v)
{
	setIntColor(v[0], v[1], v[2], v[3]);
}

static void dllColor4d(GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha)
{
	setFloatColor(red, green, blue, alpha);
}

static void dllColor4dv(const GLdouble *v)
{
	setFloatColor(v[0], v[1], v[2], v[3]);
}

static void dllColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
	setFloatColor(red, green, blue, alpha);
}

static void dllColor4fv(const GLfloat *v)
{
	setFloatColor(v[0], v[1], v[2], v[3]);
}

static void dllColor4i(GLint red, GLint green, GLint blue, GLint alpha)
{
	setIntColor(red, green, blue, alpha);
}

static void dllColor4iv(const GLint *v)
{
	setIntColor(v[0], v[1], v[2], v[3]);
}

static void dllColor4s(GLshort red, GLshort green, GLshort blue, GLshort alpha)
{
	setIntColor(red, green, blue, alpha);
}

static void dllColor4sv(const GLshort *v)
{
	setIntColor(v[0], v[1], v[2], v[3]);
}

static void dllColor4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha)
{
	setIntColor(red, green, blue, alpha);
}

static void dllColor4ubv(const GLubyte *v)
{
	setIntColor(v[0], v[1], v[2], v[3]);
}

static void dllColor4ui(GLuint red, GLuint green, GLuint blue, GLuint alpha)
{
	setIntColor(red, green, blue, alpha);
}

static void dllColor4uiv(const GLuint *v)
{
	setIntColor(v[0], v[1], v[2], v[3]);
}

static void dllColor4us(GLushort red, GLushort green, GLushort blue, GLushort alpha)
{
	setIntColor(red, green, blue, alpha);
}

static void dllColor4usv(const GLushort *v)
{
	setIntColor(v[0], v[1], v[2], v[3]);
}

static void dllColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{
	DWORD m = 0;
	if (red) m |= D3DCOLORWRITEENABLE_RED;
	if (green) m |= D3DCOLORWRITEENABLE_GREEN;
	if (blue) m |= D3DCOLORWRITEENABLE_BLUE;
	if (alpha) m |= D3DCOLORWRITEENABLE_ALPHA;
	glw_state->device->SetRenderState(D3DRS_COLORWRITEENABLE, m);
}

static void dllColorMaterial(GLenum face, GLenum mode)
{
	assert(0);
}

static void dllColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
	assert(!glw_state->inDrawBlock);
	assert(size == 4 && type == GL_UNSIGNED_BYTE);
	
	stride = (stride == 0) ? sizeof(GLint) : stride;

	glw_state->colorPointer = pointer; 
	glw_state->colorStride = stride;
}

static void dllCopyPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type)
{
	assert(0);
}

static void dllCopyTexImage1D(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLint border)
{
	assert(0);
}

/**********
copies a portion of the backbuffer to the current texture.
the current texture must be a linear format texture, if
a swizzled texture format is needed, use
dllCopyBackBufferToTexEXT
**********/
static void dllCopyTexImage2D(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border)
{
	// check to make sure everything passed in is supported
	assert((target == GL_TEXTURE_2D) && (level == 0) && (border == 0));

	// locals
	RECT						rSrc;
	POINT						ptUpperLeft;
	LPDIRECT3DSURFACE8			tSurf;
	LPDIRECT3DSURFACE8			backbuffer;
	glwstate_t::TextureInfo*	tex;
	HRESULT						res;
		
	// get the current texture
	tex = _getCurrentTexture(glw_state->serverTU);
	if (tex == NULL)
	{
		return;
	}

	// set up the source rectangle
	rSrc.left	= x;
	rSrc.right	= x + width;
	rSrc.top	= (480 - y) - height;
	rSrc.bottom	= (480 - y);

	// set up the target point
	ptUpperLeft.x	= 0;
	ptUpperLeft.y	= 0;

	// attach the current texture to a surface
	tex->mipmap->GetSurfaceLevel(0, &tSurf);

	// attach the back buffer to a surface
	res = glw_state->device->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &backbuffer);

	// copy the data
	res = glw_state->device->CopyRects(backbuffer, &rSrc, 0, tSurf, &ptUpperLeft);

	// release surfaces
	tSurf->Release();
	backbuffer->Release();
}

static void dllCopyTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width)
{
	assert(0);
}

static void dllCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
	assert(0);
}

static void dllCullFace(GLenum mode)
{
	switch (mode)
	{
	default: case GL_BACK: glw_state->cullMode = D3DCULL_CW; break;
	case GL_FRONT: glw_state->cullMode = D3DCULL_CCW; break;
	}

	glw_state->device->SetRenderState(D3DRS_CULLMODE, glw_state->cullMode);
}

static void dllDeleteLists(GLuint lnum, GLsizei range)
{
	assert(0);
}

static void dllDeleteTextures(GLsizei n, const GLuint *textures)
{
	glw_state->textureStageDirty[0] = true;
	glw_state->textureStageDirty[1] = true;
	glw_state->device->SetTexture(0, NULL);
	glw_state->device->SetTexture(1, NULL);
//	glw_state->device->SetTexture(2, NULL);
//	glw_state->device->SetTexture(3, NULL);

	for (int t = 0; t < n; ++t)
	{
		glwstate_t::texturexlat_t::iterator i = 
			glw_state->textureXlat.find(textures[t]);

		if (i != glw_state->textureXlat.end())
		{
#if MEMORY_PROFILE
			texMemSize -= getTexMemSize(i->second.mipmap);
#endif
			i->second.mipmap->BlockUntilNotBusy();
			delete i->second.mipmap;
			glw_state->textureXlat.erase(i);
		}
	}
}

static void dllDepthFunc(GLenum func)
{
	D3DCMPFUNC f = _convertCompare(func);
	glw_state->device->SetRenderState(D3DRS_ZFUNC, f);
}

static void dllDepthMask(GLboolean flag)
{
	glw_state->device->SetRenderState(D3DRS_ZWRITEENABLE, flag);
}

static void dllDepthRange(GLclampd zNear, GLclampd zFar)
{
	glw_state->viewport.MinZ = zNear;
	glw_state->viewport.MaxZ = zFar;
	glw_state->device->SetViewport(&glw_state->viewport);
}

#ifdef _XBOX
static void setPresent(bool vsync)
{
	//extern void ShowOSMemory();
	//ShowOSMemory();

	D3DPRESENT_PARAMETERS pp;
	pp.BackBufferWidth = glConfig.vidWidth;
	pp.BackBufferHeight = glConfig.vidHeight;
	pp.BackBufferFormat = D3DFMT_X8R8G8B8;
	pp.BackBufferCount = 1;
	pp.MultiSampleType  = D3DMULTISAMPLE_NONE; //D3DMULTISAMPLE_4_SAMPLES_SUPERSAMPLE_LINEAR;
	pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	pp.hDeviceWindow = 0;
	pp.Windowed  = FALSE;
	pp.EnableAutoDepthStencil = TRUE;
	pp.AutoDepthStencilFormat = D3DFMT_D24S8;
	pp.Flags = 0;
	pp.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
	pp.FullScreen_PresentationInterval = 
		vsync ? D3DPRESENT_INTERVAL_DEFAULT : D3DPRESENT_INTERVAL_IMMEDIATE;
	pp.BufferSurfaces[0] = pp.BufferSurfaces[1] = pp.BufferSurfaces[2] = 0;
	pp.DepthStencilSurface = 0;
	glw_state->device->PersistDisplay();
	glw_state->device->Reset(&pp);

	//ShowOSMemory();
}
#endif

static void setCap(GLenum cap, bool flag)
{
	switch (cap)
	{
	case GL_ALPHA_TEST: glw_state->device->SetRenderState(D3DRS_ALPHATESTENABLE, flag); break;
	case GL_BLEND: glw_state->device->SetRenderState(D3DRS_ALPHABLENDENABLE, flag); break;
	case GL_CULL_FACE:
		glw_state->cullEnable = flag;
		glw_state->device->SetRenderState(D3DRS_CULLMODE, 
			flag ? glw_state->cullMode : D3DCULL_NONE);
		break;
	case GL_DEPTH_TEST: glw_state->device->SetRenderState(D3DRS_ZENABLE, flag); break;
	case GL_LIGHTING: glw_state->device->SetRenderState(D3DRS_LIGHTING, flag); break;
#ifdef _XBOX
	case GL_POLYGON_OFFSET_POINT:
		glw_state->device->SetRenderState(D3DRS_POINTOFFSETENABLE, flag);
		break;
	case GL_POLYGON_OFFSET_LINE:
		glw_state->device->SetRenderState(D3DRS_WIREFRAMEOFFSETENABLE, flag);
		break;
	case GL_POLYGON_OFFSET_FILL:
		glw_state->device->SetRenderState(D3DRS_SOLIDOFFSETENABLE, flag);
		break;
	case GL_SCISSOR_TEST:
		glw_state->scissorEnable = flag;
		glw_state->device->SetScissors(flag ? 1 : 0, FALSE, &glw_state->scissorBox);
		break;
#endif
	case GL_STENCIL_TEST: glw_state->device->SetRenderState(D3DRS_STENCILENABLE, flag); break;
	case GL_TEXTURE_2D: 
		glw_state->textureStageEnable[glw_state->serverTU] = flag;
		glw_state->textureStageDirty[glw_state->serverTU] = true;
		break;
	case GL_FOG:
		glw_state->device->SetRenderState(D3DRS_FOGENABLE, flag);
		break;
#ifdef _XBOX
	case GL_VSYNC:
		setPresent(flag);
		break;
#endif
	default: break;
	}
}

static void dllDisable(GLenum cap)
{
	setCap(cap, false);
}

static void setArrayState(GLenum cap, bool state)
{
	switch (cap)
	{
	case GL_COLOR_ARRAY: glw_state->colorArrayState = state; break; 
	case GL_TEXTURE_COORD_ARRAY: glw_state->texCoordArrayState[glw_state->clientTU] = state; break;
	case GL_VERTEX_ARRAY: glw_state->vertexArrayState = state; break;
	case GL_NORMAL_ARRAY: glw_state->normalArrayState = state; break;
	}
}

static void dllDisableClientState(GLenum array)
{
	assert(!glw_state->inDrawBlock);
	setArrayState(array, false);
}

#ifdef _WINDOWS
static void _convertQuadsToTris(GLint first, GLsizei count)
{
	glw_state->vertexPointerBack = glw_state->vertexPointer;
	glw_state->normalPointerBack = glw_state->normalPointer;
	glw_state->colorPointerBack = glw_state->colorPointer;
	glw_state->texCoordPointerBack[0] = glw_state->texCoordPointer[0];
	glw_state->texCoordPointerBack[1] = glw_state->texCoordPointer[1];
	
	{
		glw_state->vertexPointer = 
			Z_Malloc(count * glw_state->vertexStride * 3 / 2, 
			TAG_TEMP_WORKSPACE, qfalse);
		for (int i = 0; i < count; i += 4)
		{
			int stride = glw_state->vertexStride / sizeof(float);
			float* dst = (float*)glw_state->vertexPointer + (i * 3 / 2) * stride;
			const float* src = (const float*)glw_state->vertexPointerBack + 
				(first + i) * stride;
			
			for (int j = 0; j < 3; ++j)
			{
				dst[0 * stride + j] = src[0 * stride + j];
				dst[1 * stride + j] = src[1 * stride + j];
				dst[2 * stride + j] = src[2 * stride + j];
				dst[3 * stride + j] = src[0 * stride + j];
				dst[4 * stride + j] = src[2 * stride + j];
				dst[5 * stride + j] = src[3 * stride + j];
			}
		}
	}
	
	if (glw_state->normalArrayState)
	{
		glw_state->normalPointer = 
			Z_Malloc(count * glw_state->normalStride * 3 / 2, 
			TAG_TEMP_WORKSPACE, qfalse);
		
		for (int i = 0; i < count; i += 4)
		{
			int stride = glw_state->normalStride / sizeof(float);
			float* dst = (float*)glw_state->normalPointer + (i * 3 / 2) * stride;
			const float* src = (const float*)glw_state->normalPointerBack + 
				(first + i) * stride;
			
			for (int j = 0; j < 3; ++j)
			{
				dst[0 * stride + j] = src[0 * stride + j];
				dst[1 * stride + j] = src[1 * stride + j];
				dst[2 * stride + j] = src[2 * stride + j];
				dst[3 * stride + j] = src[0 * stride + j];
				dst[4 * stride + j] = src[2 * stride + j];
				dst[5 * stride + j] = src[3 * stride + j];
			}
		}
	}
	
	if (glw_state->colorArrayState)
	{
		glw_state->colorPointer = 
			Z_Malloc(count * glw_state->colorStride * 3 / 2, 
			TAG_TEMP_WORKSPACE, qfalse);
		
		for (int i = 0; i < count; i += 4)
		{
			int stride = glw_state->colorStride / sizeof(DWORD);
			DWORD* dst = (DWORD*)glw_state->colorPointer + (i * 3 / 2) * stride;
			const DWORD* src = (const DWORD*)glw_state->colorPointerBack + 
				(first + i) * stride;
			
			dst[0 * stride] = src[0 * stride];
			dst[1 * stride] = src[1 * stride];
			dst[2 * stride] = src[2 * stride];
			dst[3 * stride] = src[0 * stride];
			dst[4 * stride] = src[2 * stride];
			dst[5 * stride] = src[3 * stride];
		}
	}
	
	for (int t = 0; t < GLW_MAX_TEXTURE_STAGES; ++t)
	{
		if (glw_state->texCoordArrayState[t])
		{
			glw_state->texCoordPointer[t] = 
				Z_Malloc(count * glw_state->texCoordStride[t] * 3 / 2, 
				TAG_TEMP_WORKSPACE, qfalse);
			
			for (int i = 0; i < count; i += 4)
			{
				int stride = glw_state->texCoordStride[t] / sizeof(float);
				float* dst = (float*)glw_state->texCoordPointer[t] + (i * 3 / 2) * stride;
				const float* src = (const float*)glw_state->texCoordPointerBack[t] + 
					(first + i) * stride;
				
				for (int j = 0; j < 2; ++j)
				{
					dst[0 * stride + j] = src[0 * stride + j];
					dst[1 * stride + j] = src[1 * stride + j];
					dst[2 * stride + j] = src[2 * stride + j];
					dst[3 * stride + j] = src[0 * stride + j];
					dst[4 * stride + j] = src[2 * stride + j];
					dst[5 * stride + j] = src[3 * stride + j];
				}
			}
		}
	}
}

static void _cleanupQuadsToTris(void)
{
	Z_Free(const_cast<void*>(glw_state->vertexPointer));
	glw_state->vertexPointer = glw_state->vertexPointerBack;
	
	if (glw_state->normalArrayState)
	{
		Z_Free(const_cast<void*>(glw_state->normalPointer));
		glw_state->normalPointer = glw_state->normalPointerBack;
	}
	
	if (glw_state->colorArrayState)
	{
		Z_Free(const_cast<void*>(glw_state->colorPointer));
		glw_state->colorPointer = glw_state->colorPointerBack;
	}

	for (int t = 0; t < GLW_MAX_TEXTURE_STAGES; ++t)
	{
		if (glw_state->texCoordArrayState[t])
		{
			Z_Free(const_cast<void*>(glw_state->texCoordPointer[t]));
			glw_state->texCoordPointer[t] = glw_state->texCoordPointerBack[t];
		}
	}
}
#endif

// NOTE: This is a core draw routine.  It should be fast.
static void dllDrawArrays(GLenum mode, GLint first, GLsizei count)
{
#ifdef _WINDOWS
	if (mode == GL_QUADS)
	{
		_convertQuadsToTris(first, count);
		count = count * 3 / 2;
		first = 0;
	}
#endif

	// start the draw mode
	qglBeginEXT(mode, count, glw_state->colorArrayState ? count : 0, 
		glw_state->normalArrayState ? count : 0,
		glw_state->texCoordArrayState[0] ? count : 0,
		glw_state->texCoordArrayState[1] ? count : 0);

	// get the draw function we need
	drawarrayfunc_t func = _drawArrayFuncTable[_getDrawFunc()];

#ifndef _XBOX
	DWORD* base = glw_state->drawArray;
#endif

	int inc = glw_state->maxVertices;
	// loop taking care not to draw too much at a time
	for (int start = first; ; start += inc)//glw_state->maxVertices)
	{
		// draw glw_state->maxVertices amount of geometry
		func(start, start + glw_state->maxVertices);
		
		// are we done yet?
		glw_state->totalVertices -= glw_state->maxVertices;
		if (glw_state->totalVertices <= 0)
		{
			glw_state->numVertices = glw_state->maxVertices;
			break;
		}
		
		// ready for another cycle
		glw_state->drawArray += glw_state->maxVertices *
			glw_state->drawStride;
		glw_state->maxVertices = _getMaxVerts();

		glw_state->drawArray = _restartDrawPacket(
			glw_state->drawArray, glw_state->maxVertices); 
	}
	
#ifndef _XBOX
	glw_state->drawArray = base;
#endif

#ifdef _WINDOWS
	if (mode == GL_QUADS)
	{
		_cleanupQuadsToTris();
	}
#endif

	// finish up the draw
	qglEnd();
}

static void dllDrawBuffer(GLenum mode)
{
	//FIXME
}


static void PushIndices(GLsizei count, const GLushort *indices)
{
	// open the index packet
	// can only send 2047 indices thru at a time
	// BUT, Microsoft recommends 511 pairs at a time (?)
	int num_packets, numpairs;
	bool singleindex = false;

	numpairs = count / 2;

	if(numpairs <= 511) 
	{
		num_packets = 1;

		if(glw_state->maxIndices % 2)
		{
			glw_state->maxIndices -= 1;
			singleindex = true;
		}
	} else 
	{
		num_packets = (count / glw_state->maxIndices) + (!!(count % glw_state->maxIndices));
	}
	
	glw_state->drawArray = _restartIndexPacket(glw_state->drawArray, glw_state->maxIndices);

	int inc = glw_state->maxIndices;
	for (int start = 0; ; start += inc)
	{
		// memcpy is faster than looping copy:
		memcpy( glw_state->drawArray, indices+start, glw_state->maxIndices * sizeof(WORD) );
		glw_state->drawArray += glw_state->maxIndices / 2;

		/*
		for(int i = start; i < start + glw_state->maxIndices; i += 2)
		{
			*glw_state->drawArray++ = (DWORD)(((WORD)indices[i + 1] << 16) + (WORD)indices[i]);
		}
		*/

		// are we done yet?
		glw_state->totalIndices -= glw_state->maxIndices;
		if (glw_state->totalIndices <= 1)
		{
			glw_state->numIndices = glw_state->maxIndices;
			break;
		}

		// ready for another cycle
		//glw_state->drawArray += glw_state->maxVertices * glw_state->drawStride;
		glw_state->maxIndices = _getMaxIndices();

		if(glw_state->maxIndices % 2)
		{
			glw_state->maxIndices -= 1;
			singleindex = true;
		}

		glw_state->drawArray = _restartIndexPacket(glw_state->drawArray, glw_state->maxIndices);
	}

#define CMD_DRAW_INDEX_LAST 0x1808 
	if(singleindex)
	{
		*glw_state->drawArray++ = D3DPUSH_ENCODE(CMD_DRAW_INDEX_LAST, 1);
		*glw_state->drawArray++ = indices[count - 1];
	}
}

// NOTE: This is a core draw routine.  It should be fast.
static void dllDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices)
{
	int normals, tex0, tex1, num_streams = 2;

	assert(type == GL_UNSIGNED_SHORT);

	normals = glw_state->normalArrayState ? tess.numVertexes : 0;
	tex0 = glw_state->texCoordArrayState[0] ? tess.numVertexes : 0;
	tex1 = glw_state->texCoordArrayState[1] ? tess.numVertexes : 0;

	num_streams += ((normals > 0) + (tex0 > 0) + (tex1 > 0));

	// start the draw block
	glw_state->inDrawBlock = true;
	glw_state->primitiveMode = _convertPrimMode(mode);

	// update DX with any pending state changes
	if(tess.currentPass == 0)
	{
        _updateDrawStride(normals, tex0, tex1);
		glw_state->drawStride += normals ? 2 : 1;
	}
	else
	{
		glw_state->drawStride = 1;
		if(normals && !tess.pNormal) glw_state->drawStride += 4;
		if(tex0) glw_state->drawStride += 2;
		if(tex1) glw_state->drawStride += 2;
	}
	_updateShader(normals, tex0, tex1);
	_updateTextures();
	_updateMatrices();

	glw_state->numIndices = 0;
	glw_state->totalIndices = count;
	glw_state->maxIndices = _getMaxIndices();

	glw_state->device->SetStreamSource(0, NULL, glw_state->drawStride * 4);

	int vert_size = glw_state->drawStride * tess.numVertexes;
	int index_size = count / 2; 

	glw_state->device->BeginPush(vert_size + index_size + 60, &glw_state->drawArray);

	glw_state->drawArray = (DWORD*)*((DWORD*)glw_state->device);

	DWORD *jumpaddress = 0, *stream = 0;

	// Only copy the geometry in on the first pass
	// Multiple passes will reuse this geometry (except tex coords)
	if(tess.currentPass == 0)
	{
		// Determine where the end of the vertex data is gonna be,
		// that's where we're going to jump to
        jumpaddress = (DWORD*)*((DWORD*)glw_state->device) + (vert_size + 1);

		// Write the jump address
		*glw_state->drawArray++ = ((DWORD)jumpaddress & 0x7fffffff) | 1;

		// Set up our own fake vertex buffer
		stream = glw_state->drawArray;

		memcpy(glw_state->drawArray, tess.xyz, sizeof(vec4_t) * tess.numVertexes);
		glw_state->drawArray += tess.numVertexes * 4;

		if(normals)
		{
			memcpy(glw_state->drawArray, tess.normal, sizeof(vec4_t) * tess.numVertexes);
			glw_state->drawArray += tess.numVertexes * 4;
		}

		if(glw_state->colorArrayState)
		{
			memcpy(glw_state->drawArray, tess.svars.colors, sizeof(D3DCOLOR) * tess.numVertexes);
		}
		else
		{
			for( int v = 0; v < tess.numVertexes; ++v )
				glw_state->drawArray[v] = glw_state->currentColor;
		}
		glw_state->drawArray += tess.numVertexes;

		if(tex0)
		{
			memcpy(glw_state->drawArray, tess.svars.texcoords[0], sizeof(vec2_t) * tess.numVertexes);
			glw_state->drawArray += tess.numVertexes * 2;
		}

		if(tex1)
		{
			memcpy(glw_state->drawArray, tess.svars.texcoords[1], sizeof(vec2_t) * tess.numVertexes);
			glw_state->drawArray += tess.numVertexes * 2;
		}
	}
	else
	{
		// Determine where the end of the vertex data is gonna be,
		// that's where we're going to jump to
        jumpaddress = (DWORD*)*((DWORD*)glw_state->device) + (vert_size + 1);

		// Write the jump address
		*glw_state->drawArray++ = ((DWORD)jumpaddress & 0x7fffffff) | 1;

		// Set up our own fake vertex buffer
		stream = glw_state->drawArray;

		if(normals && !tess.pNormal)
		{
			memcpy(glw_state->drawArray, tess.normal, sizeof(vec4_t) * tess.numVertexes);
			glw_state->drawArray += tess.numVertexes * 4;
		}

		if(glw_state->colorArrayState)
		{
			memcpy(glw_state->drawArray, tess.svars.colors, sizeof(D3DCOLOR) * tess.numVertexes);
		}
		else
		{
			for( int v = 0; v < tess.numVertexes; ++v )
				glw_state->drawArray[v] = glw_state->currentColor;
		}
		glw_state->drawArray += tess.numVertexes;

		if(tex0)
		{
			memcpy(glw_state->drawArray, tess.svars.texcoords[0], sizeof(vec2_t) * tess.numVertexes);
			glw_state->drawArray += tess.numVertexes * 2;
		}

		if(tex1)
		{
			memcpy(glw_state->drawArray, tess.svars.texcoords[1], sizeof(vec2_t) * tess.numVertexes);
			glw_state->drawArray += tess.numVertexes * 2;
		}
	}

	// Write the vertex shader
#define CMD_STREAM_STRIDEANDTYPE0 0x1760
	*glw_state->drawArray++ = D3DPUSH_ENCODE(CMD_STREAM_STRIDEANDTYPE0, 16);
	*glw_state->drawArray++ = (16 << 8)|D3DVSDT_FLOAT3;

	if(1)
	{
		*glw_state->drawArray++ = ((glw_state->drawStride * 4) << 8) | D3DVSDT_NONE;
	}

	if(normals)
	{
		*glw_state->drawArray++ = (16 << 8) | D3DVSDT_FLOAT3;
	}
	else
	{
		*glw_state->drawArray++ = ((glw_state->drawStride * 4) << 8) | D3DVSDT_NONE;
	}

	*glw_state->drawArray++ = (4 << 8) | D3DVSDT_D3DCOLOR;

	for(int i = 0; i < 5; i++)
	{
		*glw_state->drawArray++ = ((glw_state->drawStride * 4) << 8) | D3DVSDT_NONE;
	}

	if(tex0)
	{
		*glw_state->drawArray++ = (8 << 8) | D3DVSDT_FLOAT2;
	}
	else
	{
		*glw_state->drawArray++ = ((glw_state->drawStride * 4) << 8) | D3DVSDT_NONE;
	}

	if(tex1)
	{
		*glw_state->drawArray++ = (8 << 8) | D3DVSDT_FLOAT2;
	}
	else
	{
		*glw_state->drawArray++ = ((glw_state->drawStride * 4) << 8) | D3DVSDT_NONE;
	}

	for(i = 0; i < 5; i++)
	{
		*glw_state->drawArray++ = ((glw_state->drawStride * 4) << 8) | D3DVSDT_NONE;
	}

//	 Write the indicator to our vertex stream
#define CMD_VERTEXSTREAM_XYZ		0x1720
#define CMD_VERTEXSTREAM_NORMAL		0x1728
#define CMD_VERTEXSTREAM_COLOR		0x172c
#define CMD_VERTEXSTREAM_TEX0		0x1744
#define CMD_VERTEXSTREAM_TEX1		0x1748

	// On multiple passes, just write the address to the previous geometry
	*glw_state->drawArray++ = D3DPUSH_ENCODE(CMD_VERTEXSTREAM_XYZ, 1);
	if(tess.currentPass == 0)
	{
        *glw_state->drawArray++ = (DWORD)stream & 0x7fffffff;
		tess.pXyz = stream;
		stream += tess.numVertexes * 4;
	}
	else
	{
		*glw_state->drawArray++ = (DWORD)tess.pXyz & 0x7fffffff;
	}

	if(normals)
	{
		*glw_state->drawArray++ = D3DPUSH_ENCODE(CMD_VERTEXSTREAM_NORMAL, 1);
		if(!tess.pNormal)
		{
            *glw_state->drawArray++ = (DWORD)stream & 0x7fffffff;
			tess.pNormal = stream;
			stream += tess.numVertexes * 4;
		}
		else
		{
			*glw_state->drawArray++ = (DWORD)tess.pNormal & 0x7fffffff;
		}
	}

	*glw_state->drawArray++ = D3DPUSH_ENCODE(CMD_VERTEXSTREAM_COLOR, 1);
	*glw_state->drawArray++ = (DWORD)stream & 0x7fffffff;
	stream += tess.numVertexes;

	if(tex0)
	{
		*glw_state->drawArray++ = D3DPUSH_ENCODE(CMD_VERTEXSTREAM_TEX0, 1);
		*glw_state->drawArray++ = (DWORD)stream & 0x7fffffff;
		tess.pTex1 = stream;
		stream += tess.numVertexes * 2;
	}

	if(tex1)
	{
		*glw_state->drawArray++ = D3DPUSH_ENCODE(CMD_VERTEXSTREAM_TEX1, 1);
		*glw_state->drawArray++ = (DWORD)stream & 0x7fffffff;
		tess.pTex2 = stream;
	}

	// Send thru the index data
	PushIndices(count, (GLushort*)indices);

	// finish up the draw
	glw_state->inDrawBlock = false;

	DWORD* push = _terminateIndexPacket(glw_state->drawArray);

	glw_state->device->EndPush(push);
}

static void dllDrawPixels(GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels)
{
	assert(0);
}

static void dllEdgeFlag(GLboolean flag)
{
	assert(0);
}

static void dllEdgeFlagPointer(GLsizei stride, const GLvoid *pointer)
{
	assert(0);
}

static void dllEdgeFlagv(const GLboolean *flag)
{
	assert(0);
}

static void dllEnable(GLenum cap)
{
	setCap(cap, true);
}

static void dllEnableClientState(GLenum array)
{
	assert(!glw_state->inDrawBlock);
	setArrayState(array, true);
}

static void dllEnd(void)
{
	assert(glw_state->inDrawBlock);
	glw_state->inDrawBlock = false;
#ifdef _XBOX
	// on Xbox, just close the draw packet
	DWORD* push = _terminateDrawPacket(
		&glw_state->drawArray[glw_state->numVertices * 
		glw_state->drawStride]);
	
	glw_state->device->EndPush(push);
#else
	// on the PC, use DrawPrimitiveUp (a little slow)
	int num = 0;
	switch (glw_state->primitiveMode)
	{
	case D3DPT_POINTLIST: num = glw_state->numVertices; break;
	case D3DPT_LINELIST: num = glw_state->numVertices / 2; break;
	case D3DPT_LINESTRIP: num = glw_state->numVertices - 1; break;
	case D3DPT_TRIANGLELIST: num = glw_state->numVertices / 3; break;
	case D3DPT_TRIANGLESTRIP: num = glw_state->numVertices - 2; break;
	case D3DPT_TRIANGLEFAN: num = glw_state->numVertices - 2; break;
	}
	
	glw_state->device->DrawPrimitiveUP(
		glw_state->primitiveMode, num, 
		glw_state->drawArray, glw_state->drawStride * sizeof(DWORD));
#endif
}

#if YELLOW_MODE

static DWORD YellowModePixelShader	 = -1;
bool enableYellowMode	= false;
static void initYellowMode( void )
{
	if(!(CreatePixelShader("D:\\base\\media\\yellow.xpu", &YellowModePixelShader)))
		return;
}

static void renderYellowMode( void )
{
	if(!enableYellowMode || YellowModePixelShader == -1)
		return;

	if(cls.state == CA_CINEMATIC)
		return;

	if(!tr.screenImage)
		return;

	// bind tr.screenImage
	GL_Bind(tr.screenImage);
	GL_State(0);

	// copy backbuffer
	//qglCopyBackBufferToTexEXT(width, height, u1, v1, u2, v2);
	qglCopyBackBufferToTexEXT(512.0f, 256.0f, 2.0f, 2.0f, 640.0f, 480.0f);

	// set up the yellow mode shader
	DWORD oldShader;
	glw_state->device->GetPixelShader(&oldShader);
	glw_state->device->SetPixelShader(YellowModePixelShader);

	// draw our polygon
	qglBeginEXT(GL_QUADS,4,0,0,4,0);

	qglTexCoord2f( 0,0 );
	qglVertex2f( 0, 0 );

	qglTexCoord2f( 1,0 );
	qglVertex2f( 640, 0 );

	qglTexCoord2f( 1,1 );
	qglVertex2f( 640, 480);

	qglTexCoord2f( 0,1 );
	qglVertex2f( 0, 480);

	qglEnd ();

	glw_state->device->SetPixelShader(oldShader);
}

#endif // YELLOW_MODE

// EXTENSION: End drawing for a frame
bool connectSwapOverride = false;
static void dllEndFrame(void)
{
	assert(!glw_state->inDrawBlock);

	// the blend state can get reset by Present()...
	GLboolean blend = qglIsEnabled(GL_BLEND);
	
#if YELLOW_MODE
	if( !connectSwapOverride )
		renderYellowMode();
#endif // YELLOW_MODE

	glw_state->device->EndScene();
	
	qglViewport(0, 0, glConfig.vidWidth, glConfig.vidHeight);
	if( !connectSwapOverride )
		glw_state->device->Present(NULL, NULL, NULL, NULL);

	// restore the pre-Present state
	if (blend) qglEnable(GL_BLEND);
	else qglDisable(GL_BLEND);
}

// EXTENSION: End shadow draw mode
static void dllEndShadow(void)
{
	//Intentionally left blank
}

static void dllEndList(void)
{
	assert(0);
}

static void dllEvalCoord1d(GLdouble u)
{
	assert(0);
}

static void dllEvalCoord1dv(const GLdouble *u)
{
	assert(0);
}

static void dllEvalCoord1f(GLfloat u)
{
	assert(0);
}

static void dllEvalCoord1fv(const GLfloat *u)
{
	assert(0);
}

static void dllEvalCoord2d(GLdouble u, GLdouble v)
{
	assert(0);
}

static void dllEvalCoord2dv(const GLdouble *u)
{
	assert(0);
}

static void dllEvalCoord2f(GLfloat u, GLfloat v)
{
	assert(0);
}

static void dllEvalCoord2fv(const GLfloat *u)
{
	assert(0);
}

static void dllEvalMesh1(GLenum mode, GLint i1, GLint i2)
{
	assert(0);
}

static void dllEvalMesh2(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2)
{
	assert(0);
}

static void dllEvalPoint1(GLint i)
{
	assert(0);
}

static void dllEvalPoint2(GLint i, GLint j)
{
	assert(0);
}

static void dllFeedbackBuffer(GLsizei size, GLenum type, GLfloat *buffer)
{
	assert(0);
}

static void dllFinish(void)
{
#ifdef _XBOX
	glw_state->device->BlockUntilIdle();
#endif
}

static void dllFlush(void)
{
#ifdef _XBOX
	glw_state->device->BlockUntilIdle();
#endif
}

// EXTENSION: Draw the shadow
static void dllFlushShadow(void)
{
	//Intentionally left blank
}

static D3DFOGMODE _convertFogMode(GLint param)
{
	switch(param)
	{
	case GL_LINEAR: return D3DFOG_LINEAR; break;
	case GL_EXP: return D3DFOG_EXP; break;
	case GL_EXP2: return D3DFOG_EXP2; break;
	}

	return D3DFOG_NONE;
}

static void dllFogf(GLenum pname, GLfloat param)
{
	assert(pname == GL_FOG_DENSITY || pname == GL_FOG_START || pname == GL_FOG_END);	

	switch(pname)
	{
	case GL_FOG_DENSITY: glw_state->device->SetRenderState( D3DRS_FOGDENSITY, *(DWORD*)&param ); break;
	case GL_FOG_START: glw_state->device->SetRenderState( D3DRS_FOGSTART, *(DWORD*)&param ); break;
	case GL_FOG_END: glw_state->device->SetRenderState( D3DRS_FOGEND, *(DWORD*)&param ); break;
	}
}

static void dllFogfv(GLenum pname, const GLfloat *params)
{
	assert(pname == GL_FOG_COLOR);

	D3DCOLOR color = D3DCOLOR_ARGB(0x00, 
								  (int)(params[0] * 255.0f),
								  (int)(params[1] * 255.0f),
								  (int)(params[2] * 255.0f));

	glw_state->device->SetRenderState( D3DRS_FOGCOLOR, color );
}

static void dllFogi(GLenum pname, GLint param)
{
	assert(pname == GL_FOG_MODE);

	glw_state->device->SetRenderState( D3DRS_FOGTABLEMODE, _convertFogMode(param) );
}

static void dllFogiv(GLenum pname, const GLint *params)
{
	assert(0);
}

static void dllFrontFace(GLenum mode)
{
	assert(0);
}

static void dllFrustum(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)
{
	D3DXMATRIX m;
	D3DXMatrixPerspectiveOffCenterRH(&m, left, right, bottom, top, zNear, zFar);
	glw_state->matrixStack[glw_state->matrixMode]->MultMatrix(&m);
	glw_state->matricesDirty[glw_state->matrixMode] = true;
}

GLuint dllGenLists(GLsizei range)
{
	assert(0);
	return 0;
}

static void dllGenTextures(GLsizei n, GLuint *textures)
{
	for (int i = 0; i < n; ++i)
	{
		textures[i] = glw_state->textureBindNum++;
	}
}

// Implemented only the states we use.
template <typename T>
static void _getState(GLenum pname, T *params)
{
	switch (pname)
	{
	case GL_CULL_FACE: params[0] = (T)glw_state->cullEnable; break;
	case GL_MAX_TEXTURE_SIZE: params[0] = (T)512; break;
	case GL_MAX_ACTIVE_TEXTURES_ARB: params[0] = GLW_MAX_TEXTURE_STAGES; break;
	case GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT: params[0] = 4; break;
	default:
		assert(0);
		params[0] = (T)0;
		break;
	}
}

static void dllGetBooleanv(GLenum pname, GLboolean *params)
{
	_getState(pname, params);
}

static void dllGetClipPlane(GLenum plane, GLdouble *equation)
{
	assert(0);
}

static void dllGetDoublev(GLenum pname, GLdouble *params)
{
	_getState(pname, params);
}

GLenum dllGetError(void)
{
	return 0;
}

static void dllGetFloatv(GLenum pname, GLfloat *params)
{
	_getState(pname, params);
}

static void dllGetIntegerv(GLenum pname, GLint *params)
{
	_getState(pname, params);
}

static void dllGetLightfv(GLenum light, GLenum pname, GLfloat *params)
{
	assert(0);
}

static void dllGetLightiv(GLenum light, GLenum pname, GLint *params)
{
	assert(0);
}

static void dllGetMapdv(GLenum target, GLenum query, GLdouble *v)
{
	assert(0);
}

static void dllGetMapfv(GLenum target, GLenum query, GLfloat *v)
{
	assert(0);
}

static void dllGetMapiv(GLenum target, GLenum query, GLint *v)
{
	assert(0);
}

static void dllGetMaterialfv(GLenum face, GLenum pname, GLfloat *params)
{
	assert(0);
}

static void dllGetMaterialiv(GLenum face, GLenum pname, GLint *params)
{
	assert(0);
}

static void dllGetPixelMapfv(GLenum map, GLfloat *values)
{
	assert(0);
}

static void dllGetPixelMapuiv(GLenum map, GLuint *values)
{
	assert(0);
}

static void dllGetPixelMapusv(GLenum map, GLushort *values)
{
	assert(0);
}

static void dllGetPointerv(GLenum pname, GLvoid* *params)
{
	assert(0);
}

static void dllGetPolygonStipple(GLubyte *mask)
{
	assert(0);
}

const GLubyte * dllGetString(GLenum name)
{
	switch (name)
	{
	case GL_VENDOR: return (const unsigned char*)"Vicarious Visions";
	case GL_RENDERER: return (const unsigned char*)"Optimized DX8/OpenGL Layer";
	case GL_VERSION: return (const unsigned char*)"0.1";
	case GL_EXTENSIONS:
		return (const unsigned char*)
			"EXT_texture_env_add GL_ARB_multitexture EXT_texture_filter_anisotropic";
	default: return (const unsigned char*)"";
	}
}

static void dllGetTexEnvfv(GLenum target, GLenum pname, GLfloat *params)
{
	assert(0);
}

static void dllGetTexEnviv(GLenum target, GLenum pname, GLint *params)
{
	assert(0);
}

static void dllGetTexGendv(GLenum coord, GLenum pname, GLdouble *params)
{
	assert(0);
}

static void dllGetTexGenfv(GLenum coord, GLenum pname, GLfloat *params)
{
	assert(0);
}

static void dllGetTexGeniv(GLenum coord, GLenum pname, GLint *params)
{
	assert(0);
}

static void dllGetTexImage(GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels)
{
	assert(0);
}

static void dllGetTexLevelParameterfv(GLenum target, GLint level, GLenum pname, GLfloat *params)
{
	assert(0);
}

static void dllGetTexLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint *params)
{
	assert(0);
}

static void dllGetTexParameterfv(GLenum target, GLenum pname, GLfloat *params)
{
	assert(0);
}

static void dllGetTexParameteriv(GLenum target, GLenum pname, GLint *params)
{
	assert(0);
}

static void dllHint(GLenum target, GLenum mode)
{
	assert(0);
}

// Convert an triangle index array (indices) to a 
// triangle strip index array (dest) with primitive 
// length array.
static void buildStrips(GLuint* len, GLsizei* num_lens, GLushort* dest, GLsizei* num_indices, const GLushort* src)
{
	GLushort last[3];

	// prime the strip
	GLsizei cur_index = 0;
	dest[cur_index++] = src[0];
	dest[cur_index++] = src[1];
	dest[cur_index++] = src[2];
	GLuint cur_length = 3;
	GLsizei num_strips = 0;

	GLuint max_length = GLW_MAX_DRAW_PACKET_SIZE / glw_state->drawStride;
	
	last[0] = src[0];
	last[1] = src[1];
	last[2] = src[2];

	qboolean even = qfalse;

	for ( GLsizei i = 3; i < *num_indices; i += 3 )
	{
		// odd numbered triangle in potential strip
		if ( !even )
		{
			// check previous triangle to see if we're continuing a strip
			if ( ( src[i+0] == last[2] ) && ( src[i+1] == last[1] ) &&
				cur_length < max_length )
			{
				++cur_length;
				dest[cur_index++] = src[i+2];
				even = qtrue;
			}
			// otherwise we're done with this strip so finish it and start
			// a new one
			else
			{
				len[num_strips++] = cur_length;
				cur_length = 3;

				dest[cur_index++] = src[i+0];
				dest[cur_index++] = src[i+1];
				dest[cur_index++] = src[i+2];

				even = qfalse;
			}
		}
		else
		{
			// check previous triangle to see if we're continuing a strip
			if ( ( last[2] == src[i+1] ) && ( last[0] == src[i+0] ) &&
				cur_length < max_length )
			{
				++cur_length;
				dest[cur_index++] = src[i+2];
				even = qfalse;
			}
			// otherwise we're done with this strip so finish it and start
			// a new one
			else
			{
				len[num_strips++] = cur_length;
				cur_length = 3;

				dest[cur_index++] = src[i+0];
				dest[cur_index++] = src[i+1];
				dest[cur_index++] = src[i+2];

				even = qfalse;
			}
		}

		// cache the last three vertices
		last[0] = src[i+0];
		last[1] = src[i+1];
		last[2] = src[i+2];
	}

	len[num_strips++] = cur_length;
	*num_lens = num_strips;
	*num_indices = cur_index;

	assert(num_strips <= GLW_MAX_STRIPS);
}

#ifdef _XBOX
void renderObject_Light( int numIndexes, const glIndex_t *indexes )
{
	int i;

	// start the draw mode
	assert(!glw_state->inDrawBlock);

	glw_state->inDrawBlock = true;
	glw_state->primitiveMode = D3DPT_TRIANGLELIST;

	glw_state->drawStride = 14;

	glw_state->numIndices = 0;
	glw_state->totalIndices = numIndexes;
	glw_state->maxIndices = _getMaxIndices();

	glw_state->device->SetStreamSource(0, NULL, glw_state->drawStride * 4);	

	int vert_size;
	if(!tess.pNormal)
		vert_size = 8 * tess.numVertexes;
	else
		vert_size = 4 * tess.numVertexes;

	int index_size = numIndexes / 2;
	
	glw_state->device->BeginPush(vert_size + index_size + 60, &glw_state->drawArray);

	glw_state->drawArray = (DWORD*)*((DWORD*)glw_state->device);

	DWORD *jumpaddress = 0, *stream = 0;

	// Determine where the end of the vertex data is gonna be,
	// that's where we're going to jump to
	jumpaddress = (DWORD*)*((DWORD*)glw_state->device) + (vert_size + 1);

	// Write the jump address
	*glw_state->drawArray++ = ((DWORD)jumpaddress & 0x7fffffff) | 1;

	// Set up our own fake vertex buffer
	stream = glw_state->drawArray;

	if(!tess.pNormal)
	{
        memcpy(glw_state->drawArray, tess.normal, sizeof(vec4_t) * tess.numVertexes);
		glw_state->drawArray += tess.numVertexes * 4;
	}

	memcpy(glw_state->drawArray, tess.tangent, sizeof(vec4_t) * tess.numVertexes);
	glw_state->drawArray += tess.numVertexes * 4;

	// Write the vertex shader
#define CMD_STREAM_STRIDEANDTYPE0 0x1760
	*glw_state->drawArray++ = D3DPUSH_ENCODE(CMD_STREAM_STRIDEANDTYPE0, 16);

	// Position
	*glw_state->drawArray++ = (16 << 8)|D3DVSDT_FLOAT3;

	// Normal
	*glw_state->drawArray++ = (16 << 8) | D3DVSDT_FLOAT3;

	// Tex Coord
	*glw_state->drawArray++ = (8 << 8) | D3DVSDT_FLOAT2;

	// Tangent
	*glw_state->drawArray++ = (16 << 8) | D3DVSDT_FLOAT3;

	for(i = 0; i < 12; i++)
	{
		*glw_state->drawArray++ = ((glw_state->drawStride * 4) << 8) | D3DVSDT_NONE;
	}

	//	 Write the indicator to our vertex stream
	*glw_state->drawArray++ = D3DPUSH_ENCODE(0x1720, 1);
	*glw_state->drawArray++ = (DWORD)tess.pXyz & 0x7fffffff;

	*glw_state->drawArray++ = D3DPUSH_ENCODE(0x1724, 1);
	if(!tess.pNormal)
	{
        *glw_state->drawArray++ = (DWORD)stream & 0x7fffffff;
		stream += tess.numVertexes * 4;
	}
	else
		*glw_state->drawArray++ = (DWORD)tess.pNormal & 0x7fffffff;

	*glw_state->drawArray++ = D3DPUSH_ENCODE(0x1728, 1);
	*glw_state->drawArray++ = (DWORD)tess.pTex1 & 0x7fffffff;

	*glw_state->drawArray++ = D3DPUSH_ENCODE(0x172c, 1);
	*glw_state->drawArray++ = (DWORD)stream & 0x7fffffff;

	// Send thru the index data
	PushIndices(numIndexes, (GLushort*)indexes);

	// finish up the draw
	glw_state->inDrawBlock = false;

	DWORD* push = _terminateIndexPacket(glw_state->drawArray);

	glw_state->device->EndPush(push);
}


void renderObject_Shadow( int primType, int numIndexes, const unsigned short *indexes )
{
	int i;

	// start the draw mode
	assert(!glw_state->inDrawBlock);

	glw_state->inDrawBlock = true;
	glw_state->primitiveMode = (D3DPRIMITIVETYPE)primType;

	glw_state->drawStride = 5;

	glw_state->numIndices = 0;
	glw_state->totalIndices = numIndexes;
	glw_state->maxIndices = _getMaxIndices();

	glw_state->device->SetStreamSource(0, NULL, glw_state->drawStride * 4);	

	int vert_size = glw_state->drawStride * (tess.numVertexes * 2);
	int index_size = numIndexes / 2;
	
	glw_state->device->BeginPush(vert_size + index_size + 60, &glw_state->drawArray);

	glw_state->drawArray = (DWORD*)*((DWORD*)glw_state->device);

	DWORD *jumpaddress = 0, *stream = 0;

	if(!StencilShadower.pVerts)
	{
		// Determine where the end of the vertex data is gonna be,
		// that's where we're going to jump to
		jumpaddress = (DWORD*)*((DWORD*)glw_state->device) + (vert_size + 1);

		// Write the jump address
		*glw_state->drawArray++ = ((DWORD)jumpaddress & 0x7fffffff) | 1;

		// Set up our own fake vertex buffer
		stream = glw_state->drawArray;

        memcpy(glw_state->drawArray, tess.xyz, sizeof(vec4_t) * (tess.numVertexes * 2));
		glw_state->drawArray += (tess.numVertexes * 2) * 4;

		// Write the extrusion indicators
		memset(glw_state->drawArray, 0x0, sizeof(float) * tess.numVertexes);
		glw_state->drawArray += tess.numVertexes;
		memcpy(glw_state->drawArray, StencilShadower.m_extrusionIndicators, sizeof(float) * tess.numVertexes);
		glw_state->drawArray += tess.numVertexes;
	}
	
	// Write the vertex shader
#define CMD_STREAM_STRIDEANDTYPE0 0x1760
	*glw_state->drawArray++ = D3DPUSH_ENCODE(CMD_STREAM_STRIDEANDTYPE0, 16);

	// Position
	*glw_state->drawArray++ = (16 << 8)|D3DVSDT_FLOAT3;

	// Extrusion determinant
	*glw_state->drawArray++ = (4 << 8)|D3DVSDT_FLOAT1;

	for(i = 0; i < 14; i++)
	{
		*glw_state->drawArray++ = ((glw_state->drawStride * 4) << 8) | D3DVSDT_NONE;
	}

	//	 Write the indicator to our vertex stream
	*glw_state->drawArray++ = D3DPUSH_ENCODE(0x1720, 1);
	if(!StencilShadower.pVerts)
	{
        *glw_state->drawArray++ = (DWORD)stream & 0x7fffffff;
		StencilShadower.pVerts = stream;
		stream += (tess.numVertexes * 2) * 4;
	}
	else
	{
		*glw_state->drawArray++ = (DWORD)StencilShadower.pVerts & 0x7fffffff;
	}

	*glw_state->drawArray++ = D3DPUSH_ENCODE(0x1724, 1);
	if(!StencilShadower.pExtrusions)
	{
        *glw_state->drawArray++ = (DWORD)stream & 0x7fffffff;
		StencilShadower.pExtrusions = stream;
		stream += (tess.numVertexes * 2);
	}
	else
	{
		*glw_state->drawArray++ = (DWORD)StencilShadower.pExtrusions & 0x7fffffff;
	}

	// Send thru the index data
	PushIndices(numIndexes, (GLushort*)indexes);

	// finish up the draw
	glw_state->inDrawBlock = false;

	DWORD* push = _terminateIndexPacket(glw_state->drawArray);

	glw_state->device->EndPush(push);
}


void renderObject_Bump()
{
	// start the draw mode
	assert(!glw_state->inDrawBlock);

	glw_state->inDrawBlock = true;
	glw_state->primitiveMode = D3DPT_TRIANGLELIST;

	glw_state->drawStride = 16;

	glw_state->numIndices = 0;
	glw_state->totalIndices = tess.numIndexes;
	glw_state->maxIndices = _getMaxIndices();

	glw_state->device->SetStreamSource(0, NULL, glw_state->drawStride * 4);	

	int vert_size = glw_state->drawStride * tess.numVertexes;
	int index_size = tess.numIndexes / 2;

	glw_state->device->BeginPush(vert_size + index_size + 60, &glw_state->drawArray);

	glw_state->drawArray = (DWORD*)*((DWORD*)glw_state->device);

	DWORD *jumpaddress = 0, *stream = 0;

	// Determine where the end of the vertex data is gonna be,
	// that's where we're going to jump to
	jumpaddress = (DWORD*)*((DWORD*)glw_state->device) + (vert_size + 1);

	// Write the jump address
	*glw_state->drawArray++ = ((DWORD)jumpaddress & 0x7fffffff) | 1;

	// Set up our own fake vertex buffer
	stream = glw_state->drawArray;

	memcpy(glw_state->drawArray, tess.xyz, sizeof(vec4_t) * tess.numVertexes);
	glw_state->drawArray += tess.numVertexes * 4;

	memcpy(glw_state->drawArray, tess.normal, sizeof(vec4_t) * tess.numVertexes);
	glw_state->drawArray += tess.numVertexes * 4;

	memcpy(glw_state->drawArray, tess.svars.texcoords[0], sizeof(vec2_t) * tess.numVertexes);
	glw_state->drawArray += tess.numVertexes * 2;

	memcpy(glw_state->drawArray, tess.svars.texcoords[1], sizeof(vec2_t) * tess.numVertexes);
	glw_state->drawArray += tess.numVertexes * 2;

	memcpy(glw_state->drawArray, tess.tangent, sizeof(vec4_t) * tess.numVertexes);
	glw_state->drawArray += tess.numVertexes * 4;

	// Write the vertex shader
#define CMD_STREAM_STRIDEANDTYPE0 0x1760
	*glw_state->drawArray++ = D3DPUSH_ENCODE(CMD_STREAM_STRIDEANDTYPE0, 16);

	// Position
	*glw_state->drawArray++ = (16 << 8)|D3DVSDT_FLOAT3;

	// Normal
	*glw_state->drawArray++ = (16 << 8) | D3DVSDT_FLOAT3;

	// Tex Coord 0
	*glw_state->drawArray++ = (8 << 8) | D3DVSDT_FLOAT2;

	// Tex Coord 1
	*glw_state->drawArray++ = (8 << 8) | D3DVSDT_FLOAT2;

	// Tangent
	*glw_state->drawArray++ = (16 << 8) | D3DVSDT_FLOAT3;

	for(int i = 0; i < 11; i++)
	{
		*glw_state->drawArray++ = ((glw_state->drawStride * 4) << 8) | D3DVSDT_NONE;
	}

	//	 Write the indicator to our vertex stream
	*glw_state->drawArray++ = D3DPUSH_ENCODE(0x1720, 1);
	*glw_state->drawArray++ = (DWORD)stream & 0x7fffffff;
	tess.pXyz = stream;
	stream += tess.numVertexes * 4;

	*glw_state->drawArray++ = D3DPUSH_ENCODE(0x1724, 1);
	*glw_state->drawArray++ = (DWORD)stream & 0x7fffffff;
	tess.pNormal = stream;
	stream += tess.numVertexes * 4;

	*glw_state->drawArray++ = D3DPUSH_ENCODE(0x1728, 1);
	*glw_state->drawArray++ = (DWORD)stream & 0x7fffffff;
	stream += tess.numVertexes * 2;

	*glw_state->drawArray++ = D3DPUSH_ENCODE(0x172c, 1);
	*glw_state->drawArray++ = (DWORD)stream & 0x7fffffff;
	stream += tess.numVertexes * 2;

	*glw_state->drawArray++ = D3DPUSH_ENCODE(0x1730, 1);
	*glw_state->drawArray++ = (DWORD)stream & 0x7fffffff;
	stream += tess.numVertexes * 4;

	// Send thru the index data
	PushIndices(tess.numIndexes, (GLushort*)tess.indexes);

	// finish up the draw
	glw_state->inDrawBlock = false;

	DWORD* push = _terminateIndexPacket(glw_state->drawArray);

	glw_state->device->EndPush(push);
}

void renderObject_Env()
{
	// start the draw mode
	assert(!glw_state->inDrawBlock);

	glw_state->inDrawBlock = true;
	glw_state->primitiveMode = D3DPT_TRIANGLELIST;

	glw_state->drawStride = 9;
	_updateTextures();

	glw_state->numIndices = 0;
	glw_state->totalIndices = tess.numIndexes;
	glw_state->maxIndices = _getMaxIndices();

	glw_state->device->SetStreamSource(0, NULL, glw_state->drawStride * 4);	

	int vert_size = glw_state->drawStride * tess.numVertexes;
	int index_size = tess.numIndexes / 2;

	glw_state->device->BeginPush(vert_size + index_size + 60, &glw_state->drawArray);

	glw_state->drawArray = (DWORD*)*((DWORD*)glw_state->device);

	DWORD *jumpaddress = 0, *stream = 0;

	// Determine where the end of the vertex data is gonna be,
	// that's where we're going to jump to
	jumpaddress = (DWORD*)*((DWORD*)glw_state->device) + (vert_size + 1);

	// Write the jump address
	*glw_state->drawArray++ = ((DWORD)jumpaddress & 0x7fffffff) | 1;

	// Set up our own fake vertex buffer
	stream = glw_state->drawArray;

	memcpy(glw_state->drawArray, tess.xyz, sizeof(vec4_t) * tess.numVertexes);
	glw_state->drawArray += tess.numVertexes * 4;

	memcpy(glw_state->drawArray, tess.normal, sizeof(vec4_t) * tess.numVertexes);
	glw_state->drawArray += tess.numVertexes * 4; 

	memcpy(glw_state->drawArray, tess.svars.colors, sizeof(D3DCOLOR) * tess.numVertexes);
	glw_state->drawArray += tess.numVertexes;
 
	// Write the vertex shader
#define CMD_STREAM_STRIDEANDTYPE0 0x1760
	*glw_state->drawArray++ = D3DPUSH_ENCODE(CMD_STREAM_STRIDEANDTYPE0, 16);

	// Position
	*glw_state->drawArray++ = (16 << 8)|D3DVSDT_FLOAT3;

	// Normal
	*glw_state->drawArray++ = (16 << 8) | D3DVSDT_FLOAT3;

	// Color
	*glw_state->drawArray++ = (4 << 8) | D3DVSDT_D3DCOLOR;

	for(int i = 0; i < 13; i++)
	{
		*glw_state->drawArray++ = ((glw_state->drawStride * 4) << 8) | D3DVSDT_NONE;
	}

	//	 Write the indicator to our vertex stream
	*glw_state->drawArray++ = D3DPUSH_ENCODE(0x1720, 1);
	*glw_state->drawArray++ = (DWORD)stream & 0x7fffffff;
	tess.pXyz = stream;
	stream += tess.numVertexes * 4;

	*glw_state->drawArray++ = D3DPUSH_ENCODE(0x1724, 1);
	*glw_state->drawArray++ = (DWORD)stream & 0x7fffffff;
	tess.pNormal = stream;
	stream += tess.numVertexes * 4;

	*glw_state->drawArray++ = D3DPUSH_ENCODE(0x1728, 1);
	*glw_state->drawArray++ = (DWORD)stream & 0x7fffffff;
	stream += tess.numVertexes;

	// Send thru the index data
	PushIndices(tess.numIndexes, (GLushort*)tess.indexes);

	// finish up the draw
	glw_state->inDrawBlock = false;

	DWORD* push = _terminateIndexPacket(glw_state->drawArray);

	glw_state->device->EndPush(push);
}
#endif

// EXTENSION: Take an array of triangle indices and draw
// the appropriate triangle strips.  Virtually ALL geometry
// is drawn with this function so it better be fast.
static void dllIndexedTriToStrip(GLsizei count, const GLushort *indices)
{
//#ifndef _XBOX
#ifdef GLW_USE_TRI_STRIPS

	// update the render state
	_updateDrawStride(glw_state->normalArrayState,
		glw_state->texCoordArrayState[0] ? count : 0,
		glw_state->texCoordArrayState[1] ? count : 0);
	_updateShader(glw_state->normalArrayState,
		glw_state->texCoordArrayState[0],
		glw_state->texCoordArrayState[1]);
	_updateTextures();
	_updateMatrices();
	
	// convert triangles to strips -- guarantees that
	// no strip exceeds the max draw packet size
	if(tess.currentPass == 0)
	{
		buildStrips(glw_state->strip_lengths, 
			&glw_state->num_strip_lengths, glw_state->strip_dest, &count, indices);
	}

	// Yeah, its a hack, but I gotta do this so bumpmapping
	// doesnt go all crazy on the 'force speed' effect and 
	// 'disintegration' effect
	if(tess.shader && 
		tess.shader->isBumpMap && 
		(backEnd.currentEntity->e.renderfx & 
// VVFIXME : This is probably wrong. It looks like RF_ALPHA_FADE is renamed
// RF_RGB_TINT in MP. Substitute?
#ifndef _JK2MP
		(RF_ALPHA_FADE | RF_DISINTEGRATE1 | RF_DISINTEGRATE2)))
#else
		(RF_DISINTEGRATE1 | RF_DISINTEGRATE2)))
#endif
	{
		if(tess.currentPass != 2)
			return;
	}

#ifdef _XBOX
	glw_state->primitiveMode = D3DPT_TRIANGLESTRIP;

	// get the necessary draw function
	drawelemfunc_t func = _drawElementFuncTable[_getDrawFunc()];
	int stride = glw_state->drawStride;
	
	int index = 0;
	for (int l = 0; l < glw_state->num_strip_lengths; ++l)
	{
		int cur_len = glw_state->strip_lengths[l];
		
		// start a draw packet
		DWORD* push;
		glw_state->device->BeginPush(stride * cur_len + 5, &push);
		push = _restartDrawPacket(push, cur_len);
		
		// draw the geometry
		glw_state->drawArray = push;
		func(cur_len, &glw_state->strip_dest[index]);
		index += cur_len;
		
		// finish the draw packet
		push = _terminateDrawPacket(&push[stride * cur_len]);
		glw_state->device->EndPush(push);
	}
#else _XBOX
	// simplified render on the PC
	int index = 0;
	for (int l = 0; l < glw_state->num_strip_lengths; ++l)
	{
		dllDrawElements(GL_TRIANGLE_STRIP, glw_state->strip_lengths[l], 
			GL_UNSIGNED_SHORT, &glw_state->strip_dest[index]);
		index += glw_state->strip_lengths[l];
	}
#endif _XBOX

#else GLW_USE_TRI_STRIPS
	// just render simple triangles
	dllDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_SHORT, indices);
#endif GLW_USE_TRI_STRIPS
//#endif 
}

static void dllIndexMask(GLuint mask)
{
	assert(0);
}

static void dllIndexPointer(GLenum type, GLsizei stride, const GLvoid *pointer)
{
	assert(0);
}

static void dllIndexd(GLdouble c)
{
	assert(0);
}

static void dllIndexdv(const GLdouble *c)
{
	assert(0);
}

static void dllIndexf(GLfloat c)
{
	assert(0);
}

static void dllIndexfv(const GLfloat *c)
{
	assert(0);
}

static void dllIndexi(GLint c)
{
	assert(0);
}

static void dllIndexiv(const GLint *c)
{
	assert(0);
}

static void dllIndexs(GLshort c)
{
	assert(0);
}

static void dllIndexsv(const GLshort *c)
{
	assert(0);
}

static void dllIndexub(GLubyte c)
{
	assert(0);
}

static void dllIndexubv(const GLubyte *c)
{
	assert(0);
}

static void dllInitNames(void)
{
	assert(0);
}

static void dllInterleavedArrays(GLenum format, GLsizei stride, const GLvoid *pointer)
{
	assert(0);
}

GLboolean dllIsEnabled(GLenum cap)
{
	DWORD flag;
	switch (cap)
	{
	case GL_ALPHA_TEST: glw_state->device->GetRenderState(D3DRS_ALPHATESTENABLE, &flag); break;
	case GL_BLEND: glw_state->device->GetRenderState(D3DRS_ALPHABLENDENABLE, &flag); break;
	case GL_CULL_FACE: return glw_state->cullEnable;
	case GL_DEPTH_TEST: glw_state->device->GetRenderState(D3DRS_ZENABLE, &flag); break;
	case GL_FOG: glw_state->device->GetRenderState(D3DRS_FOGENABLE, &flag); break;
	case GL_LIGHTING: glw_state->device->GetRenderState(D3DRS_LIGHTING, &flag); break;
#ifdef _XBOX
	case GL_POLYGON_OFFSET_FILL: glw_state->device->GetRenderState(D3DRS_SOLIDOFFSETENABLE, &flag); break;
#else
	case GL_POLYGON_OFFSET_FILL: return FALSE;
#endif
	case GL_SCISSOR_TEST: return glw_state->scissorEnable;
	case GL_STENCIL_TEST: glw_state->device->GetRenderState(D3DRS_STENCILENABLE, &flag); break;
	case GL_TEXTURE_2D: return glw_state->textureStageEnable[glw_state->serverTU];
	default: return FALSE;
	}
	return flag;
}

GLboolean dllIsList(GLuint lnum)
{
	assert(0);
	return 1;
}

GLboolean dllIsTexture(GLuint texture)
{
	assert(0);
	return 1;
}

static void dllLightModelf(GLenum pname, GLfloat param)
{
	assert(0);
}

static void dllLightModelfv(GLenum pname, const GLfloat *params)
{
	assert(0);
}

static void dllLightModeli(GLenum pname, GLint param)
{
	assert(0);
}

static void dllLightModeliv(GLenum pname, const GLint *params)
{
	assert(0);
}

static void dllLightf(GLenum light, GLenum pname, GLfloat param)
{
	assert(0);
}

static void dllLightfv(GLenum light, GLenum pname, const GLfloat *params)
{
	switch(pname)
	{
	case GL_AMBIENT:
		{
			glw_state->dirLight.Ambient.r = params[0] / 255.0f;
			glw_state->dirLight.Ambient.g = params[1] / 255.0f;
			glw_state->dirLight.Ambient.b = params[2] / 255.0f;
		}
		break;

	case GL_DIFFUSE:
		{
			glw_state->dirLight.Diffuse.r = params[0] / 255.0f;
			glw_state->dirLight.Diffuse.g = params[1] / 255.0f;
			glw_state->dirLight.Diffuse.b = params[2] / 255.0f;
		}
		break;

	case GL_SPECULAR:
		{
			glw_state->dirLight.Specular.r = params[0] / 255.0f;
			glw_state->dirLight.Specular.g = params[1] / 255.0f;
			glw_state->dirLight.Specular.b = params[2] / 255.0f;
		}
		break;
	case GL_POSITION:
		{
			glw_state->dirLight.Position.x = params[0];
			glw_state->dirLight.Position.y = params[1];
			glw_state->dirLight.Position.z = params[2];
		}
		break;

	case GL_SPOT_DIRECTION:
		{
			glw_state->dirLight.Direction.x = -params[0];
			glw_state->dirLight.Direction.y = -params[1];
			glw_state->dirLight.Direction.z = -params[2];
		}
		break;

	default:
		assert(0);
		break;
	}

	glw_state->device->SetLight(light, &glw_state->dirLight);
	glw_state->device->LightEnable(light, TRUE);
}

static void dllLighti(GLenum light, GLenum pname, GLint param)
{
	assert(0);
}

static void dllLightiv(GLenum light, GLenum pname, const GLint *params)
{
	assert(0);
}

static void dllLineStipple(GLint factor, GLushort pattern)
{
	assert(0);
}

static void dllLineWidth(GLfloat width)
{
//	assert(0);
}

static void dllListBase(GLuint base)
{
	assert(0);
}

static void dllLoadIdentity(void)
{
	glw_state->matrixStack[glw_state->matrixMode]->LoadIdentity();
	glw_state->matricesDirty[glw_state->matrixMode] = true;
}

static void dllLoadMatrixd(const GLdouble *m)
{
	assert(0);
}

static void dllLoadMatrixf(const GLfloat *m)
{
	glw_state->matrixStack[glw_state->matrixMode]->LoadMatrix((D3DXMATRIX*)m);
	glw_state->matricesDirty[glw_state->matrixMode] = true;
}

static void dllLoadName(GLuint name)
{
	assert(0);
}

static void dllLogicOp(GLenum opcode)
{
	assert(0);
}

static void dllMap1d(GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points)
{
	assert(0);
}

static void dllMap1f(GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points)
{
	assert(0);
}

static void dllMap2d(GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points)
{
	assert(0);
}

static void dllMap2f(GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points)
{
	assert(0);
}

static void dllMapGrid1d(GLint un, GLdouble u1, GLdouble u2)
{
	assert(0);
}

static void dllMapGrid1f(GLint un, GLfloat u1, GLfloat u2)
{
	assert(0);
}

static void dllMapGrid2d(GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2)
{
	assert(0);
}

static void dllMapGrid2f(GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2)
{
	assert(0);
}

static void dllMaterialf(GLenum face, GLenum pname, GLfloat param)
{
	assert(0);
}

static void dllMaterialfv(GLenum face, GLenum pname, const GLfloat *params)
{
	switch(pname)
	{
	case GL_AMBIENT:
		glw_state->mtrl.Ambient.r = params[0] / 255.0f;
		glw_state->mtrl.Ambient.g = params[1] / 255.0f;
		glw_state->mtrl.Ambient.b = params[2] / 255.0f;
		glw_state->mtrl.Ambient.a = params[3] / 255.0f;
		break;

	case GL_DIFFUSE:
		glw_state->mtrl.Diffuse.r = params[0] / 255.0f;
		glw_state->mtrl.Diffuse.g = params[1] / 255.0f;
		glw_state->mtrl.Diffuse.b = params[2] / 255.0f;
		glw_state->mtrl.Diffuse.a = params[3] / 255.0f;
		break;

	case GL_SPECULAR:
		glw_state->mtrl.Specular.r = params[0] / 255.0f;
		glw_state->mtrl.Specular.g = params[1] / 255.0f;
		glw_state->mtrl.Specular.b = params[2] / 255.0f;
		glw_state->mtrl.Specular.a = params[3] / 255.0f;
		break;

	case GL_EMISSION:
		glw_state->mtrl.Emissive.r = params[0] / 255.0f;
		glw_state->mtrl.Emissive.g = params[1] / 255.0f;
		glw_state->mtrl.Emissive.b = params[2] / 255.0f;
		glw_state->mtrl.Emissive.a = params[3] / 255.0f;
		break;

	default:
		assert(0);
		break;
	}

	glw_state->device->SetMaterial(&glw_state->mtrl);
}

static void dllMateriali(GLenum face, GLenum pname, GLint param)
{
	assert(0);
}

static void dllMaterialiv(GLenum face, GLenum pname, const GLint *params)
{
	assert(0);
}

static void dllMatrixMode(GLenum mode)
{
	switch (mode)
	{
	case GL_MODELVIEW: glw_state->matrixMode = glwstate_t::MatrixMode_Model; break;
	case GL_PROJECTION: glw_state->matrixMode = glwstate_t::MatrixMode_Projection; break;
#ifdef _XBOX
	case GL_TEXTURE0: glw_state->matrixMode = glwstate_t::MatrixMode_Texture0; break;
	case GL_TEXTURE1: glw_state->matrixMode = glwstate_t::MatrixMode_Texture1; break;
#endif
	default: assert(false); break;
	}
}

static void dllMultMatrixd(const GLdouble *m)
{
	assert(0);
}

static void dllMultMatrixf(const GLfloat *m)
{
	glw_state->matrixStack[glw_state->matrixMode]->MultMatrixLocal((D3DXMATRIX*)m);
	glw_state->matricesDirty[glw_state->matrixMode] = true;
}

static void dllNewList(GLuint lnum, GLenum mode)
{
	assert(0);
}

static void setNormal(float x, float y, float z)
{
	assert(glw_state->inDrawBlock);

	_handleDrawOverflow();
	
	DWORD* push = &glw_state->drawArray[glw_state->numVertices * glw_state->drawStride + 4];
	push[0] = *((DWORD*)&x);
	push[1] = *((DWORD*)&y);
	push[2] = *((DWORD*)&z);
	push[3] = glw_state->currentColor;
}
static void dllNormal3b(GLbyte nx, GLbyte ny, GLbyte nz)
{
	assert(0);
}

static void dllNormal3bv(const GLbyte *v)
{
	assert(0);
}

static void dllNormal3d(GLdouble nx, GLdouble ny, GLdouble nz)
{
	assert(0);
}

static void dllNormal3dv(const GLdouble *v)
{
	assert(0);
}

static void dllNormal3f(GLfloat nx, GLfloat ny, GLfloat nz)
{
	setNormal(nx, ny, nz);
}

static void dllNormal3fv(const GLfloat *v)
{
	setNormal(v[0], v[1], v[2]);
}

static void dllNormal3i(GLint nx, GLint ny, GLint nz)
{
	assert(0);
}

static void dllNormal3iv(const GLint *v)
{
	assert(0);
}

static void dllNormal3s(GLshort nx, GLshort ny, GLshort nz)
{
	assert(0);
}

static void dllNormal3sv(const GLshort *v)
{
	assert(0);
}

static void dllNormalPointer(GLenum type, GLsizei stride, const GLvoid *pointer)
{
	assert(type == GL_FLOAT);
	
	stride = (stride == 0) ? (sizeof(GLfloat) * 3) : stride;

	glw_state->normalPointer = pointer;
	glw_state->normalStride = stride;
}

static void dllOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)
{
	D3DXMATRIX m;
	D3DXMatrixOrthoOffCenterRH(&m, left, right, top, bottom, zNear, zFar);
	glw_state->matrixStack[glw_state->matrixMode]->MultMatrix(&m);
	glw_state->matricesDirty[glw_state->matrixMode] = true;
}

static void dllPassThrough(GLfloat token)
{
	assert(0);
}

static void dllPixelMapfv(GLenum map, GLsizei mapsize, const GLfloat *values)
{
	assert(0);
}

static void dllPixelMapuiv(GLenum map, GLsizei mapsize, const GLuint *values)
{
	assert(0);
}

static void dllPixelMapusv(GLenum map, GLsizei mapsize, const GLushort *values)
{
	assert(0);
}

static void dllPixelStoref(GLenum pname, GLfloat param)
{
	assert(0);
}

static void dllPixelStorei(GLenum pname, GLint param)
{
	assert(0);
}

static void dllPixelTransferf(GLenum pname, GLfloat param)
{
	assert(0);
}

static void dllPixelTransferi(GLenum pname, GLint param)
{
	assert(0);
}

static void dllPixelZoom(GLfloat xfactor, GLfloat yfactor)
{
	assert(0);
}

static void dllPointSize(GLfloat size)
{
	glw_state->device->SetRenderState(D3DRS_POINTSCALEENABLE, TRUE);
	glw_state->device->SetRenderState(D3DRS_POINTSIZE, *((DWORD*)&size));
}

static void dllPolygonMode(GLenum face, GLenum mode)
{
	D3DFILLMODE m;
	switch (mode)
	{
	case GL_POINT: m = D3DFILL_POINT; break;
	case GL_LINE: m = D3DFILL_WIREFRAME; break;
	case GL_FILL: m = D3DFILL_SOLID; break;
	default: assert(0); break;
	}

	switch (face)
	{
	case GL_FRONT:
		glw_state->device->SetRenderState(D3DRS_FILLMODE, m);
		break;
	case GL_BACK:
#ifdef _XBOX
		glw_state->device->SetRenderState(D3DRS_BACKFILLMODE, m);
#endif
		break;
	case GL_FRONT_AND_BACK:
		glw_state->device->SetRenderState(D3DRS_FILLMODE, m);
#ifdef _XBOX
		glw_state->device->SetRenderState(D3DRS_BACKFILLMODE, m);
#endif
		break;
	}
}

static void dllPolygonOffset(GLfloat factor, GLfloat units)
{
#ifdef _XBOX
	glw_state->device->SetRenderState(D3DRS_POLYGONOFFSETZOFFSET, *((DWORD*)&factor));
	glw_state->device->SetRenderState(D3DRS_POLYGONOFFSETZSLOPESCALE, *((DWORD*)&units));
#endif
}

static void dllPolygonStipple(const GLubyte *mask)
{
	assert(0);
}

static void dllPopAttrib(void)
{
	assert(0);
}

static void dllPopClientAttrib(void)
{
	assert(0);
}

static void dllPopMatrix(void)
{
	glw_state->matrixStack[glw_state->matrixMode]->Pop();
	glw_state->matricesDirty[glw_state->matrixMode] = true;
}

static void dllPopName(void)
{
	assert(0);
}

static void dllPrioritizeTextures(GLsizei n, const GLuint *textures, const GLclampf *priorities)
{
	assert(0);
}

static void dllPushAttrib(GLbitfield mask)
{
	assert(0);
}

static void dllPushClientAttrib(GLbitfield mask)
{
	assert(0);
}

static void dllPushMatrix(void)
{
	glw_state->matrixStack[glw_state->matrixMode]->Push();
	glw_state->matricesDirty[glw_state->matrixMode] = true;
}

static void dllPushName(GLuint name)
{
	assert(0);
}

static void dllRasterPos2d(GLdouble x, GLdouble y)
{
	assert(0);
}

static void dllRasterPos2dv(const GLdouble *v)
{
	assert(0);
}

static void dllRasterPos2f(GLfloat x, GLfloat y)
{
	assert(0);
}

static void dllRasterPos2fv(const GLfloat *v)
{
	assert(0);
}

static void dllRasterPos2i(GLint x, GLint y)
{
	assert(0);
}

static void dllRasterPos2iv(const GLint *v)
{
	assert(0);
}

static void dllRasterPos2s(GLshort x, GLshort y)
{
	assert(0);
}

static void dllRasterPos2sv(const GLshort *v)
{
	assert(0);
}

static void dllRasterPos3d(GLdouble x, GLdouble y, GLdouble z)
{
	assert(0);
}

static void dllRasterPos3dv(const GLdouble *v)
{
	assert(0);
}

static void dllRasterPos3f(GLfloat x, GLfloat y, GLfloat z)
{
	assert(0);
}

static void dllRasterPos3fv(const GLfloat *v)
{
	assert(0);
}

static void dllRasterPos3i(GLint x, GLint y, GLint z)
{
	assert(0);
}

static void dllRasterPos3iv(const GLint *v)
{
	assert(0);
}

static void dllRasterPos3s(GLshort x, GLshort y, GLshort z)
{
	assert(0);
}

static void dllRasterPos3sv(const GLshort *v)
{
	assert(0);
}

static void dllRasterPos4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
	assert(0);
}

static void dllRasterPos4dv(const GLdouble *v)
{
	assert(0);
}

static void dllRasterPos4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
	assert(0);
}

static void dllRasterPos4fv(const GLfloat *v)
{
	assert(0);
}

static void dllRasterPos4i(GLint x, GLint y, GLint z, GLint w)
{
	assert(0);
}

static void dllRasterPos4iv(const GLint *v)
{
	assert(0);
}

static void dllRasterPos4s(GLshort x, GLshort y, GLshort z, GLshort w)
{
	assert(0);
}

static void dllRasterPos4sv(const GLshort *v)
{
	assert(0);
}

static void dllReadBuffer(GLenum mode)
{
	assert(0);
}

//static void dllReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLsizei twidth, GLsizei theight, GLvoid *pixels)
static void dllReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels)
{
	assert( 0 );
	return;

}

/**********
dllCopyBackBufferToTex
Does a direct copy of the backbuffer to the current texture. The current texture
must be linear, and it must be 640 x 480 in size. If a more complex copy is
needed, use dllCopyBackBufferToTexEXT.
**********/
static void dllCopyBackBufferToTex()
{
	glwstate_t::TextureInfo* info = _getCurrentTexture(glw_state->serverTU);
	if (info == NULL) return;

	LPDIRECT3DSURFACE8 surf;
	LPDIRECT3DSURFACE8 backbuffer;

	info->mipmap->GetSurfaceLevel(0, &surf);
	glw_state->device->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &backbuffer);

	glw_state->device->CopyRects(backbuffer, NULL, 0, surf, NULL);

	surf->Release();
	backbuffer->Release();
}

/**********
dllCopyBackBufferToTexEXT
Copies a portion of the backbuffer to a texture
If the destination is a DXT1 texture, then the buffer will be compressed
width	- width of the backbuffer polygon rendered to the destination texture
height	- height of the backbuffer polygon rendered to the destination texture
u,v		- describes the potion of the backbuffer to be copied in screen coords

The active texture (that we're replacing) NEEDS to already have enough space!
**********/
static void dllCopyBackBufferToTexEXT(float width, float height, float u1, float v1, float u2, float v2)
{
	glwstate_t::TextureInfo* info = _getCurrentTexture(glw_state->serverTU);
	if (info == NULL) return;

	struct QUAD { D3DXVECTOR4 p; FLOAT tu,tv;} q[4];
	q[0].p	= D3DXVECTOR4( 0.0f, 0.0f, 1.0f, 1.0f );
	q[0].tu = u1;		q[0].tv = v1;
	q[1].p	= D3DXVECTOR4( width, 0.0f, 1.0f, 1.0f );
	q[1].tu = u2;	q[1].tv = v1;
	q[2].p	= D3DXVECTOR4( 0.0f, height , 1.0f, 1.0f );
	q[2].tu = u1;		q[2].tv = v2;
	q[3].p	= D3DXVECTOR4( width, height, 1.0f, 1.0f );
	q[3].tu = u2;	q[3].tv = v2;


	LPDIRECT3DSURFACE8	pSurface;
	LPDIRECT3DSURFACE8	pBackBuffer;
	LPDIRECT3DSURFACE8	pStencilBuffer;
	D3DSURFACE_DESC		desc;
	D3DTexture*			pRenderTex;
	int					w	= 0;
	int					h	= 0;

	DWORD srcblend, destblend, alphablend, alphatest, zwrite, zenable, vShader, pShader;
	DWORD colorop, colorarg1, addressu, addressv, minfilter, magfilter, colorwriteenable;

	// save the current state
	glw_state->device->GetRenderState( D3DRS_SRCBLEND, &srcblend );
	glw_state->device->GetRenderState( D3DRS_DESTBLEND, &destblend );
	glw_state->device->GetRenderState( D3DRS_ALPHABLENDENABLE, &alphablend );
	glw_state->device->GetRenderState( D3DRS_ALPHATESTENABLE, &alphatest );
	glw_state->device->GetRenderState( D3DRS_ZWRITEENABLE, &zwrite );
	glw_state->device->GetRenderState( D3DRS_ZENABLE, &zenable );
	glw_state->device->GetRenderState( D3DRS_COLORWRITEENABLE, &colorwriteenable);
	glw_state->device->GetVertexShader( &vShader );
	glw_state->device->GetPixelShader( &pShader );
	// This function no longer makes ANY attempt to restore texture stages
	glw_state->device->SetTexture(0, NULL);
	glw_state->device->SetTexture(1, NULL);
	glw_state->device->SetTexture(2, NULL);
	glw_state->device->SetTexture(3, NULL);
	glw_state->device->GetTextureStageState(0, D3DTSS_COLOROP, &colorop);
	glw_state->device->GetTextureStageState(0, D3DTSS_COLORARG1, &colorarg1);
	glw_state->device->GetTextureStageState(0, D3DTSS_ADDRESSU, &addressu);
	glw_state->device->GetTextureStageState(0, D3DTSS_ADDRESSV, &addressv);
	glw_state->device->GetTextureStageState(0, D3DTSS_MINFILTER, &minfilter);
	glw_state->device->GetTextureStageState(0, D3DTSS_MAGFILTER, &magfilter);

	// get the buffers
	glw_state->device->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer);
	glw_state->device->GetDepthStencilSurface(&pStencilBuffer);

	// get a surface desc
	info->mipmap->GetLevelDesc(0, &desc);

	// Check to see if the texture needs to be resized
	if( desc.Width != width || desc.Height != height)
	{
		// We don't actually destroy/create the texture anymore.
		// We just adjust the texture header. Thus, make sure the texture
		// is big enough when you make it the first time!

		// Replacing this with a while( IsBusy() ) loop makes it hang sometimes
		// But I can't figure out who the fuck has a lock on the texture, or
		// and it doesn't seem to cause any problems. (ie: using push on cloaked guys)
		info->mipmap->BlockUntilNotBusy();

		// Change the texture size
		XGSetTextureHeader( width,
							height,
							1,
							0,
							desc.Format,
							0,
							info->mipmap,
							0,
							0 );

		// Re-register the data:
		info->mipmap->Register( info->data );
	}

	// check to see if we want a compressed output texture
	if( desc.Format == D3DFMT_DXT1)
	{

		w	= desc.Width;
		h	= desc.Height;

		// create a new texture to use as a render target
		_d3d_check(glw_state->device->CreateTexture( w,
													 h,
													 1,
													 0,
													 D3DFMT_LIN_X8R8G8B8,
													 0,
													 &pRenderTex ), "CreateTexture");
	}
	else
	{
		pRenderTex = info->mipmap;
		
	}

	// make our current surface a render target
	pRenderTex->GetSurfaceLevel(0, &pSurface);
	glw_state->device->SetRenderTarget( pSurface, NULL );
	
	// set texture 0 to the back buffer data
	glw_state->device->SetTexture(0,(LPDIRECT3DTEXTURE8)pBackBuffer);

	// set the texture 0 state
	glw_state->device->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1 );
    glw_state->device->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
    glw_state->device->SetTextureStageState( 0, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP );
    glw_state->device->SetTextureStageState( 0, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP );
    glw_state->device->SetTextureStageState( 0, D3DTSS_MINFILTER, D3DTEXF_LINEAR );
    glw_state->device->SetTextureStageState( 0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR );
	
	// set the render state
	glw_state->device->SetRenderState( D3DRS_ZENABLE,         FALSE );
    glw_state->device->SetRenderState( D3DRS_ALPHATESTENABLE, FALSE );
	glw_state->device->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE);
	glw_state->device->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
	glw_state->device->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_SRCALPHA | D3DBLEND_INVSRCALPHA );
	glw_state->device->SetRenderState( D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALL);

	// set our vertex shader and draw the backbuffer to the texture
	glw_state->device->SetVertexShader( D3DFVF_XYZRHW|D3DFVF_TEX1 );
	glw_state->device->SetPixelShader( NULL );
	glw_state->device->Clear(NULL,NULL,D3DCLEAR_TARGET,D3DCOLOR_COLORVALUE(1.0f, 1.0f, 1.0f, 1.0f), 1.0f, 0);
	glw_state->device->DrawPrimitiveUP( D3DPT_QUADSTRIP, 1, q, sizeof(QUAD) );

	// now that everything is rendered, check again to see
	// if we want a compressed texture
	if( desc.Format == D3DFMT_DXT1)
	{
		LPDIRECT3DTEXTURE8	pSrcTex;
		LPDIRECT3DTEXTURE8	pDstTex;
		D3DLOCKED_RECT		srcLock;
		D3DLOCKED_RECT		dstLock;

		pSrcTex	= pRenderTex;
		pDstTex	= info->mipmap;

		// lock our textures
		pSrcTex->LockRect(0, &srcLock, NULL, 0);
		pDstTex->LockRect(0, &dstLock, NULL, 0);

		// compress the texture
		XGCompressRect(	dstLock.pBits,
						D3DFMT_DXT1,
						dstLock.Pitch,
						w,
						h,
						srcLock.pBits,
						D3DFMT_LIN_X8R8G8B8,
						srcLock.Pitch,
						1,
						0 );

		// unlock
		pSrcTex->UnlockRect(0);
		pDstTex->UnlockRect(0);

		// release the render texture
		pRenderTex->Release();
	}

	// return our state
	glw_state->device->SetRenderState( D3DRS_SRCBLEND, srcblend );
	glw_state->device->SetRenderState( D3DRS_DESTBLEND, destblend );
	glw_state->device->SetRenderState( D3DRS_ALPHABLENDENABLE, alphablend );
	glw_state->device->SetRenderState( D3DRS_ALPHATESTENABLE, alphatest );
	glw_state->device->SetRenderState( D3DRS_ZWRITEENABLE, zwrite );
	glw_state->device->SetRenderState( D3DRS_ZENABLE, zenable );
	glw_state->device->SetRenderState( D3DRS_COLORWRITEENABLE, colorwriteenable);

	// Clear stage zero again. We're not being nice.
	glw_state->device->SetTexture(0, NULL);
	glw_state->textureStageDirty[0] = true;
	glw_state->textureStageDirty[1] = true;

	glw_state->device->SetTextureStageState(0, D3DTSS_COLOROP, colorop);
	glw_state->device->SetTextureStageState(0, D3DTSS_COLORARG1, colorarg1);
	glw_state->device->SetTextureStageState(0, D3DTSS_ADDRESSU, addressu);
	glw_state->device->SetTextureStageState(0, D3DTSS_ADDRESSV, addressv);
	glw_state->device->SetTextureStageState(0, D3DTSS_MINFILTER, minfilter);
	glw_state->device->SetTextureStageState(0, D3DTSS_MAGFILTER, magfilter);

	glw_state->device->SetVertexShader( vShader );
	glw_state->device->SetPixelShader( pShader );

	glw_state->device->SetRenderTarget( pBackBuffer, pStencilBuffer );

	pSurface->Release();
	pBackBuffer->Release();
	pStencilBuffer->Release();
}

static void dllRectd(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2)
{
	assert(0);
}

static void dllRectdv(const GLdouble *v1, const GLdouble *v2)
{
	assert(0);
}

static void dllRectf(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)
{
	assert(0);
}

static void dllRectfv(const GLfloat *v1, const GLfloat *v2)
{
	assert(0);
}

static void dllRecti(GLint x1, GLint y1, GLint x2, GLint y2)
{
	assert(0);
}

static void dllRectiv(const GLint *v1, const GLint *v2)
{
	assert(0);
}

static void dllRects(GLshort x1, GLshort y1, GLshort x2, GLshort y2)
{
	assert(0);
}

static void dllRectsv(const GLshort *v1, const GLshort *v2)
{
	assert(0);
}

GLint dllRenderMode(GLenum mode)
{
	assert(0);
	return 0;
}

static void dllRotated(GLdouble angle, GLdouble x, GLdouble y, GLdouble z)
{
	assert(0);
}

static void dllRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
	D3DXVECTOR3 v(x, y, z);
	glw_state->matrixStack[glw_state->matrixMode]->RotateAxisLocal(&v, angle);
	glw_state->matricesDirty[glw_state->matrixMode] = true;
}

static void dllScaled(GLdouble x, GLdouble y, GLdouble z)
{
	assert(0);
}

static void dllScalef(GLfloat x, GLfloat y, GLfloat z)
{
	glw_state->matrixStack[glw_state->matrixMode]->Scale(x, y, z);
	glw_state->matricesDirty[glw_state->matrixMode] = true;
}

static void dllScissor(GLint x, GLint y, GLsizei width, GLsizei height)
{
#ifdef _XBOX
	_fixupScreenCoords(x, y, width, height);

	glw_state->scissorBox.x1 = x;
	glw_state->scissorBox.y1 = y;
	glw_state->scissorBox.x2 = x + width;
	glw_state->scissorBox.y2 = y + height;
	
	if (glw_state->scissorEnable)
	{
		glw_state->device->SetScissors(1, FALSE, &glw_state->scissorBox);
	}
#endif
}

static void dllSelectBuffer(GLsizei size, GLuint *buffer)
{
	assert(0);
}

static void dllShadeModel(GLenum mode)
{
	D3DSHADEMODE m;
	switch (mode)
	{
	case GL_FLAT: m = D3DSHADE_FLAT; break;
	case GL_SMOOTH: default: m = D3DSHADE_GOURAUD; break;
	}
	
	glw_state->device->SetRenderState(D3DRS_SHADEMODE, m);
}

static void dllStencilFunc(GLenum func, GLint ref, GLuint mask)
{
	D3DCMPFUNC f = _convertCompare(func);

	glw_state->device->SetRenderState(D3DRS_STENCILFUNC, f);
	glw_state->device->SetRenderState(D3DRS_STENCILREF, ref);
	glw_state->device->SetRenderState(D3DRS_STENCILMASK, mask);
}

static void dllStencilMask(GLuint mask)
{
	glw_state->device->SetRenderState(D3DRS_STENCILWRITEMASK, mask);
}

static D3DSTENCILOP _convertStencilOp(GLenum op)
{
	switch (op)
	{
	default: case GL_KEEP: return D3DSTENCILOP_KEEP;
	case GL_ZERO: return D3DSTENCILOP_ZERO;
	case GL_REPLACE: return D3DSTENCILOP_REPLACE;
	case GL_INCR: return D3DSTENCILOP_INCR;
	case GL_DECR: return D3DSTENCILOP_DECR;
	case GL_INVERT: return D3DSTENCILOP_INVERT;
	}
}

static void dllStencilOp(GLenum fail, GLenum zfail, GLenum zpass)
{
	D3DSTENCILOP f = _convertStencilOp(fail);
	D3DSTENCILOP zf = _convertStencilOp(zfail);
	D3DSTENCILOP zp = _convertStencilOp(zpass);

	glw_state->device->SetRenderState(D3DRS_STENCILFAIL, f);
	glw_state->device->SetRenderState(D3DRS_STENCILZFAIL, zf);
	glw_state->device->SetRenderState(D3DRS_STENCILPASS, zp);
}

static void dllTexCoord1d(GLdouble s)
{
	assert(0);
}

static void dllTexCoord1dv(const GLdouble *v)
{
	assert(0);
}

static void dllTexCoord1f(GLfloat s)
{
	assert(0);
}

static void dllTexCoord1fv(const GLfloat *v)
{
	assert(0);
}

static void dllTexCoord1i(GLint s)
{
	assert(0);
}

static void dllTexCoord1iv(const GLint *v)
{
	assert(0);
}

static void dllTexCoord1s(GLshort s)
{
	assert(0);
}

static void dllTexCoord1sv(const GLshort *v)
{
	assert(0);
}

static void setTexCoord(float s, float t)
{
	assert(glw_state->inDrawBlock);

	_handleDrawOverflow();

	int off = 0;
	if(glw_state->normalArrayState)
		off = 3;
	
	DWORD* push = &glw_state->drawArray[
		glw_state->numVertices * glw_state->drawStride + 
		4 + off + glw_state->serverTU * 2];
	
	*push++ = *((DWORD*)&s);
	*push++ = *((DWORD*)&t);
}

static void dllTexCoord2d(GLdouble s, GLdouble t)
{
	assert(0);
}

static void dllTexCoord2dv(const GLdouble *v)
{
	assert(0);
}

static void dllTexCoord2f(GLfloat s, GLfloat t)
{
	setTexCoord(s, t);
}

static void dllTexCoord2fv(const GLfloat *v)
{
	setTexCoord(v[0], v[1]);
}

static void dllTexCoord2i(GLint s, GLint t)
{
	assert(0);
}

static void dllTexCoord2iv(const GLint *v)
{
	assert(0);
}

static void dllTexCoord2s(GLshort s, GLshort t)
{
	assert(0);
}

static void dllTexCoord2sv(const GLshort *v)
{
	assert(0);
}

static void dllTexCoord3d(GLdouble s, GLdouble t, GLdouble r)
{
	assert(0);
}

static void dllTexCoord3dv(const GLdouble *v)
{
	assert(0);
}

static void dllTexCoord3f(GLfloat s, GLfloat t, GLfloat r)
{
	assert(0);
}

static void dllTexCoord3fv(const GLfloat *v)
{
	assert(0);
}

static void dllTexCoord3i(GLint s, GLint t, GLint r)
{
	assert(0);
}

static void dllTexCoord3iv(const GLint *v)
{
	assert(0);
}

static void dllTexCoord3s(GLshort s, GLshort t, GLshort r)
{
	assert(0);
}

static void dllTexCoord3sv(const GLshort *v)
{
	assert(0);
}

static void dllTexCoord4d(GLdouble s, GLdouble t, GLdouble r, GLdouble q)
{
	assert(0);
}

static void dllTexCoord4dv(const GLdouble *v)
{
	assert(0);
}

static void dllTexCoord4f(GLfloat s, GLfloat t, GLfloat r, GLfloat q)
{
	assert(0);
}

static void dllTexCoord4fv(const GLfloat *v)
{
	assert(0);
}

static void dllTexCoord4i(GLint s, GLint t, GLint r, GLint q)
{
	assert(0);
}

static void dllTexCoord4iv(const GLint *v)
{
	assert(0);
}

static void dllTexCoord4s(GLshort s, GLshort t, GLshort r, GLshort q)
{
	assert(0);
}

static void dllTexCoord4sv(const GLshort *v)
{
	assert(0);
}

static void dllTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
	assert(size == 2 && type == GL_FLOAT);

	stride = (stride == 0) ? (sizeof(GLfloat) * 2) : stride;
	
	glw_state->texCoordPointer[glw_state->clientTU] = pointer;
	glw_state->texCoordStride[glw_state->clientTU] = stride;
}

static void dllTexEnvf(GLenum target, GLenum pname, GLfloat param)
{
	qglTexEnvi(target, pname, (GLint)param);
}

static void dllTexEnvfv(GLenum target, GLenum pname, const GLfloat *params)
{
	assert(0);
}

static void dllTexEnvi(GLenum target, GLenum pname, GLint param)
{
	assert(target == GL_TEXTURE_ENV && pname == GL_TEXTURE_ENV_MODE);

	/*glwstate_t::TextureInfo* info = _getCurrentTexture(glw_state->serverTU);
	if (!info) return;*/

	D3DTEXTUREOP env;
	switch (param)
	{
	case GL_MODULATE: default: env = D3DTOP_MODULATE; break;
	case GL_REPLACE: env = D3DTOP_SELECTARG1; break;
	// MATT! - I use GL_DECAL as the bumpmapping state
	case GL_DECAL: env = D3DTOP_DOTPRODUCT3; break;
	case GL_ADD: env = D3DTOP_ADD; break;
	case GL_NONE: env = D3DTOP_DISABLE; break;
	}

	if (glw_state->textureEnv[glw_state->serverTU] != env)
	{
		glw_state->textureEnv[glw_state->serverTU] = env;
		glw_state->textureStageDirty[glw_state->serverTU] = true;
	}
}

static void dllTexEnviv(GLenum target, GLenum pname, const GLint *params)
{
	assert(0);
}

static void dllTexGend(GLenum coord, GLenum pname, GLdouble param)
{
	assert(0);
}

static void dllTexGendv(GLenum coord, GLenum pname, const GLdouble *params)
{
	assert(0);
}

static void dllTexGenf(GLenum coord, GLenum pname, GLfloat param)
{
	assert(0);
}

static void dllTexGenfv(GLenum coord, GLenum pname, const GLfloat *params)
{
	assert(0);
}

static void dllTexGeni(GLenum coord, GLenum pname, GLint param)
{
	assert(0);
}

static void dllTexGeniv(GLenum coord, GLenum pname, const GLint *params)
{
	assert(0);
}

static void dllTexImage1D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels)
{
	assert(0);
}

static void _texImageDDS(glwstate_t::TextureInfo* info, GLint numlevels, GLsizei width, GLsizei height, GLenum format, const GLvoid *pixels)
{
	D3DFORMAT f = D3DFMT_UNKNOWN;
	switch( format )
	{
	case GL_DDS1_EXT:
		f = D3DFMT_DXT1;
		break;
	case GL_DDS5_EXT:
		f = D3DFMT_DXT5;
		break;
	case GL_DDS_RGB16_EXT:
		f = D3DFMT_R5G6B5;
		break;
	case GL_DDS_RGBA32_EXT:
		f = D3DFMT_A8R8G8B8;
		break;
	}

	if( numlevels == 0)
		numlevels = 1;

	info->mipmap = new IDirect3DTexture9;
	DWORD pixelSize = XGSetTextureHeader( width,
						height,
						numlevels,
						0,
						f,
						0,
						info->mipmap,
						0,
						0 );

	DWORD fileSize = Z_Size(const_cast<void*>(pixels));
	info->data = gTextures.Allocate( pixelSize, glw_state->currentTexture[glw_state->serverTU] );
	// Lightmaps need to be swizzled, they're in 565:
	if( f == D3DFMT_R5G6B5 )
	{
		byte *pSrc = ((byte *)pixels)+(fileSize-pixelSize);
		byte *pDst = (byte *)info->data;
		DWORD level = numlevels;
		DWORD curWidth = width;
		DWORD curHeight = height;
		while (level--)
		{
			XGSwizzleRect(pSrc, 0, NULL, pDst, curWidth, curHeight, NULL, 2);
			pSrc += curWidth*curHeight*2;
			pDst += curWidth*curHeight*2;
			curWidth >>= 1;
			curHeight >>= 1;
		}
	}
	else
	{
		memcpy( info->data, ((byte *)pixels)+(fileSize-pixelSize), pixelSize );
	}
	info->mipmap->Register( info->data );
}

static void _texImageRGBA(glwstate_t::TextureInfo* info, GLint numlevels, GLint internalformat, GLsizei width, GLsizei height, GLenum format, const GLvoid *pixels)
{
	// Fix number of levels:
	if( numlevels == 0 )
		numlevels = 1;

	// What format should the resultant texture be:
	D3DFORMAT dstFormat = D3DFMT_UNKNOWN;
	switch(internalformat)
	{
		case GL_RGB5:
		case GL_RGB4_S3TC:
			dstFormat = D3DFMT_R5G6B5;
			break;

		case GL_RGBA4:
			dstFormat = D3DFMT_A4R4G4B4;
			break;

		case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
			dstFormat = D3DFMT_DXT1;
			break;

		case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
			dstFormat = D3DFMT_DXT5;
			break;

		case GL_RGB8:
		case 3:
			dstFormat = D3DFMT_X8R8G8B8;
			break;

		case GL_LIN_RGBA8:
			dstFormat = D3DFMT_LIN_A8R8G8B8;
			break;
		case GL_LIN_RGB8:
			dstFormat = D3DFMT_LIN_X8R8G8B8;
			break;

		case GL_RGBA8:
		case 4:
			dstFormat = D3DFMT_A8R8G8B8;
			break;
		case GL_RGB:
			dstFormat = D3DFMT_X8R8G8B8;
			break;

		default:
			assert(0);
	}

	// What format is our source data in:
	D3DFORMAT srcFormat = D3DFMT_UNKNOWN;
	float bpp;
	int pitch;
	switch(format)
	{
		case GL_RGB:
			srcFormat = D3DFMT_X8R8G8B8;
			bpp = 3;
			break;
			
		case GL_RGBA:
			srcFormat = D3DFMT_A8R8G8B8;
			bpp = 4;
			break;

		case GL_LIN_RGBA:
			srcFormat = D3DFMT_LIN_A8R8G8B8;
			bpp = 4;
			break;

		case GL_LIN_RGB:
			srcFormat = D3DFMT_LIN_X8R8G8B8;
			bpp = 4;
			break;

		case GL_LIN_RGB8:
			srcFormat = D3DFMT_LIN_X8R8G8B8;
			bpp = 4;
			break;

		case GL_RGB8:
			srcFormat = D3DFMT_X8R8G8B8;
			bpp = 4;
			break;

		case GL_RGB_SWIZZLE_EXT:
			srcFormat = D3DFMT_R5G6B5;
			bpp = 2;
			break;

		case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
			srcFormat = D3DFMT_DXT1;
			bpp = 0.5;
			break;

		default:
			assert(0);
	}

	pitch = (int)((float)width * bpp);

	RECT	srcRect;

	srcRect.top = 0;
	srcRect.left = 0;
	srcRect.right = width;
	srcRect.bottom = height;

	info->mipmap = new IDirect3DTexture9;
	DWORD pixelSize = XGSetTextureHeader( width,
						height,
						numlevels,
						0,
						dstFormat,
						0,
						info->mipmap,
						0,
						0 );

	info->data = gTextures.Allocate( pixelSize, glw_state->currentTexture[glw_state->serverTU] );
	info->mipmap->Register( info->data );

	IDirect3DSurface8 *pSurf = NULL;
	info->mipmap->GetSurfaceLevel( 0, &pSurf );

	D3DXLoadSurfaceFromMemory( pSurf,
							   NULL,
							   NULL,
							   pixels,
							   srcFormat,
							   pitch,
							   NULL,
							   &srcRect,
							   D3DX_DEFAULT,
							   0 );

	pSurf->Release();

	// Generate mipmaps
	if( numlevels > 1)
	{
		D3DXFilterTexture( info->mipmap,
						   NULL,
						   D3DX_DEFAULT,
						   D3DX_DEFAULT );
	}
}

// EXTENSION: glTexImage2D plus "numlevels" number of mipmaps
static void dllTexImage2DEXT(GLenum target, GLint level, GLint numlevels, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels)
{
	assert(target == GL_TEXTURE_2D && border == 0 && type == GL_UNSIGNED_BYTE);

	// In Direct3D, setting 0 for number of mipmap 
	// levels means create the whole chain....
	/*if(numlevels == 0)
		numlevels = 1;*/

	glwstate_t::TextureInfo* info;

	glwstate_t::texturexlat_t::iterator current = 
		glw_state->textureXlat.find(
		glw_state->currentTexture[glw_state->serverTU]);

	// If we already have a texture bound to this ID, remove it.
	if (current != glw_state->textureXlat.end())
	{
		info = &current->second;

		delete info->mipmap;
		assert( 0 );		// Why is this happening? We're leaking texture memory!
	}
	// Otherwise, initialize it.
	else
	{
		info = &glw_state->textureXlat[
			glw_state->currentTexture[glw_state->serverTU]];

		info->minFilter = D3DTEXF_NONE;
		info->mipFilter = D3DTEXF_NONE;
		info->magFilter = D3DTEXF_NONE;
		info->anisotropy = 1.f;
		info->wrapU = D3DTADDRESS_CLAMP;
		info->wrapV = D3DTADDRESS_CLAMP;

		glw_state->textureStageDirty[glw_state->serverTU] = true;
	}

	// force any DX allocs to temp memory
//	Z_SetNewDeleteTemporary(true);

	if (format == GL_DDS1_EXT || 
		format == GL_DDS5_EXT || 
		format == GL_DDS_RGB16_EXT || 
		format == GL_DDS_RGBA32_EXT)
	{
		_texImageDDS(info, numlevels, width, height, format, pixels);
	}
	else
	{
		_texImageRGBA(info, numlevels, 
			internalformat, width, height, 
			format, pixels); 
	}

	// Done DX calls to new and delete
//	Z_SetNewDeleteTemporary(false);

#if MEMORY_PROFILE
	texMemSize += getTexMemSize(info->mipmap);
#endif
}

static void dllTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels)
{
	dllTexImage2DEXT(target, level, 1, internalformat, width, height, border, format, type, pixels);
}

static void dllTexParameteri(GLenum target, GLenum pname, GLint param)
{
	assert(target == GL_TEXTURE_2D);
	
	if (glw_state->currentTexture[glw_state->serverTU] == 0) return;

	glwstate_t::TextureInfo* info = _getCurrentTexture(glw_state->serverTU);
	if (!info) return;

	glw_state->textureStageDirty[glw_state->serverTU] = true;
	
	switch (pname)
	{
	case GL_TEXTURE_MIN_FILTER:
		switch (param)
		{
		case GL_NEAREST:
			info->minFilter = D3DTEXF_POINT;
			info->mipFilter = D3DTEXF_NONE;
			break;
		case GL_LINEAR:
			info->minFilter = D3DTEXF_LINEAR;
			info->mipFilter = D3DTEXF_NONE;
			break;
		case GL_NEAREST_MIPMAP_NEAREST:
			info->minFilter = D3DTEXF_POINT;
			info->mipFilter = D3DTEXF_POINT;
			break;
		case GL_LINEAR_MIPMAP_NEAREST:
			info->minFilter = D3DTEXF_LINEAR;
			info->mipFilter = D3DTEXF_POINT;
			break;
		case GL_NEAREST_MIPMAP_LINEAR:
			info->minFilter = D3DTEXF_POINT;
			info->mipFilter = D3DTEXF_LINEAR;
			break;
		case GL_LINEAR_MIPMAP_LINEAR:
			info->minFilter = D3DTEXF_LINEAR;
			info->mipFilter = D3DTEXF_LINEAR;
			break;
		}
		info->anisotropy = 1.f;
		break;
	case GL_TEXTURE_MAG_FILTER:
		switch (param)
		{
		case GL_NEAREST:
			info->magFilter = D3DTEXF_POINT;
			break;
		case GL_LINEAR:
			info->magFilter = D3DTEXF_LINEAR;
			break;
		}
		info->anisotropy = 1.f;
		break;
	case GL_TEXTURE_WRAP_S:
		switch (param)
		{
		case GL_REPEAT: info->wrapU = D3DTADDRESS_WRAP; break;
		case GL_CLAMP: info->wrapU = D3DTADDRESS_CLAMP; break;
		}
		break;
	case GL_TEXTURE_WRAP_T:
		switch (param)
		{
		case GL_REPEAT: info->wrapV = D3DTADDRESS_WRAP; break;
		case GL_CLAMP: info->wrapV = D3DTADDRESS_CLAMP; break;
		}
		break;
	case GL_TEXTURE_MAX_ANISOTROPY_EXT:
		info->anisotropy = (float)param;
		info->minFilter = D3DTEXF_ANISOTROPIC;
		info->magFilter = D3DTEXF_ANISOTROPIC;
		break;
	}
}

static void dllTexParameterf(GLenum target, GLenum pname, GLfloat param)
{
	dllTexParameteri(target, pname, param);
}

static void dllTexParameterfv(GLenum target, GLenum pname, const GLfloat *params)
{
	// Intentionally left blank
}

static void dllTexParameteriv(GLenum target, GLenum pname, const GLint *params)
{
	// Intentionally left blank
}

static void dllTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels)
{
	assert(0);
}

static void dllTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels)
{
	assert(target == GL_TEXTURE_2D && level == 0 && type == GL_UNSIGNED_BYTE);

	glwstate_t::TextureInfo* info = _getCurrentTexture(glw_state->serverTU);
	if (info == NULL) return;

	RECT sr;
	sr.top = 0;
	sr.left = 0;
	sr.right = width;
	sr.bottom = height;

	RECT dr;
	dr.top = xoffset;
	dr.left = yoffset;
	dr.right = xoffset + width;
	dr.bottom = yoffset + height;

	Z_SetNewDeleteTemporary(true);

	LPDIRECT3DSURFACE8 surf;
	info->mipmap->GetSurfaceLevel(0, &surf);
	
	// We use the supplied format to handle pixel data correctly, the way OGL would
	D3DFORMAT srcFormat;
	switch(format)
	{
		case GL_RGB:
			srcFormat = D3DFMT_LIN_X8R8G8B8;
			break;

		case GL_RGBA:
			srcFormat = D3DFMT_LIN_A8R8G8B8;
			break;

		default:
			assert(0 && "Unsupported format in dllTexSubImage2D");
			return;
	}

	D3DXLoadSurfaceFromMemory(surf, NULL, &dr, pixels,
		srcFormat, width * 4, NULL, &sr, D3DX_DEFAULT, 0);

	surf->Release();

	Z_SetNewDeleteTemporary(false);
}

static void dllTranslated(GLdouble x, GLdouble y, GLdouble z)
{
	assert(0);
}

static void dllTranslatef(GLfloat x, GLfloat y, GLfloat z)
{
	glw_state->matrixStack[glw_state->matrixMode]->TranslateLocal(x, y, z);
	glw_state->matricesDirty[glw_state->matrixMode] = true;
}

static void setVertex(float x, float y, float z)
{
	assert(glw_state->inDrawBlock);

	_handleDrawOverflow();
	
	DWORD* push = &glw_state->drawArray[glw_state->numVertices * glw_state->drawStride];
	push[0] = *((DWORD*)&x);
	push[1] = *((DWORD*)&y);
	push[2] = *((DWORD*)&z);
	push[3] = glw_state->currentColor;

	++glw_state->numVertices;
}

static void dllVertex2d(GLdouble x, GLdouble y)
{
	assert(0);
}

static void dllVertex2dv(const GLdouble *v)
{
	assert(0);
}

static void dllVertex2f(GLfloat x, GLfloat y)
{
	setVertex(x, y, 0.f);
}

static void dllVertex2fv(const GLfloat *v)
{
	setVertex(v[0], v[1], 0.f);
}

static void dllVertex2i(GLint x, GLint y)
{
	assert(0);
}

static void dllVertex2iv(const GLint *v)
{
	assert(0);
}

static void dllVertex2s(GLshort x, GLshort y)
{
	assert(0);
}

static void dllVertex2sv(const GLshort *v)
{
	assert(0);
}

static void dllVertex3d(GLdouble x, GLdouble y, GLdouble z)
{
	assert(0);
}

static void dllVertex3dv(const GLdouble *v)
{
	assert(0);
}

static void dllVertex3f(GLfloat x, GLfloat y, GLfloat z)
{
	setVertex(x, y, z);
}

static void dllVertex3fv(const GLfloat *v)
{
	setVertex(v[0], v[1], v[2]);
}

static void dllVertex3i(GLint x, GLint y, GLint z)
{
	assert(0);
}

static void dllVertex3iv(const GLint *v)
{
	assert(0);
}

static void dllVertex3s(GLshort x, GLshort y, GLshort z)
{
	assert(0);
}

static void dllVertex3sv(const GLshort *v)
{
	assert(0);
}

static void dllVertex4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
	assert(0);
}

static void dllVertex4dv(const GLdouble *v)
{
	assert(0);
}

static void dllVertex4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
	setVertex(x, y, z);
}

static void dllVertex4fv(const GLfloat *v)
{
	setVertex(v[0], v[1], v[2]);
}

static void dllVertex4i(GLint x, GLint y, GLint z, GLint w)
{
	assert(0);
}

static void dllVertex4iv(const GLint *v)
{
	assert(0);
}

static void dllVertex4s(GLshort x, GLshort y, GLshort z, GLshort w)
{
	assert(0);
}

static void dllVertex4sv(const GLshort *v)
{
	assert(0);
}

static void dllVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
	assert(size == 3 && type == GL_FLOAT);
	
	stride = (stride == 0) ? (sizeof(GLfloat) * 3) : stride;

	glw_state->vertexPointer = pointer;
	glw_state->vertexStride = stride;
}

static void dllViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
	_fixupScreenCoords(x, y, width, height);

	glw_state->viewport.X = x;
	glw_state->viewport.Y = y;
	glw_state->viewport.Width = width;
	glw_state->viewport.Height = height;
	glw_state->device->SetViewport(&glw_state->viewport);
}


static void dllMultiTexCoord2fARB(GLenum texture, GLfloat s, GLfloat t)
{
	assert(glw_state->inDrawBlock);

	_handleDrawOverflow();

	DWORD* push = &glw_state->drawArray[
		glw_state->numVertices * glw_state->drawStride + 
		4 + (texture - GL_TEXTURE0_ARB) * 2];
	
	*push++ = *((DWORD*)&s);
	*push++ = *((DWORD*)&t);
}

static void dllActiveTextureARB(GLenum texture)
{
	assert(GLW_MAX_TEXTURE_STAGES > texture - GL_TEXTURE0_ARB);
	glw_state->serverTU = texture - GL_TEXTURE0_ARB;
}

static void dllClientActiveTextureARB(GLenum texture)
{
	assert(GLW_MAX_TEXTURE_STAGES > texture - GL_TEXTURE0_ARB);
	glw_state->clientTU = texture - GL_TEXTURE0_ARB;
}


/*
** QGL_Shutdown
**
** Unloads the specified DLL then nulls out all the proc pointers.  This
** is only called during a hard shutdown of the OGL subsystem (e.g. vid_restart).
*/
void QGL_Shutdown( void )
{
	VID_Printf( PRINT_ALL, "...shutting down QGL\n" );

	qglAccum                     = NULL;
	qglAlphaFunc                 = NULL;
	qglAreTexturesResident       = NULL;
	qglArrayElement              = NULL;
	qglBegin                     = NULL;
	qglBeginEXT                  = NULL;
	qglBeginFrame                = NULL;
	qglBeginShadow               = NULL;
	qglBindTexture               = NULL;
	qglBitmap                    = NULL;
	qglBlendFunc                 = NULL;
	qglCallList                  = NULL;
	qglCallLists                 = NULL;
	qglClear                     = NULL;
	qglClearAccum                = NULL;
	qglClearColor                = NULL;
	qglClearDepth                = NULL;
	qglClearIndex                = NULL;
	qglClearStencil              = NULL;
	qglClipPlane                 = NULL;
	qglColor3b                   = NULL;
	qglColor3bv                  = NULL;
	qglColor3d                   = NULL;
	qglColor3dv                  = NULL;
	qglColor3f                   = NULL;
	qglColor3fv                  = NULL;
	qglColor3i                   = NULL;
	qglColor3iv                  = NULL;
	qglColor3s                   = NULL;
	qglColor3sv                  = NULL;
	qglColor3ub                  = NULL;
	qglColor3ubv                 = NULL;
	qglColor3ui                  = NULL;
	qglColor3uiv                 = NULL;
	qglColor3us                  = NULL;
	qglColor3usv                 = NULL;
	qglColor4b                   = NULL;
	qglColor4bv                  = NULL;
	qglColor4d                   = NULL;
	qglColor4dv                  = NULL;
	qglColor4f                   = NULL;
	qglColor4fv                  = NULL;
	qglColor4i                   = NULL;
	qglColor4iv                  = NULL;
	qglColor4s                   = NULL;
	qglColor4sv                  = NULL;
	qglColor4ub                  = NULL;
	qglColor4ubv                 = NULL;
	qglColor4ui                  = NULL;
	qglColor4uiv                 = NULL;
	qglColor4us                  = NULL;
	qglColor4usv                 = NULL;
	qglColorMask                 = NULL;
	qglColorMaterial             = NULL;
	qglColorPointer              = NULL;
	qglCopyPixels                = NULL;
	qglCopyTexImage1D            = NULL;
	qglCopyTexImage2D            = NULL;
	qglCopyTexSubImage1D         = NULL;
	qglCopyTexSubImage2D         = NULL;
	qglCullFace                  = NULL;
	qglDeleteLists               = NULL;
	qglDeleteTextures            = NULL;
	qglDepthFunc                 = NULL;
	qglDepthMask                 = NULL;
	qglDepthRange                = NULL;
	qglDisable                   = NULL;
	qglDisableClientState        = NULL;
	qglDrawArrays                = NULL;
	qglDrawBuffer                = NULL;
	qglDrawElements              = NULL;
	qglDrawPixels                = NULL;
	qglEdgeFlag                  = NULL;
	qglEdgeFlagPointer           = NULL;
	qglEdgeFlagv                 = NULL;
	qglEnable                    = NULL;
	qglEnableClientState         = NULL;
	qglEnd                       = NULL;
	qglEndFrame                  = NULL;
	qglEndShadow                 = NULL;
	qglEndList                   = NULL;
	qglEvalCoord1d               = NULL;
	qglEvalCoord1dv              = NULL;
	qglEvalCoord1f               = NULL;
	qglEvalCoord1fv              = NULL;
	qglEvalCoord2d               = NULL;
	qglEvalCoord2dv              = NULL;
	qglEvalCoord2f               = NULL;
	qglEvalCoord2fv              = NULL;
	qglEvalMesh1                 = NULL;
	qglEvalMesh2                 = NULL;
	qglEvalPoint1                = NULL;
	qglEvalPoint2                = NULL;
	qglFeedbackBuffer            = NULL;
	qglFinish                    = NULL;
	qglFlush                     = NULL;
	qglFlushShadow               = NULL;
	qglFogf                      = NULL;
	qglFogfv                     = NULL;
	qglFogi                      = NULL;
	qglFogiv                     = NULL;
	qglFrontFace                 = NULL;
	qglFrustum                   = NULL;
	qglGenLists                  = NULL;
	qglGenTextures               = NULL;
	qglGetBooleanv               = NULL;
	qglGetClipPlane              = NULL;
	qglGetDoublev                = NULL;
	qglGetError                  = NULL;
	qglGetFloatv                 = NULL;
	qglGetIntegerv               = NULL;
	qglGetLightfv                = NULL;
	qglGetLightiv                = NULL;
	qglGetMapdv                  = NULL;
	qglGetMapfv                  = NULL;
	qglGetMapiv                  = NULL;
	qglGetMaterialfv             = NULL;
	qglGetMaterialiv             = NULL;
	qglGetPixelMapfv             = NULL;
	qglGetPixelMapuiv            = NULL;
	qglGetPixelMapusv            = NULL;
	qglGetPointerv               = NULL;
	qglGetPolygonStipple         = NULL;
	qglGetString                 = NULL;
	qglGetTexEnvfv               = NULL;
	qglGetTexEnviv               = NULL;
	qglGetTexGendv               = NULL;
	qglGetTexGenfv               = NULL;
	qglGetTexGeniv               = NULL;
	qglGetTexImage               = NULL;
	qglGetTexLevelParameterfv    = NULL;
	qglGetTexLevelParameteriv    = NULL;
	qglGetTexParameterfv         = NULL;
	qglGetTexParameteriv         = NULL;
	qglHint                      = NULL;
	qglIndexedTriToStrip         = NULL;
	qglIndexMask                 = NULL;
	qglIndexPointer              = NULL;
	qglIndexd                    = NULL;
	qglIndexdv                   = NULL;
	qglIndexf                    = NULL;
	qglIndexfv                   = NULL;
	qglIndexi                    = NULL;
	qglIndexiv                   = NULL;
	qglIndexs                    = NULL;
	qglIndexsv                   = NULL;
	qglIndexub                   = NULL;
	qglIndexubv                  = NULL;
	qglInitNames                 = NULL;
	qglInterleavedArrays         = NULL;
	qglIsEnabled                 = NULL;
	qglIsList                    = NULL;
	qglIsTexture                 = NULL;
	qglLightModelf               = NULL;
	qglLightModelfv              = NULL;
	qglLightModeli               = NULL;
	qglLightModeliv              = NULL;
	qglLightf                    = NULL;
	qglLightfv                   = NULL;
	qglLighti                    = NULL;
	qglLightiv                   = NULL;
	qglLineStipple               = NULL;
	qglLineWidth                 = NULL;
	qglListBase                  = NULL;
	qglLoadIdentity              = NULL;
	qglLoadMatrixd               = NULL;
	qglLoadMatrixf               = NULL;
	qglLoadName                  = NULL;
	qglLogicOp                   = NULL;
	qglMap1d                     = NULL;
	qglMap1f                     = NULL;
	qglMap2d                     = NULL;
	qglMap2f                     = NULL;
	qglMapGrid1d                 = NULL;
	qglMapGrid1f                 = NULL;
	qglMapGrid2d                 = NULL;
	qglMapGrid2f                 = NULL;
	qglMaterialf                 = NULL;
	qglMaterialfv                = NULL;
	qglMateriali                 = NULL;
	qglMaterialiv                = NULL;
	qglMatrixMode                = NULL;
	qglMultMatrixd               = NULL;
	qglMultMatrixf               = NULL;
	qglNewList                   = NULL;
	qglNormal3b                  = NULL;
	qglNormal3bv                 = NULL;
	qglNormal3d                  = NULL;
	qglNormal3dv                 = NULL;
	qglNormal3f                  = NULL;
	qglNormal3fv                 = NULL;
	qglNormal3i                  = NULL;
	qglNormal3iv                 = NULL;
	qglNormal3s                  = NULL;
	qglNormal3sv                 = NULL;
	qglNormalPointer             = NULL;
	qglOrtho                     = NULL;
	qglPassThrough               = NULL;
	qglPixelMapfv                = NULL;
	qglPixelMapuiv               = NULL;
	qglPixelMapusv               = NULL;
	qglPixelStoref               = NULL;
	qglPixelStorei               = NULL;
	qglPixelTransferf            = NULL;
	qglPixelTransferi            = NULL;
	qglPixelZoom                 = NULL;
	qglPointSize                 = NULL;
	qglPolygonMode               = NULL;
	qglPolygonOffset             = NULL;
	qglPolygonStipple            = NULL;
	qglPopAttrib                 = NULL;
	qglPopClientAttrib           = NULL;
	qglPopMatrix                 = NULL;
	qglPopName                   = NULL;
	qglPrioritizeTextures        = NULL;
	qglPushAttrib                = NULL;
	qglPushClientAttrib          = NULL;
	qglPushMatrix                = NULL;
	qglPushName                  = NULL;
	qglRasterPos2d               = NULL;
	qglRasterPos2dv              = NULL;
	qglRasterPos2f               = NULL;
	qglRasterPos2fv              = NULL;
	qglRasterPos2i               = NULL;
	qglRasterPos2iv              = NULL;
	qglRasterPos2s               = NULL;
	qglRasterPos2sv              = NULL;
	qglRasterPos3d               = NULL;
	qglRasterPos3dv              = NULL;
	qglRasterPos3f               = NULL;
	qglRasterPos3fv              = NULL;
	qglRasterPos3i               = NULL;
	qglRasterPos3iv              = NULL;
	qglRasterPos3s               = NULL;
	qglRasterPos3sv              = NULL;
	qglRasterPos4d               = NULL;
	qglRasterPos4dv              = NULL;
	qglRasterPos4f               = NULL;
	qglRasterPos4fv              = NULL;
	qglRasterPos4i               = NULL;
	qglRasterPos4iv              = NULL;
	qglRasterPos4s               = NULL;
	qglRasterPos4sv              = NULL;
	qglReadBuffer                = NULL;
	qglReadPixels                = NULL;
	qglRectd                     = NULL;
	qglRectdv                    = NULL;
	qglRectf                     = NULL;
	qglRectfv                    = NULL;
	qglRecti                     = NULL;
	qglRectiv                    = NULL;
	qglRects                     = NULL;
	qglRectsv                    = NULL;
	qglRenderMode                = NULL;
	qglRotated                   = NULL;
	qglRotatef                   = NULL;
	qglScaled                    = NULL;
	qglScalef                    = NULL;
	qglScissor                   = NULL;
	qglSelectBuffer              = NULL;
	qglShadeModel                = NULL;
	qglStencilFunc               = NULL;
	qglStencilMask               = NULL;
	qglStencilOp                 = NULL;
	qglTexCoord1d                = NULL;
	qglTexCoord1dv               = NULL;
	qglTexCoord1f                = NULL;
	qglTexCoord1fv               = NULL;
	qglTexCoord1i                = NULL;
	qglTexCoord1iv               = NULL;
	qglTexCoord1s                = NULL;
	qglTexCoord1sv               = NULL;
	qglTexCoord2d                = NULL;
	qglTexCoord2dv               = NULL;
	qglTexCoord2f                = NULL;
	qglTexCoord2fv               = NULL;
	qglTexCoord2i                = NULL;
	qglTexCoord2iv               = NULL;
	qglTexCoord2s                = NULL;
	qglTexCoord2sv               = NULL;
	qglTexCoord3d                = NULL;
	qglTexCoord3dv               = NULL;
	qglTexCoord3f                = NULL;
	qglTexCoord3fv               = NULL;
	qglTexCoord3i                = NULL;
	qglTexCoord3iv               = NULL;
	qglTexCoord3s                = NULL;
	qglTexCoord3sv               = NULL;
	qglTexCoord4d                = NULL;
	qglTexCoord4dv               = NULL;
	qglTexCoord4f                = NULL;
	qglTexCoord4fv               = NULL;
	qglTexCoord4i                = NULL;
	qglTexCoord4iv               = NULL;
	qglTexCoord4s                = NULL;
	qglTexCoord4sv               = NULL;
	qglTexCoordPointer           = NULL;
	qglTexEnvf                   = NULL;
	qglTexEnvfv                  = NULL;
	qglTexEnvi                   = NULL;
	qglTexEnviv                  = NULL;
	qglTexGend                   = NULL;
	qglTexGendv                  = NULL;
	qglTexGenf                   = NULL;
	qglTexGenfv                  = NULL;
	qglTexGeni                   = NULL;
	qglTexGeniv                  = NULL;
	qglTexImage1D                = NULL;
	qglTexImage2D                = NULL;
	qglTexImage2DEXT             = NULL;
	qglTexParameterf             = NULL;
	qglTexParameterfv            = NULL;
	qglTexParameteri             = NULL;
	qglTexParameteriv            = NULL;
	qglTexSubImage1D             = NULL;
	qglTexSubImage2D             = NULL;
	qglTranslated                = NULL;
	qglTranslatef                = NULL;
	qglVertex2d                  = NULL;
	qglVertex2dv                 = NULL;
	qglVertex2f                  = NULL;
	qglVertex2fv                 = NULL;
	qglVertex2i                  = NULL;
	qglVertex2iv                 = NULL;
	qglVertex2s                  = NULL;
	qglVertex2sv                 = NULL;
	qglVertex3d                  = NULL;
	qglVertex3dv                 = NULL;
	qglVertex3f                  = NULL;
	qglVertex3fv                 = NULL;
	qglVertex3i                  = NULL;
	qglVertex3iv                 = NULL;
	qglVertex3s                  = NULL;
	qglVertex3sv                 = NULL;
	qglVertex4d                  = NULL;
	qglVertex4dv                 = NULL;
	qglVertex4f                  = NULL;
	qglVertex4fv                 = NULL;
	qglVertex4i                  = NULL;
	qglVertex4iv                 = NULL;
	qglVertex4s                  = NULL;
	qglVertex4sv                 = NULL;
	qglVertexPointer             = NULL;
	qglViewport                  = NULL;

	qglActiveTextureARB          = NULL;
	qglClientActiveTextureARB    = NULL;
	qglMultiTexCoord2fARB        = NULL;
}

/*
** QGL_Init
**
** This is responsible for binding our qgl function pointers to 
** the appropriate GL stuff.  In Windows this means doing a 
** LoadLibrary and a bunch of calls to GetProcAddress.  On other
** operating systems we need to do the right thing, whatever that
** might be.
*/
qboolean QGL_Init( const char *dllname )
{
	qglAccum                     = dllAccum;
	qglAlphaFunc                 = dllAlphaFunc;
	qglAreTexturesResident       = dllAreTexturesResident;
	qglArrayElement              = dllArrayElement;
	qglBegin                     = dllBegin;
	qglBeginEXT                  = dllBeginEXT;
	qglBeginFrame                = dllBeginFrame;
	qglBeginShadow               = dllBeginShadow;
	qglBindTexture               = dllBindTexture;
	qglBitmap                    = dllBitmap;
	qglBlendFunc                 = dllBlendFunc;
	qglCallList                  = dllCallList;
	qglCallLists                 = dllCallLists;
	qglClear                     = dllClear;
	qglClearAccum                = dllClearAccum;
	qglClearColor                = dllClearColor;
	qglClearDepth                = dllClearDepth;
	qglClearIndex                = dllClearIndex;
	qglClearStencil              = dllClearStencil;
	qglClipPlane                 = dllClipPlane;
	qglColor3b                   = dllColor3b;
	qglColor3bv                  = dllColor3bv;
	qglColor3d                   = dllColor3d;
	qglColor3dv                  = dllColor3dv;
	qglColor3f                   = dllColor3f;
	qglColor3fv                  = dllColor3fv;
	qglColor3i                   = dllColor3i;
	qglColor3iv                  = dllColor3iv;
	qglColor3s                   = dllColor3s;
	qglColor3sv                  = dllColor3sv;
	qglColor3ub                  = dllColor3ub;
	qglColor3ubv                 = dllColor3ubv;
	qglColor3ui                  = dllColor3ui;
	qglColor3uiv                 = dllColor3uiv;
	qglColor3us                  = dllColor3us;
	qglColor3usv                 = dllColor3usv;
	qglColor4b                   = dllColor4b;
	qglColor4bv                  = dllColor4bv;
	qglColor4d                   = dllColor4d;
	qglColor4dv                  = dllColor4dv;
	qglColor4f                   = dllColor4f;
	qglColor4fv                  = dllColor4fv;
	qglColor4i                   = dllColor4i;
	qglColor4iv                  = dllColor4iv;
	qglColor4s                   = dllColor4s;
	qglColor4sv                  = dllColor4sv;
	qglColor4ub                  = dllColor4ub;
	qglColor4ubv                 = dllColor4ubv;
	qglColor4ui                  = dllColor4ui;
	qglColor4uiv                 = dllColor4uiv;
	qglColor4us                  = dllColor4us;
	qglColor4usv                 = dllColor4usv;
	qglColorMask                 = dllColorMask;
	qglColorMaterial             = dllColorMaterial;
	qglColorPointer              = dllColorPointer;
	qglCopyPixels                = dllCopyPixels;
	qglCopyTexImage1D            = dllCopyTexImage1D;
	qglCopyTexImage2D            = dllCopyTexImage2D;
	qglCopyTexSubImage1D         = dllCopyTexSubImage1D;
	qglCopyTexSubImage2D         = dllCopyTexSubImage2D;
	qglCullFace                  = dllCullFace;
	qglDeleteLists               = dllDeleteLists;
	qglDeleteTextures            = dllDeleteTextures;
	qglDepthFunc                 = dllDepthFunc;
	qglDepthMask                 = dllDepthMask;
	qglDepthRange                = dllDepthRange;
	qglDisable                   = dllDisable;
	qglDisableClientState        = dllDisableClientState;
	qglDrawArrays                = dllDrawArrays;
	qglDrawBuffer                = dllDrawBuffer;
	qglDrawElements              = dllDrawElements;
	qglDrawPixels                = dllDrawPixels;
	qglEdgeFlag                  = dllEdgeFlag;
	qglEdgeFlagPointer           = dllEdgeFlagPointer;
	qglEdgeFlagv                 = dllEdgeFlagv;
	qglEnable                    = 	dllEnable                   ;
	qglEnableClientState         = 	dllEnableClientState        ;
	qglEnd                       = 	dllEnd                      ;
	qglEndFrame                  = 	dllEndFrame                 ;
	qglEndShadow                 = 	dllEndShadow                ;
	qglEndList                   = 	dllEndList                  ;
	qglEvalCoord1d				 = 	dllEvalCoord1d				;
	qglEvalCoord1dv              = 	dllEvalCoord1dv             ;
	qglEvalCoord1f               = 	dllEvalCoord1f              ;
	qglEvalCoord1fv              = 	dllEvalCoord1fv             ;
	qglEvalCoord2d               = 	dllEvalCoord2d              ;
	qglEvalCoord2dv              = 	dllEvalCoord2dv             ;
	qglEvalCoord2f               = 	dllEvalCoord2f              ;
	qglEvalCoord2fv              = 	dllEvalCoord2fv             ;
	qglEvalMesh1                 = 	dllEvalMesh1                ;
	qglEvalMesh2                 = 	dllEvalMesh2                ;
	qglEvalPoint1                = 	dllEvalPoint1               ;
	qglEvalPoint2                = 	dllEvalPoint2               ;
	qglFeedbackBuffer            = 	dllFeedbackBuffer           ;
	qglFinish                    = 	dllFinish                   ;
	qglFlush                     = 	dllFlush                    ;
	qglFlushShadow               = 	dllFlushShadow              ;
	qglFogf                      = 	dllFogf                     ;
	qglFogfv                     = 	dllFogfv                    ;
	qglFogi                      = 	dllFogi                     ;
	qglFogiv                     = 	dllFogiv                    ;
	qglFrontFace                 = 	dllFrontFace                ;
	qglFrustum                   = 	dllFrustum                  ;
	qglGenLists                  = 	dllGenLists                 ;
	qglGenTextures               = 	dllGenTextures              ;
	qglGetBooleanv               = 	dllGetBooleanv              ;
	qglGetClipPlane              = 	dllGetClipPlane             ;
	qglGetDoublev                = 	dllGetDoublev               ;
	qglGetError                  = 	dllGetError                 ;
	qglGetFloatv                 = 	dllGetFloatv                ;
	qglGetIntegerv               = 	dllGetIntegerv              ;
	qglGetLightfv                = 	dllGetLightfv               ;
	qglGetLightiv                = 	dllGetLightiv               ;
	qglGetMapdv                  = 	dllGetMapdv                 ;
	qglGetMapfv                  = 	dllGetMapfv                 ;
	qglGetMapiv                  = 	dllGetMapiv                 ;
	qglGetMaterialfv             = 	dllGetMaterialfv            ;
	qglGetMaterialiv             = 	dllGetMaterialiv            ;
	qglGetPixelMapfv             = 	dllGetPixelMapfv            ;
	qglGetPixelMapuiv            = 	dllGetPixelMapuiv           ;
	qglGetPixelMapusv            = 	dllGetPixelMapusv           ;
	qglGetPointerv               = 	dllGetPointerv              ;
	qglGetPolygonStipple         = 	dllGetPolygonStipple        ;
	qglGetString                 = 	dllGetString                ;
	qglGetTexEnvfv               = 	dllGetTexEnvfv              ;
	qglGetTexEnviv               = 	dllGetTexEnviv              ;
	qglGetTexGendv               = 	dllGetTexGendv              ;
	qglGetTexGenfv               = 	dllGetTexGenfv              ;
	qglGetTexGeniv               = 	dllGetTexGeniv              ;
	qglGetTexImage               = 	dllGetTexImage              ;
//	qglGetTexLevelParameterfv    = 	dllGetTexLevelParameterfv   ;
//	qglGetTexLevelParameteriv    = 	dllGetTexLevelParameteriv   ;
	qglGetTexParameterfv         = 	dllGetTexParameterfv        ;
	qglGetTexParameteriv         = 	dllGetTexParameteriv        ;
	qglHint                      = 	dllHint                     ;
	qglIndexedTriToStrip         =  dllIndexedTriToStrip        ;
	qglIndexMask                 = 	dllIndexMask                ;
	qglIndexPointer              = 	dllIndexPointer             ;
	qglIndexd                    = 	dllIndexd                   ;
	qglIndexdv                   = 	dllIndexdv                  ;
	qglIndexf                    = 	dllIndexf                   ;
	qglIndexfv                   = 	dllIndexfv                  ;
	qglIndexi                    = 	dllIndexi                   ;
	qglIndexiv                   = 	dllIndexiv                  ;
	qglIndexs                    = 	dllIndexs                   ;
	qglIndexsv                   = 	dllIndexsv                  ;
	qglIndexub                   = 	dllIndexub                  ;
	qglIndexubv                  = 	dllIndexubv                 ;
	qglInitNames                 = 	dllInitNames                ;
	qglInterleavedArrays         = 	dllInterleavedArrays        ;
	qglIsEnabled                 = 	dllIsEnabled                ;
	qglIsList                    = 	dllIsList                   ;
	qglIsTexture                 = 	dllIsTexture                ;
	qglLightModelf               = 	dllLightModelf              ;
	qglLightModelfv              = 	dllLightModelfv             ;
	qglLightModeli               = 	dllLightModeli              ;
	qglLightModeliv              = 	dllLightModeliv             ;
	qglLightf                    = 	dllLightf                   ;
	qglLightfv                   = 	dllLightfv                  ;
	qglLighti                    = 	dllLighti                   ;
	qglLightiv                   = 	dllLightiv                  ;
	qglLineStipple               = 	dllLineStipple              ;
	qglLineWidth                 = 	dllLineWidth                ;
	qglListBase                  = 	dllListBase                 ;
	qglLoadIdentity              = 	dllLoadIdentity             ;
	qglLoadMatrixd               = 	dllLoadMatrixd              ;
	qglLoadMatrixf               = 	dllLoadMatrixf              ;
	qglLoadName                  = 	dllLoadName                 ;
	qglLogicOp                   = 	dllLogicOp                  ;
	qglMap1d                     = 	dllMap1d                    ;
	qglMap1f                     = 	dllMap1f                    ;
	qglMap2d                     = 	dllMap2d                    ;
	qglMap2f                     = 	dllMap2f                    ;
	qglMapGrid1d                 = 	dllMapGrid1d                ;
	qglMapGrid1f                 = 	dllMapGrid1f                ;
	qglMapGrid2d                 = 	dllMapGrid2d                ;
	qglMapGrid2f                 = 	dllMapGrid2f                ;
	qglMaterialf                 = 	dllMaterialf                ;
	qglMaterialfv                = 	dllMaterialfv               ;
	qglMateriali                 = 	dllMateriali                ;
	qglMaterialiv                = 	dllMaterialiv               ;
	qglMatrixMode                = 	dllMatrixMode               ;
	qglMultMatrixd               = 	dllMultMatrixd              ;
	qglMultMatrixf               = 	dllMultMatrixf              ;
	qglNewList                   = 	dllNewList                  ;
	qglNormal3b                  = 	dllNormal3b                 ;
	qglNormal3bv                 = 	dllNormal3bv                ;
	qglNormal3d                  = 	dllNormal3d                 ;
	qglNormal3dv                 = 	dllNormal3dv                ;
	qglNormal3f                  = 	dllNormal3f                 ;
	qglNormal3fv                 = 	dllNormal3fv                ;
	qglNormal3i                  = 	dllNormal3i                 ;
	qglNormal3iv                 = 	dllNormal3iv                ;
	qglNormal3s                  = 	dllNormal3s                 ;
	qglNormal3sv                 = 	dllNormal3sv                ;
	qglNormalPointer             = 	dllNormalPointer            ;
	qglOrtho                     = 	dllOrtho                    ;
	qglPassThrough               = 	dllPassThrough              ;
	qglPixelMapfv                = 	dllPixelMapfv               ;
	qglPixelMapuiv               = 	dllPixelMapuiv              ;
	qglPixelMapusv               = 	dllPixelMapusv              ;
	qglPixelStoref               = 	dllPixelStoref              ;
	qglPixelStorei               = 	dllPixelStorei              ;
	qglPixelTransferf            = 	dllPixelTransferf           ;
	qglPixelTransferi            = 	dllPixelTransferi           ;
	qglPixelZoom                 = 	dllPixelZoom                ;
	qglPointSize                 = 	dllPointSize                ;
	qglPolygonMode               = 	dllPolygonMode              ;
	qglPolygonOffset             = 	dllPolygonOffset            ;
	qglPolygonStipple            = 	dllPolygonStipple           ;
	qglPopAttrib                 = 	dllPopAttrib                ;
	qglPopClientAttrib           = 	dllPopClientAttrib          ;
	qglPopMatrix                 = 	dllPopMatrix                ;
	qglPopName                   = 	dllPopName                  ;
	qglPrioritizeTextures        = 	dllPrioritizeTextures       ;
	qglPushAttrib                = 	dllPushAttrib               ;
	qglPushClientAttrib          = 	dllPushClientAttrib         ;
	qglPushMatrix                = 	dllPushMatrix               ;
	qglPushName                  = 	dllPushName                 ;
	qglRasterPos2d               = 	dllRasterPos2d              ;
	qglRasterPos2dv              = 	dllRasterPos2dv             ;
	qglRasterPos2f               = 	dllRasterPos2f              ;
	qglRasterPos2fv              = 	dllRasterPos2fv             ;
	qglRasterPos2i               = 	dllRasterPos2i              ;
	qglRasterPos2iv              = 	dllRasterPos2iv             ;
	qglRasterPos2s               = 	dllRasterPos2s              ;
	qglRasterPos2sv              = 	dllRasterPos2sv             ;
	qglRasterPos3d               = 	dllRasterPos3d              ;
	qglRasterPos3dv              = 	dllRasterPos3dv             ;
	qglRasterPos3f               = 	dllRasterPos3f              ;
	qglRasterPos3fv              = 	dllRasterPos3fv             ;
	qglRasterPos3i               = 	dllRasterPos3i              ;
	qglRasterPos3iv              = 	dllRasterPos3iv             ;
	qglRasterPos3s               = 	dllRasterPos3s              ;
	qglRasterPos3sv              = 	dllRasterPos3sv             ;
	qglRasterPos4d               = 	dllRasterPos4d              ;
	qglRasterPos4dv              = 	dllRasterPos4dv             ;
	qglRasterPos4f               = 	dllRasterPos4f              ;
	qglRasterPos4fv              = 	dllRasterPos4fv             ;
	qglRasterPos4i               = 	dllRasterPos4i              ;
	qglRasterPos4iv              = 	dllRasterPos4iv             ;
	qglRasterPos4s               = 	dllRasterPos4s              ;
	qglRasterPos4sv              = 	dllRasterPos4sv             ;
	qglReadBuffer                = 	dllReadBuffer               ;
	qglReadPixels                = 	dllReadPixels               ;
	qglCopyBackBufferToTexEXT	 =	dllCopyBackBufferToTexEXT	;
	qglCopyBackBufferToTex		 =	dllCopyBackBufferToTex		;
	qglRectd                     = 	dllRectd                    ;
	qglRectdv                    = 	dllRectdv                   ;
	qglRectf                     = 	dllRectf                    ;
	qglRectfv                    = 	dllRectfv                   ;
	qglRecti                     = 	dllRecti                    ;
	qglRectiv                    = 	dllRectiv                   ;
	qglRects                     = 	dllRects                    ;
	qglRectsv                    = 	dllRectsv                   ;
	qglRenderMode                = 	dllRenderMode               ;
	qglRotated                   = 	dllRotated                  ;
	qglRotatef                   = 	dllRotatef                  ;
	qglScaled                    = 	dllScaled                   ;
	qglScalef                    = 	dllScalef                   ;
	qglScissor                   = 	dllScissor                  ;
	qglSelectBuffer              = 	dllSelectBuffer             ;
	qglShadeModel                = 	dllShadeModel               ;
	qglStencilFunc               = 	dllStencilFunc              ;
	qglStencilMask               = 	dllStencilMask              ;
	qglStencilOp                 = 	dllStencilOp                ;
	qglTexCoord1d                = 	dllTexCoord1d               ;
	qglTexCoord1dv               = 	dllTexCoord1dv              ;
	qglTexCoord1f                = 	dllTexCoord1f               ;
	qglTexCoord1fv               = 	dllTexCoord1fv              ;
	qglTexCoord1i                = 	dllTexCoord1i               ;
	qglTexCoord1iv               = 	dllTexCoord1iv              ;
	qglTexCoord1s                = 	dllTexCoord1s               ;
	qglTexCoord1sv               = 	dllTexCoord1sv              ;
	qglTexCoord2d                = 	dllTexCoord2d               ;
	qglTexCoord2dv               = 	dllTexCoord2dv              ;
	qglTexCoord2f                = 	dllTexCoord2f               ;
	qglTexCoord2fv               = 	dllTexCoord2fv              ;
	qglTexCoord2i                = 	dllTexCoord2i               ;
	qglTexCoord2iv               = 	dllTexCoord2iv              ;
	qglTexCoord2s                = 	dllTexCoord2s               ;
	qglTexCoord2sv               = 	dllTexCoord2sv              ;
	qglTexCoord3d                = 	dllTexCoord3d               ;
	qglTexCoord3dv               = 	dllTexCoord3dv              ;
	qglTexCoord3f                = 	dllTexCoord3f               ;
	qglTexCoord3fv               = 	dllTexCoord3fv              ;
	qglTexCoord3i                = 	dllTexCoord3i               ;
	qglTexCoord3iv               = 	dllTexCoord3iv              ;
	qglTexCoord3s                = 	dllTexCoord3s               ;
	qglTexCoord3sv               = 	dllTexCoord3sv              ;
	qglTexCoord4d                = 	dllTexCoord4d               ;
	qglTexCoord4dv               = 	dllTexCoord4dv              ;
	qglTexCoord4f                = 	dllTexCoord4f               ;
	qglTexCoord4fv               = 	dllTexCoord4fv              ;
	qglTexCoord4i                = 	dllTexCoord4i               ;
	qglTexCoord4iv               = 	dllTexCoord4iv              ;
	qglTexCoord4s                = 	dllTexCoord4s               ;
	qglTexCoord4sv               = 	dllTexCoord4sv              ;
	qglTexCoordPointer           = 	dllTexCoordPointer          ;
	qglTexEnvf                   = 	dllTexEnvf                  ;
	qglTexEnvfv                  = 	dllTexEnvfv                 ;
	qglTexEnvi                   = 	dllTexEnvi                  ;
	qglTexEnviv                  = 	dllTexEnviv                 ;
	qglTexGend                   = 	dllTexGend                  ;
	qglTexGendv                  = 	dllTexGendv                 ;
	qglTexGenf                   = 	dllTexGenf                  ;
	qglTexGenfv                  = 	dllTexGenfv                 ;
	qglTexGeni                   = 	dllTexGeni                  ;
	qglTexGeniv                  = 	dllTexGeniv                 ;
	qglTexImage1D                = 	dllTexImage1D               ;
	qglTexImage2D                = 	dllTexImage2D               ;
	qglTexImage2DEXT             = 	dllTexImage2DEXT            ;
	qglTexParameterf             = 	dllTexParameterf            ;
	qglTexParameterfv            = 	dllTexParameterfv           ;
	qglTexParameteri             = 	dllTexParameteri            ;
	qglTexParameteriv            = 	dllTexParameteriv           ;
	qglTexSubImage1D             = 	dllTexSubImage1D            ;
	qglTexSubImage2D             = 	dllTexSubImage2D            ;
	qglTranslated                = 	dllTranslated               ;
	qglTranslatef                = 	dllTranslatef               ;
	qglVertex2d                  = 	dllVertex2d                 ;
	qglVertex2dv                 = 	dllVertex2dv                ;
	qglVertex2f                  = 	dllVertex2f                 ;
	qglVertex2fv                 = 	dllVertex2fv                ;
	qglVertex2i                  = 	dllVertex2i                 ;
	qglVertex2iv                 = 	dllVertex2iv                ;
	qglVertex2s                  = 	dllVertex2s                 ;
	qglVertex2sv                 = 	dllVertex2sv                ;
	qglVertex3d                  = 	dllVertex3d                 ;
	qglVertex3dv                 = 	dllVertex3dv                ;
	qglVertex3f                  = 	dllVertex3f                 ;
	qglVertex3fv                 = 	dllVertex3fv                ;
	qglVertex3i                  = 	dllVertex3i                 ;
	qglVertex3iv                 = 	dllVertex3iv                ;
	qglVertex3s                  = 	dllVertex3s                 ;
	qglVertex3sv                 = 	dllVertex3sv                ;
	qglVertex4d                  = 	dllVertex4d                 ;
	qglVertex4dv                 = 	dllVertex4dv                ;
	qglVertex4f                  = 	dllVertex4f                 ;
	qglVertex4fv                 = 	dllVertex4fv                ;
	qglVertex4i                  = 	dllVertex4i                 ;
	qglVertex4iv                 = 	dllVertex4iv                ;
	qglVertex4s                  = 	dllVertex4s                 ;
	qglVertex4sv                 = 	dllVertex4sv                ;
	qglVertexPointer             = 	dllVertexPointer            ;
	qglViewport                  = 	dllViewport                 ;

	qglActiveTextureARB          =  dllActiveTextureARB         ;
	qglClientActiveTextureARB    =  dllClientActiveTextureARB   ;
	qglMultiTexCoord2fARB        =  dllMultiTexCoord2fARB       ;

	return qtrue;
}

void QGL_EnableLogging( qboolean enable )
{
}

// Extra functions bound to d3d_ commands for controlling crazy D3D performance things
#ifndef FINAL_BUILD

// D3D_AutoPerfData controls automatic display of performance information:
// framerate, push buffer data, etc... Usage:
// d3d_autoperf				- Toggle on and off
// d3d_autoperf n			- Set display frequency in ms (default 5000)
static void D3D_AutoPerfData_f( void )
{
	static DWORD sdwInterval = 5000;
	static bool sbEnabled = false;

	int numArgs = Cmd_Argc();

	if (numArgs > 2)
	{
		Com_Printf("D3D_AutoPerfData_f: Too many arguments.\n");
	}
	else if (numArgs <= 1)
	{
		sbEnabled = !sbEnabled;
		D3DPERF_SetShowFrameRateInterval(sbEnabled ? sdwInterval : 0);
	}
	else // numArgs == 2 -> Exactly one real argument
	{
		int new_interval = atoi(Cmd_Argv(1));

		if (!new_interval)
		{
			// Fancy way to turn it off, don't change stored interval
			sbEnabled = false;
		}
		else
		{
			// Force it on
			sdwInterval = new_interval;
			sbEnabled = true;
		}
		D3DPERF_SetShowFrameRateInterval(sbEnabled ? sdwInterval : 0);
	}
}

#endif

extern void GLimp_SetGamma(float);


static void _createWindow(int width, int height, int colorbits, qboolean cdsFullscreen)
{
	glConfig.colorBits = colorbits;

	if ( r_depthbits->integer == 0 ) {
		if ( colorbits > 16 ) {
			glConfig.depthBits = 24;
		} else {
			glConfig.depthBits = 16;
		}
	} else {
		glConfig.depthBits = r_depthbits->integer;
	}
	
	glConfig.stencilBits = r_stencilbits->integer;
	if ( glConfig.depthBits < 24 )
	{
		glConfig.stencilBits = 0;
	}
	
	glConfig.displayFrequency = 75;
	glConfig.stereoEnabled = qfalse;

	// VVFIXME : This is surely wrong.
	glConfig.vidHeight = height;
	glConfig.vidWidth = width;

}

enum VideoModes
{
	VM_480i = 0,
	VM_480p,
	VM_720p,
	VM_1080i
};

bool bHadPersistedSurface = false;

void GLW_Init(int width, int height, int colorbits, qboolean cdsFullscreen)
{
	glw_state = new glwstate_t;
	int mode = VM_480i;

	glw_state->isWidescreen = false;
	if( XGetVideoFlags() & XC_VIDEO_FLAGS_WIDESCREEN )
	{
		glw_state->isWidescreen = true;
		width = 640;

		if( XGetVideoFlags() & XC_VIDEO_FLAGS_HDTV_480p )
		{
			width = 640;
			height = 480;
			mode = VM_480p;
		}

		/*if( XGetVideoFlags() & XC_VIDEO_FLAGS_HDTV_720p )
		{
			width = 1280;
			height = 720;
			mode = VM_720p;
		}

		if( XGetVideoFlags() & XC_VIDEO_FLAGS_HDTV_1080i )
		{
			width = 1920;
			height = 1080;
			mode = VM_1080i;
		}*/
	}

	_createWindow(width, height, colorbits, cdsFullscreen);
	
	glw_state->matrixMode = glwstate_t::MatrixMode_Model;
	glw_state->inDrawBlock = false;
	
	glw_state->serverTU = 0;
	glw_state->clientTU = 0;
	
	glw_state->colorArrayState = false;
	glw_state->vertexArrayState = false;
	glw_state->normalArrayState = false;

	glw_state->cullEnable = true;
	glw_state->cullMode = D3DCULL_CCW;

	glw_state->scissorEnable = false;
	glw_state->scissorBox.x1 = 0;
	glw_state->scissorBox.y1 = 0;
	glw_state->scissorBox.x2 = glConfig.vidWidth;
	glw_state->scissorBox.y2 = glConfig.vidHeight;

	glw_state->shaderMask = 0;

	glw_state->clearColor = D3DCOLOR_RGBA(255, 255, 255, 255);
	glw_state->clearDepth = 1.f;
	glw_state->clearStencil = 0;

	glw_state->currentColor = D3DCOLOR_RGBA(255, 255, 255, 255);

	glw_state->viewport.MinZ = 0.f;
	glw_state->viewport.MaxZ = 1.f;

	for (int t = 0; t < GLW_MAX_TEXTURE_STAGES; ++t)
	{
		glw_state->textureEnv[t] = D3DTOP_MODULATE;
		glw_state->texCoordArrayState[t] = false;
		glw_state->currentTexture[t] = 0;
		glw_state->textureStageDirty[t] = false;
	}

	glw_state->textureBindNum = 1;
	
D3DPRESENT_PARAMETERS present;
	present.BackBufferWidth = width;
	present.BackBufferHeight = height;
	present.BackBufferFormat = D3DFMT_A8R8G8B8;
	present.BackBufferCount = 1;
	present.MultiSampleType = D3DMULTISAMPLE_NONE;
	present.SwapEffect = D3DSWAPEFFECT_DISCARD;
	present.hDeviceWindow = 0;
	present.Windowed = FALSE;
	present.EnableAutoDepthStencil = TRUE;
	present.AutoDepthStencilFormat = D3DFMT_LIN_D24S8;
	present.Flags = 0;
	if( glw_state->isWidescreen )
	{
		present.Flags = D3DPRESENTFLAG_WIDESCREEN;
		extern void CGCam_SetWidescreen(qboolean widescreen);
		if(mode == VM_480p)
		{
			present.Flags |= D3DPRESENTFLAG_PROGRESSIVE;
		}
		//else if(mode == VM_720p)
		//{
		//	present.Flags |= D3DPRESENTFLAG_PROGRESSIVE;
		//}
		//else if(mode == VM_1080i)
		//{
		//	present.Flags |= D3DPRESENTFLAG_INTERLACED; // | D3DPRESENTFLAG_FIELD;
		//}


        present.Flags |= D3DPRESENTFLAG_WIDESCREEN;
		CGCam_SetWidescreen(qtrue);
	}

	present.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
	present.FullScreen_PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;
	present.BufferSurfaces[0] = NULL;
	present.BufferSurfaces[1] = NULL;
	present.BufferSurfaces[2] = NULL;
	present.DepthStencilSurface = NULL;

	if (IDirect3D8::CreateDevice(D3DADAPTER_DEFAULT,
								D3DDEVTYPE_HAL,
								NULL,
								D3DCREATE_HARDWARE_VERTEXPROCESSING,
								&present,
								&glw_state->device) != D3D_OK)
	{
		Com_Printf("Failed to create device. That's bad.\n");
	}
//	qglEnable(GL_VSYNC);
	
	// Immediately check to see if there's 1.2MB being wasted on a persisted surface:
	IDirect3DSurface8 *pPersistedSurf;
	glw_state->device->GetPersistedSurface( &pPersistedSurf );
	bHadPersistedSurface = (pPersistedSurf != NULL);

	for (int m = 0; m < glwstate_t::Num_MatrixModes; ++m)
	{
		D3DXCreateMatrixStack(0, &glw_state->matrixStack[m]);
		glw_state->matrixStack[m]->LoadIdentity();
		glw_state->matricesDirty[m] = false;
	}

	// VVFIXME: Hack - turn off lighting
	dllDisable(GL_LIGHTING);

	// Set a material (for lighting)
	memset( &glw_state->mtrl, 0, sizeof(D3DMATERIAL8) );
	glw_state->mtrl.Diffuse.r = glw_state->mtrl.Ambient.r = 1.0f;
	glw_state->mtrl.Diffuse.g = glw_state->mtrl.Ambient.g = 1.0f;
	glw_state->mtrl.Diffuse.b = glw_state->mtrl.Ambient.b = 1.0f;
	glw_state->mtrl.Diffuse.a = glw_state->mtrl.Ambient.a = 1.0f;
	glw_state->device->SetMaterial( &glw_state->mtrl );
	// Gamma hack
	GLimp_SetGamma(1.3f);

	// Set up our directional light (used for diffuse lighting)
	memset(&glw_state->dirLight, 0, sizeof(D3DLIGHT8));

	// Set up a white point light.
	glw_state->dirLight.Type = D3DLIGHT_DIRECTIONAL;
	glw_state->dirLight.Diffuse.r  = 1.0f;
	glw_state->dirLight.Diffuse.g  = 1.0f;
	glw_state->dirLight.Diffuse.b  = 1.0f;
	glw_state->dirLight.Direction.x = 1.0f;
	glw_state->dirLight.Direction.y = 0.0f;
	glw_state->dirLight.Direction.z = 0.0f;

	// Don't attenuate.
	glw_state->dirLight.Attenuation0 = 1.0f; 
	glw_state->dirLight.Range        = 1000.0f;

	//glw_state->drawArray = new DWORD[SHADER_MAX_VERTEXES * 12];
	glw_state->drawArray = NULL;

#ifdef _XBOX
#ifdef VV_LIGHTING
//	glw_state->flareEffect = new FlareEffect;
//	glw_state->flareEffect->Initialize();
	glw_state->lightEffects = new LightEffects;
	StencilShadower.Initialize();
#endif // VV_LIGHTING
//	HDREffect.Initialize();
#endif

#ifndef FINAL_BUILD
	Cmd_AddCommand("d3d_autoperf", D3D_AutoPerfData_f);
#endif

#if YELLOW_MODE
	initYellowMode();
#endif // YELLOW_MODE
}

void GLW_Shutdown(void)
{
#ifdef _XBOX
#ifdef VV_LIGHTING
	delete glw_state->lightEffects;
#endif
#endif

	for (int m = 0; m < glwstate_t::Num_MatrixModes; ++m)
	{
		glw_state->matrixStack[m]->Release();
	}

	glw_state->device->Release();

#ifdef _XBOX
//	delete glw_state->flareEffect;
#endif

	delete glw_state;
}

//-----------------------------------------------------------------------------
// Compressed Screen Shot code for the save game system
//-----------------------------------------------------------------------------

//#define DISABLE_SCREENSHOT
#define CSS_IMAGE_HDR_SIZE			2048									
#define	CSS_IMAGE_DATA_SIZE			((SAVE_GAME_IMAGE_W * SAVE_GAME_IMAGE_H) / 2 )	

struct XprImageHeader
{
    XPR_HEADER        xpr;           // Standard XPR struct
    IDirect3DTexture9 txt;           // Standard D3D texture struct
    DWORD             dwEndOfHeader; // 0xFFFFFFFF
};

struct XprImage
{
    XprImageHeader hdr;
    CHAR           strPad[ CSS_IMAGE_HDR_SIZE - sizeof( XprImageHeader ) ];
    BYTE           pBits[ CSS_IMAGE_DATA_SIZE ];     // data bits
};

//-----------------------------------------------------------------------------
// SaveCompressedScreenshot
// Saves two screenshots - one is full-screen at 256x128, and placed in
// Z:\screenshot.xbx. The other is cropped, and 64x64 for the dashboard,
// and placed in  Z:\saveimage.xbx
//-----------------------------------------------------------------------------
void SaveCompressedScreenshot( void )
{
#ifndef DISABLE_SCREENSHOT
	LPDIRECT3DSURFACE8 screenShot = 0;
	HRESULT res;
	glw_state->device->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &screenShot);

	// Copy over the screen shot to the new image that is CSS_IMAGE_WH x CSS_IMAGE_WH
	LPDIRECT3DSURFACE8 compressedSaveGameImage = 0;
	// New: we only screenshot the title-safe area, to make the image clearer in the ui:
	RECT srcRect;
	srcRect.left = 48;
	srcRect.top = 36;
	srcRect.right = 592;
	srcRect.bottom = 444;
    glw_state->device->CreateImageSurface( SAVE_GAME_IMAGE_W, SAVE_GAME_IMAGE_H, D3DFMT_DXT1, &compressedSaveGameImage );
	D3DXLoadSurfaceFromSurface( compressedSaveGameImage, NULL, NULL, screenShot, NULL, &srcRect, D3DX_DEFAULT, D3DCOLOR( 0 ) );
  
	// Write out the large screenshot (our 256x128 for display in the UI)
	res = XGWriteSurfaceOrTextureToXPR( compressedSaveGameImage, "Z:\\screenshot.xbx", TRUE );

	// Free the compressed 256x128 image
    if ( compressedSaveGameImage )
		compressedSaveGameImage->Release();

	// Re-load the file and build a signature. File should always be less than 32k
	// Then append to the file. This is a poor example of how-not-to-write-code:
	byte xbxFile[32*1024];
	DWORD dwSize;

	// Open and read:
	HANDLE hFile = CreateFile( "Z:\\screenshot.xbx", GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0 );
	ReadFile( hFile, xbxFile, sizeof(xbxFile), &dwSize, NULL );

	// Build a signature:
	XCALCSIG_SIGNATURE xbxSig;
	HANDLE hSig = XCalculateSignatureBegin( XCALCSIG_FLAG_SAVE_GAME );
	XCalculateSignatureUpdate( hSig, xbxFile, dwSize );
	XCalculateSignatureEnd( hSig, &xbxSig );

	// Append:
	SetFilePointer( hFile, 0, NULL, FILE_END );
	WriteFile( hFile, &xbxSig, sizeof(xbxSig), &dwSize, NULL );
	CloseHandle( hFile );

	// Make another surface, this one only 64x64 for the dashboard:
	glw_state->device->CreateImageSurface( 64, 64, D3DFMT_DXT1, &compressedSaveGameImage );

	// We take a 240x240 region from the center of the screen, offset if in third person:
	srcRect;
	srcRect.left = (glConfig.vidWidth / 2) - 120;
	srcRect.right = srcRect.left + 240;

	srcRect.top = (glConfig.vidHeight / 2) - 120;
	if( Cvar_VariableValue( "cg_thirdPerson" ) )
		srcRect.top += 60;
	srcRect.bottom = srcRect.top + 240;

	D3DXLoadSurfaceFromSurface( compressedSaveGameImage, NULL, NULL, screenShot, NULL, &srcRect, D3DX_DEFAULT, D3DCOLOR( 0 ) );

	// Write out the small screenshot (64x64)
	res = XGWriteSurfaceOrTextureToXPR( compressedSaveGameImage, "Z:\\saveimage.xbx", TRUE );

	// Free the compressed 64x64 image
    if ( compressedSaveGameImage )
		compressedSaveGameImage->Release();

	// Free the big screenshot 640x480x4?
	if ( screenShot )
		screenShot->Release();

#endif
}

//-----------------------------------------------------------------------------
// LoadCompressedScreenshot
// Loads a .xbx screenshot file and replaces the current texture
//-----------------------------------------------------------------------------
BOOL LoadCompressedScreenshot(const char* filename)
{
#ifndef DISABLE_SCREENSHOT
	// get the current texture
	glwstate_t::TextureInfo* info = _getCurrentTexture(glw_state->serverTU);
	if (info == NULL) return FALSE;

	// locals
	DWORD				dwBytesRead;
	BOOL				bSuccess;

	// See if the image file for this saved game exists
	HANDLE hFile = CreateFile( filename, GENERIC_READ, 0, NULL, OPEN_EXISTING,0, NULL );

	if( hFile == INVALID_HANDLE_VALUE )
	{
		// Extreme failure case. Skip other work below, but still blank out the screenshot
		// ONE: Take all textures out of stages, in case ours is locked:
		glw_state->device->SetTexture( 0, NULL );
		glw_state->device->SetTexture( 1, NULL );
		glw_state->textureStageDirty[0] = true;
		glw_state->textureStageDirty[1] = true;

		// TWO: Now wait to be sure that the texture we're about to replace isn't locked by GPU:
		while( info->mipmap->IsBusy() )
		{
			// Do nothing
		}

		// THREE: Use LockRect to get a pointer to the texture's data:
		D3DLOCKED_RECT lr;
		info->mipmap->LockRect( 0, &lr, NULL, D3DLOCK_TILED );

		// FOUR: Copy the texture data from the file:
		memset( lr.pBits, 0, CSS_IMAGE_DATA_SIZE );		
		info->mipmap->UnlockRect( 0 );

		return false;
	}

	// Non-signature portion of the file, which we read all at once:
	XprImage image;

	// Read everything but the signature from disk:
	bSuccess = ReadFile( hFile, &image, sizeof( XprImage ), &dwBytesRead, NULL );

	// Validate that data:
	bSuccess &=	dwBytesRead					== sizeof( XprImage ) &&
				image.hdr.xpr.dwMagic		== XPR_MAGIC_VALUE &&
				image.hdr.xpr.dwTotalSize	== CSS_IMAGE_HDR_SIZE + CSS_IMAGE_DATA_SIZE &&
				image.hdr.xpr.dwHeaderSize	== CSS_IMAGE_HDR_SIZE &&
				image.hdr.dwEndOfHeader		== 0xFFFFFFFF;

	// Now read the signature:
	XCALCSIG_SIGNATURE xbxSig;
	bSuccess &= ReadFile( hFile, &xbxSig, sizeof( xbxSig ), &dwBytesRead, NULL );

	// Verify that we're at the end of the file (it's properly sized)
	if( SetFilePointer( hFile, 0, NULL, FILE_END ) != (sizeof( XprImage ) + sizeof( xbxSig )) )
		bSuccess = false;
	CloseHandle( hFile );

	// Re-sign the data:
	XCALCSIG_SIGNATURE datSig;
	HANDLE hSig = XCalculateSignatureBegin( XCALCSIG_FLAG_SAVE_GAME );
	XCalculateSignatureUpdate( hSig, (const BYTE *) &image, sizeof( image ) );
	XCalculateSignatureEnd( hSig, &datSig );

	// Compare the signatures:
	if( memcmp( &xbxSig, &datSig, sizeof( xbxSig ) ) != 0 )
		bSuccess = false;

	// Now we update the texture. We do this even if the file was bad, using black instead:

	// ONE: Take all textures out of stages, in case ours is locked:
	glw_state->device->SetTexture( 0, NULL );
	glw_state->device->SetTexture( 1, NULL );
	glw_state->textureStageDirty[0] = true;
	glw_state->textureStageDirty[1] = true;

	// TWO: Now wait to be sure that the texture we're about to replace isn't locked by GPU:
	while( info->mipmap->IsBusy() )
	{
		// Do nothing
	}

	// THREE: Use LockRect to get a pointer to the texture's data:
	D3DLOCKED_RECT lr;
	info->mipmap->LockRect( 0, &lr, NULL, D3DLOCK_TILED );

	if( bSuccess )
	{
		// FOUR: Copy the texture data from the file:
		memcpy( lr.pBits, image.pBits, sizeof( image.pBits ) );
	}
	else
	{
		memset( lr.pBits, 0, CSS_IMAGE_DATA_SIZE );
	}
	
	info->mipmap->UnlockRect( 0 );

	return bSuccess;
#else
	return FALSE;
#endif
}

bool CreateVertexShader( const CHAR* strFilename, const DWORD* pdwVertexDecl, DWORD* pdwVertexShader )
{
    HRESULT hr;

    // Open the vertex shader file
    HANDLE hFile;
    DWORD dwNumBytesRead;
    hFile = CreateFile( strFilename, GENERIC_READ, FILE_SHARE_READ, NULL,
                        OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL );
    if( hFile == INVALID_HANDLE_VALUE )
        return false;

    // Allocate memory to read the vertex shader file
    DWORD dwSize = GetFileSize(hFile, NULL);
    BYTE* pData  = new BYTE[dwSize+4];
    if( NULL == pData )
    {
        CloseHandle( hFile );
        return false;
    }
    ZeroMemory( pData, dwSize+4 );

    // Read the pre-compiled vertex shader microcode
    ReadFile(hFile, pData, dwSize, &dwNumBytesRead, 0);
    
    // Create the vertex shader
    hr = glw_state->device->CreateVertexShader( pdwVertexDecl, (const DWORD*)pData,
                                        pdwVertexShader, 0 );

    // Cleanup and return
    CloseHandle( hFile );
    delete [] pData;

	if(hr == S_OK)
		return true;

	return false;
}


bool CreatePixelShader( const CHAR* strFilename, DWORD* pdwPixelShader )
{
    HRESULT hr;

    // Open the pixel shader file
    HANDLE hFile;
    DWORD dwNumBytesRead;
    hFile = CreateFile( strFilename, GENERIC_READ, FILE_SHARE_READ, NULL,
                        OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL );
    if( hFile == INVALID_HANDLE_VALUE )
        return false;

    // Load the pre-compiled pixel shader microcode
    D3DPIXELSHADERDEF_FILE psdf;
    ReadFile( hFile, &psdf, sizeof(D3DPIXELSHADERDEF_FILE), &dwNumBytesRead, NULL );
    
    // Make sure the pixel shader is valid
    if( psdf.FileID != D3DPIXELSHADERDEF_FILE_ID )
    {
        CloseHandle( hFile );
        return false;
    }

    // Create the pixel shader
    if( FAILED( hr = glw_state->device->CreatePixelShader( &(psdf.Psd), pdwPixelShader ) ) )
    {
        CloseHandle( hFile );
        return false;
    }

    // Cleanup
    CloseHandle( hFile );

    return true;
}
