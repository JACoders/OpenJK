#pragma once

#if defined( __LINT__ )
#	include <GL/gl.h>
#elif defined( _WIN32 )
#	include <windows.h>
#	include <gl/gl.h>
#elif defined(MACOS_X)
// Prevent OS X from including its own out-of-date glext.h
#	define GL_GLEXT_LEGACY
#	include <OpenGL/gl.h>
#elif defined( __linux__ )
#	include <GL/gl.h>
#	include <GL/glx.h>
// bk001129 - from cvs1.17 (mkv)
#	if defined(__FX__)
#		include <GL/fxmesa.h>
#	endif
#elif defined( __FreeBSD__ ) // rb010123
#	include <GL/gl.h>
#	include <GL/glx.h>
#	if defined(__FX__)
#		include <GL/fxmesa.h>
#	endif
#else
#	include <gl.h>
#endif

#include "glext.h"

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

// Drawing commands
extern PFNGLDRAWRANGEELEMENTSPROC qglDrawRangeElements;
extern PFNGLDRAWARRAYSINSTANCEDPROC qglDrawArraysInstanced;
extern PFNGLDRAWELEMENTSINSTANCEDPROC qglDrawElementsInstanced;
extern PFNGLDRAWELEMENTSBASEVERTEXPROC qglDrawElementsBaseVertex;
extern PFNGLDRAWRANGEELEMENTSBASEVERTEXPROC qglDrawRangeElementsBaseVertex;
extern PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXPROC qglDrawElementsInstancedBaseVertex;
extern PFNGLMULTIDRAWARRAYSPROC qglMultiDrawArrays;
extern PFNGLMULTIDRAWELEMENTSPROC qglMultiDrawElements;
extern PFNGLMULTIDRAWELEMENTSBASEVERTEXPROC qglMultiDrawElementsBaseVertex;

// Vertex arrays
extern PFNGLVERTEXATTRIBPOINTERPROC qglVertexAttribPointer;
extern PFNGLVERTEXATTRIBIPOINTERPROC qglVertexAttribIPointer;
extern PFNGLENABLEVERTEXATTRIBARRAYPROC qglEnableVertexAttribArray;
extern PFNGLDISABLEVERTEXATTRIBARRAYPROC qglDisableVertexAttribArray;

// Vertex array objects
extern PFNGLGENVERTEXARRAYSPROC qglGenVertexArrays;
extern PFNGLDELETEVERTEXARRAYSPROC qglDeleteVertexArrays;
extern PFNGLBINDVERTEXARRAYPROC qglBindVertexArray;
extern PFNGLISVERTEXARRAYPROC qglIsVertexArray;

// Buffer objects
extern PFNGLBINDBUFFERPROC qglBindBuffer;
extern PFNGLDELETEBUFFERSPROC qglDeleteBuffers;
extern PFNGLGENBUFFERSPROC qglGenBuffers;
extern PFNGLISBUFFERPROC qglIsBuffer;
extern PFNGLBUFFERDATAPROC qglBufferData;
extern PFNGLBUFFERSUBDATAPROC qglBufferSubData;
extern PFNGLGETBUFFERSUBDATAPROC qglGetBufferSubData;
extern PFNGLGETBUFFERPARAMETERIVPROC qglGetBufferParameteriv;
extern PFNGLGETBUFFERPARAMETERI64VPROC qglGetBufferParameteri64v;
extern PFNGLGETBUFFERPOINTERVPROC qglGetBufferPointerv;
extern PFNGLBINDBUFFERRANGEPROC qglBindBufferRange;
extern PFNGLBINDBUFFERBASEPROC qglBindBufferBase;
extern PFNGLMAPBUFFERRANGEPROC qglMapBufferRange;
extern PFNGLMAPBUFFERPROC qglMapBuffer;
extern PFNGLFLUSHMAPPEDBUFFERRANGEPROC qglFlushMappedBufferRange;
extern PFNGLUNMAPBUFFERPROC qglUnmapBuffer;
extern PFNGLCOPYBUFFERSUBDATAPROC qglCopyBufferSubData;
extern PFNGLISBUFFERPROC qglIsBuffer;

// Shader objects
extern PFNGLCREATESHADERPROC qglCreateShader;
extern PFNGLSHADERSOURCEPROC qglShaderSource;
extern PFNGLCOMPILESHADERPROC qglCompileShader;
extern PFNGLDELETESHADERPROC qglDeleteShader;
extern PFNGLISSHADERPROC qglIsShader;
extern PFNGLGETSHADERIVPROC qglGetShaderiv;
extern PFNGLGETSHADERINFOLOGPROC qglGetShaderInfoLog;
extern PFNGLGETSHADERSOURCEPROC qglGetShaderSource;

// Program objects
extern PFNGLCREATEPROGRAMPROC qglCreateProgram;
extern PFNGLATTACHSHADERPROC qglAttachShader;
extern PFNGLDETACHSHADERPROC qglDetachShader;
extern PFNGLLINKPROGRAMPROC qglLinkProgram;
extern PFNGLUSEPROGRAMPROC qglUseProgram;
extern PFNGLDELETEPROGRAMPROC qglDeleteProgram;
extern PFNGLVALIDATEPROGRAMPROC qglValidateProgram;
extern PFNGLISPROGRAMPROC qglIsProgram;
extern PFNGLGETPROGRAMIVPROC qglGetProgramiv;
extern PFNGLGETATTACHEDSHADERSPROC qglGetAttachedShaders;
extern PFNGLGETPROGRAMINFOLOGPROC qglGetProgramInfoLog;
extern PFNGLBINDFRAGDATALOCATIONPROC qglBindFragDataLocation;

// Vertex attributes
extern PFNGLGETACTIVEATTRIBPROC qglGetActiveAttrib;
extern PFNGLGETATTRIBLOCATIONPROC qglGetAttribLocation;
extern PFNGLBINDATTRIBLOCATIONPROC qglBindAttribLocation;
extern PFNGLGETVERTEXATTRIBDVPROC qglGetVertexAttribdv;
extern PFNGLGETVERTEXATTRIBFVPROC qglGetVertexAttribfv;
extern PFNGLGETVERTEXATTRIBIVPROC qglGetVertexAttribiv;
extern PFNGLGETVERTEXATTRIBIIVPROC qglGetVertexAttribIiv;
extern PFNGLGETVERTEXATTRIBIUIVPROC qglGetVertexAttribIuiv;

// Varying variables
extern PFNGLTRANSFORMFEEDBACKVARYINGSPROC qglTransformFeedbackVaryings;
extern PFNGLGETTRANSFORMFEEDBACKVARYINGPROC qglGetTransformFeedbackVarying;

// Uniform variables
extern PFNGLGETUNIFORMLOCATIONPROC qglGetUniformLocation;
extern PFNGLGETUNIFORMBLOCKINDEXPROC qglGetUniformBlockIndex;
extern PFNGLGETACTIVEUNIFORMBLOCKNAMEPROC qglGetActiveUniformBlockName;
extern PFNGLGETACTIVEUNIFORMBLOCKIVPROC qglGetActiveUniformBlockiv;
extern PFNGLGETUNIFORMINDICESPROC qglGetUniformIndices;
extern PFNGLGETACTIVEUNIFORMNAMEPROC qglGetActiveUniformName;
extern PFNGLGETACTIVEUNIFORMPROC qglGetActiveUniform;
extern PFNGLGETACTIVEUNIFORMSIVPROC qglGetActiveUniformsiv;
extern PFNGLUNIFORM1IPROC qglUniform1i;
extern PFNGLUNIFORM2IPROC qglUniform2i;
extern PFNGLUNIFORM3IPROC qglUniform3i;
extern PFNGLUNIFORM4IPROC qglUniform4i;
extern PFNGLUNIFORM1FPROC qglUniform1f;
extern PFNGLUNIFORM2FPROC qglUniform2f;
extern PFNGLUNIFORM3FPROC qglUniform3f;
extern PFNGLUNIFORM4FPROC qglUniform4f;
extern PFNGLUNIFORM1IVPROC qglUniform1iv;
extern PFNGLUNIFORM2IVPROC qglUniform2iv;
extern PFNGLUNIFORM3IVPROC qglUniform3iv;
extern PFNGLUNIFORM4IVPROC qglUniform4iv;
extern PFNGLUNIFORM1FVPROC qglUniform1fv;
extern PFNGLUNIFORM2FVPROC qglUniform2fv;
extern PFNGLUNIFORM3FVPROC qglUniform3fv;
extern PFNGLUNIFORM4FVPROC qglUniform4fv;
extern PFNGLUNIFORM1UIPROC qglUniform1ui;
extern PFNGLUNIFORM2UIPROC qglUniform2ui;
extern PFNGLUNIFORM3UIPROC qglUniform3ui;
extern PFNGLUNIFORM4UIPROC qglUniform4ui;
extern PFNGLUNIFORM1UIVPROC qglUniform1uiv;
extern PFNGLUNIFORM2UIVPROC qglUniform2uiv;
extern PFNGLUNIFORM3UIVPROC qglUniform3uiv;
extern PFNGLUNIFORM4UIVPROC qglUniform4uiv;
extern PFNGLUNIFORMMATRIX2FVPROC qglUniformMatrix2fv;
extern PFNGLUNIFORMMATRIX3FVPROC qglUniformMatrix3fv;
extern PFNGLUNIFORMMATRIX4FVPROC qglUniformMatrix4fv;
extern PFNGLUNIFORMMATRIX2X3FVPROC qglUniformMatrix2x3fv;
extern PFNGLUNIFORMMATRIX3X2FVPROC qglUniformMatrix3x2fv;
extern PFNGLUNIFORMMATRIX2X4FVPROC qglUniformMatrix2x4fv;
extern PFNGLUNIFORMMATRIX4X2FVPROC qglUniformMatrix4x2fv;
extern PFNGLUNIFORMMATRIX3X4FVPROC qglUniformMatrix3x4fv;
extern PFNGLUNIFORMMATRIX4X3FVPROC qglUniformMatrix4x3fv;
extern PFNGLUNIFORMBLOCKBINDINGPROC qglUniformBlockBinding;
extern PFNGLGETUNIFORMFVPROC qglGetUniformfv;
extern PFNGLGETUNIFORMIVPROC qglGetUniformiv;
extern PFNGLGETUNIFORMUIVPROC qglGetUniformuiv;

// Transform feedback
extern PFNGLBEGINTRANSFORMFEEDBACKPROC qglBeginTransformFeedback;
extern PFNGLENDTRANSFORMFEEDBACKPROC qglEndTransformFeedback;

// Texture compression
extern PFNGLCOMPRESSEDTEXIMAGE3DPROC qglCompressedTexImage3D;
extern PFNGLCOMPRESSEDTEXIMAGE2DPROC qglCompressedTexImage2D;
extern PFNGLCOMPRESSEDTEXIMAGE1DPROC qglCompressedTexImage1D;
extern PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC qglCompressedTexSubImage3D;
extern PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC qglCompressedTexSubImage2D;
extern PFNGLCOMPRESSEDTEXSUBIMAGE1DPROC qglCompressedTexSubImage1D;
extern PFNGLGETCOMPRESSEDTEXIMAGEPROC qglGetCompressedTexImage;

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

// Framebuffers and renderbuffers
extern PFNGLISRENDERBUFFERPROC qglIsRenderbuffer;
extern PFNGLBINDRENDERBUFFERPROC qglBindRenderbuffer;
extern PFNGLDELETERENDERBUFFERSPROC qglDeleteRenderbuffers;
extern PFNGLGENRENDERBUFFERSPROC qglGenRenderbuffers;
extern PFNGLRENDERBUFFERSTORAGEPROC qglRenderbufferStorage;
extern PFNGLGETRENDERBUFFERPARAMETERIVPROC qglGetRenderbufferParameteriv;
extern PFNGLISFRAMEBUFFERPROC qglIsFramebuffer;
extern PFNGLBINDFRAMEBUFFERPROC qglBindFramebuffer;
extern PFNGLDELETEFRAMEBUFFERSPROC qglDeleteFramebuffers;
extern PFNGLGENFRAMEBUFFERSPROC qglGenFramebuffers;
extern PFNGLCHECKFRAMEBUFFERSTATUSPROC qglCheckFramebufferStatus;
extern PFNGLFRAMEBUFFERTEXTURE1DPROC qglFramebufferTexture1D;
extern PFNGLFRAMEBUFFERTEXTURE2DPROC qglFramebufferTexture2D;
extern PFNGLFRAMEBUFFERTEXTURE3DPROC qglFramebufferTexture3D;
extern PFNGLFRAMEBUFFERRENDERBUFFERPROC qglFramebufferRenderbuffer;
extern PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC qglGetFramebufferAttachmentParameteriv;
extern PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC qglRenderbufferStorageMultisample;
extern PFNGLBLITFRAMEBUFFERPROC qglBlitFramebuffer;
extern PFNGLGENERATEMIPMAPPROC qglGenerateMipmap;
extern PFNGLDRAWBUFFERSPROC qglDrawBuffers;

// Query objects
extern PFNGLGENQUERIESPROC qglGenQueries;
extern PFNGLDELETEQUERIESPROC qglDeleteQueries;
extern PFNGLISQUERYPROC qglIsQuery;
extern PFNGLBEGINQUERYPROC qglBeginQuery;
extern PFNGLENDQUERYPROC qglEndQuery;
extern PFNGLGETQUERYIVPROC qglGetQueryiv;
extern PFNGLGETQUERYOBJECTIVPROC qglGetQueryObjectiv;
extern PFNGLGETQUERYOBJECTUIVPROC qglGetQueryObjectuiv;

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

// GL_ARB_texture_storage
extern PFNGLTEXSTORAGE1DPROC qglTexStorage1D;
extern PFNGLTEXSTORAGE2DPROC qglTexStorage2D;
extern PFNGLTEXSTORAGE3DPROC qglTexStorage3D;
#ifndef GL_ARB_texture_storage
#define GL_TEXTURE_IMMUTABLE_FORMAT                0x912F
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

extern PFNGLCLEARBUFFERFVPROC qglClearBufferfv;


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