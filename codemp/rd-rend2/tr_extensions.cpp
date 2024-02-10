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

#define GL_GetProcAddress ri.GL_GetProcAddress

// Stencil commands
PFNGLSTENCILOPSEPARATEPROC qglStencilOpSeparate;

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
PFNGLVERTEXATTRIBDIVISORPROC qglVertexAttribDivisor;
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

// Texturing
PFNGLACTIVETEXTUREPROC qglActiveTexture;
PFNGLTEXIMAGE3DPROC qglTexImage3D;

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
PFNGLBINDFRAGDATALOCATIONPROC qglBindFragDataLocation;

// Vertex attributes
PFNGLGETACTIVEATTRIBPROC qglGetActiveAttrib;
PFNGLGETATTRIBLOCATIONPROC qglGetAttribLocation;
PFNGLBINDATTRIBLOCATIONPROC qglBindAttribLocation;
PFNGLGETVERTEXATTRIBDVPROC qglGetVertexAttribdv;
PFNGLGETVERTEXATTRIBFVPROC qglGetVertexAttribfv;
PFNGLGETVERTEXATTRIBIVPROC qglGetVertexAttribiv;
PFNGLGETVERTEXATTRIBIIVPROC qglGetVertexAttribIiv;
PFNGLGETVERTEXATTRIBIUIVPROC qglGetVertexAttribIuiv;

PFNGLVERTEXATTRIB1FPROC qglVertexAttrib1f;
PFNGLVERTEXATTRIB2FPROC qglVertexAttrib2f;
PFNGLVERTEXATTRIB3FPROC qglVertexAttrib3f;
PFNGLVERTEXATTRIB4FPROC qglVertexAttrib4f;

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
PFNGLFRAMEBUFFERTEXTUREPROC qglFramebufferTexture;
PFNGLFRAMEBUFFERTEXTURELAYERPROC qglFramebufferTextureLayer;
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

// GL state
PFNGLGETSTRINGIPROC qglGetStringi;

// Sync objects and fences
PFNGLFENCESYNCPROC qglFenceSync;
PFNGLDELETESYNCPROC qglDeleteSync;
PFNGLCLIENTWAITSYNCPROC qglClientWaitSync;
PFNGLWAITSYNCPROC qglWaitSync;

// GL_ARB_texture_storage
PFNGLTEXSTORAGE1DPROC qglTexStorage1D;
PFNGLTEXSTORAGE2DPROC qglTexStorage2D;
PFNGLTEXSTORAGE3DPROC qglTexStorage3D;

// GL_ARB_buffer_storage
PFNGLBUFFERSTORAGEPROC qglBufferStorage;

// GL_ARB_debug_output
PFNGLDEBUGMESSAGECONTROLARBPROC qglDebugMessageControlARB;
PFNGLDEBUGMESSAGEINSERTARBPROC qglDebugMessageInsertARB;
PFNGLDEBUGMESSAGECALLBACKARBPROC qglDebugMessageCallbackARB;
PFNGLGETDEBUGMESSAGELOGARBPROC qglGetDebugMessageLogARB;

// GL_ARB_timer_query
PFNGLQUERYCOUNTERPROC qglQueryCounter;
PFNGLGETQUERYOBJECTI64VPROC qglGetQueryObjecti64v;
PFNGLGETQUERYOBJECTUI64VPROC qglGetQueryObjectui64v;

static qboolean GLimp_HaveExtension(const char *ext)
{
	const char *ptr = Q_stristr( glConfigExt.originalExtensionString, ext );
	if (ptr == NULL)
		return qfalse;
	ptr += strlen(ext);
	return (qboolean)((*ptr == ' ') || (*ptr == '\0'));  // verify it's complete string.
}

template<typename GLFuncType>
static qboolean GetGLFunction ( GLFuncType& glFunction, const char *glFunctionString, qboolean errorOnFailure )
{
	glFunction = (GLFuncType)GL_GetProcAddress (glFunctionString);
	if ( glFunction == NULL )
	{
		if ( errorOnFailure )
		{
			Com_Error (ERR_FATAL, "ERROR: OpenGL function '%s' could not be found.\n", glFunctionString);
		}

		return qfalse;
	}

	return qtrue;
}


static void QCALL GLimp_OnError(GLenum source, GLenum type, GLuint id, GLenum severity,
									GLsizei length, const GLchar *message, const void *userParam)
{
	const char *severityText = "";
	const char *typeText = "";
	const char *sourceText = "";

	switch ( source )
	{
		case GL_DEBUG_SOURCE_API_ARB: sourceText = "API"; break;
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB: sourceText = "WS"; break;
		case GL_DEBUG_SOURCE_SHADER_COMPILER_ARB: sourceText = "SC"; break;
		case GL_DEBUG_SOURCE_THIRD_PARTY_ARB: sourceText = "3rd"; break;
		case GL_DEBUG_SOURCE_APPLICATION_ARB: sourceText = "App"; break;
		case GL_DEBUG_SOURCE_OTHER_ARB: sourceText = "Oth"; break;
	}

	switch ( severity )
	{
		case GL_DEBUG_SEVERITY_HIGH_ARB: severityText = "High"; break;
		case GL_DEBUG_SEVERITY_MEDIUM_ARB: severityText = "Medium"; break;
		case GL_DEBUG_SEVERITY_LOW_ARB: severityText = "Low"; break;
	}

	switch ( type )
	{
		case GL_DEBUG_TYPE_ERROR_ARB: typeText = "Error"; break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB: typeText = "Deprecated"; break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB: typeText = "Undefined"; break;
		case GL_DEBUG_TYPE_PORTABILITY_ARB: typeText = "Portability"; break;
		case GL_DEBUG_TYPE_PERFORMANCE_ARB: typeText = "Performance"; break;
		case GL_DEBUG_TYPE_OTHER_ARB: typeText = "Other"; break;
	}

	Com_Printf( S_COLOR_YELLOW "OpenGL -> [%s][%s][%s] %s\n", sourceText, severityText, typeText, message );
}

void GLimp_InitCoreFunctions()
{
	Com_Printf("Initializing OpenGL 3.2 functions\n");

	// Drawing commands
	GetGLFunction (qglDrawRangeElements, "glDrawRangeElements", qtrue);
	GetGLFunction (qglDrawArraysInstanced, "glDrawArraysInstanced", qtrue);
	GetGLFunction (qglDrawElementsInstanced, "glDrawElementsInstanced", qtrue);
	GetGLFunction (qglDrawElementsBaseVertex, "glDrawElementsBaseVertex", qtrue);
	GetGLFunction (qglDrawRangeElementsBaseVertex, "glDrawRangeElementsBaseVertex", qtrue);
	GetGLFunction (qglDrawElementsInstancedBaseVertex, "glDrawElementsInstancedBaseVertex", qtrue);
	GetGLFunction (qglMultiDrawArrays, "glMultiDrawArrays", qtrue);
	GetGLFunction (qglMultiDrawElements, "glMultiDrawElements", qtrue);
	GetGLFunction (qglMultiDrawElementsBaseVertex, "glMultiDrawElementsBaseVertex", qtrue);

	// Vertex arrays
	GetGLFunction (qglVertexAttribPointer, "glVertexAttribPointer", qtrue);
	GetGLFunction (qglVertexAttribIPointer, "glVertexAttribIPointer", qtrue);
	GetGLFunction (qglVertexAttribDivisor, "glVertexAttribDivisor", qtrue);
	GetGLFunction (qglEnableVertexAttribArray, "glEnableVertexAttribArray", qtrue);
	GetGLFunction (qglDisableVertexAttribArray, "glDisableVertexAttribArray", qtrue);

	// Vertex array objects
	GetGLFunction (qglGenVertexArrays, "glGenVertexArrays", qtrue);
	GetGLFunction (qglDeleteVertexArrays, "glDeleteVertexArrays", qtrue);
	GetGLFunction (qglBindVertexArray, "glBindVertexArray", qtrue);
	GetGLFunction (qglIsVertexArray, "glIsVertexArray", qtrue);

	GetGLFunction (qglVertexAttrib1f, "glVertexAttrib1f", qtrue);
	GetGLFunction (qglVertexAttrib2f, "glVertexAttrib2f", qtrue);
	GetGLFunction (qglVertexAttrib3f, "glVertexAttrib3f", qtrue);
	GetGLFunction (qglVertexAttrib4f, "glVertexAttrib4f", qtrue);

	// Buffer objects
	qglGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &glRefConfig.uniformBufferOffsetAlignment);
	qglGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &glRefConfig.maxUniformBlockSize);
	qglGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &glRefConfig.maxUniformBufferBindings);
	GetGLFunction (qglBindBuffer, "glBindBuffer", qtrue);
	GetGLFunction (qglDeleteBuffers, "glDeleteBuffers", qtrue);
	GetGLFunction (qglGenBuffers, "glGenBuffers", qtrue);
	GetGLFunction (qglBufferData, "glBufferData", qtrue);
	GetGLFunction (qglBufferSubData, "glBufferSubData", qtrue);
	GetGLFunction (qglGetBufferSubData, "glGetBufferSubData", qtrue);
	GetGLFunction (qglGetBufferParameteriv, "glGetBufferParameteriv", qtrue);
	GetGLFunction (qglGetBufferParameteri64v, "glGetBufferParameteri64v", qtrue);
	GetGLFunction (qglGetBufferPointerv, "glGetBufferPointerv", qtrue);
	GetGLFunction (qglBindBufferRange, "glBindBufferRange", qtrue);
	GetGLFunction (qglBindBufferBase, "glBindBufferBase", qtrue);
	GetGLFunction (qglMapBufferRange, "glMapBufferRange", qtrue);
	GetGLFunction (qglMapBuffer, "glMapBuffer", qtrue);
	GetGLFunction (qglFlushMappedBufferRange, "glFlushMappedBufferRange", qtrue);
	GetGLFunction (qglUnmapBuffer, "glUnmapBuffer", qtrue);
	GetGLFunction (qglCopyBufferSubData, "glCopyBufferSubData", qtrue);
	GetGLFunction (qglIsBuffer, "glIsBuffer", qtrue);

	// Texturing
	GetGLFunction (qglActiveTexture, "glActiveTexture", qtrue);
	GetGLFunction (qglTexImage3D, "glTexImage3D", qtrue);

	// Shader objects
	GetGLFunction (qglCreateShader, "glCreateShader", qtrue);
	GetGLFunction (qglShaderSource, "glShaderSource", qtrue);
	GetGLFunction (qglCompileShader, "glCompileShader", qtrue);
	GetGLFunction (qglDeleteShader, "glDeleteShader", qtrue);
	GetGLFunction (qglIsShader, "glIsShader", qtrue);
	GetGLFunction (qglGetShaderiv, "glGetShaderiv", qtrue);
	GetGLFunction (qglGetShaderInfoLog, "glGetShaderInfoLog", qtrue);
	GetGLFunction (qglGetShaderSource, "glGetShaderSource", qtrue);

	// Program objects
	GetGLFunction (qglCreateProgram, "glCreateProgram", qtrue);
	GetGLFunction (qglAttachShader, "glAttachShader", qtrue);
	GetGLFunction (qglDetachShader, "glDetachShader", qtrue);
	GetGLFunction (qglLinkProgram, "glLinkProgram", qtrue);
	GetGLFunction (qglUseProgram, "glUseProgram", qtrue);
	GetGLFunction (qglDeleteProgram, "glDeleteProgram", qtrue);
	GetGLFunction (qglValidateProgram, "glValidateProgram", qtrue);
	GetGLFunction (qglIsProgram, "glIsProgram", qtrue);
	GetGLFunction (qglGetProgramiv, "glGetProgramiv", qtrue);
	GetGLFunction (qglGetAttachedShaders, "glGetAttachedShaders", qtrue);
	GetGLFunction (qglGetProgramInfoLog, "glGetProgramInfoLog", qtrue);
	GetGLFunction (qglBindFragDataLocation, "glBindFragDataLocation", qtrue);

	// Vertex attributes
	GetGLFunction (qglGetActiveAttrib, "glGetActiveAttrib", qtrue);
	GetGLFunction (qglGetAttribLocation, "glGetAttribLocation", qtrue);
	GetGLFunction (qglBindAttribLocation, "glBindAttribLocation", qtrue);
	GetGLFunction (qglGetVertexAttribdv, "glGetVertexAttribdv", qtrue);
	GetGLFunction (qglGetVertexAttribfv, "glGetVertexAttribfv", qtrue);
	GetGLFunction (qglGetVertexAttribiv, "glGetVertexAttribiv", qtrue);
	GetGLFunction (qglGetVertexAttribIiv, "glGetVertexAttribIiv", qtrue);
	GetGLFunction (qglGetVertexAttribIuiv, "glGetVertexAttribIuiv", qtrue);

	// Varying variables
	GetGLFunction (qglTransformFeedbackVaryings, "glTransformFeedbackVaryings", qtrue);
	GetGLFunction (qglGetTransformFeedbackVarying, "glGetTransformFeedbackVarying", qtrue);

	// Uniform variables
	GetGLFunction (qglGetUniformLocation, "glGetUniformLocation", qtrue);
	GetGLFunction (qglGetUniformBlockIndex, "glGetUniformBlockIndex", qtrue);
	GetGLFunction (qglGetActiveUniformBlockName, "glGetActiveUniformBlockName", qtrue);
	GetGLFunction (qglGetActiveUniformBlockiv, "glGetActiveUniformBlockiv", qtrue);
	GetGLFunction (qglGetUniformIndices, "glGetUniformIndices", qtrue);
	GetGLFunction (qglGetActiveUniformName, "glGetActiveUniformName", qtrue);
	GetGLFunction (qglGetActiveUniform, "glGetActiveUniform", qtrue);
	GetGLFunction (qglGetActiveUniformsiv, "glGetActiveUniformsiv", qtrue);
	GetGLFunction (qglUniform1i, "glUniform1i", qtrue);
	GetGLFunction (qglUniform2i, "glUniform2i", qtrue);
	GetGLFunction (qglUniform3i, "glUniform3i", qtrue);
	GetGLFunction (qglUniform4i, "glUniform4i", qtrue);
	GetGLFunction (qglUniform1f, "glUniform1f", qtrue);
	GetGLFunction (qglUniform2f, "glUniform2f", qtrue);
	GetGLFunction (qglUniform3f, "glUniform3f", qtrue);
	GetGLFunction (qglUniform4f, "glUniform4f", qtrue);
	GetGLFunction (qglUniform1iv, "glUniform1iv", qtrue);
	GetGLFunction (qglUniform2iv, "glUniform2iv", qtrue);
	GetGLFunction (qglUniform3iv, "glUniform3iv", qtrue);
	GetGLFunction (qglUniform4iv, "glUniform4iv", qtrue);
	GetGLFunction (qglUniform1fv, "glUniform1fv", qtrue);
	GetGLFunction (qglUniform2fv, "glUniform2fv", qtrue);
	GetGLFunction (qglUniform3fv, "glUniform3fv", qtrue);
	GetGLFunction (qglUniform4fv, "glUniform4fv", qtrue);
	GetGLFunction (qglUniform1ui, "glUniform1ui", qtrue);
	GetGLFunction (qglUniform2ui, "glUniform2ui", qtrue);
	GetGLFunction (qglUniform3ui, "glUniform3ui", qtrue);
	GetGLFunction (qglUniform4ui, "glUniform4ui", qtrue);
	GetGLFunction (qglUniform1uiv, "glUniform1uiv", qtrue);
	GetGLFunction (qglUniform2uiv, "glUniform2uiv", qtrue);
	GetGLFunction (qglUniform3uiv, "glUniform3uiv", qtrue);
	GetGLFunction (qglUniform4uiv, "glUniform4uiv", qtrue);
	GetGLFunction (qglUniformMatrix2fv, "glUniformMatrix2fv", qtrue);
	GetGLFunction (qglUniformMatrix3fv, "glUniformMatrix3fv", qtrue);
	GetGLFunction (qglUniformMatrix4fv, "glUniformMatrix4fv", qtrue);
	GetGLFunction (qglUniformMatrix2x3fv, "glUniformMatrix2x3fv", qtrue);
	GetGLFunction (qglUniformMatrix3x2fv, "glUniformMatrix3x2fv", qtrue);
	GetGLFunction (qglUniformMatrix2x4fv, "glUniformMatrix2x4fv", qtrue);
	GetGLFunction (qglUniformMatrix4x2fv, "glUniformMatrix4x2fv", qtrue);
	GetGLFunction (qglUniformMatrix3x4fv, "glUniformMatrix3x4fv", qtrue);
	GetGLFunction (qglUniformMatrix4x3fv, "glUniformMatrix4x3fv", qtrue);
	GetGLFunction (qglUniformBlockBinding, "glUniformBlockBinding", qtrue);
	GetGLFunction (qglGetUniformfv, "glGetUniformfv", qtrue);
	GetGLFunction (qglGetUniformiv, "glGetUniformiv", qtrue);
	GetGLFunction (qglGetUniformuiv, "glGetUniformuiv", qtrue);

	// Transform feedback
	GetGLFunction (qglBeginTransformFeedback, "glBeginTransformFeedback", qtrue);
	GetGLFunction (qglEndTransformFeedback, "glEndTransformFeedback", qtrue);

	// Texture compression
	GetGLFunction (qglCompressedTexImage3D, "glCompressedTexImage3D", qtrue);
	GetGLFunction (qglCompressedTexImage2D, "glCompressedTexImage2D", qtrue);
	GetGLFunction (qglCompressedTexImage1D, "glCompressedTexImage1D", qtrue);
	GetGLFunction (qglCompressedTexSubImage3D, "glCompressedTexSubImage3D", qtrue);
	GetGLFunction (qglCompressedTexSubImage2D, "glCompressedTexSubImage2D", qtrue);
	GetGLFunction (qglCompressedTexSubImage1D, "glCompressedTexSubImage1D", qtrue);
	GetGLFunction (qglGetCompressedTexImage, "glGetCompressedTexImage", qtrue);

	// GLSL
	{
		char version[256];
		Q_strncpyz( version, (const char *) qglGetString (GL_SHADING_LANGUAGE_VERSION), sizeof( version ) );
		sscanf(version, "%d.%d", &glRefConfig.glslMajorVersion, &glRefConfig.glslMinorVersion);

		ri.Printf(PRINT_ALL, "...using GLSL version %s\n", version);
	}

	// Framebuffer and renderbuffers
	qglGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &glRefConfig.maxRenderbufferSize);
	qglGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &glRefConfig.maxColorAttachments);

	GetGLFunction (qglIsRenderbuffer, "glIsRenderbuffer", qtrue);
	GetGLFunction (qglBindRenderbuffer, "glBindRenderbuffer", qtrue);
	GetGLFunction (qglDeleteRenderbuffers, "glDeleteRenderbuffers", qtrue);
	GetGLFunction (qglGenRenderbuffers, "glGenRenderbuffers", qtrue);
	GetGLFunction (qglRenderbufferStorage, "glRenderbufferStorage", qtrue);
	GetGLFunction (qglGetRenderbufferParameteriv, "glGetRenderbufferParameteriv", qtrue);
	GetGLFunction (qglIsFramebuffer, "glIsFramebuffer", qtrue);
	GetGLFunction (qglBindFramebuffer, "glBindFramebuffer", qtrue);
	GetGLFunction (qglDeleteFramebuffers, "glDeleteFramebuffers", qtrue);
	GetGLFunction (qglGenFramebuffers, "glGenFramebuffers", qtrue);
	GetGLFunction (qglCheckFramebufferStatus, "glCheckFramebufferStatus", qtrue);
	GetGLFunction (qglFramebufferTexture1D, "glFramebufferTexture1D", qtrue);
	GetGLFunction (qglFramebufferTexture2D, "glFramebufferTexture2D", qtrue);
	GetGLFunction (qglFramebufferTexture3D, "glFramebufferTexture3D", qtrue);
	GetGLFunction (qglFramebufferTexture, "glFramebufferTexture", qtrue);
	GetGLFunction (qglFramebufferTextureLayer, "glFramebufferTextureLayer", qtrue);
	GetGLFunction (qglFramebufferRenderbuffer, "glFramebufferRenderbuffer", qtrue);
	GetGLFunction (qglGetFramebufferAttachmentParameteriv, "glGetFramebufferAttachmentParameteriv", qtrue);
	GetGLFunction (qglRenderbufferStorageMultisample, "glRenderbufferStorageMultisample", qtrue);
	GetGLFunction (qglBlitFramebuffer, "glBlitFramebuffer", qtrue);
	GetGLFunction (qglGenerateMipmap, "glGenerateMipmap", qtrue);
	GetGLFunction (qglDrawBuffers, "glDrawBuffers", qtrue);
	GetGLFunction (qglClearBufferfv, "glClearBufferfv", qtrue);
	GetGLFunction (qglStencilOpSeparate, "glStencilOpSeparate", qtrue);

	// Queries
	GetGLFunction (qglGenQueries, "glGenQueries", qtrue);
	GetGLFunction (qglDeleteQueries, "glDeleteQueries", qtrue);
	GetGLFunction (qglIsQuery, "glIsQuery", qtrue);
	GetGLFunction (qglBeginQuery, "glBeginQuery", qtrue);
	GetGLFunction (qglEndQuery, "glEndQuery", qtrue);
	GetGLFunction (qglGetQueryiv, "glGetQueryiv", qtrue);
	GetGLFunction (qglGetQueryObjectiv, "glGetQueryObjectiv", qtrue);
	GetGLFunction (qglGetQueryObjectuiv, "glGetQueryObjectuiv", qtrue);

	// GL state
	GetGLFunction (qglGetStringi, "glGetStringi", qtrue);

	// Sync objects and fences
	GetGLFunction (qglFenceSync, "glFenceSync", qtrue);
	GetGLFunction (qglDeleteSync, "glDeleteSync", qtrue);
	GetGLFunction (qglClientWaitSync, "glClientWaitSync", qtrue);
	GetGLFunction (qglWaitSync, "glWaitSync", qtrue);

}

void GLW_InitTextureCompression( void );
void GLimp_InitExtensions()
{
	const char *extension;
	const char* result[3] = { "...ignoring %s\n", "...using %s\n", "...%s not found\n" };

	Com_Printf ("Initializing OpenGL extensions\n" );

	// Select our tc scheme
	GLW_InitTextureCompression();

	// GL_EXT_texture_filter_anisotropic
	glConfig.maxTextureFilterAnisotropy = 0;
	if ( GLimp_HaveExtension( "EXT_texture_filter_anisotropic" ) )
	{
		qglGetFloatv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &glConfig.maxTextureFilterAnisotropy );
		Com_Printf ("...GL_EXT_texture_filter_anisotropic available\n" );

		if ( r_ext_texture_filter_anisotropic->integer > 1 )
		{
			Com_Printf ("...using GL_EXT_texture_filter_anisotropic\n" );
		}
		else
		{
			Com_Printf ("...ignoring GL_EXT_texture_filter_anisotropic\n" );
		}
		ri.Cvar_SetValue( "r_ext_texture_filter_anisotropic_avail", glConfig.maxTextureFilterAnisotropy );
		if ( r_ext_texture_filter_anisotropic->value > glConfig.maxTextureFilterAnisotropy )
		{
			ri.Cvar_SetValue( "r_ext_texture_filter_anisotropic_avail", glConfig.maxTextureFilterAnisotropy );
		}
	}
	else
	{
		Com_Printf ("...GL_EXT_texture_filter_anisotropic not found\n" );
		ri.Cvar_Set( "r_ext_texture_filter_anisotropic_avail", "0" );
	}

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

		ri.Printf(PRINT_ALL, result[r_ext_compressed_textures->integer ? 1 : 0], extension);
	}
	else
	{
		ri.Printf(PRINT_ALL, result[2], extension);
	}

	// GL_ARB_texture_compression_bptc
	extension = "GL_ARB_texture_compression_bptc";
	if (GLimp_HaveExtension(extension))
	{
		if (r_ext_compressed_textures->integer >= 2)
			glRefConfig.textureCompression |= TCR_BPTC;

		ri.Printf(PRINT_ALL, result[(r_ext_compressed_textures->integer >= 2) ? 1 : 0], extension);
	}
	else
	{
		ri.Printf(PRINT_ALL, result[2], extension);
	}

	// GL_ARB_texture_storage
	extension = "GL_ARB_texture_storage";
	glRefConfig.immutableTextures = qfalse;
	if( GLimp_HaveExtension( extension ) )
	{
		qboolean loaded = qtrue;

		loaded = (qboolean)(loaded && GetGLFunction (qglTexStorage3D, "glTexStorage3D", qfalse));
		loaded = (qboolean)(loaded && GetGLFunction (qglTexStorage1D, "glTexStorage1D", qfalse));
		loaded = (qboolean)(loaded && GetGLFunction (qglTexStorage2D, "glTexStorage2D", qfalse));

		glRefConfig.immutableTextures = loaded;

		ri.Printf(PRINT_ALL, result[loaded], extension);
	}
	else
	{
		ri.Printf(PRINT_ALL, result[2], extension);
	}

	// GL_ARB_buffer_storage
	extension = "GL_ARB_buffer_storage";
	glRefConfig.immutableBuffers = qfalse;
	if( GLimp_HaveExtension( extension ) )
	{
		qboolean loaded = qtrue;

		if ( r_arb_buffer_storage->integer )
		{
			loaded = (qboolean)(loaded && GetGLFunction (qglBufferStorage, "glBufferStorage", qfalse));
		}
		else
		{
			loaded = qfalse;
		}

		glRefConfig.immutableBuffers = loaded;
		ri.Printf(PRINT_ALL, result[loaded], extension);
	}
	else
	{
		ri.Printf(PRINT_ALL, result[2], extension);
	}

	// GL_ARB_debug_output
	extension = "GL_ARB_debug_output";
	if ( GLimp_HaveExtension( extension ) )
	{
		qboolean loaded = qtrue;

		if ( r_debugContext->integer )
		{
			loaded = (qboolean)(loaded && GetGLFunction (qglDebugMessageControlARB, "glDebugMessageControlARB", qfalse));
			loaded = (qboolean)(loaded && GetGLFunction (qglDebugMessageInsertARB, "glDebugMessageInsertARB", qfalse));
			loaded = (qboolean)(loaded && GetGLFunction (qglDebugMessageCallbackARB, "glDebugMessageCallbackARB", qfalse));
			loaded = (qboolean)(loaded && GetGLFunction (qglGetDebugMessageLogARB, "glGetDebugMessageLogARB", qfalse));
		}
		else
		{
			loaded = qfalse;
		}

		glRefConfig.debugContext = loaded;
		ri.Printf(PRINT_ALL, result[loaded], extension);
	}

	// GL_ARB_timer_query
	extension = "GL_ARB_timer_query";
	if ( GLimp_HaveExtension( extension ) )
	{
		qboolean loaded = qtrue;

		loaded = (qboolean)(loaded && GetGLFunction(qglQueryCounter, "glQueryCounter", qfalse));
		loaded = (qboolean)(loaded && GetGLFunction(qglGetQueryObjecti64v, "glGetQueryObjecti64v", qfalse));
		loaded = (qboolean)(loaded && GetGLFunction(qglGetQueryObjectui64v, "glGetQueryObjectui64v", qfalse));

		glRefConfig.timerQuery = loaded;

		ri.Printf(PRINT_ALL, result[loaded], extension);
	}

	// use float lightmaps?
	glRefConfig.floatLightmap = (qboolean)(r_floatLightmap->integer && r_hdr->integer);

	if ( glRefConfig.debugContext )
	{
		qglEnable( GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB );
		qglDebugMessageCallbackARB(GLimp_OnError, NULL);
	}
}
