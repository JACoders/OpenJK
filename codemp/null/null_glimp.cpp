#include "../game/q_shared.h"
#include "../renderer/tr_local.h"
#ifdef __linux__
typedef unsigned int GLenum;
#endif

#ifdef _WIN32
#include <windows.h>
BOOL (WINAPI * qwglSwapIntervalEXT)( int interval );
//void (APIENTRY * qglMultiTexCoord2fARB )( GLenum texture, float s, float t );
//void (APIENTRY * qglActiveTextureARB )( GLenum texture );
//void (APIENTRY * qglClientActiveTextureARB )( GLenum texture );


void (APIENTRY * qglLockArraysEXT)( int, int);
void (APIENTRY * qglUnlockArraysEXT) ( void );


void		GLimp_EndFrame( void ) {
}

void 		GLimp_Init( void )
{
}

void		GLimp_Shutdown( void ) {
}

void		GLimp_EnableLogging( qboolean enable ) {
}

void GLimp_LogComment( char *comment ) {
}

qboolean QGL_Init( const char *dllname ) {
	return qtrue;
}

void		QGL_Shutdown( void ) {
}
#else
qboolean ( * qwglSwapIntervalEXT)( int interval );
void ( * qglMultiTexCoord2fARB )( GLenum texture, float s, float t );
void ( * qglActiveTextureARB )( GLenum texture );
void ( * qglClientActiveTextureARB )( GLenum texture );


void ( * qglLockArraysEXT)( int, int);
void ( * qglUnlockArraysEXT) ( void );


void		GLimp_EndFrame( void ) {
}

void 		GLimp_Init( void )
{
}

void		GLimp_Shutdown( void ) {
}

void		GLimp_EnableLogging( qboolean enable ) {
}

void GLimp_LogComment( char *comment ) {
}

qboolean QGL_Init( const char *dllname ) {
	return qtrue;
}

void		QGL_Shutdown( void ) {
}
#endif // !WIN32
