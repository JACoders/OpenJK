#include "qcommon/qcommon.h"
#include "unix_qgl.h"

bool g_bTextureRectangleHack = false;

void (APIENTRYP qglActiveTextureARB) (GLenum texture);
void (APIENTRYP qglClientActiveTextureARB) (GLenum texture);
void (APIENTRYP qglMultiTexCoord2fARB) (GLenum target, GLfloat s, GLfloat t);

void (APIENTRYP qglCombinerParameterfvNV) (GLenum pname,const GLfloat *params);
void (APIENTRYP qglCombinerParameterivNV) (GLenum pname,const GLint *params);
void (APIENTRYP qglCombinerParameterfNV) (GLenum pname,GLfloat param);
void (APIENTRYP qglCombinerParameteriNV) (GLenum pname,GLint param);
void (APIENTRYP qglCombinerInputNV) (GLenum stage,GLenum portion,GLenum variable,GLenum input,GLenum mapping,
		GLenum componentUsage);
void (APIENTRYP qglCombinerOutputNV) (GLenum stage,GLenum portion,GLenum abOutput,GLenum cdOutput,GLenum sumOutput,
		GLenum scale, GLenum bias,GLboolean abDotProduct,GLboolean cdDotProduct,
		GLboolean muxSum);

void (APIENTRYP qglFinalCombinerInputNV) (GLenum variable,GLenum input,GLenum mapping,GLenum componentUsage);
void (APIENTRYP qglGetCombinerInputParameterfvNV) (GLenum stage,GLenum portion,GLenum variable,GLenum pname,GLfloat *params);
void (APIENTRYP qglGetCombinerInputParameterivNV) (GLenum stage,GLenum portion,GLenum variable,GLenum pname,GLint *params);
void (APIENTRYP qglGetCombinerOutputParameterfvNV) (GLenum stage,GLenum portion,GLenum pname,GLfloat *params);
void (APIENTRYP qglGetCombinerOutputParameterivNV) (GLenum stage,GLenum portion,GLenum pname,GLint *params);
void (APIENTRYP qglGetFinalCombinerInputParameterfvNV) (GLenum variable,GLenum pname,GLfloat *params);
void (APIENTRYP qglGetFinalCombinerInputParameterivNV) (GLenum variable,GLenum pname,GLfloat *params);

PFNGLPROGRAMSTRINGARBPROC qglProgramStringARB = NULL;
PFNGLBINDPROGRAMARBPROC qglBindProgramARB = NULL;
PFNGLDELETEPROGRAMSARBPROC qglDeleteProgramsARB = NULL;
PFNGLGENPROGRAMSARBPROC qglGenProgramsARB = NULL;
PFNGLPROGRAMENVPARAMETER4DARBPROC qglProgramEnvParameter4dARB = NULL;
PFNGLPROGRAMENVPARAMETER4DVARBPROC qglProgramEnvParameter4dvARB = NULL;
PFNGLPROGRAMENVPARAMETER4FARBPROC qglProgramEnvParameter4fARB = NULL;
PFNGLPROGRAMENVPARAMETER4FVARBPROC qglProgramEnvParameter4fvARB = NULL;
PFNGLPROGRAMLOCALPARAMETER4DARBPROC qglProgramLocalParameter4dARB = NULL;
PFNGLPROGRAMLOCALPARAMETER4DVARBPROC qglProgramLocalParameter4dvARB = NULL;
PFNGLPROGRAMLOCALPARAMETER4FARBPROC qglProgramLocalParameter4fARB = NULL;
PFNGLPROGRAMLOCALPARAMETER4FVARBPROC qglProgramLocalParameter4fvARB = NULL;
PFNGLGETPROGRAMENVPARAMETERDVARBPROC qglGetProgramEnvParameterdvARB = NULL;
PFNGLGETPROGRAMENVPARAMETERFVARBPROC qglGetProgramEnvParameterfvARB = NULL;
PFNGLGETPROGRAMLOCALPARAMETERDVARBPROC qglGetProgramLocalParameterdvARB = NULL;
PFNGLGETPROGRAMLOCALPARAMETERFVARBPROC qglGetProgramLocalParameterfvARB = NULL;
PFNGLGETPROGRAMIVARBPROC qglGetProgramivARB = NULL;
PFNGLGETPROGRAMSTRINGARBPROC qglGetProgramStringARB = NULL;
PFNGLISPROGRAMARBPROC qglIsProgramARB = NULL;

void ( * qglLockArraysEXT)( int, int);
void ( * qglUnlockArraysEXT) ( void );

void		GLimp_EndFrame( void )
{
}

int 		GLimp_Init( void )
{
}

void		GLimp_Shutdown( void )
{
}

void		GLimp_EnableLogging( qboolean enable )
{
}

void 		GLimp_LogComment( char *comment )
{
}

void		GLimp_SetGamma( unsigned char red[256], unsigned char green[256], unsigned char blue[256] )
{
}
