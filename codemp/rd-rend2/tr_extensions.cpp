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
PFNGLGETVERTEXATTRIBDVPROC qglGetVertexAttribdv;
PFNGLGETVERTEXATTRIBFVPROC qglGetVertexAttribfv;
PFNGLGETVERTEXATTRIBIVPROC qglGetVertexAttribiv;
PFNGLGETVERTEXATTRIBIIVPROC qglGetVertexAttribIiv;
PFNGLGETVERTEXATTRIBIUIVPROC qglGetVertexAttribIuiv;

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

template<typename GLFuncType>
static qboolean GetGLFunction ( GLFuncType& glFunction, const char *glFunctionString )
{
	glFunction = (GLFuncType)GL_GetProcAddress (glFunctionString);
	if ( glFunction == NULL )
	{
		Com_Error (ERR_FATAL, "ERROR: OpenGL function '%s' could not be found.\n", glFunctionString);
		return qfalse;
	}

	return qtrue;
}

void GLimp_InitExtraExtensions()
{
	char *extension;
	const char* result[3] = { "...ignoring %s\n", "...using %s\n", "...%s not found\n" };

	// Drawing commands
	GetGLFunction (qglDrawRangeElements, "glDrawRangeElements");
	GetGLFunction (qglDrawArraysInstanced, "glDrawArraysInstanced");
	GetGLFunction (qglDrawElementsInstanced, "glDrawElementsInstanced");
	GetGLFunction (qglDrawElementsBaseVertex, "glDrawElementsBaseVertex");
	GetGLFunction (qglDrawRangeElementsBaseVertex, "glDrawRangeElementsBaseVertex");
	GetGLFunction (qglDrawElementsInstancedBaseVertex, "glDrawElementsInstancedBaseVertex");
	GetGLFunction (qglMultiDrawArrays, "glMultiDrawArrays");
	GetGLFunction (qglMultiDrawElements, "glMultiDrawElements");
	GetGLFunction (qglMultiDrawElementsBaseVertex, "glMultiDrawElementsBaseVertex");

	// Vertex arrays
	GetGLFunction (qglVertexAttribPointer, "glVertexAttribPointer");
	GetGLFunction (qglVertexAttribIPointer, "glVertexAttribIPointer");
	GetGLFunction (qglEnableVertexAttribArray, "glEnableVertexAttribArray");
	GetGLFunction (qglDisableVertexAttribArray, "glDisableVertexAttribArray");

	// Vertex array objects
	GetGLFunction (qglGenVertexArrays, "glGenVertexArrays");
	GetGLFunction (qglDeleteVertexArrays, "glDeleteVertexArrays");
	GetGLFunction (qglBindVertexArray, "glBindVertexArray");
	GetGLFunction (qglIsVertexArray, "glIsVertexArray");
	
	// Buffer objects
	GetGLFunction (qglBindBuffer, "glBindBuffer");
	GetGLFunction (qglDeleteBuffers, "glDeleteBuffers");
	GetGLFunction (qglGenBuffers, "glGenBuffers");
	GetGLFunction (qglBufferData, "glBufferData");
	GetGLFunction (qglBufferSubData, "glBufferSubData");
	GetGLFunction (qglGetBufferSubData, "glGetBufferSubData");
	GetGLFunction (qglGetBufferParameteriv, "glGetBufferParameteriv");
	GetGLFunction (qglGetBufferParameteri64v, "glGetBufferParameteri64v");
	GetGLFunction (qglGetBufferPointerv, "glGetBufferPointerv");
	GetGLFunction (qglBindBufferRange, "glBindBufferRange");
	GetGLFunction (qglBindBufferBase, "glBindBufferBase");
	GetGLFunction (qglMapBufferRange, "glMapBufferRange");
	GetGLFunction (qglMapBuffer, "glMapBuffer");
	GetGLFunction (qglFlushMappedBufferRange, "glFlushMappedBufferRange");
	GetGLFunction (qglUnmapBuffer, "glUnmapBuffer");
	GetGLFunction (qglCopyBufferSubData, "glCopyBufferSubData");
	GetGLFunction (qglIsBuffer, "glIsBuffer");

	// Shader objects
	GetGLFunction (qglCreateShader, "glCreateShader");
	GetGLFunction (qglShaderSource, "glShaderSource");
	GetGLFunction (qglCompileShader, "glCompileShader");
	GetGLFunction (qglDeleteShader, "glDeleteShader");
	GetGLFunction (qglIsShader, "glIsShader");
	GetGLFunction (qglGetShaderiv, "glGetShaderiv");
	GetGLFunction (qglGetShaderInfoLog, "glGetShaderInfoLog");
	GetGLFunction (qglGetShaderSource, "glGetShaderSource");

	// Program objects
	GetGLFunction (qglCreateProgram, "glCreateProgram");
	GetGLFunction (qglAttachShader, "glAttachShader");
	GetGLFunction (qglDetachShader, "glDetachShader");
	GetGLFunction (qglLinkProgram, "glLinkProgram");
	GetGLFunction (qglUseProgram, "glUseProgram");
	GetGLFunction (qglDeleteProgram, "glDeleteProgram");
	GetGLFunction (qglValidateProgram, "glValidateProgram");
	GetGLFunction (qglIsProgram, "glIsProgram");
	GetGLFunction (qglGetProgramiv, "glGetProgramiv");
	GetGLFunction (qglGetAttachedShaders, "glGetAttachedShaders");
	GetGLFunction (qglGetProgramInfoLog, "glGetProgramInfoLog");

	// Vertex attributes
	GetGLFunction (qglGetActiveAttrib, "glGetActiveAttrib");
	GetGLFunction (qglGetAttribLocation, "glGetAttribLocation");
	GetGLFunction (qglBindAttribLocation, "glBindAttribLocation");
	GetGLFunction (qglGetVertexAttribdv, "glGetVertexAttribdv");
	GetGLFunction (qglGetVertexAttribfv, "glGetVertexAttribfv");
	GetGLFunction (qglGetVertexAttribiv, "glGetVertexAttribiv");
	GetGLFunction (qglGetVertexAttribIiv, "glGetVertexAttribIiv");
	GetGLFunction (qglGetVertexAttribIuiv, "glGetVertexAttribIuiv");

	// Varying variables
	GetGLFunction (qglTransformFeedbackVaryings, "glTransformFeedbackVaryings");
	GetGLFunction (qglGetTransformFeedbackVarying, "glGetTransformFeedbackVarying");

	// Uniform variables
	GetGLFunction (qglGetUniformLocation, "glGetUniformLocation");
	GetGLFunction (qglGetUniformBlockIndex, "glGetUniformBlockIndex");
	GetGLFunction (qglGetActiveUniformBlockName, "glGetActiveUniformBlockName");
	GetGLFunction (qglGetActiveUniformBlockiv, "glGetActiveUniformBlockiv");
	GetGLFunction (qglGetUniformIndices, "glGetUniformIndices");
	GetGLFunction (qglGetActiveUniformName, "glGetActiveUniformName");
	GetGLFunction (qglGetActiveUniform, "glGetActiveUniform");
	GetGLFunction (qglGetActiveUniformsiv, "glGetActiveUniformsiv");
	GetGLFunction (qglUniform1i, "glUniform1i");
	GetGLFunction (qglUniform2i, "glUniform2i");
	GetGLFunction (qglUniform3i, "glUniform3i");
	GetGLFunction (qglUniform4i, "glUniform4i");
	GetGLFunction (qglUniform1f, "glUniform1f");
	GetGLFunction (qglUniform2f, "glUniform2f");
	GetGLFunction (qglUniform3f, "glUniform3f");
	GetGLFunction (qglUniform4f, "glUniform4f");
	GetGLFunction (qglUniform1iv, "glUniform1iv");
	GetGLFunction (qglUniform2iv, "glUniform2iv");
	GetGLFunction (qglUniform3iv, "glUniform3iv");
	GetGLFunction (qglUniform4iv, "glUniform4iv");
	GetGLFunction (qglUniform1fv, "glUniform1fv");
	GetGLFunction (qglUniform2fv, "glUniform2fv");
	GetGLFunction (qglUniform3fv, "glUniform3fv");
	GetGLFunction (qglUniform4fv, "glUniform4fv");
	GetGLFunction (qglUniform1ui, "glUniform1ui");
	GetGLFunction (qglUniform2ui, "glUniform2ui");
	GetGLFunction (qglUniform3ui, "glUniform3ui");
	GetGLFunction (qglUniform4ui, "glUniform4ui");
	GetGLFunction (qglUniform1uiv, "glUniform1uiv");
	GetGLFunction (qglUniform2uiv, "glUniform2uiv");
	GetGLFunction (qglUniform3uiv, "glUniform3uiv");
	GetGLFunction (qglUniform4uiv, "glUniform4uiv");
	GetGLFunction (qglUniformMatrix2fv, "glUniformMatrix2fv");
	GetGLFunction (qglUniformMatrix3fv, "glUniformMatrix3fv");
	GetGLFunction (qglUniformMatrix4fv, "glUniformMatrix4fv");
	GetGLFunction (qglUniformMatrix2x3fv, "glUniformMatrix2x3fv");
	GetGLFunction (qglUniformMatrix3x2fv, "glUniformMatrix3x2fv");
	GetGLFunction (qglUniformMatrix2x4fv, "glUniformMatrix2x4fv");
	GetGLFunction (qglUniformMatrix4x2fv, "glUniformMatrix4x2fv");
	GetGLFunction (qglUniformMatrix3x4fv, "glUniformMatrix3x4fv");
	GetGLFunction (qglUniformMatrix4x3fv, "glUniformMatrix4x3fv");
	GetGLFunction (qglUniformBlockBinding, "glUniformBlockBinding");
	GetGLFunction (qglGetUniformfv, "glGetUniformfv");
	GetGLFunction (qglGetUniformiv, "glGetUniformiv");
	GetGLFunction (qglGetUniformuiv, "glGetUniformuiv");

	// Transform feedback
	GetGLFunction (qglBeginTransformFeedback, "glBeginTransformFeedback");
	GetGLFunction (qglEndTransformFeedback, "glEndTransformFeedback");

	// Texture compression
	GetGLFunction (qglCompressedTexImage3D, "glCompressedTexImage3D");
	GetGLFunction (qglCompressedTexImage2D, "glCompressedTexImage2D");
	GetGLFunction (qglCompressedTexImage1D, "glCompressedTexImage1D");
	GetGLFunction (qglCompressedTexSubImage3D, "glCompressedTexSubImage3D");
	GetGLFunction (qglCompressedTexSubImage2D, "glCompressedTexSubImage2D");
	GetGLFunction (qglCompressedTexSubImage1D, "glCompressedTexSubImage1D");
	GetGLFunction (qglGetCompressedTexImage, "glGetCompressedTexImage");

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

	GetGLFunction (qglIsRenderbuffer, "glIsRenderbuffer");
	GetGLFunction (qglBindRenderbuffer, "glBindRenderbuffer");
	GetGLFunction (qglDeleteRenderbuffers, "glDeleteRenderbuffers");
	GetGLFunction (qglGenRenderbuffers, "glGenRenderbuffers");
	GetGLFunction (qglRenderbufferStorage, "glRenderbufferStorage");
	GetGLFunction (qglGetRenderbufferParameteriv, "glGetRenderbufferParameteriv");
	GetGLFunction (qglIsFramebuffer, "glIsFramebuffer");
	GetGLFunction (qglBindFramebuffer, "glBindFramebuffer");
	GetGLFunction (qglDeleteFramebuffers, "glDeleteFramebuffers");
	GetGLFunction (qglGenFramebuffers, "glGenFramebuffers");
	GetGLFunction (qglCheckFramebufferStatus, "glCheckFramebufferStatus");
	GetGLFunction (qglFramebufferTexture1D, "glFramebufferTexture1D");
	GetGLFunction (qglFramebufferTexture2D, "glFramebufferTexture2D");
	GetGLFunction (qglFramebufferTexture3D, "glFramebufferTexture3D");
	GetGLFunction (qglFramebufferRenderbuffer, "glFramebufferRenderbuffer");
	GetGLFunction (qglGetFramebufferAttachmentParameteriv, "glGetFramebufferAttachmentParameteriv");
	GetGLFunction (qglRenderbufferStorageMultisample, "glRenderbufferStorageMultisample");
	GetGLFunction (qglBlitFramebuffer, "glBlitFramebuffer");
	GetGLFunction (qglGenerateMipmap, "glGenerateMipmap");
	GetGLFunction (qglDrawBuffers, "glDrawBuffers");
	GetGLFunction (qglClearBufferfv, "glClearBufferfv");

	// Queries
	GetGLFunction (qglGenQueries, "glGenQueries");
	GetGLFunction (qglDeleteQueries, "glDeleteQueries");
	GetGLFunction (qglIsQuery, "glIsQuery");
	GetGLFunction (qglBeginQuery, "glBeginQuery");
	GetGLFunction (qglEndQuery, "glEndQuery");
	GetGLFunction (qglGetQueryiv, "glGetQueryiv");
	GetGLFunction (qglGetQueryObjectiv, "glGetQueryObjectiv");
	GetGLFunction (qglGetQueryObjectuiv, "glGetQueryObjectuiv");

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
