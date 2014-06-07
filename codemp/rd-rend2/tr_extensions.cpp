/*
===========================================================================
Copyright (C) 2011 James Canete (use.less01@gmail.com)

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
// tr_extensions.c - extensions needed by the renderer not in sdl_glimp.c

#include "tr_local.h"

#ifndef _WIN32
#include <SDL.h>
#define GL_GetProcAddress SDL_GL_GetProcAddress
#else
#include "../win32/glw_win.h"
extern glwstate_t glw_state;
#define GL_GetProcAddress qwglGetProcAddress
#endif

// Drawing commands
PFNGLDRAWRANGEELEMENTSPROC qglDrawRangeElements;
PFNGLDRAWARRAYSINSTANCEDPROC qglDrawArraysInstanced;
PFNGLDRAWELEMENTSINSTANCEDPROC qglDrawElementsInstanced;
PFNGLDRAWELEMENTSBASEVERTEXPROC qglDrawElementsBaseVertex;
PFNGLDRAWRANGEELEMENTSBASEVERTEXPROC qglDrawRangeElementsBaseVertex;
PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXPROC qglDrawElementsInstancedBaseVertex;
PFNGLMULTIDRAWARRAYSPROC qglMultiDrawArrays;
PFNGLMULTIDRAWELEMENTSPROC qglMultiDrawElements;
PFNGLMULTIDRAWELEMENTSBASEVERTEXPROC qglMultiDrawElementsBaseVertex;

// Vertex arrays
PFNGLVERTEXATTRIBPOINTERPROC qglVertexAttribPointer;
PFNGLVERTEXATTRIBIPOINTERPROC qglVertexAttribIPointer;
PFNGLENABLEVERTEXATTRIBARRAYPROC qglEnableVertexAttribArray;
PFNGLDISABLEVERTEXATTRIBARRAYPROC qglDisableVertexAttribArray;

// Vertex array objects
PFNGLGENVERTEXARRAYSPROC qglGenVertexArrays;
PFNGLDELETEVERTEXARRAYSPROC qglDeleteVertexArrays;
PFNGLBINDVERTEXARRAYPROC qglBindVertexArray;
PFNGLISVERTEXARRAYPROC qglIsVertexArray;

// Buffer objects
PFNGLBINDBUFFERPROC qglBindBuffer;
PFNGLDELETEBUFFERSPROC qglDeleteBuffers;
PFNGLGENBUFFERSPROC qglGenBuffers;
PFNGLBUFFERDATAPROC qglBufferData;
PFNGLBUFFERSUBDATAPROC qglBufferSubData;
PFNGLGETBUFFERSUBDATAPROC qglGetBufferSubData;
PFNGLGETBUFFERPARAMETERIVPROC qglGetBufferParameteriv;
PFNGLGETBUFFERPARAMETERI64VPROC qglGetBufferParameteri64v;
PFNGLGETBUFFERPOINTERVPROC qglGetBufferPointerv;
PFNGLBINDBUFFERRANGEPROC qglBindBufferRange;
PFNGLBINDBUFFERBASEPROC qglBindBufferBase;
PFNGLMAPBUFFERRANGEPROC qglMapBufferRange;
PFNGLMAPBUFFERPROC qglMapBuffer;
PFNGLFLUSHMAPPEDBUFFERRANGEPROC qglFlushMappedBufferRange;
PFNGLUNMAPBUFFERPROC qglUnmapBuffer;
PFNGLCOPYBUFFERSUBDATAPROC qglCopyBufferSubData;
PFNGLISBUFFERPROC qglIsBuffer;

// Shader objects
PFNGLCREATESHADERPROC qglCreateShader;
PFNGLSHADERSOURCEPROC qglShaderSource;
PFNGLCOMPILESHADERPROC qglCompileShader;
PFNGLDELETESHADERPROC qglDeleteShader;
PFNGLISSHADERPROC qglIsShader;
PFNGLGETSHADERIVPROC qglGetShaderiv;
PFNGLGETSHADERINFOLOGPROC qglGetShaderInfoLog;
PFNGLGETSHADERSOURCEPROC qglGetShaderSource;

// Program objects
PFNGLCREATEPROGRAMPROC qglCreateProgram;
PFNGLATTACHSHADERPROC qglAttachShader;
PFNGLDETACHSHADERPROC qglDetachShader;
PFNGLLINKPROGRAMPROC qglLinkProgram;
PFNGLUSEPROGRAMPROC qglUseProgram;
PFNGLDELETEPROGRAMPROC qglDeleteProgram;
PFNGLVALIDATEPROGRAMPROC qglValidateProgram;
PFNGLISPROGRAMPROC qglIsProgram;
PFNGLGETPROGRAMIVPROC qglGetProgramiv;
PFNGLGETATTACHEDSHADERSPROC qglGetAttachedShaders;
PFNGLGETPROGRAMINFOLOGPROC qglGetProgramInfoLog;

// Vertex attributes
PFNGLGETACTIVEATTRIBPROC qglGetActiveAttrib;
PFNGLGETATTRIBLOCATIONPROC qglGetAttribLocation;
PFNGLBINDATTRIBLOCATIONPROC qglBindAttribLocation;
PFNGLGETVERTEXATTRIBDVPROC qglGetVertxAttribdv;
PFNGLGETVERTEXATTRIBFVPROC qglGetVertxAttribfv;
PFNGLGETVERTEXATTRIBIVPROC qglGetVertxAttribiv;
PFNGLGETVERTEXATTRIBIIVPROC qglGetVertxAttribIiv;
PFNGLGETVERTEXATTRIBIUIVPROC qglGetVertxAttribIuiv;

// Varying variables
PFNGLTRANSFORMFEEDBACKVARYINGSPROC qglTransformFeedbackVaryings;
PFNGLGETTRANSFORMFEEDBACKVARYINGPROC qglGetTransformFeedbackVarying;

// Uniform variables
PFNGLGETUNIFORMLOCATIONPROC qglGetUniformLocation;
PFNGLGETUNIFORMBLOCKINDEXPROC qglGetUniformBlockIndex;
PFNGLGETACTIVEUNIFORMBLOCKNAMEPROC qglGetActiveUniformBlockName;
PFNGLGETACTIVEUNIFORMBLOCKIVPROC qglGetActiveUniformBlockiv;
PFNGLGETUNIFORMINDICESPROC qglGetUniformIndices;
PFNGLGETACTIVEUNIFORMNAMEPROC qglGetActiveUniformName;
PFNGLGETACTIVEUNIFORMPROC qglGetActiveUniform;
PFNGLGETACTIVEUNIFORMSIVPROC qglGetActiveUniformsiv;
PFNGLUNIFORM1IPROC qglUniform1i;
PFNGLUNIFORM2IPROC qglUniform2i;
PFNGLUNIFORM3IPROC qglUniform3i;
PFNGLUNIFORM4IPROC qglUniform4i;
PFNGLUNIFORM1FPROC qglUniform1f;
PFNGLUNIFORM2FPROC qglUniform2f;
PFNGLUNIFORM3FPROC qglUniform3f;
PFNGLUNIFORM4FPROC qglUniform4f;
PFNGLUNIFORM1IVPROC qglUniform1iv;
PFNGLUNIFORM2IVPROC qglUniform2iv;
PFNGLUNIFORM3IVPROC qglUniform3iv;
PFNGLUNIFORM4IVPROC qglUniform4iv;
PFNGLUNIFORM1FVPROC qglUniform1fv;
PFNGLUNIFORM2FVPROC qglUniform2fv;
PFNGLUNIFORM3FVPROC qglUniform3fv;
PFNGLUNIFORM4FVPROC qglUniform4fv;
PFNGLUNIFORM1UIPROC qglUniform1ui;
PFNGLUNIFORM2UIPROC qglUniform2ui;
PFNGLUNIFORM3UIPROC qglUniform3ui;
PFNGLUNIFORM4UIPROC qglUniform4ui;
PFNGLUNIFORM1UIVPROC qglUniform1uiv;
PFNGLUNIFORM2UIVPROC qglUniform2uiv;
PFNGLUNIFORM3UIVPROC qglUniform3uiv;
PFNGLUNIFORM4UIVPROC qglUniform4uiv;
PFNGLUNIFORMMATRIX2FVPROC qglUniformMatrix2fv;
PFNGLUNIFORMMATRIX3FVPROC qglUniformMatrix3fv;
PFNGLUNIFORMMATRIX4FVPROC qglUniformMatrix4fv;
PFNGLUNIFORMMATRIX2X3FVPROC qglUniformMatrix2x3fv;
PFNGLUNIFORMMATRIX3X2FVPROC qglUniformMatrix3x2fv;
PFNGLUNIFORMMATRIX2X4FVPROC qglUniformMatrix2x4fv;
PFNGLUNIFORMMATRIX4X2FVPROC qglUniformMatrix4x2fv;
PFNGLUNIFORMMATRIX3X4FVPROC qglUniformMatrix3x4fv;
PFNGLUNIFORMMATRIX4X3FVPROC qglUniformMatrix4x3fv;
PFNGLUNIFORMBLOCKBINDINGPROC qglUniformBlockBinding;
PFNGLGETUNIFORMFVPROC qglGetUniformfv;
PFNGLGETUNIFORMIVPROC qglGetUniformiv;
PFNGLGETUNIFORMUIVPROC qglGetUniformuiv;

// Transform feedback
PFNGLBEGINTRANSFORMFEEDBACKPROC qglBeginTransformFeedback;
PFNGLENDTRANSFORMFEEDBACKPROC qglEndTransformFeedback;

// Texture compression
PFNGLCOMPRESSEDTEXIMAGE3DPROC qglCompressedTexImage3D;
PFNGLCOMPRESSEDTEXIMAGE2DPROC qglCompressedTexImage2D;
PFNGLCOMPRESSEDTEXIMAGE1DPROC qglCompressedTexImage1D;
PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC qglCompressedTexSubImage3D;
PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC qglCompressedTexSubImage2D;
PFNGLCOMPRESSEDTEXSUBIMAGE1DPROC qglCompressedTexSubImage1D;
PFNGLGETCOMPRESSEDTEXIMAGEPROC qglGetCompressedTexImage;

// Framebuffers
PFNGLISRENDERBUFFERPROC qglIsRenderbuffer;
PFNGLBINDRENDERBUFFERPROC qglBindRenderbuffer;
PFNGLDELETERENDERBUFFERSPROC qglDeleteRenderbuffers;
PFNGLGENRENDERBUFFERSPROC qglGenRenderbuffers;
PFNGLRENDERBUFFERSTORAGEPROC qglRenderbufferStorage;
PFNGLGETRENDERBUFFERPARAMETERIVPROC qglGetRenderbufferParameteriv;
PFNGLISFRAMEBUFFERPROC qglIsFramebuffer;
PFNGLBINDFRAMEBUFFERPROC qglBindFramebuffer;
PFNGLDELETEFRAMEBUFFERSPROC qglDeleteFramebuffers;
PFNGLGENFRAMEBUFFERSPROC qglGenFramebuffers;
PFNGLCHECKFRAMEBUFFERSTATUSPROC qglCheckFramebufferStatus;
PFNGLFRAMEBUFFERTEXTURE1DPROC qglFramebufferTexture1D;
PFNGLFRAMEBUFFERTEXTURE2DPROC qglFramebufferTexture2D;
PFNGLFRAMEBUFFERTEXTURE3DPROC qglFramebufferTexture3D;
PFNGLFRAMEBUFFERRENDERBUFFERPROC qglFramebufferRenderbuffer;
PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC qglGetFramebufferAttachmentParameteriv;
PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC qglRenderbufferStorageMultisample;
PFNGLBLITFRAMEBUFFERPROC qglBlitFramebuffer;
PFNGLGENERATEMIPMAPPROC qglGenerateMipmap;
PFNGLCLEARBUFFERFVPROC qglClearBufferfv;
PFNGLDRAWBUFFERSPROC qglDrawBuffers;

// Query objects
PFNGLGENQUERIESPROC qglGenQueries;
PFNGLDELETEQUERIESPROC qglDeleteQueries;
PFNGLISQUERYPROC qglIsQuery;
PFNGLBEGINQUERYPROC qglBeginQuery;
PFNGLENDQUERYPROC qglEndQuery;
PFNGLGETQUERYIVPROC qglGetQueryiv;
PFNGLGETQUERYOBJECTIVPROC qglGetQueryObjectiv;
PFNGLGETQUERYOBJECTUIVPROC qglGetQueryObjectuiv;

// GL_ARB_texture_storage
PFNGLTEXSTORAGE1DPROC qglTexStorage1D;
PFNGLTEXSTORAGE2DPROC qglTexStorage2D;
PFNGLTEXSTORAGE3DPROC qglTexStorage3D;

static qboolean GLimp_HaveExtension(const char *ext)
{
	const char *ptr = Q_stristr( glConfigExt.originalExtensionString, ext );
	if (ptr == NULL)
		return qfalse;
	ptr += strlen(ext);
	return (qboolean)((*ptr == ' ') || (*ptr == '\0'));  // verify it's complete string.
}

void GLimp_InitExtraExtensions()
{
	char *extension;
	const char* result[3] = { "...ignoring %s\n", "...using %s\n", "...%s not found\n" };

	// Drawing commands
	qglDrawRangeElements = (PFNGLDRAWRANGEELEMENTSPROC)GL_GetProcAddress("glDrawRangeElements");
	qglDrawArraysInstanced = (PFNGLDRAWARRAYSINSTANCEDPROC)GL_GetProcAddress("glDrawArraysInstanced");
	qglDrawElementsInstanced = (PFNGLDRAWELEMENTSINSTANCEDPROC)GL_GetProcAddress("glDrawElementsInstanced");
	qglDrawElementsBaseVertex = (PFNGLDRAWELEMENTSBASEVERTEXPROC)GL_GetProcAddress("glDrawElementsBaseVertex");
	qglDrawRangeElementsBaseVertex = (PFNGLDRAWRANGEELEMENTSBASEVERTEXPROC)GL_GetProcAddress("glDrawRangeElementsBaseVertex");
	qglDrawElementsInstancedBaseVertex = (PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXPROC)GL_GetProcAddress("glDrawElementsInstancedBaseVertex");
	qglMultiDrawArrays = (PFNGLMULTIDRAWARRAYSPROC)GL_GetProcAddress("glMultiDrawArrays");
	qglMultiDrawElements = (PFNGLMULTIDRAWELEMENTSPROC)GL_GetProcAddress("glMultiDrawElements");
	qglMultiDrawElementsBaseVertex = (PFNGLMULTIDRAWELEMENTSBASEVERTEXPROC)GL_GetProcAddress("glMultiDrawElementsBaseVertex");

	// Vertex arrays
	qglVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)GL_GetProcAddress("glVertexAttribPointer");
	qglVertexAttribIPointer = (PFNGLVERTEXATTRIBIPOINTERPROC)GL_GetProcAddress("glVertexAttribIPointer");
	qglEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)GL_GetProcAddress("glEnableVertexAttribArray");
	qglDisableVertexAttribArray = (PFNGLDISABLEVERTEXATTRIBARRAYPROC)GL_GetProcAddress("glDisableVertexAttribArray");

	// Vertex array objects
	qglGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC)GL_GetProcAddress("glGenVertexArrays");
	qglDeleteVertexArrays = (PFNGLDELETEVERTEXARRAYSPROC)GL_GetProcAddress("glDeleteVertexArrays");
	qglBindVertexArray = (PFNGLBINDVERTEXARRAYPROC)GL_GetProcAddress("glBindVertexArray");
	qglIsVertexArray = (PFNGLISVERTEXARRAYPROC)GL_GetProcAddress("glIsVertexArray");
	
	// Buffer objects
	qglBindBuffer = (PFNGLBINDBUFFERPROC)GL_GetProcAddress("glBindBuffer");
	qglDeleteBuffers = (PFNGLDELETEBUFFERSPROC)GL_GetProcAddress("glDeleteBuffers");
	qglGenBuffers = (PFNGLGENBUFFERSPROC)GL_GetProcAddress("glGenBuffers");
	qglBufferData = (PFNGLBUFFERDATAPROC)GL_GetProcAddress("glBufferData");
	qglBufferSubData = (PFNGLBUFFERSUBDATAPROC)GL_GetProcAddress("glBufferSubData");
	qglGetBufferSubData = (PFNGLGETBUFFERSUBDATAPROC)GL_GetProcAddress("glGetBufferSubData");
	qglGetBufferParameteriv = (PFNGLGETBUFFERPARAMETERIVPROC)GL_GetProcAddress("glGetBufferParameteriv");
	qglGetBufferParameteri64v = (PFNGLGETBUFFERPARAMETERI64VPROC)GL_GetProcAddress("glGetBufferParameteri64v");
	qglGetBufferPointerv = (PFNGLGETBUFFERPOINTERVPROC)GL_GetProcAddress("glGetBufferPointerv");
	qglBindBufferRange = (PFNGLBINDBUFFERRANGEPROC)GL_GetProcAddress("glBindBufferRange");
	qglBindBufferBase = (PFNGLBINDBUFFERBASEPROC)GL_GetProcAddress("glBindBufferBase");
	qglMapBufferRange = (PFNGLMAPBUFFERRANGEPROC)GL_GetProcAddress("glMapBufferRange");
	qglMapBuffer = (PFNGLMAPBUFFERPROC)GL_GetProcAddress("glMapBuffer");
	qglFlushMappedBufferRange = (PFNGLFLUSHMAPPEDBUFFERRANGEPROC)GL_GetProcAddress("glFlushMappedBufferRange");
	qglUnmapBuffer = (PFNGLUNMAPBUFFERPROC)GL_GetProcAddress("glUnmapBuffer");
	qglCopyBufferSubData = (PFNGLCOPYBUFFERSUBDATAPROC)GL_GetProcAddress("glCopyBufferSubData");
	qglIsBuffer = (PFNGLISBUFFERPROC)GL_GetProcAddress("glIsBuffer");

	// Shader objects
	qglCreateShader = (PFNGLCREATESHADERPROC)GL_GetProcAddress("glCreateShader");
	qglShaderSource = (PFNGLSHADERSOURCEPROC)GL_GetProcAddress("glShaderSource");
	qglCompileShader = (PFNGLCOMPILESHADERPROC)GL_GetProcAddress("glCompileShader");
	qglDeleteShader = (PFNGLDELETESHADERPROC)GL_GetProcAddress("glDeleteShader");
	qglIsShader = (PFNGLISSHADERPROC)GL_GetProcAddress("glIsShader");
	qglGetShaderiv = (PFNGLGETSHADERIVPROC)GL_GetProcAddress("glGetShaderiv");
	qglGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)GL_GetProcAddress("glGetShaderInfoLog");
	qglGetShaderSource = (PFNGLGETSHADERSOURCEPROC)GL_GetProcAddress("glGetShaderSource");

	// Program objects
	qglCreateProgram = (PFNGLCREATEPROGRAMPROC)GL_GetProcAddress("glCreateProgram");
	qglAttachShader = (PFNGLATTACHSHADERPROC)GL_GetProcAddress("glAttachShader");
	qglDetachShader = (PFNGLDETACHSHADERPROC)GL_GetProcAddress("glDetachShader");
	qglLinkProgram = (PFNGLLINKPROGRAMPROC)GL_GetProcAddress("glLinkProgram");
	qglUseProgram = (PFNGLUSEPROGRAMPROC)GL_GetProcAddress("glUseProgram");
	qglDeleteProgram = (PFNGLDELETEPROGRAMPROC)GL_GetProcAddress("glDeleteProgram");
	qglValidateProgram = (PFNGLVALIDATEPROGRAMPROC)GL_GetProcAddress("glValidateProgram");
	qglIsProgram = (PFNGLISPROGRAMPROC)GL_GetProcAddress("glIsProgram");
	qglGetProgramiv = (PFNGLGETPROGRAMIVPROC)GL_GetProcAddress("glGetProgramiv");
	qglGetAttachedShaders = (PFNGLGETATTACHEDSHADERSPROC)GL_GetProcAddress("glGetAttachedShaders");
	qglGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)GL_GetProcAddress("glGetProgramInfoLog");

	// Vertex attributes
	qglGetActiveAttrib = (PFNGLGETACTIVEATTRIBPROC)GL_GetProcAddress("glGetActiveAttrib");
	qglGetAttribLocation = (PFNGLGETATTRIBLOCATIONPROC)GL_GetProcAddress("glGetAttribLocation");
	qglBindAttribLocation = (PFNGLBINDATTRIBLOCATIONPROC)GL_GetProcAddress("glBindAttribLocation");
	qglGetVertxAttribdv = (PFNGLGETVERTEXATTRIBDVPROC)GL_GetProcAddress("glGetVertxAttribdv");
	qglGetVertxAttribfv = (PFNGLGETVERTEXATTRIBFVPROC)GL_GetProcAddress("glGetVertxAttribfv");
	qglGetVertxAttribiv = (PFNGLGETVERTEXATTRIBIVPROC)GL_GetProcAddress("glGetVertxAttribiv");
	qglGetVertxAttribIiv = (PFNGLGETVERTEXATTRIBIIVPROC)GL_GetProcAddress("glGetVertxAttribIiv");
	qglGetVertxAttribIuiv = (PFNGLGETVERTEXATTRIBIUIVPROC)GL_GetProcAddress("glGetVertxAttribIuiv");

	// Varying variables
	qglTransformFeedbackVaryings = (PFNGLTRANSFORMFEEDBACKVARYINGSPROC)GL_GetProcAddress("glTransformFeedbackVaryings");
	qglGetTransformFeedbackVarying = (PFNGLGETTRANSFORMFEEDBACKVARYINGPROC)GL_GetProcAddress("glGetTransformFeedbackVarying");

	// Uniform variables
	qglGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)GL_GetProcAddress("glGetUniformLocation");
	qglGetUniformBlockIndex = (PFNGLGETUNIFORMBLOCKINDEXPROC)GL_GetProcAddress("glGetUniformBlockIndex");
	qglGetActiveUniformBlockName = (PFNGLGETACTIVEUNIFORMBLOCKNAMEPROC)GL_GetProcAddress("glGetActiveUniformBlockName");
	qglGetActiveUniformBlockiv = (PFNGLGETACTIVEUNIFORMBLOCKIVPROC)GL_GetProcAddress("glGetActiveUniformBlockiv");
	qglGetUniformIndices = (PFNGLGETUNIFORMINDICESPROC)GL_GetProcAddress("glGetUniformIndices");
	qglGetActiveUniformName = (PFNGLGETACTIVEUNIFORMNAMEPROC)GL_GetProcAddress("glGetActiveUniformName");
	qglGetActiveUniform = (PFNGLGETACTIVEUNIFORMPROC)GL_GetProcAddress("glGetActiveUniform");
	qglGetActiveUniformsiv = (PFNGLGETACTIVEUNIFORMSIVPROC)GL_GetProcAddress("glGetActiveUniformsiv");
	qglUniform1i = (PFNGLUNIFORM1IPROC)GL_GetProcAddress("glUniform1i");
	qglUniform2i = (PFNGLUNIFORM2IPROC)GL_GetProcAddress("glUniform2i");
	qglUniform3i = (PFNGLUNIFORM3IPROC)GL_GetProcAddress("glUniform3i");
	qglUniform4i = (PFNGLUNIFORM4IPROC)GL_GetProcAddress("glUniform4i");
	qglUniform1f = (PFNGLUNIFORM1FPROC)GL_GetProcAddress("glUniform1f");
	qglUniform2f = (PFNGLUNIFORM2FPROC)GL_GetProcAddress("glUniform2f");
	qglUniform3f = (PFNGLUNIFORM3FPROC)GL_GetProcAddress("glUniform3f");
	qglUniform4f = (PFNGLUNIFORM4FPROC)GL_GetProcAddress("glUniform4f");
	qglUniform1iv = (PFNGLUNIFORM1IVPROC)GL_GetProcAddress("glUniform1iv");
	qglUniform2iv = (PFNGLUNIFORM2IVPROC)GL_GetProcAddress("glUniform2iv");
	qglUniform3iv = (PFNGLUNIFORM3IVPROC)GL_GetProcAddress("glUniform3iv");
	qglUniform4iv = (PFNGLUNIFORM4IVPROC)GL_GetProcAddress("glUniform4iv");
	qglUniform1fv = (PFNGLUNIFORM1FVPROC)GL_GetProcAddress("glUniform1fv");
	qglUniform2fv = (PFNGLUNIFORM2FVPROC)GL_GetProcAddress("glUniform2fv");
	qglUniform3fv = (PFNGLUNIFORM3FVPROC)GL_GetProcAddress("glUniform3fv");
	qglUniform4fv = (PFNGLUNIFORM4FVPROC)GL_GetProcAddress("glUniform4fv");
	qglUniform1ui = (PFNGLUNIFORM1UIPROC)GL_GetProcAddress("glUniform1ui");
	qglUniform2ui = (PFNGLUNIFORM2UIPROC)GL_GetProcAddress("glUniform2ui");
	qglUniform3ui = (PFNGLUNIFORM3UIPROC)GL_GetProcAddress("glUniform3ui");
	qglUniform4ui = (PFNGLUNIFORM4UIPROC)GL_GetProcAddress("glUniform4ui");
	qglUniform1uiv = (PFNGLUNIFORM1UIVPROC)GL_GetProcAddress("glUniform1uiv");
	qglUniform2uiv = (PFNGLUNIFORM2UIVPROC)GL_GetProcAddress("glUniform2uiv");
	qglUniform3uiv = (PFNGLUNIFORM3UIVPROC)GL_GetProcAddress("glUniform3uiv");
	qglUniform4uiv = (PFNGLUNIFORM4UIVPROC)GL_GetProcAddress("glUniform4uiv");
	qglUniformMatrix2fv = (PFNGLUNIFORMMATRIX2FVPROC)GL_GetProcAddress("glUniformMatrix2fv");
	qglUniformMatrix3fv = (PFNGLUNIFORMMATRIX3FVPROC)GL_GetProcAddress("glUniformMatrix3fv");
	qglUniformMatrix4fv = (PFNGLUNIFORMMATRIX4FVPROC)GL_GetProcAddress("glUniformMatrix4fv");
	qglUniformMatrix2x3fv = (PFNGLUNIFORMMATRIX2X3FVPROC)GL_GetProcAddress("glUniformMatrix2x3fv");
	qglUniformMatrix3x2fv = (PFNGLUNIFORMMATRIX3X2FVPROC)GL_GetProcAddress("glUniformMatrix3x2fv");
	qglUniformMatrix2x4fv = (PFNGLUNIFORMMATRIX2X4FVPROC)GL_GetProcAddress("glUniformMatrix2x4fv");
	qglUniformMatrix4x2fv = (PFNGLUNIFORMMATRIX4X2FVPROC)GL_GetProcAddress("glUniformMatrix4x2fv");
	qglUniformMatrix3x4fv = (PFNGLUNIFORMMATRIX3X4FVPROC)GL_GetProcAddress("glUniformMatrix3x4fv");
	qglUniformMatrix4x3fv = (PFNGLUNIFORMMATRIX4X3FVPROC)GL_GetProcAddress("glUniformMatrix4x3fv");
	qglUniformBlockBinding = (PFNGLUNIFORMBLOCKBINDINGPROC)GL_GetProcAddress("glUniformBlockBinding");
	qglGetUniformfv = (PFNGLGETUNIFORMFVPROC)GL_GetProcAddress("glGetUniformfv");
	qglGetUniformiv = (PFNGLGETUNIFORMIVPROC)GL_GetProcAddress("glGetUniformiv");
	qglGetUniformuiv = (PFNGLGETUNIFORMUIVPROC)GL_GetProcAddress("glGetUniformuiv");

	// Transform feedback
	qglBeginTransformFeedback = (PFNGLBEGINTRANSFORMFEEDBACKPROC)GL_GetProcAddress("glBeginTransformFeedback");
	qglEndTransformFeedback = (PFNGLENDTRANSFORMFEEDBACKPROC)GL_GetProcAddress("glEndTransformFeedback");

	// Texture compression
	qglCompressedTexImage3D = (PFNGLCOMPRESSEDTEXIMAGE3DPROC)GL_GetProcAddress("glCompressedTexImage3D");
	qglCompressedTexImage2D = (PFNGLCOMPRESSEDTEXIMAGE2DPROC)GL_GetProcAddress("glCompressedTexImage2D");
	qglCompressedTexImage1D = (PFNGLCOMPRESSEDTEXIMAGE1DPROC)GL_GetProcAddress("glCompressedTexImage1D");
	qglCompressedTexSubImage3D = (PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC)GL_GetProcAddress("glCompressedTexSubImage3D");
	qglCompressedTexSubImage2D = (PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC)GL_GetProcAddress("glCompressedTexSubImage2D");
	qglCompressedTexSubImage1D = (PFNGLCOMPRESSEDTEXSUBIMAGE1DPROC)GL_GetProcAddress("glCompressedTexSubImage1D");
	qglGetCompressedTexImage = (PFNGLGETCOMPRESSEDTEXIMAGEPROC)GL_GetProcAddress("glGetCompressedTexImage");

	// GLSL
	{
		char version[256];

		Q_strncpyz( version, (const char *) qglGetString (GL_SHADING_LANGUAGE_VERSION), sizeof( version ) );

		sscanf(version, "%d.%d", &glRefConfig.glslMajorVersion, &glRefConfig.glslMinorVersion);

		ri->Printf(PRINT_ALL, "...using GLSL version %s\n", version);
	}

	// Framebuffer and renderbuffers
	qglGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &glRefConfig.maxRenderbufferSize);
	qglGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &glRefConfig.maxColorAttachments);

	qglIsRenderbuffer = (PFNGLISRENDERBUFFERPROC) GL_GetProcAddress("glIsRenderbuffer");
	qglBindRenderbuffer = (PFNGLBINDRENDERBUFFERPROC) GL_GetProcAddress("glBindRenderbuffer");
	qglDeleteRenderbuffers = (PFNGLDELETERENDERBUFFERSPROC) GL_GetProcAddress("glDeleteRenderbuffers");
	qglGenRenderbuffers = (PFNGLGENRENDERBUFFERSPROC) GL_GetProcAddress("glGenRenderbuffers");
	qglRenderbufferStorage = (PFNGLRENDERBUFFERSTORAGEPROC) GL_GetProcAddress("glRenderbufferStorage");
	qglGetRenderbufferParameteriv = (PFNGLGETRENDERBUFFERPARAMETERIVPROC) GL_GetProcAddress("glGetRenderbufferParameteriv");
	qglIsFramebuffer = (PFNGLISFRAMEBUFFERPROC) GL_GetProcAddress("glIsFramebuffer");
	qglBindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC) GL_GetProcAddress("glBindFramebuffer");
	qglDeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSPROC) GL_GetProcAddress("glDeleteFramebuffers");
	qglGenFramebuffers = (PFNGLGENFRAMEBUFFERSPROC) GL_GetProcAddress("glGenFramebuffers");
	qglCheckFramebufferStatus = (PFNGLCHECKFRAMEBUFFERSTATUSPROC) GL_GetProcAddress("glCheckFramebufferStatus");
	qglFramebufferTexture1D = (PFNGLFRAMEBUFFERTEXTURE1DPROC) GL_GetProcAddress("glFramebufferTexture1D");
	qglFramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2DPROC) GL_GetProcAddress("glFramebufferTexture2D");
	qglFramebufferTexture3D = (PFNGLFRAMEBUFFERTEXTURE3DPROC) GL_GetProcAddress("glFramebufferTexture3D");
	qglFramebufferRenderbuffer = (PFNGLFRAMEBUFFERRENDERBUFFERPROC) GL_GetProcAddress("glFramebufferRenderbuffer");
	qglGetFramebufferAttachmentParameteriv = (PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC) GL_GetProcAddress("glGetFramebufferAttachmentParameteriv");
	qglRenderbufferStorageMultisample = (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC)GL_GetProcAddress("glRenderbufferStorageMultisample");
	qglBlitFramebuffer = (PFNGLBLITFRAMEBUFFERPROC)GL_GetProcAddress("glBlitFramebuffer");
	qglGenerateMipmap = (PFNGLGENERATEMIPMAPPROC) GL_GetProcAddress("glGenerateMipmap");
	qglDrawBuffers = (PFNGLDRAWBUFFERSPROC) GL_GetProcAddress("glDrawBuffers");
	qglClearBufferfv = (PFNGLCLEARBUFFERFVPROC)GL_GetProcAddress ("glClearBufferfv");

	// Queries
	qglGenQueries = (PFNGLGENQUERIESPROC) GL_GetProcAddress("glGenQueries");
	qglDeleteQueries = (PFNGLDELETEQUERIESPROC) GL_GetProcAddress("glDeleteQueries");
	qglIsQuery = (PFNGLISQUERYPROC) GL_GetProcAddress("glIsQuery");
	qglBeginQuery = (PFNGLBEGINQUERYPROC) GL_GetProcAddress("glBeginQuery");
	qglEndQuery = (PFNGLENDQUERYPROC) GL_GetProcAddress("glEndQuery");
	qglGetQueryiv = (PFNGLGETQUERYIVPROC) GL_GetProcAddress("glGetQueryiv");
	qglGetQueryObjectiv = (PFNGLGETQUERYOBJECTIVPROC) GL_GetProcAddress("glGetQueryObjectiv");
	qglGetQueryObjectuiv = (PFNGLGETQUERYOBJECTUIVPROC) GL_GetProcAddress("glGetQueryObjectuiv");

	// Memory info
	glRefConfig.memInfo = MI_NONE;

	if( GLimp_HaveExtension( "GL_NVX_gpu_memory_info" ) )
	{
		glRefConfig.memInfo = MI_NVX;
	}
	else if( GLimp_HaveExtension( "GL_ATI_meminfo" ) )
	{
		glRefConfig.memInfo = MI_ATI;
	}

	glRefConfig.textureCompression = TCR_NONE;

	// GL_EXT_texture_compression_latc
	extension = "GL_EXT_texture_compression_latc";
	if (GLimp_HaveExtension(extension))
	{
		if (r_ext_compressed_textures->integer)
			glRefConfig.textureCompression |= TCR_LATC;

		ri->Printf(PRINT_ALL, result[r_ext_compressed_textures->integer ? 1 : 0], extension);
	}
	else
	{
		ri->Printf(PRINT_ALL, result[2], extension);
	}

	// GL_ARB_texture_compression_bptc
	extension = "GL_ARB_texture_compression_bptc";
	if (GLimp_HaveExtension(extension))
	{
		if (r_ext_compressed_textures->integer >= 2)
			glRefConfig.textureCompression |= TCR_BPTC;

		ri->Printf(PRINT_ALL, result[(r_ext_compressed_textures->integer >= 2) ? 1 : 0], extension);
	}
	else
	{
		ri->Printf(PRINT_ALL, result[2], extension);
	}

	// GL_ARB_texture_storage
	extension = "GL_ARB_texture_storage";
	glRefConfig.immutableTextures = qfalse;
	if( GLimp_HaveExtension( extension ) )
	{
		qglTexStorage1D = (PFNGLTEXSTORAGE1DPROC)GL_GetProcAddress("glTexStorage1D");
		qglTexStorage2D = (PFNGLTEXSTORAGE2DPROC)GL_GetProcAddress("glTexStorage2D");
		qglTexStorage3D = (PFNGLTEXSTORAGE3DPROC)GL_GetProcAddress("glTexStorage3D");

		ri->Printf(PRINT_ALL, result[1], extension);
	}
	else
	{
		ri->Printf(PRINT_ALL, result[2], extension);
	}

	// use float lightmaps?
	glRefConfig.floatLightmap = (qboolean)(r_floatLightmap->integer && r_hdr->integer);
}
